/*-----------------------------------------------------------------------------
  File:   sphone_rtp.c
  Description: Common internal .h file. All C-files in sphone should include 
  this file.
  Author: Olof Hagsand, Kjell Hansson, Emmanuel Frecon
  CVS Version: $Id: sphone_rtp.c,v 1.10 2005/02/13 17:19:31 olof Exp $
 
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
 *---------------------------------------------------------------------------*/


#include <stdio.h>       /* printf() */
#include <string.h>
#include <sys/types.h>   /* u_long */

#include "sphone.h"
#include "sphone_lib.h"
#include "sphone_rtp.h"

/* NTP 32-bit timestamp to struct timeval */
void NTP2TV(struct NTP_timestamp *NTP, struct timeval *TV)
{
    static struct timeval T;

    T = gettimestamp(); /* Need only be done every 18 hours :-> */
    TV->tv_sec = NTP->sec | (T.tv_sec & 0xffff0000);
    TV->tv_usec = ((NTP->frac*15625)>>10);
}

/* struct timeval to NTP 32-bit timestamp */
void TV2NTP(struct timeval *TV, struct NTP_timestamp *NTP)
{
/* u-seconds to 16 bit fraction 1024=2^10.  Coneversion is x*65536/1000000*/
    NTP->sec = TV->tv_sec & 0xffff;
    NTP->frac = (TV->tv_usec<<10)/15625;
}

/* struct timeval to NTP 32-bit timestamp */
void TV2NTP_64(struct timeval *TV, struct NTP_timestamp_64 *NTP)
{
    NTP->sec = TV->tv_sec;
    /* conversion factor is 4294967296/1000000 */
    NTP->frac = (uint32_t)((double)TV->tv_usec*4294.967296);   
}

/* Translate NTP timestamp to floating point seconds */
double NTP2sec(struct NTP_timestamp *NTP)
{
    return (double)NTP->sec + ((double)NTP->frac)/65536.0;
}

/* Enoce RTP header: generated from from Assar Ws ydr */
char *marshal_rtp_hdr(rtp_header *o, char *ptr)
{
    { uint16_t tmp = htons((*o).flags); memcpy(ptr, (char*)&tmp, sizeof(uint16_t)); ptr += sizeof(uint16_t);}
    { uint16_t tmp = htons((*o).seq); memcpy(ptr, (char*)&tmp, sizeof(uint16_t)); ptr += sizeof(uint16_t);}
    { uint16_t tmp = htons((*o).ts.sec); memcpy(ptr, (char*)&tmp, sizeof(uint16_t)); ptr += sizeof(uint16_t);}
    { uint16_t tmp = htons((*o).ts.frac); memcpy(ptr, (char*)&tmp, sizeof(uint16_t)); ptr += sizeof(uint16_t);}
    { uint32_t tmp = htonl((*o).ssrc); memcpy(ptr, (char*)&tmp, sizeof(uint32_t)); ptr += sizeof(uint32_t);}
    return ptr;
}

char *unmarshal_rtp_hdr(rtp_header *o, char *ptr)
{
    { uint16_t tmp; memcpy ((char *)&tmp, ptr, sizeof(uint16_t)); (*o).flags = ntohs(tmp); ptr += sizeof(uint16_t);}
    { uint16_t tmp; memcpy ((char *)&tmp, ptr, sizeof(uint16_t)); (*o).seq = ntohs(tmp); ptr += sizeof(uint16_t);}
    { uint16_t tmp; memcpy ((char *)&tmp, ptr, sizeof(uint16_t)); (*o).ts.sec = ntohs(tmp); ptr += sizeof(uint16_t);}
    { uint16_t tmp; memcpy ((char *)&tmp, ptr, sizeof(uint16_t)); (*o).ts.frac = ntohs(tmp); ptr += sizeof(uint16_t);}
    { uint32_t tmp; memcpy ((char *)&tmp, ptr, sizeof(uint32_t)); (*o).ssrc = ntohl(tmp); ptr += sizeof(uint32_t);}
    return ptr;
}


/*
 * Return random unsigned 32-bit quantity. Use 'type' argument if you
 * need to generate several different values in close succession.
 */
unsigned int 
random_ssrc(int type)
{
    struct {
	int     type;
	struct  timeval tv;
	int   pid;
    } s;

    s.tv = gettimestamp();
    s.type = type;
    s.pid  = getpid();

    return md_32((char *)&s, sizeof(s));
}                               /* random32 */

/* Pretty print an rtp header and return in STATIC string (NB make copy) */
char *rtp_print_hdr(rtp_header *hdr)
{
    static char str[128];

    sprintf(str, "[V=%d, P=%d, X=%d, CC=%d, M=%d, PT=%d, seq=%d, ts=%d.%03d, ssrc=%lu]", 
	    rtp_get_v(hdr->flags),
	    rtp_get_p(hdr->flags),
	    rtp_get_x(hdr->flags),
	    rtp_get_cc(hdr->flags),
	    rtp_get_m(hdr->flags),
	    rtp_get_pt(hdr->flags),
	    hdr->seq,
	    hdr->ts.sec%1000,
	    ((hdr->ts.frac*15625)>>10)/1000, /* msecs */
	    (long unsigned)hdr->ssrc
	);
    return str;
}


int 
rtp_init(uint32_t *ssrc)
{
    if (ssrc && *ssrc == 0)
	*ssrc = random_ssrc(0);
    { /* Runtime test to see if encoding fns OK */
	rtp_header hdr;
	char buf[1500], *s;
	s = marshal_rtp_hdr(&hdr, buf);
	if (sizeof(rtp_header) != (s-buf))
	    return -1;
    }
#if 0
    {
	struct timeval t0, t1, dt = {0, 0};
	t0 = gettimestamp();
	while (dt.tv_usec == 0){
	    t1 = gettimestamp();
	    dt = timevalsub(t1, t0);
	}
	printf("Timer resolution is %d usecs\n", dt.tv_usec);
    }
#endif
    return 0;
}

/* Fill in the sending of an rtp header, T is not generated since it
 may be an earlier time. */
void rtp_set_hdr(rtp_header *hdr, struct timeval *T0, uint32_t ssrc)
{
    struct timeval T;

    /* Fill in RTP v2 information */
    hdr->flags = 0x0;
    rtp_set_v(hdr->flags, RTP_VERSION);
    rtp_set_pt(hdr->flags, RTP_PT_DYNAMIC); /* payload type */

    if (T0)
	T = *T0;
    else
	T = gettimestamp();
    TV2NTP(&T, &hdr->ts);
    hdr->ssrc = ssrc;
}


/* Check version and payload type */
int rtp_check_hdr(rtp_header *hdr)
{
    if (rtp_get_v(hdr->flags) != RTP_VERSION) {
	sphone_warning("Bad RTP version - received %d, should be %d",
		     rtp_get_v(hdr->flags), RTP_VERSION);
	return -1;
    }
    return 0; /* OK */
}





