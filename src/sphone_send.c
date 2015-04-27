/*-----------------------------------------------------------------------------
  File:   sphone_send.c
  Description:  Sphone sender file
  Author: Olof Hagsand
  CVS Version: $Id: sphone_send.c,v 1.23 2004/12/27 17:34:04 olof Exp $
 
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

int
send_session_exit(void *arg)
{
    send_session_t *ss = (send_session_t *)arg;

    if (ss->ss_rtp){
	rtp_session_exit(ss->ss_rtp);
	ss->ss_rtp = NULL;
    }
    if (ss->ss_rtcp){
	rtcp_send_stats_exit(ss->ss_rtcp);
	ss->ss_rtcp = NULL;
    }
    if (ss->ss_record_api){
	(*ss->ss_record_api->ra_exit)(ss->ss_record_api);
	ss->ss_record_api = NULL;
    }
    if (ss->ss_coding_api){
	(*ss->ss_coding_api->ca_exit)(ss->ss_coding_api);
	ss->ss_coding_api = NULL;
    }
    if (ss->ss_cp_audio){
	coding_param_exit(ss->ss_cp_audio);
	ss->ss_cp_audio = NULL;
    }
    if (ss->ss_cp_nw){
	coding_param_exit(ss->ss_cp_nw);
	ss->ss_cp_nw = NULL;
    }
    sfree(ss);
    return 0;
}


send_session_t *
send_session_new()
{
    send_session_t *ss;

    ss = (send_session_t *)smalloc(sizeof(send_session_t));
    if (ss == NULL){
	sphone_error("send_session_new: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(ss, 0, sizeof(send_session_t));
    if (exit_register(send_session_exit, ss, "send_session") < 0)
	return NULL;
    return ss;
}


/*--------------------------------------------------------------------------
  record_and_send
  Take data (samples) and length as input and send it as an RTP packet.
  Encode data to aggreed conversion format.
 *------------------------------------------------------------------------*/
int
record_and_send(char *databuf, int datalen, void *arg)
{
    send_session_t *ss = (send_session_t*)arg;
    struct rtp_session *rtp;
    static char outbuf[MAX_PKT_SIZE]; /* MTU-sized buffer (eg 1500 bytes) */ 
    char *p = outbuf;                 /* temporary pointer into buffer */
    int len = 0;                      /* temporary length of packet */
    unsigned int encodelen;
    static rtp_header hdr;            /* RTP header */
    int silent;
    coding_api *ca;
    coding_params *cp_audio = ss->ss_cp_audio;
    coding_params *cp_nw = ss->ss_cp_nw;
    int reduce_factor;

    dbg_print(DBG_SEND, "record_and_send\n");

    assert(ss && ss->ss_rtp && ss->ss_coding_api);
    ca = ss->ss_coding_api;
    rtp = ss->ss_rtp;

    dbg_coding(DBG_CODING|DBG_DETAIL, 
	       "record_and_send, before encode", databuf, datalen, 
	       cp_audio->cp_precision);

    /*
     * reduce channels (if stereo -> mono)
     */
    reduce_factor = cp_audio->cp_channels/cp_nw->cp_channels;
    if (reduce_factor > 1)
	coding_reduce(databuf, &datalen, cp_audio->cp_precision, reduce_factor);
    /* 
     * Encoding step
     */
    if ((*ca->ca_transformlen)(cp_audio, cp_nw, datalen, &encodelen) < 0)
	return -1;
    if (MAX_PKT_SIZE-len < encodelen) { /* Check if data too long */
	sphone_error("record_and_send: too long packet");
	return -1;
    }
    if(encodelen < 0){
	sphone_error("record_and_send: negative encodelen");
	return -1;
    }
    /* 
     * Check for silence
     */
    if (ss->ss_silence_detection){
	/* threshold should really be computed in an init function */
	ss->ss_silence_threshold = SILENCE_THRESHOLD_MS/
	    cp_audio->cp_size_ms;
	if ((silent = silence(databuf, datalen, cp_audio->cp_precision, 
			      &ss->ss_silence_nr, 
			      ss->ss_silence_threshold)) < 0)
	    return -1;
	if (silent==1)
	    return 0;
    }

    /* 
     * Encode payload
     */
    if((*ca->ca_encode)(ca, databuf, datalen, 
			p + sizeof(rtp_header), encodelen) < 0)
	return -1;
    dbg_coding(DBG_CODING|DBG_DETAIL, 
	       "record_and_send, after encode", p+sizeof(rtp_header), encodelen, 
	       cp_nw->cp_precision);


    /*
     * After encoding, build rtp packet:
     * Fill header with timestamp, flags, seq#, etc 
     */
    rtp_set_hdr(&hdr, NULL, rtp->rtp_ssrc);  

    /*set the PayloadType in the rtp header*/
    rtp_set_pt(hdr.flags, rtp->rtp_payload);

    /*
     * Increment rtp sequence number.
     * We must do it AFTER silence detection and BEFORE we silumate packet loss.
     */
    hdr.seq = (ss->ss_seq++)%0x10000; 
    marshal_rtp_hdr(&hdr, p); /* Translate header into network byte order */ 
    len += sizeof(rtp_header);  
    p += sizeof(rtp_header);

    /*
     * Encoded data has been written above by the encode function.
     */
    len += encodelen;
    p += encodelen;

    dbg_print_pkt(DBG_SEND|DBG_DETAIL, "record_and_send", outbuf, 32);

    /* Send buffer on udp socket */
    if (sendto(rtp->rtp_s, outbuf, len, 0x0, 
	       (struct sockaddr*)&rtp->rtp_rcv_addr, 
	       sizeof(rtp->rtp_rcv_addr)) < 0){
	sphone_error("record_and_send: sendto: %s", strerror(errno));
	return -1;
    }
    ss->ss_stats.sst_sbytes += len;
    ss->ss_stats.sst_sbytes++;
    return 0; /* OK */
}
