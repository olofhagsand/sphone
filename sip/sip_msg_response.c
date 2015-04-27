/*-----------------------------------------------------------------------------
  File:   sip_msg_response.c
  Description: SIP responses
  Author: Olof Hagsand
  CVS Version: $Id: sip_msg_response.c,v 1.2 2005/01/12 18:13:03 olof Exp $
 
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

int
sip_parse_response(struct sip_msg *sm, char *line)
{
    if (dbg_sip)
	fprintf(stderr, "RESPONSE\n");
    return 0;
}


