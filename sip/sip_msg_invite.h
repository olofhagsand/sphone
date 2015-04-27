/*-----------------------------------------------------------------------------
  File:   sip_msg_invite.h
  Description: SIP INVITE request
  Author: Olof Hagsand
  CVS Version: $Id: sip_msg_invite.h,v 1.4 2005/01/12 18:13:03 olof Exp $
 
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
#ifndef _SIP_MSG_INVITE_H_
#define _SIP_MSG_INVITE_H_

/*
 * Constants
 */

/*
 * Types
 */
struct sip_msg_invite{
    char   smi_uri[URI_LEN];
};

/*
 * Prototypes
 */ 
int sip_msg_invite_construct(struct sip_msg *sm, 
			     struct sip_transaction *st,
			     struct sip_dialog *sd, 
			     struct sip_client *sc);

int sip_msg_invite_parse(struct sip_msg *sm, char *line);
char *sip_msg_invite_pack(struct sip_msg *sm, char *line, size_t *len);


#endif  /* _SIP_MSG_INVITE_H_ */


