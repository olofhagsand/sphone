/*-----------------------------------------------------------------------------
  File:   sphone_eventloop.h
  Description: Contains the event loop (select)
  Author: Olof Hagsand
 
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


#ifndef _SPHONE_EVENTLOOP_H_
#define _SPHONE_EVENTLOOP_H_

/*
 * Constants
 */
#define EVENT_STRLEN 32

/*
 * Prototypes
 */ 
int event_init(void *arg);
int eventloop_reg_timeout(struct timeval t,  int (*fn)(int, void*), void *arg, char *str);

int eventloop_reg_fd(int fd, int (*fn)(int, void*), void *arg, char *str);
int eventloop_unreg(int s, int (*fn)(int, void*), void *arg);
int eventloop(void);

#endif  /* _SPHONE_EVENTLOOP_H_ */
