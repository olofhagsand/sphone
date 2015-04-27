/*-----------------------------------------------------------------------------
  File:   sip_transaction.c
  Description: SIP transaction.
  Author: Olof Hagsand
  CVS Version: $Id: sip_transaction.c,v 1.1 2005/01/11 08:52:08 olof Exp $
 
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
 * From rfc 3261:
 Each transaction consists of a request that invokes a particular
   method, or function, on the server and at least one response. 

      Final Response: A response that terminates a SIP transaction, as
         opposed to a provisional response that does not.  All 2xx, 3xx,
         4xx, 5xx and 6xx responses are final.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>

#include "sip.h"

struct sip_transaction*
sip_transaction_create(struct sip_dialog *sd)
{
    struct sip_transaction *st;

    if ((st = (struct sip_transaction *)malloc(sizeof(*st))) == NULL){
	perror("sip_transaction_create: malloc");
	return NULL;
    }
    memset(st, 0, sizeof(*st));
    sprintf(st->st_branch, "%s%lx", BRANCH_MAGIC, random());
    st->st_dialog = sd;
    assert(sd->sd_transaction == NULL);
    sd->sd_transaction = st;
    return st;
}

int
sip_transaction_free(struct sip_transaction *st)
{
    struct sip_msg *sm;

    while ((sm = st->st_msg) != NULL){
	st->st_msg = sm->sm_next;
	sip_msg_free(sm);
    }
    free(st);
    return 0;
}
