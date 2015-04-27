/*-----------------------------------------------------------------------------
  File:   sphone_audio_directsound_record.c
  Description: audio module for WIN32 directsound - recording part
  Author: Olof Hagsand
  CVS Version: $Id: sphone_audio_directsound_record.c,v 1.16 2004/02/10 20:00:38 olofh Exp $
 
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
#include "sphone_audio_directsound_record.h"

struct audio_directsound_record{
    /* Private fields */
    send_session_t *ar_ss;        /* Backpointer to send_session struct */
    int          (*ar_callback)();     /* Function called for each sampled packet */
    void          *ar_cb_arg;          /* Callback function argument */

    /* Specialized for directsound  */
    struct IDirectSoundCaptureBuffer *ar_ds_capbuf;
    unsigned int   ar_ds_capbuflen;
    struct timeval ar_T;      /* Next timestamp */
    uint32_t       ar_capture_offset;
    uint32_t       ar_cap_nr; /* Nr of captured packets */
    int            ar_r; /* Current record pointer */
};

/*
 * CreateCaptureBuffer
 * We use DSCBUFFERDESC1 instead of DSCBUFFERDESC since the latter contains
 * some strange effect structs that result in Invalid parameter (maybe 
 * something to do with the alpha version of .NET that I use :-)
 */
static int 
CreateCaptureBuffer(struct IDirectSoundCapture *ds_cap, 
		    struct IDirectSoundCaptureBuffer **ds_capbuf,
		    int *ds_capbuflen, 
		    HWND hwnd, 
		    int samp_sec,
		    int bits_samp) 
{ 
    DSCBUFFERDESC1 dsbdesc;
    HRESULT hr; 
    WAVEFORMATEX wf;
    DSCCAPS dsccaps; 

    /* Set up wave format structure. */
    memset(&wf, 0, sizeof(WAVEFORMATEX)); 

    wf.wFormatTag = WAVE_FORMAT_PCM; 
    wf.nChannels = 1; 
    wf.nSamplesPerSec = samp_sec; 
    wf.wBitsPerSample = bits_samp; 
    wf.nBlockAlign = wf.wBitsPerSample*wf.nChannels/8; 
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign; 

    /* Succeeded. Try to create buffer.   Set up DSBUFFERDESC structure. */
    memset(&dsbdesc, 0, sizeof(DSCBUFFERDESC1)); 
    dsbdesc.dwSize = sizeof(DSCBUFFERDESC1); 
    dsbdesc.dwFlags = 0; 
    /* Buffer size is determined by sound hardware. */
    dsbdesc.dwBufferBytes = *ds_capbuflen; 
    dsbdesc.lpwfxFormat = &wf; 

    /* Create primary sound buffer. A handle (ds_capbuf) to it is returned.*/ 
    if ((hr = IDirectSoundCapture_CreateCaptureBuffer(
	ds_cap, (DSCBUFFERDESC*)&dsbdesc, ds_capbuf, NULL)) < 0){
	directsound_error(hr, "CreateCaptureBuffer: CreateCaptureBuffer");
	goto done;
    }

    dsccaps.dwSize = sizeof(DSCCAPS); 

    /* Call GetCaps to retrieve the size of the buffer; check LOCHARDWARE flag */
    if ((hr = IDirectSoundCaptureBuffer_GetCaps(*ds_capbuf, (LPDSCBCAPS)&dsccaps)) < 0){
	directsound_error(hr, "CreateCaptureBuffer: GetCaps");
	goto done;
    } /* if(DSCCAPS_EMULDRIVER == dsccaps.dwFlags) */
     return 0;
  done:
    *ds_capbuf = NULL; 
    *ds_capbuflen = 0; 
    return -1; 
} 

/* 
 * Called when settings change 
 */
int 
audio_directsound_record_settings(record_api *ra, coding_params *cp)
{
    struct audio_directsound_record *ar = 
	(struct audio_directsound_record *)ra->ra_private;
    /* XXX    coding_params *cp = ar->ar_ss->ss_cp_audio; */

    ar->ar_capture_offset = ms2bytes(cp->cp_samp_sec, 
				     cp->cp_precision, 
				     cp->cp_channels,
				     CAPTURE_OFFSET_MS);
    return 0;
}

/*
 * audio_directsound_record_poll
 *
 */
