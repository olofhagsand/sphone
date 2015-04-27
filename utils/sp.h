/*-----------------------------------------------------------------------------
  File:   sp.h
  Description: sphone server protocol
  this file.
  Author: Olof Hagsand
  CVS Version: $Id: sp.h,v 1.30 2005/01/12 18:13:04 olof Exp $
 
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


#ifndef _SP_H_
#define _SP_H_

#define SP_VERSION        "1.2"
#define SP_VERSION_NR     12     /* Major Minor */
#define SPS_PORT          7878   /* Default server port */
#define SP_MAGIC          0xa8   /* Magic cookie for protocol */
#define SPS_LOGDIR        "log"
#define SPS_LOGPREFIX     "log"  /* Default log file prefix */
#define SP_MEAN_CALL_ARRIVAL    300 /* Mean arrival in seconds/client (sec) */
#define SP_MEAN_CALL_DURATION  (3*60) /* Mean call duration (sec) */
#define SP_BUFLEN               64 /* Default buffer length read/write */
#define SP_DEFAULT_SERVER  "192.36.125.26" /* Default server address */
#define SP_TIMEOUT_REG_NR       10   /* Nr timeouts before disconnect client */
#define SPC_INIT_INTERVAL   20  /* How often to register with server (s) */
#define SPC_SERVER_INTERVAL 120 /* How often to update registration */

enum sp_type{
    SP_TYPE_REGISTER,
    SP_TYPE_DEREGISTER,
    SP_TYPE_START_SEND,
    SP_TYPE_START_RCV,
    SP_TYPE_REPORT_SEND,
    SP_TYPE_REPORT_RCV,
    SP_TYPE_EXIT,          /* Kill process */
};

/* 
 * type-specific fields for start and report messages
 */
struct sdhdr_start{
    uint32_t           ss_testid;        /* test/log seq nr */
    uint32_t           ss_clientid_src;  /* src client id */
    uint32_t           ss_clientid_dst;  /* dst client id */
    struct sockaddr_in ss_addr_src_pub;  /* Src public data address */
    struct sockaddr_in ss_addr_dst_pub;  /* Dst public address */
    struct timeval     ss_time_start;    /* Start of call (server time) */
    struct timeval     ss_time_duration; /* Duration of call */
};

/* 
 * type-specific fields for register messages
 */
struct sdhdr_register{
    struct sockaddr_in sr_addr_pub;  /* Client public data address */
    uint8_t            sr_busy;     /* blocked with data xfer: 1 */
};

union sp_data{
    struct sdhdr_start    _sd_start;
    struct sdhdr_register _sd_reg;
};

/*
 * Protocol
 */ 
struct sp_proto{
    uint8_t        sp_magic;      /* cookie */
    uint8_t        sp_type;       /* type of message */
    uint16_t       sp_version;    /* protocol version */
    uint32_t       sp_len;        /* message length */
    uint32_t       sp_id;         /* Client id:(cant use addresses) */
    union sp_data _sp_data;       /* variable fields */
};
#define sp_starthdr    _sp_data._sd_start
#define sp_reporthdr   _sp_data._sd_start    /* same as start for now */
#define sp_reghdr      _sp_data._sd_reg

#ifndef min
#define  min(a,b)        ((a) < (b) ? (a) : (b))
#endif /* min */
#ifndef max
#define  max(a,b)        ((a) > (b) ? (a) : (b))
#endif /* max */

#ifdef WIN32
/* XXX: This is just a hack: God forbid that it will be used in a real setting!
 */
#define srandom(a)
static int
random() 
{
    struct timeval t; 
    t=gettimestamp(); 
    return t.tv_usec;
}
#endif /* WIN32 */


/*
 * Prototypes
 */ 
char *sp_addr_encode(struct sockaddr_in *sin, char *p);
char *sp_addr_decode(struct sockaddr_in *sin, char *p);
char *sptype2str(enum sp_type type);

#endif  /* _SP_H_ */
