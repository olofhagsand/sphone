/*-----------------------------------------------------------------------------
  File:   sphone_lib.h
  Description: Useful functions
  Author: Olof Hagsand
  CVS Version: $Id: sphone_lib.h,v 1.17 2005/02/13 17:19:31 olof Exp $
 
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


#ifndef _SPHONE_LIB_H_
#define _SPHONE_LIB_H_

/*
 * Constants
 */
#define smalloc(size) malloc_wrapper(__FILE__, __LINE__, (size))
#define scalloc(n, size) calloc_wrapper(__FILE__, __LINE__, (n), (size))
#define sfree(p) free_wrapper(__FILE__, __LINE__, (p))

#define INBUF_LEN 10000 /* inbuf size */


#ifdef WIN32 /* XXX: HAVE_GETTIMEOFDAY ? */
#define	timeradd(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;			      \
    if ((result)->tv_usec >= 1000000)					      \
      {									      \
	++(result)->tv_sec;						      \
	(result)->tv_usec -= 1000000;					      \
      }									      \
  } while (0)
#define	timersub(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;			      \
    if ((result)->tv_usec < 0) {					      \
      --(result)->tv_sec;						      \
      (result)->tv_usec += 1000000;					      \
    }									      \
  } while (0)
#endif /* WIN32 */


/*
 * Types
 */

/*
 * Variables
 */

/*
 * Prototypes
 */ 
void (*set_signal(int signo, void (*handler)(int)))();
struct timeval gettimestamp(void);
void timevalfix(struct timeval *);
char *timevalprint(struct timeval t1);
struct timeval timevaladd(struct timeval t1, struct timeval t2);
struct timeval timevalsub(struct timeval t1, struct timeval t2);
double timeval2usec(struct timeval t);
double timeval2sec(struct timeval t);
char *bps_print(int bps0, char *post);
uint32_t ms2bytes(uint32_t samp_sec, uint32_t bits_samp, uint8_t channels, uint32_t ms);
uint32_t bytes2ms(uint32_t samp_sec, uint32_t bits_samp, uint8_t channels, uint32_t bytes);
uint32_t bytes2samples(uint32_t bits_samp, uint32_t bytes, uint8_t channels);

/* Heap allocation wrapper functions for tracing */
void *malloc_wrapper(char *filename, int line, int size);
void *calloc_wrapper(char *filename, int line, int n, int size);
void free_wrapper(char *filename, int line, void *p);
u_long md_32(char *string, int length);

#endif  /* _SPHONE_LIB_H_ */