int 
audio_directsound_record_poll(record_api *ra)
{
    struct audio_directsound_record *ar = 
	(struct audio_directsound_record *)ra->ra_private; 
    send_session_t *ss = ar->ar_ss;
    coding_params *cp = ss->ss_cp_audio;
    struct send_stats *sst = &ss->ss_stats;
    char *buf;
    size_t len;
    HRESULT hr;
    int pos = 0;
    int safe_pos = 0;
    void *ptr1;
    int len1;
    void *ptr2;
    int len2;
    int nr_send = 0;
    
    len = cp->cp_size_bytes;
    /*
     * Get current read pointers.
     * w is 10 ms before CurrentPlayCursor
     */
    if ((hr = IDirectSoundCaptureBuffer_GetCurrentPosition(
	ar->ar_ds_capbuf, 
	(LPDWORD)&pos, 
	(LPDWORD)&safe_pos)) < 0){
	directsound_error(hr, "audio_directsound_record_poll: GetCurrentPosition");
	return -1;
    }
    pos -= ar->ar_capture_offset;
    if(pos < 0)
	pos += ar->ar_ds_capbuflen;
    /* For 16-bit samples: no odd byte values */
    if((cp->cp_precision > 8) && (pos % 2)) /* odd ? */
	pos -=1;

    if(ar->ar_cap_nr == 0){ /* first time: initialize */
	ar->ar_r = pos - len;
	if(ar->ar_r < 0) /* wrapped around ?*/
	    ar->ar_r += ar->ar_ds_capbuflen;
	ar->ar_cap_nr++;
	return 0;
    }
    /* not first */
    /* Check if we can read a packet, or we read max */
    if(((ar->ar_r + len)%ar->ar_ds_capbuflen) < pos) /*a packet to capture?*/
	if((pos - ar->ar_r) >= 0)/* in phase */
	    nr_send = (int)((pos - ar->ar_r) / len);
	else /* wrapped around */
	    nr_send = (int)((pos - ar->ar_r + ar->ar_ds_capbuflen) / len);
    
    dbg_print(DBG_RECORD, "ds_poll: pos: %d safe_pos: %d pointer:%d, nr:%d\n",
	      pos, safe_pos, ar->ar_r, nr_send);
    /*
     * Now we read the audio packet.
     * Get pointers to memory: where to read audio data.
     * You get two pointers in case the circular buffer is at
     * the end-point.
     */
    while (nr_send > 0){
	if ((hr = IDirectSoundCaptureBuffer_Lock(
	    ar->ar_ds_capbuf, 
	    ar->ar_r, 
	    len, 
	    &ptr1, 
	    (LPDWORD)&len1,
	    &ptr2, 
	    (LPDWORD)&len2, 
	    0x0)) < 0){
	    directsound_error(hr, "audio_directsound_record_poll: Lock: %d");
	    return -1;
	}
	if ((buf = smalloc(len+10)) == NULL){ /* +10 for safety */
	    sphone_error("audio_directsound_record_poll malloc %s", 
			 strerror(errno));
	    return -1;
	}
	/* read the samples from the primary buffer */
	memcpy(buf, ptr1, len1);
	if (ptr2)
	    memcpy(buf+len1, ptr2, len2);
	/* Release the pointers into the primary buffers */
	if ((hr = IDirectSoundCaptureBuffer_Unlock(
	    ar->ar_ds_capbuf, 
	    ptr1, 
	    len1,
	    ptr2,
	    len2)) < 0){
	    directsound_error(hr, "audio_directsound_record_poll: Unlock");
	    sfree(buf);
	    return -1;
	}
	ar->ar_r  = (ar->ar_r + len) % ar->ar_ds_capbuflen;
	nr_send -= 1;
	sst->sst_rpkt++;
	sst->sst_rbytes += len;
	/* e.g. record_and_send */
	if (ar->ar_callback(buf, len, ar->ar_cb_arg) < 0){
	    sfree(buf);
	    return -1; 
	}
	sfree(buf);
	ar->ar_cap_nr++;
    }/*while*/
    return 0;
}

