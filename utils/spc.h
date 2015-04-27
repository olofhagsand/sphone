/*-----------------------------------------------------------------------------
  File:   spc.h
  Description: sphone server - client side
  this file.
  Author: Olof Hagsand
  CVS Version: $Id: spc.h,v 1.9 2004/12/18 16:11:26 olof Exp $
 
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


#ifndef _SPC_H_
#define _SPC_H_

/*
 * Constants
 */
#define SPC_LOGFILE_LEN     32   /* Max length of logfile */

/*
 * Types
 * See state diagram in spc.c
 */
enum spc_state{
    SS_INIT = 0,
    SS_SERVER,
    SS_DATA_RCV,
    SS_DATA_SEND,
    SS_DATA_SEND_CLOSING, /* stopped sending, waiting for all replies */
};

enum logformat{
    LOGFMT_ORIG,  /* Original sicsophone 1.0/ taurus format */
    LOGFMT_TIME,  /* Just timestamps */
};

struct spc_info{
    char     sp_hostname[64];    /* Server IPv4 addr */
    char     sp_myhostname[64];  /* Client IPv4 addr */
    uint16_t sp_port;            /* server port number */
    int      sp_s;               /* tcp socket to server */
    int      sp_ds;              /* udp socket */
    int      sp_id;              /* unique client identifier */
    struct sockaddr_in sp_addr; /* server control address */
    struct sockaddr_in sp_mydaddr; /* Client's rtp address */
    enum spc_state sp_state;     /* keep track of my state wrp the server */
    struct coding_params sp_coding; /* audio coding parameters */
    void    *sp_data;           /* state variable, depending on state */
    enum logformat sp_logformat; /* What logformat? */
    uint32_t sp_testid;          /* Test id */
};


/*
 * Prototypes
 */
int spc_state2init(struct spc_info *spc);
int send_register(int s, struct spc_info *spc, struct sockaddr_in *addr);
char* state2str(enum spc_state state);

#endif /* _SPC_H_ */
