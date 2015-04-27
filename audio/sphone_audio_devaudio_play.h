/*-----------------------------------------------------------------------------
  File:   sphone_audio_devaudio_play.h
  Description: audio module for unix /dev/audio - playout part
  Author: Olof Hagsand
  CVS Version: $Id: sphone_audio_devaudio_play.h,v 1.2 2004/01/11 20:23:09 olofh Exp $
 
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
#ifndef _SPHONE_AUDIO_DEVAUDIO_PLAY_H_
#define _SPHONE_AUDIO_DEVAUDIO_PLAY_H_

/*
 * Prototypes
 */ 
int audio_devaudio_play_init(play_api*, rcv_session_t *rs);

#endif  /* _SPHONE_AUDIO_DEVAUDIO_PLAY_H_ */


