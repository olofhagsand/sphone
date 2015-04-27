/*-----------------------------------------------------------------------------
  File:   sphone_audio_devaudio_record.c
  Description: audio module for unix /dev/audio - record part
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
#include <sys/ioctl.h>

#include "sphone_audio_devaudio.h"
#include "sphone_audio_devaudio_record.h"


#include <sys/audioio.h>

struct audio_devaudio_record{
    /* Private fields */
    send_session_t *ar_ss;        /* Backpointer to send_session struct */
    int          (*ar_callback)();     /* Function called for each sampled packet */
    void          *ar_cb_arg;          /* Callback function argument */

    /* Specialized for devaudio  */
    struct devaudio_device *ar_devaudio; /* Pointer to devaudio truct */
    int                     ar_mode;     /* Save mode: play/rec, etc */
    struct timeval ar_T;      /* Next emulated timestamp */
};

/* Called when settings change */
int 
audio_devaudio_record_settings(record_api *ra, coding_params *cp)
{
    return 0;
}


int 
audio_devaudio_record_poll(record_api *ra)
{
    struct audio_devaudio_record *ar = 
      (struct audio_devaudio_record *)ra->ra_private; 
    struct send_stats *sst = &ar->ar_ss->ss_stats;
    coding_params *cp = ar->ar_ss->ss_cp_audio;
    char *buf;
    size_t len;
    int retval;
    int i;

    len = cp->cp_size_bytes;
    dbg_print(DBG_RECORD, "audio_devaudio_record_poll len:%d\n", len);
    /* Loop over all packets ready for transmission. */
    for(i=0; i<2; i++){ /* To avoid livelock */
	if ((buf = smalloc(len)) == NULL){
	    sphone_error("audio_devaudio_record_poll: malloc: %s\n", 
			 strerror(errno));
	    return -1;
	}
	if ((retval = read(ar->ar_devaudio->dd_fd, buf, len)) < 0){
	    if (errno == EWOULDBLOCK){
		dbg_print(DBG_RECORD, "audio_devaudio_record_poll: not ready\n");
/*		sfree(buf);*/
		break;
	    }else{
	    sphone_error("audio_devaudio_record_poll: read: %s\n", 
			 strerror(errno));
	    sfree(buf);
	    return -1;
	    }
	}
	if (retval != len){
	    sphone_error("audio_devaudio_record_poll: read returned %d (expected %d)", 
			 retval, len);
	    sfree(buf);
	    return -1;
	}
	sst->sst_rpkt++;
	sst->sst_rbytes += len;

	dbg_print(DBG_RECORD, "audio_devaudio_record_poll: sent\n");
	/* e.g. record_and_send */
	if (ar->ar_callback(buf, len, ar->ar_cb_arg) < 0){
	    sfree(buf);
	    return -1; 
	}
	sfree(buf);
    }
    return 0;
}

int 
audio_devaudio_record_poll_wrapper(int s, void *arg)
{
    record_api *ra = (record_api*)arg;
    struct audio_devaudio_record *ar = 
	(struct audio_devaudio_record *)ra->ra_private; 
    coding_params *cp = ar->ar_ss->ss_cp_audio;

    if (audio_devaudio_record_poll(ra) < 0)
	return -1;

    ar->ar_T = gettimestamp();
    ar->ar_T.tv_usec += cp->cp_size_ms*1000; /* next expected packet */
    timevalfix(&ar->ar_T);

    if (eventloop_reg_timeout(ar->ar_T, /* Next expected packet */
			      audio_devaudio_record_poll_wrapper, 
			      (void*)ra,
			      "/dev/audio record") < 0)
	return -1;
    return 0;
}

int 
audio_devaudio_record_ioctl(record_api *ra, int op, void *oparg)
{
    struct audio_devaudio_record *ar = 
	(struct audio_devaudio_record *)ra->ra_private; 

    switch (op){
    case SPHONE_AUDIO_IOCTL_FD:
	*(int*)oparg = ar->ar_devaudio->dd_fd;
	break;
    default:
	sphone_warning("audio_devaudio_record_ioctl: No such op: %d", op);
	break;
    }
    return 0;
}

