/*-----------------------------------------------------------------------------
  File:   sphone_rcv.c
  Description: Receiver file
  Author: Olof Hagsand
  CVS Version: $Id: sphone_rcv.c,v 1.28 2005/02/13 17:19:31 olof Exp $
 
  This software is a part of SICSOPHONE, a real-time, IP-based system for 
  point-to-point delivery of audio between computer end-systems.  

  Sicsophone is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Sicsophone is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Sicsophone; see the file COPYING.

 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include "sphone.h"

char *
seq2str(enum seq_type seq_type)
{
    switch(seq_type){
    case SEQ_NORMAL:
	return "SEQ_NORMAL";
	break;
    case SEQ_GAP:
	return "SEQ_GAP";
	break;
    case SEQ_RESET:
	return "SEQ_RESET";
	break;
    case SEQ_BAD:
	return "SEQ_BAD";
	break;
    case SEQ_DUP:
	return "SEQ_DUP";
	break;
    case SEQ_TALK_START:
	return "SEQ_TALK_START";
	break;
    default: 
	return "undefined";
	break;
    }
    return NULL;
}

static int
update_seq(rcv_session_t *rs, uint16_t seq, enum seq_type *seqt) 
{
    uint16_t udelta = seq - rs->seq_prev;
    const int MAX_DROPOUT = 3000;
    const int MAX_MISORDER = 100;

    if (rs->seq_initial == 0){
	rs->seq_initial++;
	*seqt = SEQ_RESET;
	goto done;
    }
    if (seq == rs->seq_prev + 1) {
	*seqt = SEQ_NORMAL;
	goto done;
    }
    if (udelta < MAX_DROPOUT) {
	/* in order, with permissible gap */
	if (seq < rs->seq_prev) {
	    /*
	     * Sequence number wrapped - count another 64K cycle.
	     */
	    rs->seq_cycles += RTP_SEQ_MOD;
	}
	/*    r->prev_pktSeq = seq; (Set later) */
	*seqt = SEQ_GAP;
	goto done;
    } 
    if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
	/* the sequence number made a very large jump */
	if (seq == rs->seq_bad) {
	    /*
	     * Two sequential packets -- assume that the other side
	     * restarted without telling us so just re-sync
	     * (i.e., pretend this was the first packet).
	     */
	    *seqt = SEQ_RESET;
	}
	else {
	    rs->seq_bad = (seq + 1) & (RTP_SEQ_MOD-1);
	    *seqt = SEQ_BAD;
	}
	goto done;
    }
    /* duplicate or reordered packet */
    *seqt =  SEQ_DUP;
  done:
    return 0;
}

/*
 * Exit function for rcv_session
 * A problem here is that the sub-structures (eg rtp/rtcp/play, etc)
 * have not registered their own exit functions. If they had, they would
 * be freed and this function might try to free an already freed struct.
 */
int
rcv_session_exit(void *arg)
{
    rcv_session_t *rs = (rcv_session_t *)arg;

    if (rs->rs_rtp){
	rtp_session_exit(rs->rs_rtp);
	rs->rs_rtp = NULL;
    }
    if (rs->rs_rtcp){
	rtcp_rcv_stats_exit(rs->rs_rtcp);
	rs->rs_rtcp = NULL;
    }
    if (rs->rs_play_api){
	(*rs->rs_play_api->pa_exit)(rs->rs_play_api);
	rs->rs_play_api = NULL;
    }
    if (rs->rs_coding_api){
	(*rs->rs_coding_api->ca_exit)(rs->rs_coding_api);
	rs->rs_coding_api = NULL;
    }
    if (rs->rs_cp_audio){
	coding_param_exit(rs->rs_cp_audio);
	rs->rs_cp_audio = NULL;
    }
    if (rs->rs_cp_nw){
	coding_param_exit(rs->rs_cp_nw);
	rs->rs_cp_nw = NULL;
    }
    if (rs->rs_playout){
	playout_exit(rs->rs_playout);
	rs->rs_playout = NULL;
    }
    if (rs->rs_logf){
	fclose(rs->rs_logf);
	rs->rs_logf = NULL;
    }
    sfree(rs);
    return 0;
}

