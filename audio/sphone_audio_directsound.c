/*-----------------------------------------------------------------------------
  File:   sphone_audio_directsound.c
  Description: audio module for WIN32 Directsound - common part
  Author: Olof Hagsand
  CVS Version: $Id: sphone_audio_directsound.c,v 1.8 2005/02/13 17:19:30 olof Exp $
 
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> /* open */

#include "sphone.h"

#ifdef HAVE_DIRECTSOUND
#include <dsound.h>
#include "sphone_audio_directsound.h"

/*
 * Variables
 */
/* Global variable for unique process resources */
struct audio_directsound *directsound;

/*
 * Common initializtion for both recording and playing of directsound
 */
int 
audio_directsound_init()
{
    if (directsound)
	return 0;
    if ((directsound = (void*)smalloc(sizeof(struct audio_directsound))) == NULL){
	sphone_error("audio_directsound_init: malloc: %s\n", 
		     strerror(errno));
	return -1;
    }
    directsound->ds_hwnd = CreateWindow("STATIC", "null", 
					0, 0, 0, 0, 0, 0, 0, 0, 0);

    return 0;
}

/*
 * Error function for directsound.
 * XXX: integrate with sphone_error()
 */
void 
directsound_error(HRESULT hr, char *template, ...)
{
  va_list args;
  char s[1024];

  va_start(args, template);
  switch(hr){
  case DSERR_ALLOCATED:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_ALLOCATED\n"
	    "The request failed because resources, such as a priority level"
	    ", were already in use by another caller.");
    sphone_verror(s, args); 
    break;
  case DSERR_CONTROLUNAVAIL:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_CONTROLUNAVAIL\n"
	    "The control (volume, pan, and so forth) requested by the caller"
	    "is not available."); 
    sphone_verror(s, args); 
    break;
  case DSERR_INVALIDPARAM:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_INVALIDPARAM\n"
	    "An invalid parameter was passed to the returning function.");
    sphone_verror(s, args); 
    break;
  case DSERR_INVALIDCALL:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_INVALIDCALL\n"
	    "This function is not valid for the current state of this object.");
    sphone_verror(s, args); 
    break; 
  case DSERR_GENERIC:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_GENERIC\n"
	    "An undetermined error occurred inside the DirectSound subsystem.");
    sphone_verror(s, args); 
    break;
  case DSERR_PRIOLEVELNEEDED:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_PRIOLEVELNEEDED\n"
	    "The caller does not have the priority level required for the "
	    "function to succeed.");
    sphone_verror(s, args); 
    break;

  case DSERR_OUTOFMEMORY:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_OUTOFMEMORY\n"
	    "The DirectSound subsystem could not allocate sufficient memory"
	    "to complete the caller's request.");
    sphone_verror(s, args); 
    break;
  case DSERR_BADFORMAT:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_BADFORMAT\n"
	    "The specified wave format is not supported.");
    sphone_verror(s, args); 
    break;
  case DSERR_UNSUPPORTED:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_UNSUPPORTED\n"
	    "The function called is not supported at this time.");
    sphone_verror(s, args); 
    break;
  case DSERR_NODRIVER:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_NODRIVER\n"
	    "No sound driver is available for use.");
    sphone_verror(s, args); 
    break;
  case DSERR_ALREADYINITIALIZED:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_ALREADYINITIALIZED\n"
	    "The object is already initialized.");
    sphone_verror(s, args); 
    break;
  case DSERR_NOAGGREGATION:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_NOAGGREGATION\n"
	    "The object does not support aggregation.");
    sphone_verror(s, args); 
    break;
  case DSERR_BUFFERLOST:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_BUFFERLOST"
	    "The buffer memory has been lost and must be restored.");
    sphone_verror(s, args); 
    break;
  case DSERR_OTHERAPPHASPRIO:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_OTHERAPPHASPRIO\n"
	    "Another application has a higher priority level, "
	    "preventing this call from succeeding");
    sphone_verror(s, args); 
    break; 
  case DSERR_UNINITIALIZED:
    sprintf(s, "%s\n%s", template, 
	    "Directsound Error: DSERR_UNINITIALIZED\n"
	    "The IDirectSound::Initialize method has not been called or has " 
	    "not been called successfully before other methods were called");
    sphone_verror(s, args); 
    break;
  case DS_OK:
    sprintf(s, "%s\n%s", template, "Directsound Error: DS_OK\n");
    sphone_verror(s, args); 
    break;
  default:
    sprintf(s, "%s\n%s", template, "Directsound Error: Unknown");
    sphone_verror(s, args); 
    break;
  }
  va_end(args);

}

#endif /* HAVE_DIRECTSOUND */
