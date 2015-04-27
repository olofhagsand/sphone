/*-----------------------------------------------------------------------------
  File:   sphone_send.h
  Description: Sphone sender file
  Author: Olof Hagsand
  CVS Version: $Id: sphone_send.h,v 1.11 2004/01/25 11:01:41 olofh Exp $
 
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


#ifndef _SPHONE_SEND_H_
#define _SPHONE_SEND_H_


/*
 * Datatypes
 */ 
/*
 * Sending statistics
 */
struct send_stats{
    uint32_t sst_sbytes;        /* Bytes sent by us */
    uint32_t sst_spkt;          /* Packets sent by us */
    uint32_t sst_rbytes;        /* Bytes recorded by us */
    uint32_t sst_rpkt;          /* Packets recorded by us */
};

struct send_session{
    struct rtp_session     *ss_rtp;     /* rtp/rtcp data */
    struct rtcp_send_stats *ss_rtcp;    /* rtcp send statistics */
    record_api *ss_record_api;          /* Audio record API */
    coding_api             *ss_coding_api;/* Coding API */
    struct send_stats      ss_stats;    /* Statistics */
    coding_params  *ss_cp_audio;  /* Setting at recording-before coding */
    coding_params  *ss_cp_nw; /* Settings on wire - after coding */

    uint32_t ss_seq;         /* nr of packet */
    int      ss_silence_detection; /* Silence detection on */
    int      ss_silence_nr;  /* Nr of silent pkts before supression */
    int      ss_silence_threshold; /* Nr of silent pkts before supression */
};
typedef struct send_session send_session_t;

/*
 * Prototypes
 */ 
send_session_t *send_session_new(void);
int record_and_send(char *databuf, int datalen, void *arg);

#endif  /* _SPHONE_SEND_H_ */
