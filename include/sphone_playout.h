/*-----------------------------------------------------------------------------
  File:   sphone_playout.h
  Description: playout indirection module
  Author: Olof Hagsand
  CVS Version: $Id: sphone_playout.h,v 1.4 2004/02/15 22:21:15 olofh Exp $
 
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
#ifndef _SPHONE_PLAYOUT_H_
#define _SPHONE_PLAYOUT_H_

/*
 * Generic playout interface function. 
 */
enum playout_type{
	PLAYOUT_FIFO,  /* Audio device only supports FIFO */
	PLAYOUT_SHMEM  /* Audio device only supports shared buffer */
}; /* What kind of playout */

/*
 * Prototypes
 */ 
int playout_init(enum playout_type, rcv_session_t *rs);
int playout_play(rcv_session_t *rs, char* buf, size_t len, int seqtype);
int playout_exit(void *arg);

#endif  /* _SPHONE_PLAYOUT_H_ */


