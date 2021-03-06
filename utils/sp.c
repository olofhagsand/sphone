/*-----------------------------------------------------------------------------
  File:   sp.c
  Description: Sicsophone protocol
  Author: Olof Hagsand
  CVS Version: $Id: sp.c,v 1.4 2004/12/09 13:51:29 olof Exp $
 
  This software is a part of SICSPHONE, a real-time, IP-based system for 
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

#ifdef HAVE_CONFIG_H
#include "config.h" /* generated by config & autoconf */
#endif

#include "sphone.h" 
#include "sp.h"

char*
sptype2str(enum sp_type type)
{
    switch (type){
    case SP_TYPE_REGISTER: return "SP_TYPE_REGISTER";
	break;
    case SP_TYPE_DEREGISTER: return "SP_TYPE_DEREGISTER";
	break;
    case SP_TYPE_START_SEND: return "SP_TYPE_START_SEND";
	break;
    case SP_TYPE_START_RCV: return "SP_TYPE_START_RCV";
	break;
    case SP_TYPE_REPORT_SEND: return "SP_TYPE_REPORT_SEND";
	break;
    case SP_TYPE_REPORT_RCV: return "SP_TYPE_REPORT_RCV";
	break;
    case SP_TYPE_EXIT: return "SP_TYPE_EXIT";
	break;
    }
    return "<null>";
}


char *
sp_addr_encode(struct sockaddr_in *sin, char *p)
{
    assert(sin && p);
    memset(p, 0, sizeof(struct sockaddr_in));
    *(uint8_t*)p  = sin->sin_family;      p += 2;  /* XXX - pad byte */
    *(uint16_t*)p = sin->sin_port;        p += 2;
    *(uint32_t*)p = sin->sin_addr.s_addr; p += 4;

    return p;
}

char *
sp_addr_decode(struct sockaddr_in *sin, char *p)
{
    assert(sin && p);
    memset(sin, 0, sizeof(struct sockaddr_in));
    sin->sin_family      = *(uint8_t*)p;     p += 2; /* XXX - pad byte */
#ifdef HAVE_SIN_LEN
    sin->sin_len         = sizeof(struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
    sin->sin_port        = *(uint16_t*)p;    p += 2;
    sin->sin_addr.s_addr = *(uint32_t*)p;    p += 4;
    return p;
}
