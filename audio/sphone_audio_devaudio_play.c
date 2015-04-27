/*-----------------------------------------------------------------------------
  File:   sphone_audio_devaudio_play.c
  Description: audio module for unix /dev/audio - playout part
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

#ifdef HAVE_SYS_AUDIOIO_H
#include <sys/mman.h> /* mmap */
#include <sys/ioctl.h>

#include "sphone_audio_devaudio.h"
#include "sphone_audio_devaudio_play.h"

#include <sys/audioio.h>

struct audio_devaudio_play{
    /* Private fields */
    rcv_session_t  *ap_rs;   /* Backpointer to receive session */

    /* Specialized for devaudio  */
    struct devaudio_device *ap_devaudio; /* Pointer to devaudio truct */
    int                     ap_mode;     /* Save mode: play/rec, etc */
    int                     ap_playout_type; /* shmem or fifo */
    int                     ap_mmap_buffer_size; /* size of mmap buffer */
    void                   *ap_mmap_buffer; /* Mmap buffer */
};


/* Called when settings change */
int 
audio_devaudio_play_settings(play_api *pa, coding_params *cp)
{
    return 0;
}


int audio_devaudio_play_ioctl(play_api *pa, int op, void *oparg)
{
    struct audio_devaudio_play *ap = 
	(struct audio_devaudio_play *)pa->pa_private; 

    switch (op){
    case SPHONE_AUDIO_IOCTL_PLAYOUT_GET:
	*(int*)oparg = ap->ap_playout_type;
	break;
    case SPHONE_AUDIO_IOCTL_PLAYOUT_SET:
	if (*(int*)oparg == PLAYOUT_SHMEM && 
	    !ap->ap_devaudio->dd_shmem){
	    sphone_warning("audio_devaudio_play_ioctl: Device does not support shared mem\n");
	    return -1;
	}
	ap->ap_playout_type = *(int*)oparg;
	/* XXX: change callbacks */
	break;
    case SPHONE_AUDIO_IOCTL_FD:
	*(int*)oparg = ap->ap_devaudio->dd_fd;
	break;
    default:
	sphone_warning("audio_devaudio_play_ioctl: No such op: %d", op);
	break;
    }
    return 0;
}

int 
audio_devaudio_play_pos(play_api *pa, uint32_t *play_pos, uint32_t *write_pos)
{
    struct audio_devaudio_play *ap = 
	(struct audio_devaudio_play *)pa->pa_private; 
    audio_offset_t ao;

    assert(ap->ap_devaudio);
    assert(ap->ap_playout_type == PLAYOUT_SHMEM);
    if (ioctl(ap->ap_devaudio->dd_fd, AUDIO_GETOOFFS, &ao) < 0){
	sphone_error("audio_devaudio_play_pos: ioctl AUDIO_GETOOFFS: %s",
		     strerror(errno));
	return -1;
    }
    *write_pos = ao.offset;
    return 0;
}

static int 
audio_devaudio_play_play_mmap(struct audio_devaudio_play *ap, 
			      char *buf, size_t len, void *pos)
{
    struct rcv_stats *rst = &ap->ap_rs->rs_stats;
    int len1 = len;
    void *pos2 = 0;
    int len2;

    dbg_print(DBG_PLAY, "audio_devaudio_play_mmap, fd: %d, len: %d\n", 
	      ap->ap_devaudio->dd_fd, len);
    if ((int)(pos + len) > ap->ap_mmap_buffer_size){
	len1 = ((int)ap->ap_mmap_buffer_size) - (int)pos;
	pos2 = ap->ap_mmap_buffer;
	len2 = len - len1;
    }
    memcpy(pos, buf, len1);
    if (pos2)
	memcpy(pos2, buf+len1, len2);

    dbg_print(DBG_PLAY, "audio_devaudio_play_play_mmap done\n");
    rst->rst_pbytes += len;
    rst->rst_ppkt++;
    return 0;
}


static int 
audio_devaudio_play_play_fifo(struct audio_devaudio_play *ap, 
			      char *pkt, size_t len)
{
    struct rcv_stats *rst = &ap->ap_rs->rs_stats;
    
    dbg_print(DBG_PLAY, "audio_devaudio_play_play_fifo, fd: %d, len: %d\n", 
	      ap->ap_devaudio->dd_fd, len);
    if (write(ap->ap_devaudio->dd_fd, pkt, len) < 0){
	sphone_error("audio_devaudio_play: write: %s\n", strerror(errno));
	return -1;
    }
    rst->rst_pbytes += len;
    rst->rst_ppkt++;
    dbg_print(DBG_PLAY, "audio_devaudio_play_play_fifo done\n");
    return 0;
}


int 
audio_devaudio_play_play(play_api *pa, char *pkt, size_t len, void *pos)
{
    struct audio_devaudio_play *ap = 
	(struct audio_devaudio_play *)pa->pa_private; 

    assert(ap->ap_devaudio);
    if (ap->ap_playout_type == PLAYOUT_FIFO)
	return audio_devaudio_play_play_fifo(ap, pkt, len);
    else
	return audio_devaudio_play_play_mmap(ap, pkt, len, pos);
}

