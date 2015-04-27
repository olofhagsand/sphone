/*-----------------------------------------------------------------------------
  File:   sphone_rtcp_send.h
  Description: Functions for sending rtcp messages.
  Author: Olof Hagsand
  CVS Version: $Id: sphone_rtcp_send.h,v 1.3 2004/01/25 11:01:41 olofh Exp $
 
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


#ifndef _SPHONE_RTCP_SEND_H_
#define _SPHONE_RTCP_SEND_H_


/* 
 * Datatypes
 */
struct rtcp_send_stats {
    uint32_t             rss_RR_pkts;         /* RR packets sent */
    uint32_t             rss_SR_pkts;         /* SR packets received */

    /* Sender: received by RR */
    uint32_t rss_peer_fraction_lost;     /* %      */
    uint32_t rss_peer_jitter;     /* ms      */
    uint32_t rss_RTT_delay;           /* ms      */
    uint32_t rss_peer_culm_lost;     /* number packets lost at peer */
    uint32_t rss_peer_last_seq;     /* last packet seq nr     */
};


/*
 * Prototypes
 */
int rtcp_send_stats_exit(struct rtcp_send_stats *rss);
struct rtcp_send_stats *rtcp_send_stats_new(void);
int rtcp_rcv_send_dispatch(int s, void *arg);
int rtcp_send_send_dispatch(int s, void *arg);
int rtcp_send_init(int s, send_session_t*);

#endif  /* _SPHONE_RTCP_SEND_H_ */
