/*-----------------------------------------------------------------------------
  File:   sphone_inet.h
  Description:  sphone internet network code
  Author: Olof Hagsand
  CVS Version: $Id: sphone_inet.h,v 1.6 2005/01/11 08:45:57 olof Exp $
 
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
#ifndef _SPHONE_INET_H_
#define _SPHONE_INET_H_

/*
 * Variables
 */ 

/*
 * Prototypes
 */ 
int inet_host2addr(char *hostname, uint16_t port, struct sockaddr_in *addr);
int inet_init(int *s0, int *s1);
int inet_bind(int s, uint16_t port);
int inet_input(int s, char *buf, int *len, struct sockaddr_in *from);
int inet_get_default_addr(struct in_addr *addr);

#endif  /* _SPHONE_INET_H_ */


