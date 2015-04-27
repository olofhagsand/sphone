/*-----------------------------------------------------------------------------
  File:   sphone_rtp.h
  Description: Common internal .h file. All C-files in sphone should include 
  this file.
  Author: Olof Hagsand, Kjell Hansson, Emmanuel Frecon
  CVS Version: $Id: sphone_rtp.h,v 1.6 2004/06/25 09:25:45 olofh Exp $
 
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

  Taken from my sid2 code, with Emmanuels rtp2/sid extensions, 
  see also rfc 1890, etc.

  [From rfc1889:]
      The RTP header has the following format:

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   The first twelve octets are present in every RTP packet, while the
   list of CSRC identifiers is present only when inserted by a mixer.

   ---
   The NTP timestamp is an int with resolution 15.25 usecs, so that
   65536 corresponds to 1 second.

   Encodings taken from RFC 3551
 *---------------------------------------------------------------------------*/

#ifndef _SPHONE_RTP_H_
#define _SPHONE_RTP_H_

/*
 * Constants
 */
#define RTCP_REPORT_INTERVAL_S   5  /* in seconds */

/* RTP v2 Constants, taken from vic, vat, vcr, etc. */
#define RTP_PT_BVC		22	/* Berkeley video codec */

/* RTP standard content encodings for video */
#define RTP_PT_RGB8		23 	/* 8-bit dithered RGB */
#define RTP_PT_HDCC		24 	/* SGI proprietary */
#define RTP_PT_CELLB		25 	/* Sun CellB */
#define RTP_PT_JPEG		26	/* JPEG */
#define RTP_PT_CUSEEME		27	/* Cornell CU-SeeMe */
#define RTP_PT_NV		28	/* Xerox PARC nv */
#define RTP_PT_PICW		29	/* BB&N PictureWindow */
#define RTP_PT_CPV		30	/* Concept/Bolter/Viewpoint codec */
#define RTP_PT_H261		31	/* ITU H.261 */
#define RTP_PT_MPEG		32 	/* MPEG-I & MPEG-II */
#define RTP_PT_MP2T		33 	/* MPEG-II either audio or video */

/* backward compat hack for decoding RTPv1 ivs streams */
#define RTP_PT_H261_COMPAT 127

/* RTP standard content encodings for audio */
#define RTP_PT_PCMU		0
#define RTP_PT_CELP		1
#define RTP_PT_GSM_610	        3 
#define RTP_PT_G723		4 
#define RTP_PT_DVI		5
#define RTP_PT_LPC		7
#define RTP_PT_PCMA		8 
#define RTP_PT_G722		9 
#define RTP_PT_L16		11 
#define RTP_PT_G728		15 
#define RTP_PT_G729		18 

/* RTP NON standard content encodings for audio*/
#define RTP_PT_DAMPS_EFR 96
#define RTP_PT_GSM_EFR   97 

#define RTP_PT_DYNAMIC 101 /* dynamic according to RFC 1890 */


#define RTP_VERSION 2   /* Version number we implement */

/* Bit operators to access the flags in an RTP header */
#define RTP_V   0xC000  /* Version number */
#define RTP_P	0x2000	/* Padding is present */
#define RTP_X	0x1000	/* Extension Header is present */
#define RTP_CC  0x0F00  /* Number of CSRC */
#define RTP_M	0x0080	/* Marker: significant event <e.g. frame boundary> */
#define RTP_PT  0x007F  /* Payload type */

#define rtp_get_v(flags)   (((flags) & RTP_V) >> 14)
#define rtp_get_p(flags)   (((flags) & RTP_P) >> 13)
#define rtp_get_x(flags)   (((flags) & RTP_X) >> 12)
#define rtp_get_cc(flags)  (((flags) & RTP_CC) >> 8)
#define rtp_get_m(flags)   (((flags) & RTP_M) >> 7)
#define rtp_get_pt(flags)  ((flags) & RTP_PT)

#define rtp_clean_v(flags)   (flags) &= ~RTP_V
#define rtp_clean_p(flags)   (flags) &= ~RTP_P
#define rtp_clean_x(flags)   (flags) &= ~RTP_X
#define rtp_clean_cc(flags)  (flags) &= ~RTP_CC
#define rtp_clean_m(flags)   (flags) &= ~RTP_M
#define rtp_clean_pt(flags)  (flags) &= ~RTP_PT

#define rtp_set_v(flags, v) rtp_clean_v(flags); (flags) |= (((v) << 14) & RTP_V)
#define rtp_set_p(flags, p) rtp_clean_p(flags); (flags) |= (((p) << 13) & RTP_P)
#define rtp_set_x(flags, x) rtp_clean_x(flags); (flags) |= (((x) << 12) & RTP_X)
#define rtp_set_cc(flags, cc) rtp_clean_cc(flags); (flags) |= (((cc) << 8) & RTP_CC)
#define rtp_set_m(flags, m) rtp_clean_m(flags); (flags) |= (((m) << 7) & RTP_M)
#define rtp_set_pt(flags, pt) rtp_clean_pt(flags); (flags) |= ((pt) & RTP_PT)

/* Macros to compare sequence numbers */
#define	SEQ_LT(a,b)	((int)((a)-(b)) < 0)
#define	SEQ_LEQ(a,b)	((int)((a)-(b)) <= 0)
#define	SEQ_GT(a,b)	((int)((a)-(b)) > 0)
#define	SEQ_GEQ(a,b)	((int)((a)-(b)) >= 0)

#define RTP_SEQ_MOD (1<<16)

/* 
 * Datatypes
 */

/*
 * Timestamps used in rtp
 */
struct NTP_timestamp {
     uint16_t sec;
     uint16_t frac;
};
typedef struct NTP_timestamp NTP_timestamp;

struct NTP_timestamp_64 {
     uint32_t sec;
     uint32_t frac;
};


struct rtp_header {
    uint16_t flags;
    uint16_t seq;
    NTP_timestamp ts;
    uint32_t ssrc;
};
typedef struct rtp_header rtp_header;

/* 
 * Global variables 
 */
extern uint32_t _ssrc;
extern uint16_t _seq;

/* 
 * Prototypes
 */
/* NTP 32-bit timestamp to struct timeval */
void NTP2TV(struct NTP_timestamp *NTP, struct timeval *TV);

/* struct timeval to NTP 32-bit timestamp */
void TV2NTP(struct timeval *TV, struct NTP_timestamp *NTP);

/* struct timeval to NTP 64-bit timestamp */
void TV2NTP_64(struct timeval *TV, struct NTP_timestamp_64 *NTP);

/* Translate NTP timestamp to floating point seconds */
double NTP2sec(struct NTP_timestamp *NTP);

/* Pretty print an rtp header and return in STATIC string (NB make copy) */
char *rtp_print_hdr(rtp_header *hdr);

int rtp_init(uint32_t *ssrc);

/* Encode RTP header: generated from from Assar Ws ydr */
char *marshal_rtp_hdr(rtp_header *o, char *ptr);

/* Decode RTP header: generated from from Assar Ws ydr */
char *unmarshal_rtp_hdr(rtp_header *o, char *ptr);

/* Fill in the sending of an rtp header */
void rtp_set_hdr(rtp_header *hdr, struct timeval *T, uint32_t ssrc);

/* Check version and payload type */
int rtp_check_hdr(rtp_header *hdr);

/* random 32-bit quantity: type used as seed */
unsigned int random_ssrc(int type);

#endif /* _SPHONE_RTP_H_ */




