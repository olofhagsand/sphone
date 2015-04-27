/*-----------------------------------------------------------------------------
  File:   sphone_audio_file_record.c
  Description: audio file emulation module - recording part
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
#include <fcntl.h>

#include "sphone.h"
#include "sphone_audio_file_record.h"

#ifndef SEEK_SET /* Patch for solaris 1, dont know what is going wrong */
#define SEEK_SET 0
#endif

/* 
 * This is the specialization for emulated audio module 
 * First part is inlined from sphone_audio_record_api
 */
struct audio_file_record{
    /* Private common fields */
    send_session_t *ar_ss;        /* Backpointer to send_session struct */
    int          (*ar_callback)();/* Function called for each sampled packet */
    void          *ar_cb_arg;     /* Callback function argument */

   /* Below is file_record specific fields */
    int            ar_fd;     /* File descriptor of recorded audio */
    int            ar_loop;   /* Set if audio should loop endlessly */
    char          *ar_buf;    /* Intermediate buffer for reading from files */
    int            dummy[2];  /* XXX: removes a memory error I have not catched */
    struct timeval ar_T;      /* Next emulated timestamp */
};


/* 
 * Add audio buffer to read from 
 * Return values: -1 if error.
 *                0 if OK - or eof (then eof is set)
 */
static int 
add_buffer(int fd, char *buf, int len, int loop, int *eof)
{
    int retval;
    int looped = 0;

    *eof = 0;
  again:
    retval = read (fd, buf, len);
    if (retval < 0){
	sphone_error("audio_file_record, add_buffer read %s", strerror(errno));
	return -1;
    }
    /* EOF */
    if (retval == 0){
	if (looped){
	    sphone_error("audio_file_record, add_buffer, double eof");
	    return -1;
	}
	if (loop){
	    looped++;
	    if (lseek(fd, 0, SEEK_SET) < 0){
		sphone_error("audio_file_record, add_buffer lseek %s", 
			 strerror(errno));
		return -1;
	    }
	    goto again;
	}
	*eof = 1;
	return 0;
    }
    if (retval < len){
	buf += retval;
	len -= retval;
	goto again;
    }
    return 0; /* OK */
}

int 
audio_file_record_ioctl(record_api *ra, int op, void *oparg)
{
    struct audio_file_record *ar = 
	(struct audio_file_record *)ra->ra_private;

    switch (op){
    case SPHONE_AUDIO_IOCTL_LOOP:
	ar->ar_loop = (int)oparg;
	break;
    default:
	sphone_error("audio_file_record_ioctl: No such op: %d", op);
	return -1;
	break;
    }
    
    return 0;
}

int 
audio_file_record_open(record_api *ra, void *arg)
{
    struct audio_file_record *ar = 
	(struct audio_file_record *)ra->ra_private;
    char *filename = (char*)arg;

    if (ar->ar_fd)
	close(ar->ar_fd);
#ifdef WIN32
    ar->ar_fd = open(filename, O_RDONLY|O_BINARY);
#else
    ar->ar_fd = open(filename, O_RDONLY);
#endif
    if (ar->ar_fd < 0){
	sphone_error("audio_file_record_open file open: \"%s\": %s", 
		     filename, strerror(errno));
	return -1;
    }
    return 0;
}

