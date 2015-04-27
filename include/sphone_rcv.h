/*-----------------------------------------------------------------------------
  File:   sphone_rcv.h
  Description: Sphone rcv file
  Author: Olof Hagsand
  CVS Version: $Id: sphone_rcv.h,v 1.16 2005/02/13 17:19:31 olof Exp $
 
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


#ifndef _SPHONE_RCV_H_
#define _SPHONE_RCV_H_


/*
 * Datatypes
 */ 
/*
 * Includes data specitic to one packet.
 */
struct packet_context{
    seq_t    pc_seq;        /* rtp sequence number for packet */
    int      pc_ts;         /* rtp timestamp for packet */
};

typedef struct packet_context pkt_ctx;

/*
 * Receive statistics
 */
struct rcv_stats{
    uint32_t rst_rbytes;        /* Bytes received by us */
    uint32_t rst_rpkt;          /* Packets received by us */
    uint32_t rst_pbytes;        /* Bytes played by us */
    uint32_t rst_ppkt;          /* Packets played by us */

    uint32_t rst_lost_pkts;    /* Lost packets */
};

struct rcv_session{
    struct rtp_session    *rs_rtp;  /* rtp/rtcp data */
    struct rtcp_rcv_stats *rs_rtcp; /* rtcp receive statistics */
    play_api *rs_play_api;          /* Audio play API */
    coding_api *rs_coding_api;      /* Coding API */
    struct rcv_stats rs_stats;      /* Statistics */
    coding_params *rs_cp_audio;  /* Settings at playout-after decoding*/
    coding_params *rs_cp_nw;     /* Settings on wire -before decoding*/
    FILE          *rs_logf;      /* Log file, if set */

    int rs_accept_all_ssrc;      /* If set, dynamically accept all ssrcs */

    int      rs_playout_type;       /* Type of playout: fifo or shbuf */
    void     *rs_playout;           /* Playout-specific data */

    seq_t    seq_initial;
    seq_t    seq_bad;         /* for detecting restarts */
    int      seq_cycles;      /* wrapping around */
    pkt_ctx  rs_cur_pkt;      /* Data about current packet */
    pkt_ctx  rs_prev_pkt;     /* Data about previous packet */
};
typedef struct rcv_session rcv_session_t;


#define ts_cur rs_cur_pkt.pc_ts
#define ts_prev rs_prev_pkt.pc_ts
#define seq_cur rs_cur_pkt.pc_seq
#define seq_prev rs_prev_pkt.pc_seq


/*
 * Return type of update_seq
 */
enum seq_type {
    SEQ_NORMAL,                /* seq nr OK */
    SEQ_GAP,                   /* Gap in sequence # (lost packet) */
    SEQ_RESET,                 /* After bad: a probable reset */
    SEQ_BAD,                   /* Very large difference in seq #: bad */
    SEQ_DUP,                   /* Duplicate sequence number */
    SEQ_TALK_START,            /* Silence period ended */
};

/*
 * Prototypes
 */ 
rcv_session_t *rcv_session_new(void);
int rcv_start(rcv_session_t *rs);
char *seq2str(enum seq_type);

#endif  /* _SPHONE_RCV_H_ */
