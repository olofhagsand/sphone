/*-----------------------------------------------------------------------------
  File:   sphone_coding.c
  Description: audio encoding module
  Author: Olof Hagsand
  CVS Version: $Id: sphone_coding.c,v 1.21 2005/01/31 18:26:18 olof Exp $
 
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

#include "sphone.h"

#include "g711.h"
#ifdef HAVE_ILBC
#define ILBC_ENCODE_LEN BLOCKL*2
#define ILBC_DECODE_LEN NO_OF_BYTES

#include "iLBC_define.h"
#include "iLBC_encode.h"
#include "iLBC_decode.h"
#endif /* HAVE_ILBC */
/*
 * Macros
 */
#define coding_err ((coding_fn*)-1)

#ifdef HAVE_ILBC
struct ilbc_private{
    iLBC_Enc_Inst_t ip_Enc_Inst;
    iLBC_Dec_Inst_t ip_Dec_Inst;
};
#endif /* HAVE_ILBC*/

/*
 * Local prototypes
 */
int coding_null(coding_api *ca, char *, size_t , char *, size_t );
int coding_l16_2alaw(coding_api *ca, char *, size_t , char *, size_t );
int coding_alaw2_l16(coding_api *ca, char *, size_t , char *, size_t );
int coding_l16_2ulaw(coding_api *ca, char *, size_t , char *, size_t );
int coding_ulaw2_l16(coding_api *ca, char *, size_t , char *, size_t );
int coding_ulaw2alaw(coding_api *ca, char *, size_t , char *, size_t );
int coding_alaw2ulaw(coding_api *ca, char *, size_t , char *, size_t );
#ifdef HAVE_ILBC
static int coding_l16_2ilbc(coding_api *, char *, size_t, char *, size_t);
static int coding_ilbc_2l16(coding_api *, char *, size_t, char *, size_t);
#endif

/*
 * Local variables
 */
/* 
 * Encode/decode function according to matrix 
 * XXX: would have liked a more generic way to add new codecs
 */
#ifdef HAVE_ILBC
static coding_fn *coding_mapping[5][5] = {               
/*         LINEAR 16        ULAW             ALAW             L8           iLBC        */
/* L16 */ {coding_null,     coding_l16_2ulaw,coding_l16_2alaw,coding_err,  coding_l16_2ilbc},
/* ULAW */{coding_ulaw2_l16,coding_null,     coding_ulaw2alaw,coding_err,  coding_err},
/* ALAW */{coding_alaw2_l16,coding_alaw2ulaw,coding_null,     coding_err,  coding_err},
/* L8 */  {coding_err,      coding_err,      coding_err,      coding_null, coding_err},
/* iLBC */{coding_ilbc_2l16, coding_err,     coding_err,      coding_err,  coding_null}
};
#else /* HAVE_ILBC */
static coding_fn *coding_mapping[4][4] = {
/*         LINEAR 16        ULAW             ALAW             L8   */
/* L16 */ {coding_null,     coding_l16_2ulaw,coding_l16_2alaw,coding_err},
/* ULAW */{coding_ulaw2_l16,coding_null,     coding_ulaw2alaw,coding_err},
/* ALAW */{coding_alaw2_l16,coding_alaw2ulaw,coding_null,     coding_err},
/* L8 */  {coding_err,      coding_err,      coding_err,      coding_null}
};
#endif /* HAVE_ILBC */


/* Translate from coding type to RTP payload */
int 
coding2rtp(enum coding_type type)
{
    int rtp_pt;

    switch (type){
    case CODING_LINEAR8:
	rtp_pt = RTP_PT_DYNAMIC;
	break;
    case CODING_LINEAR16:
	rtp_pt = RTP_PT_L16;
	break;
    case CODING_ALAW:
	rtp_pt = RTP_PT_PCMA;
	break;
    case CODING_ULAW:
	rtp_pt = RTP_PT_PCMU;
	break;
#ifdef HAVE_ILBC
    case CODING_iLBC:
	rtp_pt = RTP_PT_DYNAMIC; /* XXX */
	break;
#endif /* HAVE_ILBC */
    }
    return rtp_pt;
}

