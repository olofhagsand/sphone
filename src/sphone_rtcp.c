/*-----------------------------------------------------------------------------
  File:   sphone_rtcp.c
  Description: Common internal .h file. All C-files in sphone should include 
  this file.
  Author: Kjell Hansson, Ian Marsh, Olof Hagsand
  CVS Version: $Id: sphone_rtcp.c,v 1.6 2004/02/01 21:42:47 olofh Exp $
 
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

 *-----------------------------------------------------------------------------*/
#include <stdio.h>       /* printf() */
#include <sys/types.h>   /* u_long */

#include "sphone.h" 
#include "sphone_lib.h" 
#include "sphone_rtp.h"
#include "sphone_rtcp.h"


const int MIN_SEQUENTIAL = 1;

static uint32_t timeval2rtptimestamp(struct timeval t);

char *
rtcp_print_type(int type)
{
    switch (type) {
    case RTCP_SR:
	return "Sender report";
	break;
    case RTCP_RR:
	return "Receiver report";
	break;
    case RTCP_SDES:
	return "Source Description";
	break;
    case RTCP_BYE:
	return "Bye";
	break;
    case RTCP_APP:
	return "application defined";
	break;
    default:
	return "packet not known";
    }
}

/* 
   convert milliseconds to a RTP timestamp
*/ 
uint32_t 
ms2rtpTimestamp(double ms)
{
    uint32_t sec = ((int)(ms / 1000.0)); /* store seconds */;
    ms -= (double)sec * 1000.0;  /* remove seconds */
    return (sec << 16) + (uint32_t)(ms  * 65.536);
}

/* 
   convert a RTP timestamp to milliseconds
*/ 
uint32_t rtpTimestamp2ms(uint32_t ts)
{
    uint32_t t1=0;
    t1 = ((ts & 0xffff0000) >> 16)*1000; /* seconds to ms */
    t1+= (unsigned)(0.5 +(double)(ts & 0x0000ffff)/65.536); /*fractions to ms */
    return t1;  
}

/* 
   calculate the RTT from LSR, DLSR and a timeval timestamp
   at reception of RR
*/ 

uint32_t 
calculateRTT(uint32_t lsr, uint32_t dlsr, struct timeval t)
{
    return rtpTimestamp2ms(timeval2rtptimestamp(t)) - 
	rtpTimestamp2ms(lsr) - rtpTimestamp2ms(dlsr);  
}

/* Time from receiver received a SR to return RR to sender 
   tv_sec : number of seconds                -->> seconds
   tv_usec : number of microsec(1000000 max) -->> 1/65536 of seconds
*/ 
uint32_t timeval2dlsr(struct timeval t)
{
    uint32_t t1=0, t2=0;

    t1 = (t.tv_sec & 0xffff); /* seconds */
    t1 = t1 << 16;
    t2 = (unsigned)(((double)t.tv_usec)*0.065536); /* <= 65536 */
    return t1+t2;  
}

/* NTP 64-bit timestamp to LSR, middle 32 bit's  
   NTP->sec: |--------///LSR//| seconds
   NTP->frac |///LSR//--------| 1/65636 of second*/ 
uint32_t ntp2lsr(NTP_timestamp *ntp)
{
    uint32_t t1, t2;

    t1 = (ntp->sec & 0xffff); /* seconds */
    t1 = t1 << 16;
    t2 = (ntp->frac & 0xffff);
    return t1+t2;
}

/*
* 
*/
uint32_t timeval2rtptimestamp(struct timeval t)
{
    return timeval2dlsr(t);
}

/*
* 
*/
uint32_t ntp2rtptimestamp(NTP_timestamp *ntp)
{
    return ntp2lsr(ntp);
}


void
ntp2rtp(struct NTP_timestamp_64 *NTP, uint32_t start_rtp, uint32_t *rtp_now) 
{

    /*
     * First we need to take out the middle 32 bits of the timestamp
     * LSB 16 bits of the secs and the MSB 16 bits of the frac
     */
    uint32_t temp_sec, temp_usec;

    temp_sec = (NTP->sec) & 0x0000ffff;
    temp_usec =  NTP->frac & 0xffff0000;
    *rtp_now += temp_sec + (temp_usec >> 16) + start_rtp;
}

