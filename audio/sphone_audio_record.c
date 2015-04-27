/*-----------------------------------------------------------------------------
  File:   sphone_audio_record.c
  Description: Wrapper for all audio record modules
  Author: Olof Hagsand
 
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
#include "audio/sphone_audio_file_record.h"
#ifdef HAVE_SYS_AUDIOIO_H
#include "audio/sphone_audio_devaudio_record.h"
#endif
#ifdef HAVE_DIRECTSOUND
#include "audio/sphone_audio_directsound_record.h"
#endif

struct audio_record{
    /* Private common fields */
    send_session_t *ar_ss;        /* Backpointer to send_session struct */
    int           (*ar_callback)();/* Function called for each sampled packet */
    void           *ar_cb_arg;     /* Callback function argument */
};

int audio_record_callback(record_api *ra, int (*fn)(char*, int, void*), void* arg)
{
    struct audio_record *ar = 
	(struct audio_record *)ra->ra_private;

    ar->ar_callback = fn;
    ar->ar_cb_arg = arg;
    return 0;
}


record_api *
audio_record_init(enum audio_type type, void *arg)
{
    record_api *ra;
    int retval;
    send_session_t *ss = (send_session_t*)arg;

    if ((ra = (void*)smalloc(sizeof(record_api))) == NULL){
	sphone_error("audio_record_init: malloc: %s", strerror(errno));
	return NULL;
    }
    memset(ra, 0, sizeof(record_api));
    ra->ra_callback = audio_record_callback;

    switch (type){
    case AUDIO_FILE:
	retval = audio_file_record_init(ra, (send_session_t*)ss);
	if (retval < 0)
	    return NULL;
	break;
#ifdef HAVE_SYS_AUDIOIO_H
    case AUDIO_DEVAUDIO:
	retval = audio_devaudio_record_init(ra, ss);
	if (retval < 0)
	    return NULL;
	break;
#endif /* HAVE_SYS_AUDIOIO_H */
#ifdef HAVE_DIRECTSOUND
    case AUDIO_DIRECTSOUND:
	retval = audio_directsound_record_init(ra, ss);
	if (retval < 0)
	    return NULL;
	break;
#endif /* HAVE_DIRECTSOUND */
    default:
	sphone_error("audio_record_init: audio module %d not defined", type);
	sfree(ra);
	ra = NULL;
	break;
    }
    return ra;
}

