/*-----------------------------------------------------------------------------
  File:   sip_client.c
  Description: SIP clients
  Author: Olof Hagsand
  CVS Version: $Id: sip_client.c,v 1.4 2005/01/12 18:13:03 olof Exp $
 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sip.h"

int dbg_sip = 0;

struct sip_client*
sip_client_create(struct in_addr addr, 
		  uint16_t port,
		  struct in_addr server_addr, 
		  uint16_t server_port,
		  char *disp, char *uri)
{
    struct sip_client *sc;
    
    if ((sc = (struct sip_client *)malloc(sizeof(*sc))) == NULL){
	perror("sip_client_create: malloc");
	return NULL;
    }
    memset(sc, 0, sizeof(*sc));
    sc->sc_addr.sin_family = AF_INET;;
#ifdef HAVE_SIN_LEN
    sc->sc_addr.sin_len = sizeof(struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
    sc->sc_addr.sin_addr    = addr;
    sc->sc_addr.sin_port    = htons(port);

    sc->sc_server_addr.sin_family = AF_INET;;
#ifdef HAVE_SIN_LEN
    sc->sc_server_addr.sin_len = sizeof(struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
    sc->sc_server_addr.sin_addr    = server_addr;
    sc->sc_server_addr.sin_port    = htons(server_port);

    strncpy(sc->sc_disp, disp, DISP_LEN);
    strncpy(sc->sc_uri, uri, URI_LEN);

    return sc;
}

int
sip_client_free(struct sip_client *sc)
{
    if (sc->sc_s)
	close(sc->sc_s);
    if (sc->sc_dialog)
	sip_dialog_free(sc->sc_dialog);
    free(sc);
    return 0;
}
