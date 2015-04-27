/*-----------------------------------------------------------------------------
  File:   sphone_lib.c
  Description: Useful functions
  Author: Olof Hagsand
  CVS Version: $Id: sphone_lib.c,v 1.15 2005/02/13 17:19:31 olof Exp $
 
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
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>

#include "sphone.h"


/*
 * Set a signal handler.
 */
void (*set_signal(int signo, void (*handler)(int)))()
{
#if defined(HAVE_SIGACTION)
    struct sigaction sold, snew;

    snew.sa_handler = handler;
    sigemptyset(&snew.sa_mask);
    snew.sa_flags = 0;
    if (sigaction (signo, &snew, &sold) < 0)
	perror ("sigaction");
    return sold.sa_handler;
#elif defined(HAVE_SIGVEC)
    struct sigvec sold, snew;

    snew.sv_handler = handler;
    snew.sv_mask = 0;
    snew.sv_flags = 0;
    if (sigvec (signo, &snew, &sold) < 0)
	perror ("sigvec");
    return sold.sv_handler;
#else
    void (*old_handler)(int);

    old_handler = signal (signo, ((void (*)(int))handler));
    if ((int)old_handler == -1)
	perror ("signal");
    return old_handler;
#endif
}

void timevalfix(struct timeval *t1)
{
    if (t1->tv_usec < 0) {
        t1->tv_sec--;
        t1->tv_usec += 1000000;
    }
    if (t1->tv_usec >= 1000000) {
        t1->tv_sec++;
        t1->tv_usec -= 1000000;
    }
}
 
struct timeval timevaladd(struct timeval t1, struct timeval t2)
{
    struct timeval t = t1;
    t.tv_sec += t2.tv_sec;
    t.tv_usec += t2.tv_usec;
    timevalfix(&t);
    return t; 
}
 
struct timeval timevalsub(struct timeval t1, struct timeval t2)
{
    struct timeval t = t1;
    t.tv_sec -= t2.tv_sec;
    t.tv_usec -= t2.tv_usec;
    timevalfix(&t);
    return t; 
}

/* Translate timeval to floating point seconds */
double timeval2sec(struct timeval t)
{
    return (double)t.tv_sec + ((double)t.tv_usec)/1000000.0;
}

/* XXX: wraps around at ?? */
double timeval2usec(struct timeval t)
{
    return ((double)t.tv_sec)*1000000.0 + (double)t.tv_usec;
}

char *timevalprint(struct timeval t1)
{
    static char s[64];
    if (t1.tv_sec < 0 && t1.tv_usec > 0){
	if (t1.tv_sec == -1)
	    sprintf(s, "-%ld.%06ld", t1.tv_sec+1, 1000000-t1.tv_usec);
	else
	    sprintf(s, "%ld.%06ld", t1.tv_sec+1, 1000000-t1.tv_usec);
    }
    else
	sprintf(s, "%ld.%06ld", t1.tv_sec, t1.tv_usec);
    return s;
}

/* Returned formatted bps string, eg, 1000 bps == 1 Kbps */
char *bps_print(int bps0, char *post)
{
    static char s[64];
    double b;
    int bps = bps0;
    int i=0;

    b = bps;
    while (bps/1000) {
	b = b/1000;
	bps /= 1000;
	i++;
    }
    switch (i) {
    case 1:
	sprintf(s, "%.3f K%s", b, post);
	break;
    case 2:
	sprintf(s, "%.3f M%s", b, post);
	break;
    case 3:
	sprintf(s, "%.3f G%s", b, post);
	break;
    case 4:
	sprintf(s, "%.3f T%s", b, post);
	break;
    case 0:
    default:
	sprintf(s, "%d %s", bps0, post);
	break;
    }
    return s;
}

#ifndef WIN32
struct timeval gettimestamp(void)
{  
    struct timeval tt;

    gettimeofday(&tt, NULL);
    return tt;
}
#else
#include "windows.h"
#define exp32 4294967296.0 /* (2^32) */

#ifdef WINSOCK
#include <sys/timeb.h>
#endif

/*-------------------------------------------------------------------------
  Real-time clock on PC
  Some statistics:
  On my PentiumPRO, it takes 7 microseconds to make the realtime clock call.
  clocks calendar clocks on unix & my pc differs with 150 secs.
-------------------------------------------------------------------------*/

