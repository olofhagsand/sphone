/*-----------------------------------------------------------------------------
  File:   sphone_debug.h
  Description: Debugging functions
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

#ifndef _SPHONE_DEBUG_H_
#define _SPHONE_DEBUG_H_

/*
 * Constants
 */

/*
 *  If DEBUG is set, you may have the following debug levels that you
 * specify in dbg_print
 */
#define DBG_RCV      (1<<0)
#define DBG_SEND     (1<<1)
#define DBG_EVENT    (1<<2)
#define DBG_INET     (1<<3)
#define DBG_RTP      (1<<4)
#define DBG_RTCP     (1<<5)
#define DBG_CODING   (1<<6)  
#define DBG_PLAY     (1<<7)  
#define DBG_RECORD   (1<<8)  
#define DBG_MALLOC   (1<<9)  
#define DBG_EXIT     (1<<10)  
#define DBG_DETAIL   (1<<11)  /* Extra details */
#define DBG_APP      (1<<12)  /* Generic application: your app can use this */
#define DBG_SIP      (1<<13)  
#define DBG_ALWAYS   (1<<31)  /* Print regardless */

/*
 * Types
 */

/*
 * Variables
 */
extern unsigned int debug;

/*
 * Prototypes
 */ 
int dbg_init(FILE*);
void dbg_print(int level, char *format, ...);
void dbg_print_pkt(int level, char *str, char *buf, size_t len);

#endif  /* _SPHONE_DEBUG_H_ */