int 
audio_directsound_record_poll_wrapper(int fd, void *arg)
{
    record_api *ra = (record_api*)arg;
    struct audio_directsound_record *ar = 
      (struct audio_directsound_record *)ra->ra_private; 
    coding_params *cp = ar->ar_ss->ss_cp_audio;

    dbg_print(DBG_RECORD, "audio_directsound_record_poll_wrapper\n");
    if (audio_directsound_record_poll(ra) < 0)
	return -1;

    ar->ar_T = gettimestamp();
    ar->ar_T.tv_usec += cp->cp_size_ms*1000; /* next expected packet */
    timevalfix(&ar->ar_T);
    if (eventloop_reg_timeout(ar->ar_T, /* Next expected packet */
			      audio_directsound_record_poll_wrapper, 
			      (void*)ra,
			      "audio_directsound_record") < 0)
	return -1;
    dbg_print(DBG_RECORD, "audio_directsound_record_poll_wrapper OK\n");
    return 0;
}

/* 
 * audio_directsound_record_poll_wrapper
 * Wrapper around the poll function - just for the eventlopp timeout
 * to get right signature
 */
int 
audio_directsound_record_start(record_api *ra)
{
    struct audio_directsound_record *ar = 
	(struct audio_directsound_record *)ra->ra_private; 
    coding_params *cp = ar->ar_ss->ss_cp_audio;
    struct IDirectSoundCapture *ds_cap;/* Pointer to directsound object */
    DSCCAPS caps;
    int hr;

    /* Open DirectSound device */
    if ((hr = DirectSoundCaptureCreate(NULL, &ds_cap, NULL)) < 0){
	directsound_error(hr, "audio_directsound_record_start:"
			  " DirectSoundCaptureCreate\n");
	return -1;
    }
    /* Check capabilities. Code not used but may be necessary to make some
       sanity checks */
    memset(&caps, 0, sizeof(DSCCAPS));
    caps.dwSize=sizeof(DSCCAPS);
    if ((hr = IDirectSoundCapture_GetCaps(ds_cap, &caps)) < 0){
	directsound_error(hr, "audio_directsound_record_start:"
			  " GetCaps\n");
	return -1;
    }
    /* Create primary sound buffer.*/ 
    ar->ar_ds_capbuf = NULL; 
    ar->ar_ds_capbuflen = 32000; /* XXX */
    if (CreateCaptureBuffer(ds_cap,
			    &ar->ar_ds_capbuf, 
			    &ar->ar_ds_capbuflen, 
			    directsound->ds_hwnd, 
			    cp->cp_samp_sec, 
			    cp->cp_precision) < 0)
	return -1;
    if ((hr = IDirectSoundCaptureBuffer_Start(ar->ar_ds_capbuf, DSCBSTART_LOOPING)) != DS_OK){
	directsound_error(hr, "audio_directsound_record_start: Start");
	return -1;
    }

    if (audio_directsound_record_poll_wrapper(0, ra) < 0)
	return -1;
    return 0;
}

int 
audio_directsound_record_ioctl(record_api *ra, int op, void *oparg)
{
/*    struct audio_directsound_record *ar = 
      (struct audio_directsound_record *)ra->ra_private; */

    switch (op){
    default:
	sphone_warning("audio_directsound_record_ioctl: No such op: %d", op);
	break;
    }
    return 0;
}

int 
audio_directsound_record_open(record_api *ra, void *arg)
{
  /*    struct audio_directsound_record *ar = 
	(struct audio_directsound_record *)ra->ra_private; */

    return 0;
}


int
audio_directsound_record_exit(record_api *ra)
{
    struct audio_directsound_record *ar = 
	(struct audio_directsound_record *)ra->ra_private;

    if (ar)
	sfree(ar);
    sfree(ra);
    return 0;
}

int
audio_directsound_record_init(record_api *ra, send_session_t *ss)
{
    struct audio_directsound_record *ar;

    ra->ra_settings  = audio_directsound_record_settings;
    ra->ra_open      = audio_directsound_record_open;
    ra->ra_start     = audio_directsound_record_start;
    ra->ra_poll      = audio_directsound_record_poll;
    ra->ra_ioctl     = audio_directsound_record_ioctl;
    ra->ra_exit      = audio_directsound_record_exit;

    if ((ar = (void*)smalloc(sizeof(struct audio_directsound_record))) == NULL){
	sphone_error("audio_directsound_record_init: malloc: %s", strerror(errno));
	return -1;
    }
    memset(ar, 0, sizeof(struct audio_directsound_record));
    ra->ra_private   = ar;
    ar->ar_ss        = ss;

    if (audio_directsound_init() < 0)
	return -1;
    dbg_print(DBG_RECORD, "audio_directsound_record_init\n");
    return 0;
} 

#endif /* HAVE_DIRECTSOUND */
