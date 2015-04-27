/*-----------------------------------------------------------------------------
  File:   sip_msg.c
  Description: SIP messages: requests and responses
  Author: Olof Hagsand
  CVS Version: $Id: sip_msg.c,v 1.4 2005/01/31 18:37:24 olof Exp $
 
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
      Request: A SIP message sent from a client to a server, for the
         purpose of invoking a particular operation.

      Response: A SIP message sent from a server to a client, for
         indicating the status of a request sent from the client to the
         server.
	 A valid SIP request MUST contain: To, From, CSeq, Call-ID, 
	 Max-Forwards, and Via.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>

#include "sip.h"

/*
 * Simply alocate sip_message
 */
struct sip_msg*
sip_msg_new()
{
    struct sip_msg *sm;

    if ((sm = (struct sip_msg *)malloc(sizeof(*sm))) == NULL){
	perror("sip_msg_create: malloc");
	return NULL;
    }
    memset(sm, 0, sizeof(*sm));
    return sm;
}

/*
 * Create and initialize sip_message
 */
struct sip_msg*
sip_msg_create(struct sip_transaction *st, enum sip_msg_type type)
{
    struct sip_msg *sm;

    if ((sm = sip_msg_new()) == NULL)
	return NULL;
    sm->sm_type = type;
    sm->sm_next = st->st_msg;
    st->st_msg = sm;
    sm->sm_transaction = st;
    return sm;
}

/*
 * sip_msg_free
 * Deallocate sip_message and everything that belongs to it.
 */
int
sip_msg_free(struct sip_msg *sm)
{
    struct sip_msghdr *smh;

    if (sm->sm_body)
	free(sm->sm_body);
    while ((smh = sm->sm_msghdrs) != NULL){
	sm->sm_msghdrs = smh->smh_next;
	free(smh);
    }
    free(sm);
    return 0;
}


/*-----------------------------------------------------------------
 * Parsing / Unmarshaling
 *-----------------------------------------------------------------*/
/*
 * Identify request from response
 * response, eg: SIP/2.0 407 Proxy Authentication Required
 * request, eg: INVITE sip:000730631661@kth.se SIP/2.0
 * returns: -1 error
 *           0 OK, drop
 *           1 OK, continue
 */
static int
sip_hdrline_parse(struct sip_msg *sm, char *line)
{
    char *tag;

    tag = strsep(&line, " ");
    if (strcmp(tag, SIP_VERSION) == 0)
	return sip_parse_response(sm, line);
    else
    if (strcmp(tag, "INVITE") == 0)
	return sip_msg_invite_parse(sm, line);
    fprintf(stderr, "sip_parse_header: %s not yet supported\n", tag);
    return -1;
}

/*
 * sip_msg_parse
 * Given the internal message string as received on the network,
 * parse it and put the contents in the structured sid_msg data object.
 */
int 
sip_msg_parse(struct sip_msg *sm)
{
    char *str, *str0;
    char *line;

    str0 = str = strdup(sm->sm_msg);
    /* First request/response line */
    line = strsep(&str, "\n");      
    if (sip_hdrline_parse(sm, line) < 0){
	free(str0);
	return -1;
    }
    /* Then the message headers */
    while ((line = strsep(&str, "\n")) != NULL && strlen(line)){
	printf("line:%s\n", line);
	if (msghdr_parse(sm, line) < 0){
	    free(str0);
	    return -1;
	}
	printf("after\n");
    }
    free(str0);
    return 0;
}

/*-----------------------------------------------------------------
 * Packing / Marshaling
 *-----------------------------------------------------------------*/
/*
 * sip_msg_hdrline_pack
 * Given a completed sip_message, pack into string line.
 */
static char *
sip_msg_hdrline_pack(struct sip_msg *sm, char *line, size_t *len)
{
    switch (sm->sm_type){
    case SM_INVITE:
	line = sip_msg_invite_pack(sm, line, len);
	break;
    case SM_ACK:
	break;
    case SM_OPTIONS:
	break;
    case SM_BYE:
	break;
    case SM_CANCEL:
	break;
    case SM_REGISTER:
	break;
    case SM_RESPONSE:
	break;
    case SM_UNKNOWN:
	break;
    }
    return line;
}

/*
 * sip_msg_pack
 * Given a sid_message completely created and filled in, pack it into 
 * the (internal) message.
 */
int 
sip_msg_pack(struct sip_msg *sm)
{
    char *line;
    size_t len;
    struct sip_msghdr *smh;

    line = sm->sm_msg; /* use the internal string to marshal */
    len = SIP_MSG_LEN;
    line = sip_msg_hdrline_pack(sm, line, &len); /* First pack header */
    /* Then all message headers */
    for (smh=sm->sm_msghdrs; smh; smh=smh->smh_next)
	line = msghdr_pack(line, &len, smh);
    if (sm->sm_body){
	strncat(line, sm->sm_body, len);
	len -= strlen(sm->sm_body);
	line += strlen(sm->sm_body);
    }
    assert((SIP_MSG_LEN-len) == strlen(sm->sm_msg));
    assert(strlen(sm->sm_msg) == (line-sm->sm_msg));
    return 0;
}
