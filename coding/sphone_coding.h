/*-----------------------------------------------------------------------------
  File:   sphone_coding.h
  Description: audio encoding module
  Author: Olof Hagsand
  CVS Version: $Id: sphone_coding.h,v 1.13 2004/12/21 15:09:57 olof Exp $
 
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
/*
   The coding module uses a public API with the following indirect functions.
   Each new module need to implement these functions:
   init       Initializing the coding module
   encode     Encode one packet of PCM data
   decode     Decode one packet to PCM data
   sanity     Check if parameters is consisitent with coding. If not, 
              change them.
   exit       Deallocate all resources asociated with the coding.
 */
#ifndef _SPHONE_CODING_H_
#define _SPHONE_CODING_H_

/*
 * Constants
 */
/*
 * Specifies the encoded/decoded size in bytes per frame (20 ms)
 * for frame related speechcoders
 */
#define ENCODED_SZ_DAMPSEFR   20
#define ENCODED_SZ_GSMEFR     31
#define ENCODED_SZ_GSM610     33
#define PCM_SZ_8K_16B_20MS   320   /* size pcm 8kHz sampling, 16bit,20ms*/

/*
 * Types
 */
/* Order is significant, see coding_mapping variable in sphone_coding.c */
enum coding_type{
	/* Signed 16-bit two's complement and network byte order */
	CODING_LINEAR16,

	/* G.711 U-LAW (PCMU) */
	CODING_ULAW,

	/* G.711 A-LAW (PCMA) */
	CODING_ALAW,

	/* 8-bit ? */
	CODING_LINEAR8,
#ifdef HAVE_ILBC
	/* iLBC */
	CODING_iLBC,
#endif /* HAVE_ILBC */
}; /* What kind of coding */

/*
 * Coding context
 */
struct coding_params{
    enum coding_type cp_type;         /* Coding type (enum coding_type) */
    uint32_t         cp_samp_sec;     /* samples per second */
    uint32_t         cp_precision;    /* bits per sample */
    uint32_t         cp_size_ms;      /* packet length */
    uint32_t         cp_size_bytes;   /* packet length */
    uint8_t          cp_channels;     /* channels: 1 is mono: 2 is stereo */
};
typedef struct coding_params coding_params;

typedef struct coding_api coding_api;

typedef int (coding_fn)(coding_api *, char *, size_t, char* , size_t);

struct coding_api{
    /* Public fields */
    int (*ca_start)(coding_api *);
    coding_fn *ca_encode;
    coding_fn *ca_decode;
    int (*ca_exit)(coding_api *);
    int (*ca_transformlen)(coding_params *c1, coding_params *c2, 
			  size_t len1, size_t *len2);
    void *ca_private;                             /* private data */
};


/*
 * Prototypes
 */ 
int coding_param_exit(void *arg);
coding_params *coding_param_init(void);
int coding_reduce(char *buf, size_t *len, uint32_t precision, uint8_t reduce);
int coding_expand(char *buf, size_t *len, uint32_t precision, uint8_t expand);
coding_api *coding_init(coding_params *c1, coding_params *c2);
int coding2rtp(enum coding_type type);
enum coding_type rtp2coding(int rtp_pt);
int dbg_coding(int level, char *str, char *buf, size_t len, int precision);
int coding_test(FILE*);

#endif  /* _SPHONE_CODING_H_ */


