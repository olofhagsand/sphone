/*-----------------------------------------------------------------------------
  File:   sip_user_agent.h
  Description: SIP clients
  Author: Olof Hagsand
  CVS Version: $Id: sip_client.h,v 1.3 2005/01/11 08:45:58 olof Exp $
 
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
  A client is any network element that sends SIP requests
  and receives SIP responses.  Clients may or may not interact
  directly with a human user.  User agent clients and proxies are clients.
*/
#ifndef _SIP_CLIENT_H_
#define _SIP_CLIENT_H_

/*
 * Constants
 */

/*
 * Types
 */
struct sip_client{
    struct sip_dialog      *sc_dialog;         /* dialogs */
    int                     sc_s;              /* udp socket */
    char                    sc_disp[DISP_LEN]; /* my display */
    char                    sc_uri[URI_LEN];   /* my URI */
    struct sockaddr_in      sc_addr;           /* my addr */
    struct sockaddr_in      sc_server_addr;    /* server addr */
};

/*
 * Prototypes
 */ 
struct sip_client *sip_client_create(struct in_addr addr, 
				     uint16_t port,
				     struct in_addr server_addr,
				     uint16_t server_port,
				     char *disp,  char *uri);
int sip_client_free(struct sip_client *sc);

#endif  /* _SIP_CLIENT_H_ */


