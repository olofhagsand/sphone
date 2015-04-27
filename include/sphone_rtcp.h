/*-----------------------------------------------------------------------------
  File:   sphone_rtcp.h
  Description: Common internal .h file. All C-files in sphone should include 
  this file.
  Author: Kjell Hansson, Ian Marsh, Olof Hagsand
  CVS Version: $Id: sphone_rtcp.h,v 1.6 2004/01/11 20:23:10 olofh Exp $
 
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

 *-------------------------------------------------------------------*/

/*                          RTCP Header
 *
 *  1               2               3               4                             
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1               
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+              
 * |V=2|P|    RC   |   PT=SR=200   |             length            | header       
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+              
 * |                         SSRC of sender                        |              
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 *
 *                           Sender info
 *
 *
 *
 *
 *
 *
 *
 *                           Receiver info
 *
 */

#ifndef _SPHONE_RTCP_H_
#define _SPHONE_RTCP_H_

/* 
 * Datatypes
 */
#define  RTCP_SR    200
#define  RTCP_RR    201
#define  RTCP_SDES  202
#define  RTCP_BYE   203
#define  RTCP_APP   204

#define RTP_VERSION    2
#define RTP_SEQ_MOD (1<<16)

#define RTCP_V	               0xC000	               /* Version          */
#define RTCP_P	               0x2000	               /* Padding          */
#define RTCP_RC	               0x1F00	               /* Reception report */
#define RTCP_T	               0x00FF	               /* RTCP pkt type    */
#define RTCP_LOST_FRAC         0xFF000000              /* First byte */
#define RTCP_LOST_CULM         0x00FFFFFF              /* Three tail bytes */

#define rtcp_get_v(flags)   (((flags) & RTCP_V) >> 14)
#define rtcp_get_p(flags)   (((flags) & RTCP_P) >> 13)
#define rtcp_get_rc(flags)  (((flags) & RTCP_RC) >> 8)
#define rtcp_get_t(flags)   ((flags)  & RTCP_T)

#define rtcp_get_lost_frac(val)  (((val) & RTCP_LOST_FRAC) >> 24)
#define rtcp_get_lost_culm(val)  ((val)  & RTCP_LOST_CULM) 

#define rtcp_clean_v(flags)          (flags) &= ~RTCP_V
#define rtcp_clean_p(flags)          (flags) &= ~RTCP_P
#define rtcp_clean_rc(flags)         (flags) &= ~RTCP_RC
#define rtcp_clean_t(flags)          (flags) &= ~RTCP_T
#define rtcp_clean_lost_frac(flags)  (flags) &= ~RTCP_LOST_FRAC
#define rtcp_clean_lost_culm(flags)  (flags) &= ~RTCP_LOST_CULM

#define rtcp_set_v(flags, v)    rtcp_clean_v(flags);  (flags) |= (((v)  << 14) & RTCP_V)
#define rtcp_set_p(flags, p)    rtcp_clean_p(flags);  (flags) |= (((p)  << 13) & RTCP_P)
#define rtcp_set_rc(flags, rc)  rtcp_clean_rc(flags); (flags) |= (((rc) <<  8) & RTCP_RC)
#define rtcp_set_t(flags, t)    rtcp_clean_t(flags);  (flags) |= (((t) & RTCP_T))

#define rtcp_set_lost_frac(val, frac) rtcp_clean_lost_frac(val); (val) |= (((frac) << 24) & RTCP_LOST_FRAC)
#define rtcp_set_lost_culm(val, culm) rtcp_clean_lost_culm(val); (val) |= ((culm) & RTCP_LOST_CULM)

#define RTCP_VALID_VALUE ((RTP_VERSION << 14) | RTCP_SR)

typedef struct {
    uint16_t flags;    /* version:2 padding:1 count:5 packet type:8 */
    uint16_t length;   /* pkt len in 32 bit words -1 (see spec why) */
    uint32_t ssrc;     /* this source */
} rtcp_common_t;


/*
 * Reception report block
 */
typedef struct rtcp_rr {
    uint32_t ssrc;             /* data source being reported */
    int32_t   lost;             /* unsigned :8 fraction lost since last SR/RR 
				 signed :24  
				 XXX could this be a bug ? Not sure Ian
				*/
    uint32_t last_seq;         /* extended last seq. no. received */
    uint32_t jitter;           /* interarrival jitter */
    uint32_t lsr;              /* last SR packet from this source */
    uint32_t dlsr;             /* delay since last SR packet */
} rtcp_rr_t;


/*
 * Sender info
 */
typedef struct {
    uint32_t ntp_sec;     /* NTP timestamp integer */
    uint32_t ntp_frac;    /* NTP timestamp fractional part */
    uint32_t rtp_ts;      /* RTP timestamp */
    uint32_t psent;       /* packets sent */
    uint32_t osent;       /* octets sent */
    rtcp_rr_t rr[0];      /* report block must be present but can be zero*/
} rtcp_si_t;


/* 
 * source description (SDES) 
 */
struct rtcp_sdes {
    uint32_t          src;      /* first SSRC/CSRC */
    /*  rtcp_sdes_item_t item[1];  list of SDES items */
} sdes;

/*
 * BYE message format
 */
typedef struct bye{
    uint32_t src[1];   /* list of sources */
} rtcp_bye_t;

/*
 * Prototypes
 */
char *rtcp_print_type(int type);

/*
 * Set header, initialise rtcp source struct.
 */
void  rtcp_set_hdr(rtcp_common_t *hdr, int packet_type, uint32_t ssrc);

/*
void  rtcp_update_jitter_calc(struct timeval now, int curr_audioseq);
*/

uint32_t rtpTimestamp2ms(uint32_t ts);
uint32_t calculateRTT(uint32_t lsr, uint32_t dlsr, struct timeval t);
uint32_t ms2rtpTimestamp(double ms);
uint32_t ntp2lsr(NTP_timestamp *ntp);
/*uint32_t timeval2rtptimestamp(struct timeval t);*/
uint32_t timeval2dlsr(struct timeval t);

/*
 * Byte ordering utility functions
 */
char *encode_rtcp_header(rtcp_common_t *o, char *ptr);
char *encode_sender_info(rtcp_si_t *o, char *ptr);
char *encode_report_block(rtcp_rr_t *o, char *ptr);

char *decode_rtcp_header(rtcp_common_t *o, char *ptr); 
char *decode_sender_info(rtcp_si_t *o, char *ptr);
char *decode_report_block(rtcp_rr_t *o, char *ptr);

#endif /* _SPHONE_RTCP_H_ */