/* Translate from RTP payload to coding type */
enum coding_type 
rtp2coding(int rtp_pt)
{
    enum coding_type type;

    switch (rtp_pt){
    case RTP_PT_DYNAMIC:
	type = CODING_LINEAR8;
	break;
    case RTP_PT_L16:
	type = CODING_LINEAR16;
	break;
    case RTP_PT_PCMA:
	type = CODING_ALAW;
	break;
    case RTP_PT_PCMU:
	type = CODING_ULAW;
	break;
    default:
	type = -1;
	break;
    }
    return type;
}

int
coding_l16_2alaw(coding_api *ca, char *buf_dec, size_t len_dec, 
		     char *buf_enc, size_t len_enc)
{
    int i;

    for (i=0; i<len_enc; i++)
	buf_enc[i] = linear2alaw(((uint16_t *)buf_dec)[i]);
    return 0;
}

int
coding_alaw2_l16(coding_api *ca, char *buf_enc, size_t len_enc, 
		     char *buf_dec, size_t len_dec)
{
    int i;

    for (i=0; i<len_enc; i++)
	((uint16_t *)buf_dec)[i] = alaw2linear((buf_enc)[i]);        	  

    return 0;
}


int
coding_l16_2ulaw(coding_api *ca, char *buf_dec, size_t len_dec, 
		     char *buf_enc, size_t len_enc)
{
    int i;

    for (i=0; i<len_enc; i++)
	buf_enc[i] = linear2ulaw(((uint16_t *)buf_dec)[i]);        	  
    return 0;
}

int
coding_ulaw2_l16(coding_api *ca, char *buf_enc, size_t len_enc, 
		     char *buf_dec, size_t len_dec)
{
    int i;

    for (i=0; i<len_enc; i++)
	((uint16_t *)buf_dec)[i] = ulaw2linear((buf_enc)[i]);       	  
    return 0;
}

int
coding_alaw2ulaw(coding_api *ca, char *buf_dec, size_t len_dec, 
		     char *buf_enc, size_t len_enc)
{
    int i;

    for (i=0; i<len_enc; i++)
	buf_enc[i] = alaw2ulaw(((uint16_t *)buf_dec)[i]);        	  
    return 0;
}

int
coding_ulaw2alaw(coding_api *ca, char *buf_dec, size_t len_dec, 
		     char *buf_enc, size_t len_enc)
{
    int i;

    for (i=0; i<len_enc; i++)
	buf_enc[i] = ulaw2alaw(((uint16_t *)buf_dec)[i]);        	  
    return 0;
}


/*
 * Generic NULL(copy) encoding/decoding function
 */
int
coding_null(coding_api *ca, char *buf1, size_t len1,
	    char *buf2, size_t len2)
{
    int i;
    
    for(i=0; i<len1; i++)
	buf2[i] = buf1[i];
    return 0;
}


/*
 * Make some coding tests 
 */
static int
alaw_test(FILE *f, uint16_t l16a)
{
    uint16_t l16b;
    uint8_t alaw;

    alaw = linear2alaw(l16a);
    l16b = alaw2linear(alaw);
    fprintf(stderr, "alaw: %x --> %x --> %x\n", l16a, alaw, l16b);    
    return 0;
}

static int
ulaw_test(FILE *f, uint16_t l16a)
{
    uint16_t l16b;
    uint8_t ulaw;

    ulaw = linear2ulaw(l16a);
    l16b = ulaw2linear(ulaw);
    fprintf(stderr, "ulaw: %x --> %x --> %x\n", l16a, ulaw, l16b);    
    return 0;
}

int
coding_test(FILE *f)
{
    alaw_test(f, 0xfa32);
    alaw_test(f, 0x1111);
    alaw_test(f, 0xe111);
    alaw_test(f, 0x7d22);
    alaw_test(f, 0x8000);
    alaw_test(f, 0xffff);
    alaw_test(f, 0x0001);
    alaw_test(f, 0x0000);
    alaw_test(f, 0x1000);
    alaw_test(f, 0x2000);
    alaw_test(f, 0x3000);
    alaw_test(f, 0x4000);
    alaw_test(f, 0x5000);
    alaw_test(f, 0x6000);
    alaw_test(f, 0x7000);
    alaw_test(f, 0x8000);

    ulaw_test(f, 0xfa32);
    ulaw_test(f, 0x1111);
    ulaw_test(f, 0xe111);
    ulaw_test(f, 0x7d22);
    ulaw_test(f, 0x8000);
    ulaw_test(f, 0xffff);
    ulaw_test(f, 0x0001);
    ulaw_test(f, 0x0000);
    ulaw_test(f, 0x1000);
    ulaw_test(f, 0x2000);
    ulaw_test(f, 0x3000);
    ulaw_test(f, 0x4000);
    ulaw_test(f, 0x5000);
    ulaw_test(f, 0x6000);
    ulaw_test(f, 0x7000);
    ulaw_test(f, 0x8000);
    return 0;
}