struct timeval gettimestamp(void)
{
    struct timeval tp;
    static double _usec_res;
    static double _sec_res;
    static int firsttime = TRUE;
    static struct timeval absdiff;
    LARGE_INTEGER Time; /* Clock cykler */
    uint32_t sec;
    uint32_t usec;

    if (firsttime){
	LARGE_INTEGER Frequency; /* cykler / sek */
	QueryPerformanceFrequency (&Frequency); /* XXX: init */
	_usec_res = 1000000.0/Frequency.LowPart; /* 0.9 */
	if (_usec_res > 1)
	    sphone_error("wise_init: performance frequency factor > 1");
	_sec_res = exp32/Frequency.LowPart; /* 3500 */
    }
    QueryPerformanceCounter (&Time);
    usec = Time.LowPart*_usec_res; /* microsekunder */
    sec = usec/1000000;
    usec = usec%1000000;
    sec += Time.HighPart*_sec_res; 
    tp.tv_sec = sec;
    tp.tv_usec = usec;
    if (firsttime) {
#ifdef WINSOCK
	struct timeb fnow;
	ftime(&fnow);

	absdiff.tv_sec = fnow.time;
	absdiff.tv_usec = 1000*(uint32_t)fnow.millitm;
#else
	gettimeofday(&absdiff, NULL);
#endif
	absdiff = timevalsub(absdiff, tp);
#if 0
	absdiff.tv_usec = 0;
	absdiff.tv_sec |= 0x0000ffff; /* Only on i386 */
#endif
	firsttime = FALSE;
    }
    tp = timevaladd(tp, absdiff); 
    return tp;
}

#endif /* WIN32 */

/*
 * Translates from packet size in milliseconds to bytes given a
 * certain sampling rate and bits per samples.
 * channels = 1 is mono, 2 is stereo
 */
uint32_t 
ms2bytes(uint32_t samp_sec, uint32_t bits_samp, uint8_t channels, uint32_t ms)
{
    return ((samp_sec*bits_samp*channels*ms)/8000);
}

/*
 * Translates from packet size in bytes to milliseconds given a
 * certain sampling rate and bits per samples.
 * channels = 1 is mono, 2 is stereo
 */
uint32_t 
bytes2ms(uint32_t samp_sec, uint32_t bits_samp, uint8_t channels, uint32_t bytes)
{
    return  ((bytes*8000)/((samp_sec*bits_samp*channels)));
}

/*
 * Translates from packet size in bytes to #samples given bits per samples.
 */
uint32_t 
bytes2samples(uint32_t bits_samp, uint32_t bytes, uint8_t channels)
{
    return  ((bytes*8)/(bits_samp*channels));
}

/* Heap allocation wrapper functions for tracing */
void *malloc_wrapper(char *filename, int line, int size)
{
    void *p;

    p = (void*)malloc(size);
    dbg_print(DBG_MALLOC, "malloc: %x, size: %d ", (unsigned int)p, size);
    dbg_print(DBG_MALLOC, "(%s %d)\n", filename, line); 

    return p;
}

void *calloc_wrapper(char *filename, int line, int n, int size)
{
    void *p = (void*)calloc(n, size);

    dbg_print(DBG_MALLOC, "calloc: %x nmemb: %d, size: %d ", 
	      (unsigned int)p, n, size);
    dbg_print(DBG_MALLOC, "(%s %d)\n", filename, line); 
    return p;
}

void 
free_wrapper(char *filename, int line, void *p)
{
    dbg_print(DBG_MALLOC, "free: %x ", (unsigned int)p);
    dbg_print(DBG_MALLOC, "(%s %d)\n", filename, line); 
    free(p);
}

/*
 * Generate a random 32-bit quantity.
 * From rfc 1889
 */
u_long 
md_32(char *string, int length)
{
    union {
	char   c[16];
	u_long x[4];
    } digest;
    u_long r;
    int i;
    for (i=0; i<length; i++){
	digest.c[i%16] ^= string[i];
    }
    r = 0;
    for (i = 0; i < 3; i++) {
	r ^= digest.x[i];
    }
    return r;
}                               /* md_32 */

