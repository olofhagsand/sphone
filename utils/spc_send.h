/*-----------------------------------------------------------------------------
  File:   spc_send.h
  Description: sphone server - client side - send data
  this file.
  Author: Olof Hagsand
  CVS Version: $Id: spc_send.h,v 1.7 2004/12/29 16:54:46 olof Exp $
 
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


#ifndef _SPC_SEND_H_
#define _SPC_SEND_H_

/*
 * Constants
 */
#define SPC_SEND_TIMEOUT_S 3 /* If no echoes, give up after this time (s) */
/*
 * Types
 */
struct sp_send_session{
    uint32_t             ss_testid;   /* Number of test (given by server) */
    struct timeval       ss_duration; /* How long to send */
    struct sockaddr_in   ss_dstaddr;  /* To where */
    struct timeval       ss_t0;       /* Start time */
    struct sdhdr_start   ss_hdr;      /* sp hdr used for report */
    struct spc_info     *ss_spc;     /* backpointer to spc struct */
    uint8_t              ss_rtp_payload; /* RTP payload */
    uint32_t             ss_rtp_ssrc; /* sending src id */
    uint32_t             ss_seq;      /* nr of packet */
    uint32_t             ss_rcvd;     /* nr of received echoes */
    struct coding_params ss_coding; /* audio coding parameters */
    char                 ss_logfile[SPC_LOGFILE_LEN];/* name of logfile */
    FILE                *ss_logf;     /* log file handler */
};

/*
 * Prototypes
 */
int start_sender(struct spc_info *spc, struct sp_proto *sp);
int send_data_input(int s, struct spc_info *spc, struct sp_send_session *ss);
int send_send_report(struct spc_info *spc, struct sdhdr_start *hdr_orig, char *logfile);

#endif /* _SPC_SEND_H_ */