/*
 * Generic exit function
 */
int
coding_exit(coding_api *ca)
{
    sfree(ca);
    return 0;
}

/*
 * Generic init function
 */
int
coding_start_generic(coding_api *ca)
{
    return 0;
}

/*
 * Generic sanity function
 */
int
coding_sanity(coding_params *cp)
{
    switch (cp->cp_type){
    case CODING_LINEAR8:
	cp->cp_precision = 8;
	cp->cp_size_bytes = ms2bytes(cp->cp_samp_sec, 
				     cp->cp_precision, 
				     cp->cp_channels,
				     cp->cp_size_ms);
    case CODING_LINEAR16:
	cp->cp_precision = 16;
	cp->cp_size_bytes = ms2bytes(cp->cp_samp_sec, 
				     cp->cp_precision, 
				     cp->cp_channels,
				     cp->cp_size_ms);
	break;
    case CODING_ALAW:
    case CODING_ULAW:
	cp->cp_precision = 8;
	cp->cp_size_bytes = ms2bytes(cp->cp_samp_sec, 
				     cp->cp_precision, 
				     cp->cp_channels,
				     cp->cp_size_ms);
	break;
#ifdef HAVE_ILBC
    case CODING_iLBC:
	cp->cp_precision = 4; /* XXX */
	cp->cp_size_ms = 30;  /* XXX */
	cp->cp_size_bytes = ILBC_DECODE_LEN;
	break;
#endif /* HAVE_ILBC */
    }
    return 0;
}

/*
 * Generic size factor conversion between coders.
 * 1 is linear PCM, all others are > 1.
 */
static int 
coding_lenfactor(enum coding_type type, float *factor)
{
    float f = 1;

    assert(factor);
    switch (type){
    case CODING_LINEAR8:
    case CODING_LINEAR16:
	break;
    case CODING_ALAW:
    case CODING_ULAW:
	f = 2;
	break;
#ifdef HAVE_ILBC
    case CODING_iLBC:
	f = (((float)ILBC_ENCODE_LEN)/((float)ILBC_DECODE_LEN)); /* XXX: 4.7999 */
	f -= 0.05;
	break;
#endif /* HAVE_ILBC */

    default:
	sphone_error("coding_lenfactor: No such coding_type: shouldnt happen");
	return -1;
	break;
    }
    *factor = f;
    return 0;
}

/*
 * Generic coding length conversion:
 * Assume a packet of length len1 with coded by c1 converted to coding type c2.
 * len2 will be the length of the packet after coding to c2.
 */
static int 
coding_transformlen_generic(coding_params *c1, coding_params *c2, 
			    size_t len1, size_t *len2)
{
    float f1, f2;

    if (coding_lenfactor(c1->cp_type, &f1) < 0)
	return -1;
    if (coding_lenfactor(c2->cp_type, &f2) < 0)
	return -1;
    *len2 = (size_t)((len1*f1)/f2);
    return 0;
}


/*
 * Reduce sample set from more channels, c1 (eg 2) to fewer, c2 (eg 1).
 * Actually only works for c2 == 1.
 */
int
coding_reduce(char *buf, size_t *len, uint32_t precision, 
	      uint8_t reduce_factor)
{
    int i;
    int by_s; /* bytes per sample */
	
    by_s = precision/8;
    for (i=0; i<*len; i+=by_s*reduce_factor)
	memcpy(&buf[i/reduce_factor], &buf[i], by_s);
    *len /= reduce_factor;
    return 0;
}

/*
 * Expand sample set from fewer channels, c1 (eg 1) to more, c2 (eg 2).
 * Actually only works for c1 == 1.
 */
