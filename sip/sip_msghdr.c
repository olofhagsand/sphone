/*-----------------------------------------------------------------------------
  File:   sip_msghdr.c
  Description: SIP messages headers.
  Author: Olof Hagsand
  CVS Version: $Id: sip_msghdr.c,v 1.4 2005/01/31 18:37:24 olof Exp $
 
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sip.h"

char *msghdrs[] = {"Accept",
		   "Accept-Encoding",
		   "Accept-Language",
		   "Alert-Info",
		   "Allow",
		   "Authentication-Info",
		   "Authorization",
		   "Call-ID",
		   "Call-Info",
		   "Contact",
		   "Content-Disposition",
		   "Content-Encoding",
		   "Content-Language",
		   "Content-Length",
		   "Content-Type",
		   "CSeq",
		   "Date",
		   "Error-Info",
		   "Expires",
		   "From",
		   "In-Reply-To",
		   "Max-Forwards",
		   "MIME-Version",
		   "Min-Expires",
		   "Organization",
		   "Priority",
		   "Proxy-Authenticate",
		   "Proxy-Authorization",
		   "Proxy-Require",
		   "Record-Route",
		   "Reply-To",
		   "Require",
		   "Retry-After",
		   "Route",
		   "Server",
		   "Subject",
		   "Supported",
		   "Timestamp",
		   "To",
		   "Unsupported",
		   "User-Agent",
		   "Via",
		   "Warning",
		   "WWW-Authenticate"
};

/*
 * msghdr2enum
 * Translate from string to msghdr type
 */
enum msghdr_type
msghdr2enum(char *str)
{
    char *s;
    int i;

    s = msghdrs[0];
    for (i=0; i<sizeof(msghdrs); i++){
	s = msghdrs[i];
	if (strcmp(str, s) == 0)
	    return (enum msghdr_type)i;
    }
    return -1;
}

struct sip_msghdr*
msghdr_add(struct sip_msg *sm, enum msghdr_type type)
{
    struct sip_msghdr *smh, *smhp = NULL;

    for (smh=sm->sm_msghdrs; smh; smh=smh->smh_next)
	smhp = smh;
    if ((smh = (struct sip_msghdr*)malloc(sizeof(*smh))) == NULL){
	perror("msghdr_add: malloc");
	return NULL;
    }
    memset(smh, 0, sizeof(*smh));
    if (smhp)
	smhp->smh_next = smh;
    else
	sm->sm_msghdrs = smh;
    smh->smh_type = type;
    return smh;
}



