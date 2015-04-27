/*-----------------------------------------------------------------------------
  File:   spc_rcv.c
  Description: Sicsophone client application: receiver side
  Author: Olof Hagsand
  CVS Version: $Id: spc_rcv.c,v 1.20 2005/01/12 18:13:04 olof Exp $
 
  This software is a part of SICSPHONE, a real-time, IP-based system for 
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

#ifdef HAVE_CONFIG_H
#include "config.h" /* generated by config & autoconf */
#endif

#include "sphone.h" 

#include "sp.h"
#include "spc.h"
#include "spc_rcv.h"

static int
rcv_timeout(int dummy, void *arg)
{
    struct sp_rcv_session *rs = (struct sp_rcv_session*)arg;
    struct spc_info *spc = rs->rs_spc;
    int retval;

    if (spc->sp_state != SS_DATA_RCV)
	return 0;
    retval = send_rcv_report(spc, &rs->rs_hdr);
    spc->sp_data = NULL;
    sfree(rs);
    dbg_print(DBG_APP, "State: %s -> ", state2str(spc->sp_state));
    spc->sp_state = SS_SERVER;
    dbg_print(DBG_APP, "%s\n", state2str(spc->sp_state));
    return retval;
}

int
start_receiver(struct spc_info *spc, struct sp_proto *sp)
{
    struct timeval t;
    struct sp_rcv_session *rs;

    assert(spc->sp_state == SS_SERVER);
    if ((rs = (struct sp_rcv_session *)smalloc(sizeof(*rs))) == NULL){
	perror("start_receiver: malloc");
	return -1;
    }
    memset(rs, 0, sizeof(*rs));
    spc->sp_data = rs;
    rs->rs_testid          = ntohl(sp->sp_reporthdr.ss_testid);
    rs->rs_duration.tv_sec = ntohl(sp->sp_starthdr.ss_time_duration.tv_sec);
    rs->rs_duration.tv_usec = ntohl(sp->sp_starthdr.ss_time_duration.tv_usec);
    rs->rs_duration.tv_sec += 5; /* Some extra time to wait for data */
    rs->rs_spc             = spc;
    memcpy(&rs->rs_hdr, &sp->sp_starthdr, sizeof(struct sdhdr_start));
    sp_addr_decode(&rs->rs_srcaddr, 
		   (char*)&sp->sp_starthdr.ss_addr_src_pub);
    dbg_print(DBG_RCV, "start_receiver %d from: %s:%d\n", 
	      rs->rs_testid,
	      inet_ntoa(rs->rs_srcaddr.sin_addr),
	      ntohs(rs->rs_srcaddr.sin_port));
    dbg_print(DBG_APP, "State: %s -> ", state2str(spc->sp_state));
    spc->sp_state = SS_DATA_RCV;
    dbg_print(DBG_APP, "%s%d\n", state2str(spc->sp_state), rs->rs_testid);
    rs->rs_start = gettimestamp();
    timeradd(&rs->rs_start, &rs->rs_duration, &t);
    rs->rs_duration.tv_sec += 5; /* give extra time to wait for allpackets */
    if (eventloop_reg_timeout(t, rcv_timeout, rs, 
			      "sicsophone data receiver timeout") < 0)
	return -1;
    return 0;
}

int
rcv_data_input(int s, struct spc_info *spc, struct sp_rcv_session *rs)
{
    static char buf[1500];
    char *p;
    static rtp_header hdr;            /* RTP header */
    struct sockaddr_in from;
    int len=sizeof(buf);
    struct timeval now;

    assert(spc->sp_state == SS_DATA_RCV);
    if (inet_input(s, buf, &len, &from) < 0)
	return 0;
    now = gettimestamp();
    p = buf;
    unmarshal_rtp_hdr(&hdr, buf);
    if (rtp_check_hdr(&hdr) < 0)
	return 0; /* Ignore */
    dbg_print(DBG_RCV, "rcv_data_input seq:%d\n", hdr.seq);

    /* Echo the message */
    if ((sendto(s, buf, len, 0x0, 
		(struct sockaddr*)&from, 
		sizeof(struct sockaddr_in))) < 0){
	perror("rcv_data_input: sendto");
	return -1;
    }

    return 0;
}

/*
 * send_rcv_report
 * Just say that the receiver is done.
 */
int
send_rcv_report(struct spc_info *spc, struct sdhdr_start *hdr_orig)
{
    struct sp_proto sp;

    dbg_print(DBG_APP, "spc: send rcv log to server\n");
    /* Most of the header is same: copy and fill in differing fields */
    memset(&sp, 0, sizeof(sp));
    sp.sp_magic = SP_MAGIC;
    sp.sp_type  = SP_TYPE_REPORT_RCV;
    sp.sp_version = htons(SP_VERSION_NR);
    sp.sp_len   = htonl(sizeof(sp)); 
    sp.sp_id    = htonl(spc->sp_id); 
    /* original session hdr */
    memcpy(&sp.sp_reporthdr, hdr_orig, sizeof(*hdr_orig));
    dbg_print_pkt(DBG_RCV|DBG_DETAIL, "send_rcv_report",
		  (char*)&sp, sizeof(sp));
    if (send(spc->sp_s, (void*)&sp, sizeof(sp), 0x0) < 0){
	perror("send_rcv_report: send");
	return -1;
    }
    return 0;
}
