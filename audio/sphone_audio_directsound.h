/*-----------------------------------------------------------------------------
  File:   sphone_audio_directsound.h
  Description: audio module for WIN32 Directsound - common part
  Author: Olof Hagsand
  CVS Version: $Id: sphone_audio_directsound.h,v 1.4 2004/02/06 21:28:33 olofh Exp $
 
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
#ifndef _SPHONE_AUDIO_DIRECTSOUND_H_
#define _SPHONE_AUDIO_DIRECTSOUND_H_

/*
 * Types
 */
struct audio_directsound{
    HWND                       ds_hwnd;
};

/*
 * Variables
 */
extern struct audio_directsound *directsound;

/*
 * Prototypes
 */ 
int audio_directsound_init(void);
void directsound_error(HRESULT hr, char *template, ...);

#endif  /* _SPHONE_AUDIO_DIRECTSOUND_H_ */