/* Record */
int 
audio_file_record_poll(record_api *ra)
{
    struct audio_file_record *ar = 
	(struct audio_file_record *)ra->ra_private;
    struct send_stats *sst = &ar->ar_ss->ss_stats;
    coding_params *cp = ar->ar_ss->ss_cp_audio;
    struct timeval T, dT;
    double ms;
    int retval, eof;
    size_t len;

    T = gettimestamp();
    if (timercmp(&T, &ar->ar_T, <))
	return 0;
    timersub(&T, &ar->ar_T, &dT);
    ms = timeval2sec(dT)*1000;
    /* Loop over all packets ready for transmission. */

    while (ms > 0){ 
	len = cp->cp_size_bytes;
	sst->sst_rpkt++;
	sst->sst_rbytes += len;
	memset(ar->ar_buf, 0, len);
	retval = add_buffer(ar->ar_fd, 
			    ar->ar_buf, 
			    len, 
			    ar->ar_loop, 
			    &eof);
	if (retval < 0)
	    return -1;
	if (eof){
	    sphone_warning("end-of-file");
	    return -1;
	}

	/* e.g. record_and_send */
	if (ar->ar_callback(ar->ar_buf, len, ar->ar_cb_arg) < 0)
	    return -1; 
	ar->ar_T.tv_usec += cp->cp_size_ms*1000; /* next expected packet */

	timevalfix(&ar->ar_T);
	ms -= cp->cp_size_ms;
    }
    dbg_print(DBG_RECORD, "audio_file_record_poll\n");
    return 0;
}

/* 
 * audio_file_record_poll_wrapper
 * Wrapper around the poll function - just for the eventlopp timeout
 * to get right signature
 */
int
audio_file_record_poll_wrapper(int fd, void *arg)
{
    record_api *ra = (record_api *)arg;
    struct audio_file_record *ar = 
	(struct audio_file_record *)ra->ra_private;
    int retval = 0;

    retval = audio_file_record_poll(ra);
    if (retval < 0)
	return -1;
    if (eventloop_reg_timeout(ar->ar_T, /* Next expected packet */
			      audio_file_record_poll_wrapper, 
			      (void*)ra,
			      "audio_file_record") < 0)
	return -1;
    return retval;
}

/* Called when settings change */
int 
audio_file_record_settings(record_api *ra, coding_params *cp)
{
    return 0;
}

int 
audio_file_record_start(record_api *ra)
{
    struct audio_file_record *ar = 
	(struct audio_file_record *)ra->ra_private;
    coding_params *cp = ar->ar_ss->ss_cp_audio;
    int retval = 0;

    assert(ar->ar_fd);
    /* Initialize circular input buffers used in subsequent audio_poll() calls.
       The buffers are reused after they have been read and sent, so there is
       no need to make new malloc's or frees. */
    if ((ar->ar_buf = (char*)smalloc(cp->cp_size_bytes)) == NULL){
	sphone_error("audio_file_record_start: malloc %s", strerror(errno));
	return -1;
    }
    memset(ar->ar_buf, 0, cp->cp_size_bytes); 
    ar->ar_T = gettimestamp(); 
    ar->ar_T.tv_usec += cp->cp_size_ms*1000; /* next expected packet */
    timevalfix(&ar->ar_T);
    retval = eventloop_reg_timeout(ar->ar_T, /* Next expected packet */
				   audio_file_record_poll_wrapper, 
				   (void*)ra,
				   "audio_file record");
    dbg_print(DBG_RECORD, "audio_file_record_start\n");
    return retval;
}

int
audio_file_record_exit(record_api *ra)
{
    struct audio_file_record *ar = 
	(struct audio_file_record *)ra->ra_private;

    if (ar)
	sfree(ar);
    sfree(ra);
    return 0;
}


int
audio_file_record_init(record_api *ra, send_session_t *ss)
{
    struct audio_file_record *ar;

    ra->ra_settings = audio_file_record_settings;
    ra->ra_start    = audio_file_record_start;
    ra->ra_open     = audio_file_record_open;
    ra->ra_poll     = audio_file_record_poll;
    ra->ra_ioctl    = audio_file_record_ioctl;
    ra->ra_exit     = audio_file_record_exit;
    if ((ar = (void*)smalloc(sizeof(struct audio_file_record))) == NULL){
	sphone_error("audio_file_record_init: malloc: %s", strerror(errno));
	return -1;
    }
    memset(ar, 0, sizeof(struct audio_file_record));
    ra->ra_private = ar;
    ar->ar_ss = ss;
    dbg_print(DBG_RECORD, "audio_file_record_init\n");
    return 0;
}

