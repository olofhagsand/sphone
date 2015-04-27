/*-----------------------------------------------------------------------------
  File:   sphone_rtcp_rcv.h
  Description: Functions for receiving rtcp messages.
  Author: Olof Hagsand
  CVS Version: $Id: sphone_rtcp_rcv.h,v 1.3 2004/01/25 11:01:41 olofh Exp $
 
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


#ifndef _SPHONE_RTCP_RCV_H_
#define _SPHONE_RTCP_RCV_H_

/* 
 * Datatypes
 */
struct rtcp_rcv_stats {
    uint32_t             rrs_RR_pkts;         /* RR packets sent */
    uint32_t             rrs_SR_pkts;         /* SR packets received */

    /* Receiver: to calculate RTT back to sender */
    struct timeval       rrs_timestamp_SR;    /* Arrival time of last SR at rcv */
    struct NTP_timestamp rrs_timestamp_SR_ntp;/* Send time of last SR from sender */

    /* Receiver: received by SR */
    uint32_t rrs_bytes_sent2us;       /* bytes, SR info */
    uint32_t rrs_packets_sent2us;     /* nr packet, SR info  */

    /* Receiver: accumlative how many packets lost, packet seq */
    uint32_t rrs_prev_pktLost; /* internal RR info for bookkeeping */
    uint32_t rrs_prev_pktSeq;

    /* Sender & receiver, but only used by RTCP SR XXX: think they can be moved to rs/ss */
    uint32_t rtcp_stats_bytes;  /* rtp bytes sent by us */
    uint32_t rtcp_stats_pkt;    /* rtp pkts sent by us */
};


/*
 * Prototypes
 */
struct rtcp_rcv_stats *rtcp_rcv_stats_new(void);
int rtcp_rcv_stats_exit(struct rtcp_rcv_stats *rrs);
int rtcp_send_rcv_dispatch(int s, void *arg);
int rtcp_rcv_rcv_dispatch(int s, void *arg);
int rtcp_rcv_init(int s, rcv_session_t*);

#endif  /* _SPHONE_RTCP_RCV_H_ */