/*
 * Start devaudio for playing according to coding_params
 */
int 
audio_devaudio_play_start(play_api *pa)
{
    struct audio_devaudio_play *ap = 
	(struct audio_devaudio_play *)pa->pa_private; 
    coding_params *cp = ap->ap_rs->rs_cp_audio;
    audio_info_t ai;

    AUDIO_INITINFO(&ai);
    ai.play.encoding = coding2devaudio(cp->cp_type);
    ai.play.precision = cp->cp_precision;
    ai.play.sample_rate = cp->cp_samp_sec;
    ai.play.channels = cp->cp_channels;
    ap->ap_mode |= AUMODE_PLAY | AUMODE_PLAY_ALL;
    ai.mode = ap->ap_mode;

    if (ioctl(ap->ap_devaudio->dd_fd, AUDIO_SETINFO, &ai) < 0){
	sphone_error("audio_devaudio_play_start ioctl AUDIO_SETINFO %s", 
		     strerror(errno));
	return -1;
    }
    if (ioctl(ap->ap_devaudio->dd_fd, AUDIO_GETINFO, &ai) < 0){
	sphone_error("audio_devaudio_play_start ioctl AUDIO_GETINFO %s", 
		     strerror(errno));
	return -1;
    }
    /* channels can change! (hope nothing else changes!) */
    if (cp->cp_channels != ai.play.channels){
	sphone_warning("audio_devaudio_play_start: Warning: channels changed from %d to %d\n", 
		       cp->cp_channels, ai.play.channels);    
	cp->cp_channels =  ai.play.channels;
    }
    return 0;
}

int 
audio_devaudio_play_stop(play_api *pa)
{
    return 0;
}

int 
audio_devaudio_play_open(play_api *pa, void *arg)
{
    struct audio_devaudio_play *ap = 
	(struct audio_devaudio_play *)pa->pa_private; 
    char *devname = (char*)arg; 

    if ((ap->ap_devaudio = audio_devaudio_init(devname)) == NULL)
	return -1;
    dbg_devaudio(DBG_PLAY, ap->ap_devaudio->dd_fd);
    if (ap->ap_devaudio->dd_shmem)
	ap->ap_playout_type = PLAYOUT_SHMEM;
    else
	ap->ap_playout_type = PLAYOUT_FIFO;
    if (ap->ap_playout_type == PLAYOUT_SHMEM){
	audio_info_t ai;

	if (ioctl(ap->ap_devaudio->dd_fd, AUDIO_GETINFO, &ai) < 0){
	    sphone_error("audio_devaudio_play_start ioctl AUDIO_GETINFO %s", 
			 strerror(errno));
	    return -1;
	}
	ap->ap_mmap_buffer_size = ai.play.buffer_size;
	if ((ap->ap_mmap_buffer = mmap(0 /* addr */, 
				       ap->ap_mmap_buffer_size,
				      PROT_READ|PROT_WRITE, 
				      MAP_FILE, 
				      ap->ap_devaudio->dd_fd, 
				      0)) == MAP_FAILED){
	    sphone_error("audio_devaudio_play_init: mmap: %s", 
			 strerror(errno));
	    return -1;
	}
    }

    return 0;
} 


int
audio_devaudio_play_close(play_api *pa)
{
    struct audio_devaudio_play *ap = 
	(struct audio_devaudio_play *)pa->pa_private;

    if (ap && ap->ap_devaudio)
	/* May free and remove dd: if refcnt=0 */
	audio_devaudio_exit(ap->ap_devaudio); 
    return 0;
}


int
audio_devaudio_play_exit(play_api *pa)
{
    audio_devaudio_play_close(pa);
    if (pa->pa_private){
	sfree(pa->pa_private);
	pa->pa_private = NULL;
    }
    sfree(pa);
    return 0;
}


int
audio_devaudio_play_init(play_api *pa, rcv_session_t *rs)
{
    struct audio_devaudio_play *ap;

    if ((ap = (void*)smalloc(sizeof(struct audio_devaudio_play))) == NULL){
	sphone_error("audio_devaudio_play_init: malloc: %s", strerror(errno));
	return -1;
    }
    memset(ap, 0, sizeof(struct audio_devaudio_play));
    pa->pa_private   = ap;
    ap->ap_rs        = rs;
    ap->ap_mode      = 0;

    pa->pa_start     = audio_devaudio_play_start;
    pa->pa_open      = audio_devaudio_play_open;
    pa->pa_play      = audio_devaudio_play_play;
    pa->pa_play_pos  = audio_devaudio_play_pos;
    pa->pa_play_stop = audio_devaudio_play_stop;
    pa->pa_settings  = audio_devaudio_play_settings;
    pa->pa_ioctl     = audio_devaudio_play_ioctl;
    pa->pa_exit      = audio_devaudio_play_exit;

    dbg_print(DBG_PLAY, "audio_devaudio_play_init\n");
	
    return 0;
} 

#endif /* HAVE_SYS_AUDIOIO_H */