void 
rtcp_set_hdr(rtcp_common_t *hdr, int packet_type, uint32_t ssrc)
{

    /* 
     * Fill in RTCP sender report: 
     * Version = 2, Padding 0, Reception count = 0, type (passed in)
     */
    hdr->flags = 0x0;
    rtcp_set_v(hdr->flags, RTP_VERSION);
    rtcp_set_p(hdr->flags, 0);
    rtcp_set_rc(hdr->flags, 1); 
    rtcp_set_t(hdr->flags, packet_type); 

    /*
     * How we set the lengths in the header depends on the type
     * of course.
     */
    switch (packet_type) {
    case RTCP_SR:
	hdr->length = sizeof(rtcp_common_t) + sizeof(rtcp_si_t) - 1;
	break;
    case RTCP_RR:
	hdr->length = sizeof(rtcp_common_t) + sizeof(rtcp_rr_t) - 1;
	break;
    case RTCP_BYE:
	hdr->length = sizeof(rtcp_common_t) + sizeof(rtcp_bye_t) - 1;
	break;
    }

    /*
     * This senders unique identifier
     */
    hdr->ssrc = ssrc;
}


/* 
 * Encode RTCP header: generated from Assar Ws ydr 
 */

char 
*encode_rtcp_header(rtcp_common_t *o, char *ptr)
{
    { uint16_t tmp = htons((*o).flags); memcpy(ptr, (char*)&tmp, sizeof(uint16_t)); ptr += sizeof(uint16_t);}
    { uint16_t tmp = htons((*o).length); memcpy(ptr, (char*)&tmp, sizeof(uint16_t)); ptr += sizeof(uint16_t);}
    { uint32_t tmp = htonl((*o).ssrc); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}

    return ptr;
}

char *
encode_sender_info(rtcp_si_t *o, char *ptr)
{
    { uint32_t tmp = htonl((*o).ntp_sec); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).ntp_frac); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).rtp_ts); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).psent); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).osent); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    return ptr;
}

char *
encode_report_block(rtcp_rr_t *o, char *ptr)
{
    { uint32_t tmp = htonl((*o).ssrc); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).lost); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).last_seq); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).jitter); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).lsr); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    { uint32_t tmp = htonl((*o).dlsr); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}

    return ptr;
}

char  
*encode_rtcp_bye_message(rtcp_common_t *o, char *ptr)
{
    { uint16_t tmp = htons((*o).flags); memcpy(ptr, (char*)&tmp, sizeof(uint16_t)); ptr += sizeof(uint16_t);}
    { uint16_t tmp = htons((*o).length); memcpy(ptr, (char*)&tmp, sizeof(uint16_t)); ptr += sizeof(uint16_t);}
    { uint32_t tmp = htonl((*o).ssrc); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}

    return ptr;
}


/* 
 * Decode RTCP header: 
 */

char 
*decode_rtcp_header(rtcp_common_t *o, char *ptr)
{
    { uint16_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint16_t)); (*o).flags = ntohs(tmp); ptr += sizeof(uint16_t);}
    { uint16_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint16_t)); (*o).length = ntohs(tmp); ptr += sizeof(uint16_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).ssrc = ntohl(tmp); ptr += sizeof(uint32_t);}
    return ptr;
}

char 
*decode_sender_info(rtcp_si_t *o, char *ptr)
{
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).ntp_sec = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).ntp_frac = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).rtp_ts = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).psent = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).osent = ntohl(tmp); ptr += sizeof(uint32_t);}

    return ptr;
}

char 
*decode_report_block(rtcp_rr_t *o, char *ptr)
{
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).ssrc = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).lost = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).last_seq = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).jitter = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).lsr = ntohl(tmp); ptr += sizeof(uint32_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).dlsr = ntohl(tmp); ptr += sizeof(uint32_t);}

    return ptr;
}

char 
*decode_rtcp_bye_message(rtcp_common_t *o, char *ptr)
{
    { uint16_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint16_t)); (*o).flags = ntohs(tmp); ptr += sizeof(uint16_t);}
    { uint16_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint16_t)); (*o).length = ntohs(tmp); ptr += sizeof(uint16_t);}
    { uint32_t tmp; memcpy((char *)&tmp, ptr, sizeof(uint32_t)); (*o).ssrc = ntohl(tmp); ptr += sizeof(uint32_t);}
    return ptr;
}


