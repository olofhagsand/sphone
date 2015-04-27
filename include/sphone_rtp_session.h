/*-----------------------------------------------------------------------------
  File:   sphone_rtp_session.h
  Description: rtp session data - common for sender and receiver
  Author: Olof Hagsand, Kjell Hansson, Emmanuel Frecon
  CVS Version: $Id: sphone_rtp_session.h,v 1.5 2004/06/20 18:18:43 olofh Exp $
 
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

#ifndef _SPHONE_RTP_SESSION_H_
#define _SPHONE_RTP_SESSION_H_

/*
 * Typedefs
 */

struct rtp_session{
    int      rtp_s;       /* rtp data socket */
    uint16_t rtp_port;    /* UDP/RTP receive (dst) port (host byte order) */
    uint16_t rtp_sport;   /* UDP/RTP send (src) port - for NAPTs */
    uint8_t  rtp_payload; /* RTP payload */
    uint32_t rtp_ssrc;        /* sending src id */
    struct sockaddr_in rtp_rcv_addr; /* destination address */

    int      rtcp_s;      /* rtcp control socket */
    uint16_t rtcp_rport;  /* UDP/RTCP receive (dst) port (host byte order) */
    uint16_t rtcp_sport;  /* UDP/RTCP senders (own) port (host byte order) */
    struct sockaddr_in rtcp_dst_addr; /* rtcp other-side address */
    struct timeval rtcp_timer; /* Send timer */
};

/*
 * Prototypes
 */
int rtp_session_exit(struct rtp_session *rtp);
int rtp_rcv_session_init(struct rtp_session *rtp);
int rtp_send_session_init(struct rtp_session *rtp, char *dst_hostname);
struct rtp_session* rtp_session_new(void);

#endif /* _SPHONE_RTP_SESSION_H_ */




