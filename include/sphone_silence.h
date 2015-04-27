/*-----------------------------------------------------------------------------
  File:   sphone_silence.h
  Description: Code for silence detection
  Author: Olof Hagsand
  CVS Version: $Id: sphone_silence.h,v 1.2 2004/01/03 17:18:01 olofh Exp $
 
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
#ifndef _SPHONE_SILENCE_H_
#define _SPHONE_SILENCE_H_

/*
 * Constants
 */
#define SILENCE_LEVEL_8 3    /* Level for silence pcm 8 bit */
#define SILENCE_LEVEL_16 100 /* Level for silence pcm 16 bit */
#define SILENCE_THRESHOLD_MS 200 /* How many ms to wait before reacting on silence */

/*
 * Prototypes
 */ 
int silence(char *buf, int len, uint32_t bits_samp, 
	    int *silence_nr, int silence_threshold);


#endif  /* _SPHONE_SILENCE_H_ */