rcv_session_t *
rcv_session_new()
{
    rcv_session_t *rs;

    rs = (rcv_session_t *)smalloc(sizeof(rcv_session_t));
    if (rs == NULL){
	sphone_error("rcv_session_new: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(rs, 0, sizeof(rcv_session_t));
    rs->rs_accept_all_ssrc = 1; /* XXX good when debugging */
    if (exit_register(rcv_session_exit, rs, "rcv_session") < 0)
	return NULL;

    return rs;
}

/*
 * Function called at receiver when rtp message arrives
 * (see taurus: audio_rcv.c:receive_data()
 * Reasons for error:
        1. inet_input: 
	
XXX: check payload type. If not match, reset decoder.
 */
int 
receive_and_playout(int s, void *arg)
{
    rcv_session_t *rs = (rcv_session_t *)arg;
    static char *inbuf = NULL;/* Incoming pkt: rtp header and audio samples */
    static char *dec_buf = NULL; /* decoded incoming packet */
    char *p;     /* tmp pointer into packet */
    size_t len;  /* tmp length of packet */
    rtp_header hdr;                    /* RTP header */
    struct sockaddr_in from_addr;      /* Sender of packet */
    enum seq_type seq_type;
    int expand_factor;
    size_t dec_len;
    coding_api *ca;
    coding_params *cp_audio = rs->rs_cp_audio;
    coding_params *cp_nw = rs->rs_cp_nw;
    struct rtp_session *rtp;
    struct timeval now;

    assert(rs);
    if (rs->rs_logf)
	now = gettimestamp();
    ca = rs->rs_coding_api;
    rtp = rs->rs_rtp;
    assert(ca && rtp);

    if (inbuf){       /* remove previous packet data */
	sfree(inbuf);
	inbuf = NULL;
    }
    if (dec_buf){       /* remove previous packet data */
	sfree(dec_buf);
	dec_buf = NULL;
    }
    len = INBUF_LEN;
    inbuf = smalloc(len);

    /* 
     * Inet
     */
    if (inet_input(rtp->rtp_s, inbuf, &len, &from_addr) < 0)
	return 0;
    p = inbuf;
    rs->rs_rtcp->rtcp_stats_bytes += len;
    rs->rs_rtcp->rtcp_stats_bytes++;

    /* 
     * RTP
     */
    unmarshal_rtp_hdr(&hdr, inbuf);
    if (rtp_check_hdr(&hdr) < 0)
	return 0; /* Ignore */

    /*
     * Accept any new sender
     * but if we have locked on a specific sender, silently drop if not for us.
     */
    if (rtp->rtp_ssrc == 0){
	rtp->rtp_ssrc = hdr.ssrc;
	rtp->rtcp_dst_addr.sin_addr.s_addr = from_addr.sin_addr.s_addr;
    }
    else
	if (rtp->rtp_ssrc != hdr.ssrc){
	    if (rs->rs_accept_all_ssrc)
		rtp->rtp_ssrc = hdr.ssrc;
	    else{
		sphone_warning("Unexpected sender src: %d\n", hdr.ssrc);
		return -1;
	    }
	}
    update_seq(rs, hdr.seq, &seq_type);
    if (rs->rs_logf)
	/* -E <sec> <usec> <len> <flags> <seq> <ts sec> <ts frac> */
	fprintf(rs->rs_logf, "E %ld %06ld %d %d %d %d %d\n", 
		now.tv_sec, now.tv_usec,
		len, hdr.flags, hdr.seq, 
		hdr.ts.sec, hdr.ts.frac);
    p += sizeof(rtp_header);
    len -= sizeof(rtp_header);
    if (len == 0){
	sphone_warning("receive_and_playout: data pkt with zero length: drop\n");
	return 0;
    }
	/* XXX: len == 0 should not really be illegal, but it breaks the division below */
    dbg_print(DBG_RCV, "receive_and_playout, seq: %s\n", seq2str(seq_type));
    switch (seq_type){
    case SEQ_BAD:
    case SEQ_DUP:
	return -1;
    case SEQ_GAP:
	rs->rs_stats.rst_lost_pkts += rs->seq_cur - rs->seq_prev - 1;
    case SEQ_NORMAL:
    case SEQ_RESET:
    case SEQ_TALK_START:
	rs->seq_cur = rs->seq_cycles + hdr.seq;
	rs->ts_cur = (unsigned int)(((hdr.ts.sec*0x10000) + hdr.ts.frac)/
		      bytes2samples(cp_audio->cp_precision, len, cp_audio->cp_channels))+0.5;
    }


    /* 
     * Coding 
     */
    /* If RTP payload type has changed - change coding 
       XXX: does not work for changed bits_samp/samp_sec*/
    if (rtp_get_pt(hdr.flags) != rtp->rtp_payload){
#ifdef NOTYET /* Change CODING! */
	int newcoding = rtp2coding(rtp_get_pt(hdr.flags));
	rtp->rtp_payload = rtp_get_pt(hdr.flags);
	(*ca->ca_exit)(ca);
	if ((ca = coding_init(newcoding)) == NULL)
	    return -1;
	rs->rs_coding_api = ca;
#else
	sphone_error("receive_and_playout: Changed coding not supported");
#endif /* NOTYET */
    }
    if ((*ca->ca_transformlen)(cp_nw, cp_audio, len, &dec_len) < 0)
	return -1;
    expand_factor = cp_audio->cp_channels/cp_nw->cp_channels;
    if ((dec_buf = smalloc(dec_len*expand_factor)) == NULL){
	sphone_error("receive_and_playout: malloc: %s", strerror(errno));
	return -1;
    }
    dbg_coding(DBG_CODING|DBG_DETAIL, 
	       "receive_and_playout, before decode", p, len, 
	       cp_nw->cp_precision);
    if ((*ca->ca_decode)(ca, p, len, dec_buf, dec_len) < 0){
	sfree(dec_buf);
	return -1;
    }
    dbg_coding(DBG_CODING|DBG_DETAIL, 
	       "receive_and_playout, after decode", dec_buf, dec_len, 
	       cp_nw->cp_precision);
    if (expand_factor > 1){
	coding_expand(dec_buf, &dec_len, cp_audio->cp_precision, 
		      expand_factor);
	dbg_coding(DBG_CODING|DBG_DETAIL, 
		   "receive_and_playout, after expansion", dec_buf, dec_len, 
		   cp_audio->cp_precision);
    }
    if (playout_play(rs, dec_buf, dec_len, seq_type) < 0)
	return -1;
    rs->seq_prev = rs->seq_cur;
    rs->ts_prev = rs->ts_cur;
    return 0;
}



/*--------------------------------------------------------------------------
  rcv_start
  Get an rtp packet, parse it, and dispatch it to audio module.
 *------------------------------------------------------------------------*/
int
rcv_start(rcv_session_t *rs)
{
    struct rtp_session *rtp;
    
    rtp = rs->rs_rtp;
    if (eventloop_reg_fd(rtp->rtp_s, 
			 receive_and_playout, 
			 (void*)rs,
			 "rcv_start - rtp receive") < 0)
	return -1;
    return 0;
}
