/*-----------------------------------------------------------------------------
  File:   sphone_rtcp_msgs.c
  Description: Functions for sending rtcp messages.
  Author: Olof Hagsand
  CVS Version: $Id: sphone_rtcp_send.c,v 1.8 2004/06/05 10:11:00 olofh Exp $
 
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
rtcp_send_stats_exit(struct rtcp_send_stats *rss)
{
    sfree(rss);
    return 0;
}


struct rtcp_send_stats *
rtcp_send_stats_new()
{
    struct rtcp_send_stats *rss;

    rss = (void*)smalloc(sizeof(struct rtcp_send_stats));
    if (rss == NULL){
	sphone_error("rtcp_send_stats_new: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(rss, 0, sizeof(struct rtcp_send_stats));
    return rss;
}


/*
 * Send Sender Report
 */
static int
rtcp_send_SR(struct rtp_session     *rtp, 
	     struct rtcp_send_stats* rss, 
	     uint32_t samp_sec, 
	     uint32_t packets_sent, 
	     uint32_t bytes_sent) 
{
    rtcp_common_t     rtcp_header;
    rtcp_si_t         SR_block;
    struct            timeval now;
    struct            NTP_timestamp_64 ntp;
    int               len = 0;
    char              *p, *databuf;
    int               rtcp_type = RTCP_SR;

    /*
     * Get the time and convert to NTP format
     */
    now = gettimestamp();

    TV2NTP_64(&now, &ntp);

    rtcp_set_hdr(&rtcp_header, rtcp_type, rtp->rtp_ssrc);

    /*
     * OK, build a sender report.
     */
    SR_block.ntp_sec = ntp.sec;
    SR_block.ntp_frac = ntp.frac;

#ifdef IAN
    /*
      The RTP timestamp is calculated from the corresponding NTP
      timestamp using the relationship between the RTP timestamp
      counter and real time as maintained by periodically checking the
      wallclock time at a sampling instant.  
    */
    ntp2rtp(&ntp, start_rtp, &rtp_now);
    SR_block.rtp_ts = rtp_now;

#else /* KJELL/OH */
    /* (from the spec): The timestamp
       corresponds to the same time as the NTP timestamp (above), but
       in the same units and with the same random offset as the RTP
       timestamps in data packets. 
    */
    SR_block.rtp_ts = samp_sec*now.tv_sec + (samp_sec*now.tv_usec)/1000000; 
#endif
    SR_block.psent = packets_sent;
    SR_block.osent = bytes_sent;
#if 0
    /* 
     * Don't worry about what other receivers tell us for now.  Must
     * have at least one reception report block, but it may be zero (see
     * draft). The following sets the receiver report to 0.
     */
    SR_block.->rr[0].ssrc = 0;
    SR_block.->rr[0].lost = 0;
    SR_block.->rr[0].last_seq = 0;
    SR_block.->rr[0].jitter = 0;
    SR_block.->rr[0].lsr = 0;
    SR_block.->rr[0].dlsr = 0;
#endif
    /*  memset(SR_block.rr[0], 0, sizeof(SR_block.rr[0]));*/

    if ((databuf = (char *)smalloc(rtcp_header.length+1)) == NULL){
	sphone_error("rtcp_send_SR: malloc: %s", strerror(errno));
	return -1;
    }
    p = databuf;

    /* 
     * Make an RTCP header and convert byte order. XXX this works for now
     * because we know the length,  if we have a variable number of
     * report blocks then we have to do this last.
     */
    encode_rtcp_header(&rtcp_header, p);
    len += sizeof(rtcp_common_t);
    p += sizeof(rtcp_common_t);

    encode_sender_info(&SR_block, p); 
    len += sizeof(rtcp_si_t);
    p += sizeof(rtcp_si_t);

    dbg_print(DBG_RTCP, "Sending: RTCP Sender report\n");

    /*
     * OK send off the RTCP message to the receiver.
     */
    if (sendto(rtp->rtcp_s, databuf, len, 0x0, 
	       (struct sockaddr *)&rtp->rtcp_dst_addr, 
	       sizeof(rtp->rtcp_dst_addr)) < 0){
	sphone_error("rtcp_send_SR: sendto: %s\n",
		     strerror(errno));
	sfree(databuf);
	return -1;
    }
    sfree(databuf);

    /*
     * Fill in rtcp stats struct.
     */
    rss->rss_SR_pkts++;
  
    return 0;
}


