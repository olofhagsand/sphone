/*-----------------------------------------------------------------------------
  File:   sip_msg.h
  Description: SIP messages, see rfc 3261
  Author: Olof Hagsand
  CVS Version: $Id: sip_msg.h,v 1.4 2005/01/13 09:37:01 olof Exp $
 
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
#ifndef _SIP_MSG_H_
#define _SIP_MSG_H_

/*
 * Constants
 */
#define SIP_MSG_LEN 1400

/*
 * Types
 */
enum sip_msg_type{
    SM_UNKNOWN,
    SM_INVITE,
    SM_ACK,
    SM_OPTIONS,
    SM_BYE,
    SM_CANCEL,
    SM_REGISTER,
    SM_RESPONSE
};

union sip_msg_union{
    struct sip_msg_invite _smu_invite;
};

struct sip_msg{
    struct sip_msg      *sm_next;              /* next message */
    struct sip_transaction *sm_transaction;    /* backpointer */
    char                 sm_msg[SIP_MSG_LEN];  /* Actual message on the wire */
    enum sip_msg_type    sm_type;              /* Message type */
    union sip_msg_union _sm_msg;               /* Actual message type */
    char                *sm_body;
    struct sip_msghdr   *sm_msghdrs;      /* Linked list of msghdrs */
};

/* access macros, one for each union element */
#define sm_invite     _sm_msg._smu_invite

/*
 * Prototypes
 */ 
struct sip_msg *sip_msg_create(struct sip_transaction *st,
			       enum sip_msg_type type);
struct sip_msg *sip_msg_new(void);
int sip_msg_free(struct sip_msg *sm);
int sip_msg_parse(struct sip_msg *sm);
int sip_msg_pack(struct sip_msg *sm);

#endif  /* _SIP_MSG_H_ */