int
coding_expand(char *buf, size_t *len, uint32_t precision, uint8_t expand_factor)
{
    int i, j;
    int by_s; /* bytes per sample */
    
    by_s = precision/8;
    for (i=*len-by_s; i>=0; i -= by_s)
	for (j=expand_factor-1; j>=0; j--)
	    memcpy(&buf[i*expand_factor+j*by_s], &buf[i], by_s);
    *len *= expand_factor;
    return 0;
}

int
coding_param_exit(void *arg)
{
    sfree(arg);
    return 0;
}

coding_params *
coding_param_init()
{
    coding_params *cp;
    
    if ((cp = (void*)smalloc(sizeof(struct coding_params))) == NULL){
	sphone_error("coding_param_init: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(cp, 0, sizeof(struct coding_params));
    return cp;
}

#ifdef HAVE_ILBC
static int
coding_transformlen_ilbc(coding_params *c1, coding_params *c2, 
			 size_t len1, size_t *len2)
{
    if (c1->cp_type == CODING_iLBC){
	assert(len1 == ILBC_DECODE_LEN);
	*len2 = ILBC_ENCODE_LEN;
    }
    else{
	assert(len1 == ILBC_ENCODE_LEN);
	*len2 = ILBC_DECODE_LEN;
    }
    return 0;
}


static int
ilbc_exit(coding_api *ca)
{
    sfree(ca);
    return 0;
}

static int
ilbc_start(coding_api *ca)
{
    struct ilbc_private *ipriv = (struct ilbc_private *)ca->ca_private;

    initEncode(&ipriv->ip_Enc_Inst);
    initDecode(&ipriv->ip_Dec_Inst, 1);
    return 0;
}

/*
 * From Linear 16 to iLBC
 */
static int
coding_l16_2ilbc(coding_api *ca, char *buf_dec, size_t len_dec, 
		 char *buf_enc, size_t len_enc)
{
    struct ilbc_private *ipriv = (struct ilbc_private *)ca->ca_private;
    float block[BLOCKL];
    short stmp;
    int k;

    assert(len_enc == ILBC_DECODE_LEN && len_dec == ILBC_ENCODE_LEN); /* 50, 240 */
    /* convert signal to float */
    for(k=0; k<BLOCKL; k++) {
	memcpy(&stmp, &buf_dec[2*k], sizeof(short));
	block[k] = (float)stmp;
    }

    /* do the actual encoding */
    iLBC_encode(buf_enc, block, &ipriv->ip_Enc_Inst);
    return (ILBC_DECODE_LEN);
}

static int
coding_ilbc_2l16(coding_api *ca, char *buf_enc, size_t len_enc, 
		 char *buf_dec, size_t len_dec)
{
    struct ilbc_private *ipriv = (struct ilbc_private *)ca->ca_private;
    int k;
    float decblock[BLOCKL], ftmp;
    short stmp;

    assert(len_enc == ILBC_DECODE_LEN && len_dec == ILBC_ENCODE_LEN); /* 50, 240 */

    /* do actual decoding of block */

    iLBC_decode(decblock, buf_enc, &ipriv->ip_Dec_Inst, 1); /* (i) 0=PL, 1=Normal */

    /* convert to short */
    for(k=0; k<BLOCKL; k++){ 
	ftmp = decblock[k];
	if (ftmp<MIN_SAMPLE)
	    ftmp=MIN_SAMPLE;
	else if (ftmp>MAX_SAMPLE)
	    ftmp=MAX_SAMPLE;
	stmp = (short)ftmp;
	memcpy(&buf_dec[2*k], &stmp, sizeof(short));
    }
    return(ILBC_ENCODE_LEN);
}

static int
coding_init_ilbc(coding_api *ca, coding_params *c1, coding_params *c2)
{
    ca->ca_exit = ilbc_exit;
    ca->ca_start = ilbc_start;
    ca->ca_transformlen = coding_transformlen_ilbc;

    if ((ca->ca_private = smalloc(sizeof(struct ilbc_private))) == NULL){
	perror("coding_init_ilbc: malloc");
	return -1;
    }
    memset(ca->ca_private, 0, sizeof(struct ilbc_private));
    return 0;
}
#endif /* HAVE_ILBC */

static int
coding_init_generic(coding_api *ca, coding_params *c1, coding_params *c2)
{
    ca->ca_exit = coding_exit;
    ca->ca_start = coding_start_generic;
    ca->ca_transformlen = coding_transformlen_generic;
    return 0;
}


/*
 * Initialize coding.
 * Take two codings: c1 (from/audio) and c2 (to/network) encoding. 
 * 1. Check the sanity of c1, that is, ensure that bits_samp/samp_sec is
 *     consistent with the coding type.,
 * 2. Given the coding type of c2 and the coding parameters of c1, create the
 *    coding parameters of c2, and ensure its consistency.
 * 3. Create and return a coding_api including indirect functions for 
 *       encode: c1 -> c2
 *       decode: c2 -> c1
 */
coding_api *
coding_init(coding_params *c1, coding_params *c2)
{
    coding_api *ca;
    float f1, f2;

    /* 1. check c1 sanity */
    if (coding_sanity(c1) < 0)
	return NULL;

    /* 2. create c2 from c1. */
    c2->cp_samp_sec = c1->cp_samp_sec;
    if (coding_lenfactor(c1->cp_type, &f1) < 0)
	return NULL;
    if (coding_lenfactor(c2->cp_type, &f2) < 0)
	return NULL;
    c2->cp_channels = 1;
    c2->cp_precision = (uint32_t)c1->cp_precision*f1/f2;
    c2->cp_size_ms = c1->cp_size_ms;
    c2->cp_size_bytes = ms2bytes(c2->cp_samp_sec, c2->cp_precision, 
				 c2->cp_channels, c2->cp_size_ms);

    /* 3. Create coding_api */
    if ((ca = (void*)smalloc(sizeof(coding_api))) == NULL){
	sphone_error("coding_init: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(ca, 0, sizeof(coding_api));

    if ((ca->ca_encode = coding_mapping[c1->cp_type][c2->cp_type]) == coding_err){
	sphone_error("coding_init: coding_mapping failed between %d and %d",
		     c1->cp_type, c2->cp_type);
	sfree(ca);
	return NULL;
    }
    if ((ca->ca_decode = coding_mapping[c2->cp_type][c1->cp_type]) == coding_err){
	sphone_error("coding_init: coding_mapping failed between %d and %d",
		     c1->cp_type, c2->cp_type);
	sfree(ca);
	return NULL;
    }

#ifdef HAVE_ILBC
    if (c1->cp_type == CODING_iLBC || c2->cp_type == CODING_iLBC){
	if (coding_init_ilbc(ca, c1, c2) < 0){
	    sfree(ca);
	    return NULL;
	}
    }
    else
#endif /* HAVE_ILBC */
	{
	    if (coding_init_generic(ca, c1, c2) < 0){
		sfree(ca);
		return NULL;
	    }
	}
    dbg_print(DBG_CODING, "Audio coding: %d\n\tsamples per second: %d\n"
	      "\tprecision: %d\n\tsize (ms): %d\n"
	      "\tsize(bytes): %d\n\tchannels: %d\n", 
	      c1->cp_type, 
	      c1->cp_samp_sec, 
	      c1->cp_precision, 
	      c1->cp_size_ms, 
	      c1->cp_size_bytes, 
	      c1->cp_channels 
	      );
    dbg_print(DBG_CODING, "Network coding: %d\n\tsamples per second: %d\n"
	      "\tprecision: %d\n\tsize (ms): %d\n"
	      "\tsize(bytes): %d\n\tchannels: %d\n", 
	      c2->cp_type, 
	      c2->cp_samp_sec, 
	      c2->cp_precision, 
	      c2->cp_size_ms, 
	      c2->cp_size_bytes, 
	      c2->cp_channels 
	      );
    /* XXX: start should be later */
    if (ca->ca_start(ca) < 0){
	sfree(ca);
	return NULL;
    }
    return ca;
}

/*
 * Only works for encodings whe precision%8 = 0
 */
int
dbg_coding(int level, char *str, char *buf, size_t len, int precision)
{
    int i, j;

    if ((precision%8 == 0) && (level & (debug|DBG_ALWAYS)) == level){
	dbg_print(level, "%s, len:%d\n", str, len);
	for (i=0; i<len; i += precision/8)
	    for (j=0; j<precision/8; j++)
		dbg_print(level, "%02x ", (uint8_t)buf[i+j]);
	dbg_print(level, "\n");
    }
    return 0;
}