/*
 * This function sends a RTPC RECEIVER REPORT.
 */
static int 
rtcp_send_RR(struct rtp_session *rtp, 
	     struct rtcp_rcv_stats* rrs, 
	     double ia_jitter, 
	     int curr_pktLost, 
	     int curr_pktSeq)
{
    rtcp_common_t rtcp_header;
    rtcp_rr_t     RR_block;
    struct        timeval now, Tdiff;
    char          *p, *databuf;
    int           len=0;
    int           expected_interval;
    double        fraction = 0.0;
    unsigned int  lost;

    /*
     * We shouldn't send anything if no sender has registered with us.
     */
    if (rtp->rtp_ssrc == 0){
	dbg_print(DBG_RTCP, "RTCP send RR: No sender registered yet.\n");
	return 0;
    }

    /* 
     * Calculate the DLSR time from last received sender report SR
     * to point when we send back receiver report RR 
     */
    now = gettimestamp();
    Tdiff = timevalsub(now, rrs->rrs_timestamp_SR);

    /*
     * We need to send back information on the data we have received in a
     * receiver report. First make a rtcp_header with the right type RTCP_RR
     */
    rtcp_set_hdr(&rtcp_header, RTCP_RR, rtp->rtp_ssrc);

    /*
     * OK, build a report block. For format see rtcp.h, build it from
     * the sender struct, we update each time a sender report comes in.
     */
    lost = curr_pktLost - rrs->rrs_prev_pktLost;
    expected_interval = curr_pktSeq - rrs->rrs_prev_pktSeq; 
    if (expected_interval != 0)
	fraction = (double)lost / (double)expected_interval;
    RR_block.ssrc = rtp->rtp_ssrc;
    RR_block.last_seq = curr_pktSeq;
    RR_block.jitter = ms2rtpTimestamp(ia_jitter);

    /* 
     * Calculate the LSR time from a NTP times stamp 
     */
    RR_block.lsr = ntp2lsr(&rrs->rrs_timestamp_SR_ntp);
    RR_block.dlsr = timeval2dlsr(Tdiff); 

    if ((databuf = (char *)smalloc(rtcp_header.length + 1)) == NULL){
	sphone_error("rtcp_send_RR: malloc: %s", strerror(errno));
	return -1;
    }
    p = databuf;

    /* 
     * Make an RTCP header and convert byte order. XXX this works for now
     * because we know the length,  if we have a variable number of
     * report blocks then we have to do this last.
     */
    encode_rtcp_header(&rtcp_header, p);
    len += sizeof(rtcp_common_t);
    p += sizeof(rtcp_common_t);

    encode_report_block(&RR_block, p); 
    len += sizeof(rtcp_rr_t);
    p += sizeof(rtcp_rr_t);

    dbg_print(DBG_RTCP, "Sending: RTCP Receiver report\n");

    /*
     * OK send off the RTCP message to the receiver.
     */
    if (sendto(rtp->rtcp_s, databuf, len, 0x0, 
	       (struct sockaddr *)&rtp->rtcp_dst_addr, 
	       sizeof(rtp->rtcp_dst_addr)) < 0){
	sphone_error("rtcp_send_RR: sendto: %s\n",
		     strerror(errno));
	sfree(databuf);
	return -1;
    }
    sfree(databuf);

    /*
     * Fill in rtcp stats struct.
     */
    rrs->rrs_prev_pktLost = curr_pktLost; /*save 'til next rtcp to send*/
    rrs->rrs_prev_pktSeq = curr_pktSeq;  
    rrs->rrs_RR_pkts++;

    return 0;
}


/* 
 * Send Bye
 */