/*
 * Start devaudio for recording according to coding_params
 */
int 
audio_devaudio_record_start(record_api *ra)
{
    struct audio_devaudio_record *ar = 
	(struct audio_devaudio_record *)ra->ra_private; 
    coding_params *cp = ar->ar_ss->ss_cp_audio;
    audio_info_t ai;

    AUDIO_INITINFO(&ai);
    ai.record.encoding = coding2devaudio(cp->cp_type);
    ai.record.precision = cp->cp_precision;
    ai.record.sample_rate = cp->cp_samp_sec;
    ai.record.channels = cp->cp_channels;
    ar->ar_mode |= AUMODE_RECORD;
    ai.mode = ar->ar_mode;

    if (ioctl(ar->ar_devaudio->dd_fd, AUDIO_SETINFO, &ai) < 0){
	sphone_error("audio_devaudio_record_start ioctl AUDIO_SETINFO %s", 
		     strerror(errno));
	return -1;
    }
    if (ioctl(ar->ar_devaudio->dd_fd, AUDIO_GETINFO, &ai) < 0){
	sphone_error("audio_devaudio_record_start ioctl AUDIO_SETINFO %s", 
		     strerror(errno));
	return -1;
    }
    /* channels can change! (hope nothing else changes!) */
    if (cp->cp_channels != ai.record.channels){
	sphone_warning("Warning: channels changed from %d to %d\n", 
		       cp->cp_channels, ai.record.channels);    
	cp->cp_channels =  ai.record.channels;
    }
    if (audio_devaudio_record_poll_wrapper(ar->ar_devaudio->dd_fd, ra) < 0)
	return -1; 
    return 0;
}


int 
audio_devaudio_record_open(record_api *ra, void *arg)
{
    struct audio_devaudio_record *ar = 
	(struct audio_devaudio_record *)ra->ra_private; 
	char *devname = (char*)arg; 

    if ((ar->ar_devaudio = audio_devaudio_init(devname)) == NULL)
	return -1;
    dbg_devaudio(DBG_RECORD, ar->ar_devaudio->dd_fd);
    return 0;
}

int    
audio_devaudio_record_close(record_api *ra)
{
    struct audio_devaudio_record *ar = 
	(struct audio_devaudio_record *)ra->ra_private;
    if (ar && ar->ar_devaudio)
	/* May free and remove dd: if refcnt=0 */
	audio_devaudio_exit(ar->ar_devaudio); 
    return 0;
}

int
audio_devaudio_record_exit(record_api *ra)
{
    audio_devaudio_record_close(ra);
    if (ra->ra_private){
	sfree(ra->ra_private);
	ra->ra_private = NULL;
    }
    sfree(ra);
    return 0;
}

int
audio_devaudio_record_init(record_api *ra, send_session_t *ss)
{
    struct audio_devaudio_record *ar;

    ra->ra_settings  = audio_devaudio_record_settings;
    ra->ra_open      = audio_devaudio_record_open;
    ra->ra_start     = audio_devaudio_record_start;
    ra->ra_poll      = audio_devaudio_record_poll;
    ra->ra_ioctl     = audio_devaudio_record_ioctl;
    ra->ra_exit      = audio_devaudio_record_exit;

    if ((ar = (void*)smalloc(sizeof(struct audio_devaudio_record))) == NULL){
	sphone_error("audio_devaudio_record_init: malloc: %s", strerror(errno));
	return -1;
    }
    memset(ar, 0, sizeof(struct audio_devaudio_record));
    ra->ra_private   = ar;
    ar->ar_ss        = ss;
    ar->ar_mode      = 0;
    dbg_print(DBG_RECORD, "audio_devaudio_record_init\n");
    return 0;
} 

#endif /* HAVE_SYS_AUDIOIO_H */
