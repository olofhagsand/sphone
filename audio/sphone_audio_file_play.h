/*-----------------------------------------------------------------------------
  File:   sphone_audio_file_play.h
  Description: audio file emulation module - playout part
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
#ifndef _SPHONE_AUDIO_FILE_PLAY_H_
#define _SPHONE_AUDIO_FILE_PLAY_H_

/*
 * Prototypes
 */ 
int audio_file_play_init(play_api*, rcv_session_t *rs);

#endif  /* _SPHONE_AUDIO_FILE_PLAY_H_ */