int
rtcp_send_bye(struct rtp_session *rtp)
{
    rtcp_common_t  rtcp_header;
    int            rtcp_type = RTCP_BYE; 
    char           *p, *databuf;
    int            len=0;

    rtcp_set_hdr(&rtcp_header, rtcp_type, rtp->rtp_ssrc);

    if ((databuf = (char *)smalloc(sizeof(rtcp_header.length + 1))) == NULL){
	sphone_error("rtcp_send_bye: malloc: %s", strerror(errno));
	return -1;
    }
    p = databuf;

    /* 
     * Make an RTCP header and convert byte order. XXX this works for now
     * because we know the length,  if we have a variable number of
     * report blocks then we have to do this last.
     */
    encode_rtcp_header(&rtcp_header, p);
    len += sizeof(rtcp_common_t);

    /*
     * OK send off the RTCP BYE the receiver.
     */
    dbg_print(DBG_RTCP, "Sending: RTCP Bye\n");
    if (sendto(rtp->rtcp_s, databuf, len, 0x0, 
	       (struct sockaddr *)&rtp->rtcp_dst_addr, 
	       sizeof(rtp->rtcp_dst_addr)) < 0){
	sphone_error("rtcp_send_bye: sendto: %s\n",
		     strerror(errno));
	sfree(databuf);
	return -1;
    }
  
    sfree(databuf);

    return 0;
}
 
/*
 * Function called at sender when rtcp message arrives
 */
int 
rtcp_send_send_dispatch(int s, void *arg)
{
    send_session_t *ss = (send_session_t *)arg;
    struct rtp_session *rtp = ss->ss_rtp;
    struct timeval dt;
	
    if (rtcp_send_SR(rtp, 
		     ss->ss_rtcp, 
		     ss->ss_cp_audio->cp_samp_sec, 
		     ss->ss_stats.sst_spkt, 
		     ss->ss_stats.sst_sbytes) < 0)
	return -1;
    dt.tv_sec = RTCP_REPORT_INTERVAL_S;
    dt.tv_usec = 0;
    timeradd(&rtp->rtcp_timer, &dt, &rtp->rtcp_timer);
    if (eventloop_reg_timeout(rtp->rtcp_timer, rtcp_send_send_dispatch, 
			      ss, "RTCP send SR") < 0)
	return -1;

    return 0;
}


/*
 * Function called at receiver when rtcp message arrives
 */
int 
rtcp_rcv_send_dispatch(int s, void *arg)
{
    rcv_session_t *rs = (rcv_session_t *)arg;
    struct rtp_session *rtp = rs->rs_rtp;
    struct timeval dt;

    if (rtcp_send_RR(rtp,
		     rs->rs_rtcp, 
		     0, /* XXX: ps->w_s This is in playout_shbuf */
		     rs->rs_stats.rst_lost_pkts,
		     rs->seq_cur) < 0)
	return -1;
    dt.tv_sec = RTCP_REPORT_INTERVAL_S;
    dt.tv_usec = 0;
    timeradd(&rtp->rtcp_timer, &dt, &rtp->rtcp_timer);
    if (eventloop_reg_timeout(rtp->rtcp_timer, rtcp_rcv_send_dispatch, 
			      rs, "RTCP send RR") < 0)
	return -1;
    return 0;
}

/*
 * A sender sends a SR regularly and registers a callback for RRs.
 */
int 
rtcp_send_init(int s, send_session_t *ss)
{
    struct rtp_session *rtp = ss->ss_rtp;
    struct timeval t, dt;

    t = gettimestamp(); 
    dt.tv_sec = RTCP_REPORT_INTERVAL_S;
    dt.tv_usec = 0;
    timeradd(&t, &dt, &rtp->rtcp_timer);
    if (eventloop_reg_timeout(rtp->rtcp_timer, rtcp_send_send_dispatch, 
			      ss, "RTCP send SR") < 0)
	return -1;
    if (eventloop_reg_fd(s, rtcp_send_rcv_dispatch, ss, "RTCP send receiver") < 0)
	return -1;
    return 0;
}




