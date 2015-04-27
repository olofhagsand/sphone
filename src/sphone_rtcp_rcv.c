/*-----------------------------------------------------------------------------
  File:   sphone_rtcp_msgs.c
  Description: Functions for receiving rtcp messages.
  Author: Olof Hagsand
  CVS Version: $Id: sphone_rtcp_rcv.c,v 1.7 2004/06/05 10:11:00 olofh Exp $
 
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
rtcp_rcv_stats_exit(struct rtcp_rcv_stats *rrs)
{
    sfree(rrs);
    return 0;
}

struct rtcp_rcv_stats *
rtcp_rcv_stats_new()
{
    struct rtcp_rcv_stats *rrs;

    rrs = (void*)smalloc(sizeof(struct rtcp_rcv_stats));
    if (rrs == NULL){
	sphone_error("rtcp_rcv_stats_new: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(rrs, 0, sizeof(struct rtcp_rcv_stats));
    return rrs;
}


/* 
 * Receive Receiver Report
 */
static int
rtcp_receive_RR(char *p, struct rtp_session *rtp, struct rtcp_send_stats* rss)
{
    rtcp_rr_t           RR_block;
    struct              timeval T1;

    /* log the arrival time */
    T1 = gettimestamp();
    rss->rss_RR_pkts++;
    p = decode_report_block(&RR_block, p); 

    rss->rss_peer_fraction_lost = ((double)rtcp_get_lost_frac(RR_block.lost))/256.0;
    rss->rss_peer_jitter = rtpTimestamp2ms(RR_block.jitter);
    rss->rss_RTT_delay =  calculateRTT(RR_block.lsr, RR_block.dlsr, T1);
    rss->rss_peer_culm_lost = RR_block.lost >> 24;        
    rss->rss_peer_last_seq = RR_block.last_seq; 

    return 0;
}


/*
 * Receive Sender Report
 * Return 0 if OK or illegal src.  -1 on exit.
 * ssrc is "expected" ssrc.
 */
static int
rtcp_receive_SR(char *p, struct rtp_session *rtp, struct rtcp_rcv_stats* rrs)
{
    rtcp_si_t           SR_block;
    uint32_t            rtp_ts;

    /*
     * Record arrival time for later computation of RTT.
     */
    rrs->rrs_timestamp_SR = gettimestamp();
    rrs->rrs_SR_pkts++;
    p = decode_sender_info(&SR_block, p); 

    /*
     * print time
     */
    rrs->rrs_timestamp_SR_ntp.sec = (SR_block.ntp_sec & 0xffff);
    rrs->rrs_timestamp_SR_ntp.frac = (SR_block.ntp_frac >> 16);

    /*
     * RTP timestamp
     */
    rtp_ts = SR_block.rtp_ts;
    /*
     * Packets and bytes sent - 
     */
    rrs->rrs_packets_sent2us = SR_block.psent; 
    rrs->rrs_bytes_sent2us = SR_block.osent;

    return 0; /* OK */
}

/*
 * Receive BYE
 */
static int
rtcp_receive_bye(char *p, struct rtp_session *rtp)
{
    sphone_warning("Got bye from sender\n");
    return 0;
}



static int
rtcp_receive(struct rtp_session *rtp, void *stats, int expected)
{
    rtcp_common_t       rtcp_header;
    struct sockaddr_in  from_addr;      
    uint16_t            flags = 0x0;
    int                 len;
    char                *inbuf, *p;
    int                 type;
    int                 retval = 0;

    len = INBUF_LEN;
    if ((inbuf = smalloc(len)) == NULL){
	sphone_error("rtcp_receive: %s\n", strerror(errno));
	return -1;
    }
    if (inet_input(rtp->rtcp_s, inbuf, &len, &from_addr) < 0)
	return -1;
    p = inbuf;

    /* Decode, get flags, type and print debug */
    p = decode_rtcp_header(&rtcp_header, p);
    flags = rtcp_header.flags;
    type  = rtcp_get_t(flags);
    dbg_print(DBG_RTCP, "Receiving: RTCP %s\n", rtcp_print_type(type));

    /*
     * Accept any new sender
     * but if we have locked on a specific sender, silently drop if not for us.
     */
    if (rtp->rtp_ssrc == 0){
	rtp->rtp_ssrc = rtcp_header.ssrc;
	rtp->rtcp_dst_addr.sin_addr.s_addr = from_addr.sin_addr.s_addr;
    }
    else
	if (rtp->rtp_ssrc != rtcp_header.ssrc) {
	    sphone_warning("Unexpected source: %d\n", rtcp_header.ssrc);
	    sfree(inbuf);
	    return 0;
	}
    switch(type){
    case RTCP_SR:
	if (type != expected)
	    break;
	retval = rtcp_receive_SR(p, rtp, stats);
	break;
    case RTCP_RR:
	if (type != expected)
	    break;
	retval = rtcp_receive_RR(p, rtp, stats);
	break;
    case RTCP_BYE:
	retval = rtcp_receive_bye(p, rtp);
	break;
    default:
	sphone_warning("rtcp_receive: unexpected type: %d\n", type);
	break;
    }
    sfree(inbuf);
    return retval;
}



int 
rtcp_send_rcv_dispatch(int s, void *arg)
{
    send_session_t *ss = (send_session_t *)arg; 
    struct rtp_session *rtp = ss->ss_rtp;
	
    assert(s == rtp->rtcp_s);
    if (rtcp_receive(rtp, (void*)ss->ss_rtcp, RTCP_RR) < 0)
	return -1;
    return 0;
}


int 
rtcp_rcv_rcv_dispatch(int s, void *arg)
{
    rcv_session_t *rs = (rcv_session_t *)arg;
    struct rtp_session *rtp = rs->rs_rtp;

    assert(s == rtp->rtcp_s);
    if (rtcp_receive(rtp, (void*)rs->rs_rtcp, RTCP_SR) < 0)
	return -1;
    return 0;
}


/*
 * A receiver sends a RR regularly and registers a callback for RRs.
 */
int 
rtcp_rcv_init(int s, rcv_session_t *rs)
{
    struct rtp_session *rtp = rs->rs_rtp;
    struct timeval t, dt;

    t = gettimestamp(); 

    dt.tv_sec = RTCP_REPORT_INTERVAL_S;
    dt.tv_usec = 0;
    timeradd(&t, &dt, &rtp->rtcp_timer);

    if (eventloop_reg_timeout(rtp->rtcp_timer, rtcp_rcv_send_dispatch,
			      rs, "RTCP send RR") < 0)
	return -1;
    if (eventloop_reg_fd(s, rtcp_rcv_rcv_dispatch, rs, 
			 "RTCP rcv receiver") < 0)
	return -1;
    return 0;
}




