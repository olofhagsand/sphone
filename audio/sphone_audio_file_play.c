/*-----------------------------------------------------------------------------
  File:   sphone_audio_file_play.c
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> /* open */

#include "sphone.h"
#include "sphone_audio_file_play.h"

struct audio_file_play{
    /* Private fields */
    rcv_session_t  *ap_rs; /* Backpointer to receive session */

    /* Specialized for emulated  */
    int ap_fd;
};



int 
audio_file_play_start(play_api *pa)
{
	return 0;
}

/* Called when settings change */
int 
audio_file_play_settings(play_api *pa, coding_params *cp)
{
    return 0;
}


int audio_file_play_ioctl(play_api *pa, int op, void *oparg)
{
/*    struct audio_file_play *ap = 
      (struct audio_file_play *)pa->pa_private;*/

    switch (op){
    case SPHONE_AUDIO_IOCTL_PLAYOUT_GET:
	*(int*)oparg = PLAYOUT_FIFO;
	break;
    default:
	sphone_error("audio_file_play_ioctl: No such op: %d", op);
	return -1;
	break;
    }

    return 0;
}

int audio_file_play_open(play_api *pa, void *arg)
{
    struct audio_file_play *ap = 
	(struct audio_file_play *)pa->pa_private;
    char *filename = (char*)arg;

    if (ap->ap_fd)
	close(ap->ap_fd);
    dbg_print(DBG_PLAY, "audio_file_play_open file:%s\n", filename);
#ifdef WIN32
    ap->ap_fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY), 00666;
#else
    ap->ap_fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 00666);
#endif
    if (ap->ap_fd < 0){
	sphone_error("audio_file_play_open open: \"%s\": %s", 
		     filename, strerror(errno));
	return -1;
    }
    return 0;
}

int 
audio_file_play_pos(play_api *pa, uint32_t *play_pos, uint32_t *write_pos)
{
    return 0;
}

int 
audio_file_play_stop(play_api *pa)
{
    return 0;
}

int 
audio_file_play_play(play_api *pa, char *pkt, size_t len, void *pos)
{
    struct audio_file_play *ap = 
	(struct audio_file_play *)pa->pa_private;
    struct rcv_stats *rst = &ap->ap_rs->rs_stats;

    dbg_print(DBG_PLAY, "audio_file_play_play %d bytes\n", len);
    if (write(ap->ap_fd, pkt, len) < 0){
	sphone_error("audio_file_play: write: %s\n", strerror(errno));
	return -1;
    }
    rst->rst_pbytes += len;
    rst->rst_ppkt ++;
    return 0;
}

int
audio_file_play_exit(play_api *pa)
{
    struct audio_file_play *ap = 
	(struct audio_file_play *)pa->pa_private;

    if (ap)
	sfree(ap);
    sfree(pa);
    return 0;
}


int
audio_file_play_init(play_api *pa, rcv_session_t *rs)
{
    struct audio_file_play *ap;

    pa->pa_start     = audio_file_play_start;
    pa->pa_open      = audio_file_play_open;
    pa->pa_play      = audio_file_play_play;
    pa->pa_play_pos  = audio_file_play_pos;
    pa->pa_play_stop = audio_file_play_stop;
    pa->pa_settings  = audio_file_play_settings;
    pa->pa_ioctl     = audio_file_play_ioctl;
    pa->pa_exit      = audio_file_play_exit;

    if ((ap = (void*)smalloc(sizeof(struct audio_file_play))) == NULL){
	sphone_error("audio_file_play_init: malloc: %s", strerror(errno));
	return -1;
    }
    memset(ap, 0, sizeof(struct audio_file_play));
    pa->pa_private   = ap;
    ap->ap_rs        = rs;

    dbg_print(DBG_PLAY, "audio_file_play_init\n");

    return 0;
} 

