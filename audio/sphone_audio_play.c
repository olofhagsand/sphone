/*-----------------------------------------------------------------------------
  File:   sphone_audio_play.c
  Description: Wrapper for all audio play modules
  Author: Olof Hagsand
  CVS Version: $Id: sphone_audio_play.c,v 1.7 2004/02/01 21:42:45 olofh Exp $
 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "sphone.h"
#include "audio/sphone_audio_file_play.h"
#ifdef HAVE_SYS_AUDIOIO_H
#include "audio/sphone_audio_devaudio_play.h"
#endif /* HAVE_SYS_AUDIOIO_H */
#ifdef HAVE_DIRECTSOUND
#include "audio/sphone_audio_directsound_play.h"
#endif /* HAVE_DIRECTSOUND */

struct audio_play{
    /* Private fields */
    rcv_session_t  *ap_rs; /* Backpointer to receive session */
};


play_api *
audio_play_init(enum audio_type type, void *arg)
{
    play_api *pa;
    int retval;
    rcv_session_t *rs = (rcv_session_t*)arg;

    if ((pa = (void*)smalloc(sizeof(play_api))) == NULL){
	sphone_error("audio_play_init: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(pa, 0, sizeof(play_api));
    switch (type){
    case AUDIO_FILE:
	retval = audio_file_play_init(pa, rs);
	if (retval < 0)
	    return NULL;
	break;
#ifdef HAVE_SYS_AUDIOIO_H
    case AUDIO_DEVAUDIO:
	retval = audio_devaudio_play_init(pa, rs);
	if (retval < 0)
	    return NULL;
	break;
#endif /* HAVE_SYS_AUDIOIO_H */
#ifdef HAVE_DIRECTSOUND
    case AUDIO_DIRECTSOUND:
	retval = audio_directsound_play_init(pa, rs);
	if (retval < 0)
	    return NULL;
	break;
#endif /* HAVE_DIRECTSOUND */
    default:
	sphone_error("audio_play_init: audio module %d not defined", type);
	sfree(pa);
	pa = NULL;
	break;
    }
    return pa;
}
