/*-----------------------------------------------------------------------------
  File:   sip_msg_invite.c
  Description: SIP INVITE request
  Author: Olof Hagsand
  CVS Version: $Id: sip_msg_invite.c,v 1.4 2005/01/31 18:37:24 olof Exp $
 
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sip.h"

/*
 * sip_msg_invite_construct
 * Construct the INVITE message by producing all msg structs
 * After this, the message should be ready to be packet/marshalled 
 * (see sid_msg_pack)
 */
int 
sip_msg_invite_construct(struct sip_msg *sm, 
			 struct sip_transaction *st,
			 struct sip_dialog *sd, 
			 struct sip_client *sc)
{
    struct sip_msg_invite *si = &sm->sm_invite;
    struct sip_msghdr *smh;

    /* First fill in request line */
    strncpy(si->smi_uri, sd->sd_peer_uri, URI_LEN);

    /* Then fill in Via message header */
    if ((smh = msghdr_add(sm, MH_Via)) == NULL)
	return -1;
    smh->mh_Via.mv_addr = sc->sc_addr;
    strncpy(smh->mh_Via.mv_branch, st->st_branch, BRANCH_LEN);

    /* Max_Forwards message header */
    if ((smh = msghdr_add(sm, MH_Max_Forwards)) == NULL)
	return -1;

    /* To message header */
    if ((smh = msghdr_add(sm, MH_To)) == NULL)
	return -1;
    strncpy(smh->mh_To.mt_uri, sd->sd_peer_uri, URI_LEN);

    /* From message header */
    if ((smh = msghdr_add(sm, MH_From)) == NULL)
	return -1;
    strncpy(smh->mh_From.mf_disp, sc->sc_disp, DISP_LEN);
    strncpy(smh->mh_From.mf_uri, sc->sc_uri, URI_LEN);
    strncpy(smh->mh_From.mf_tag, sd->sd_tag, TAG_LEN);

    /* Call-ID message hea99der */
    if ((smh = msghdr_add(sm, MH_Call_ID)) == NULL)
	return -1;
    strncpy(smh->mh_Call_ID.mci_call_id, sd->sd_call_id, TAG_LEN);
    smh->mh_Call_ID.mci_in_addr = sc->sc_addr.sin_addr;

    /* CSeq message header */
    if ((smh = msghdr_add(sm, MH_CSeq)) == NULL)
	return -1;
    smh->mh_CSeq.mc_cseq = sd->sd_cseq[SM_INVITE]++;
    strncpy(smh->mh_CSeq.mc_type, "INVITE", SIP_FLEN);

    /* Contact message header */
    if ((smh = msghdr_add(sm, MH_Contact)) == NULL)
	return -1;
    strncpy(smh->mh_Contact.mc_disp, sc->sc_disp, DISP_LEN);
    smh->mh_Contact.mc_addr = sc->sc_addr;

    /* Content-Type message header */
    if ((smh = msghdr_add(sm, MH_Content_Type)) == NULL)
	return -1;

    /* Content-Length message header */
    if ((smh = msghdr_add(sm, MH_Content_Length)) == NULL)
	return -1;
    smh->mh_Content_Length.mcl_len = sm->sm_body ? strlen(sm->sm_body) : 0;

    /* Accept message header */
    if ((smh = msghdr_add(sm, MH_Accept)) == NULL)
	return -1;
    return 0;
}

/*
 * sip_msg_invite_parse
 * From line, construct msg_invite struct.
 */
int
sip_msg_invite_parse(struct sip_msg *sm, char *line)
{
    char *uri;
    char *ver;
    struct sip_msg_invite *smi = &sm->sm_invite;

    sm->sm_type = SM_INVITE;
    uri = strsep(&line, " \t");
    ver = strsep(&line, " \t");
    if (strcmp(ver, SIP_VERSION)){
	fprintf(stderr, "sip_parse_invite_header: wrong version: %s\n", ver);
	return -1;
    }
    strncpy(smi->smi_uri, uri, URI_LEN);
    if (dbg_sip)
	fprintf(stderr, "INVITE\n");
    return 0;
}

/*
 * sip_msg_invite_pack
 * Assume msg_invite struct has been constructed
 * From the struct, produce a marshaled message
 */
char*
sip_msg_invite_pack(struct sip_msg *sm, char *line, size_t *len)
{
    struct sip_msg_invite *smi = &sm->sm_invite;

    snprintf(line, *len, "INVITE %s %s\n", 
	     smi->smi_uri, 
	     SIP_VERSION);
    *len -= strlen(line);
    line += strlen(line);
    return line;
}
