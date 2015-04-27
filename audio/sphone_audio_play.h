/*-----------------------------------------------------------------------------
  File:   sphone_audio_play.h
  Description: audio playout indirection module
  Author: Olof Hagsand
  CVS Version: $Id: sphone_audio_play.h,v 1.7 2004/02/15 22:21:15 olofh Exp $
 
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
#ifndef _SPHONE_AUDIO_PLAY_H_
#define _SPHONE_AUDIO_PLAY_H_

/*
 * Generic audio interface function. 
 *
 * A structure describing an indirection interface as follows:
 *
   pa_start             Initialization 
   pa_play              Play (output) audio samples.
   
 */ 


/*
 * Audio module types. XXX: should really be in sphone_audio.h
 */
enum audio_type{
    AUDIO_FILE,
    AUDIO_DIRECTSOUND,
    AUDIO_DEVAUDIO
};

typedef struct audio_play_api play_api;
struct audio_play_api{
    int (*pa_open)(play_api *, void *);    /* Open file/device */
    int (*pa_start)(play_api *);        
    int (*pa_play)(play_api *,             /* audio-module specifics */
		   char *pkt,              /* audio samples */
		   size_t len,             /* packet size */
		   void *pos);             /* where to place packet */
    int (*pa_play_pos)(play_api *,         /* audio-module specifics */
		       uint32_t *play_pos, /* Current play cursor */
		       uint32_t *write_pos /* Current write play cursor */
		       );
    int (*pa_play_stop)(play_api *);       /* audio-module specifics */
    int (*pa_settings)(play_api *, coding_params *); /* Called when settings change */
    int (*pa_ioctl)(play_api *, int op, void *arg);        
    int (*pa_exit)(play_api *);        
    void *pa_private;                      /* private data */
};

/*
 * IOCTL's
 */ 
#define SPHONE_AUDIO_IOCTL_LOOP        1 /* int [0|1] */
#define SPHONE_AUDIO_IOCTL_PLAYOUT_GET 2 /* Return supported playout mode */
#define SPHONE_AUDIO_IOCTL_PLAYOUT_SET 3 /* Return supported playout mode */
#define SPHONE_AUDIO_IOCTL_FD          4 /* Return file descriptor */
#define SPHONE_AUDIO_IOCTL_PLAYBUFLEN  5 /* Return playbuflen */

/*
 * Prototypes
 */ 
play_api *audio_play_init(enum audio_type type, void *rs);

#endif  /* _SPHONE_AUDIO_PLAY_H_ */


