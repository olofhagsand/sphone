/*-----------------------------------------------------------------------------
  File:   sip_dialog.c
  Description: SIP dialogs
  Author: Olof Hagsand
  CVS Version: $Id: sip_dialog.c,v 1.2 2005/01/11 08:45:58 olof Exp $
 
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
#include <netinet/in.h>

#include "sip.h"

struct sip_dialog*
sip_dialog_create(struct sip_client *sc, char *uri)
{
    struct sip_dialog *sd;
    int i;

    if ((sd = (struct sip_dialog *)malloc(sizeof(*sd))) == NULL){
	perror("sip_dialog_create: malloc");
	return NULL;
    }
    memset(sd, 0, sizeof(*sd));
    sprintf(sd->sd_call_id, "%lx", random());
    for (i=0; i<SM_RESPONSE; i++)
	sd->sd_cseq[i] = SIP_CSEQ_START;
    strncpy(sd->sd_peer_uri, uri, URI_LEN);
    sprintf(sd->sd_tag, "%lx", random());
    sd->sd_client = sc;
    assert(sc->sc_dialog == NULL);
    sc->sc_dialog = sd;

    return sd;
}

int
sip_dialog_free(struct sip_dialog *sd)
{
    if (sd->sd_transaction)
	sip_transaction_free(sd->sd_transaction);
    free(sd);
    return 0;
}
