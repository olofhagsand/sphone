/*-----------------------------------------------------------------------------
  File:   sphone_rtp_session.c
  Description: Sphone RTP session
  Author: Olof Hagsand
  CVS Version: $Id: sphone_rtp_session.c,v 1.5 2004/06/20 18:18:21 olofh Exp $
 
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

/*
 * Initialize a sicsophone send session using rtp/inet
 */
int
rtp_send_session_init(struct rtp_session *rtp, char *dst_hostname)
{
    if (inet_init(&rtp->rtp_s, &rtp->rtcp_s) < 0)
	return -1;
    
    if (inet_host2addr(dst_hostname, htons(rtp->rtp_port), &rtp->rtp_rcv_addr) < 0)
	return -1;

    if (inet_host2addr(dst_hostname, htons(rtp->rtcp_rport), &rtp->rtcp_dst_addr) < 0)
	return -1;

    /* Bind rtp send port: in NAPT situations */
    if (rtp->rtp_sport)
	if (inet_bind(rtp->rtp_s, htons(rtp->rtp_sport)) < 0)
	    return -1;

    /* Bind rtcp port */
    if (inet_bind(rtp->rtcp_s, htons(rtp->rtcp_sport)) < 0)
	return -1;
    return 0;
}

/*
 * Initialize a sicsophone receive session using rtp/inet
 */
int
rtp_rcv_session_init(struct rtp_session *rtp)
{
    if (inet_init(&rtp->rtp_s, &rtp->rtcp_s) < 0)
	return -1;

    memset(&rtp->rtcp_dst_addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
    rtp->rtcp_dst_addr.sin_len = sizeof(struct sockaddr_in);
#endif
    rtp->rtcp_dst_addr.sin_family = AF_INET;
    rtp->rtcp_dst_addr.sin_port = htons(rtp->rtcp_sport);

    /* Bind rtp rcv port */
    if (inet_bind(rtp->rtp_s, htons(rtp->rtp_port)) < 0)
	return -1;

    /* Bind rtcp port */
    if (inet_bind(rtp->rtcp_s, htons(rtp->rtcp_rport)) < 0)
	return -1;
    return 0;
}

int 
rtp_session_exit(struct rtp_session *rtp)
{
    if (rtp->rtp_s)
	close(rtp->rtp_s);
    if (rtp->rtcp_s)
	close(rtp->rtcp_s);
    sfree(rtp);
    return 0;
}

struct rtp_session*
rtp_session_new()
{
    struct rtp_session *rtp;

    rtp = (void *)smalloc(sizeof(struct rtp_session));
    if (rtp == NULL){
	sphone_error("rtp_session_init: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(rtp, 0, sizeof(struct rtp_session));
    /* Exit function registered by rcv_session or send_session */
    return rtp;
}
