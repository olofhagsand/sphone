/*-----------------------------------------------------------------------------
  File:   sphone_audio_directsound_play.c
  Description: audio module for WIN32 Directsound - playout part
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

#ifdef HAVE_DIRECTSOUND
#define MAX_NO_RESTORES 100

#include <dsound.h>
#include "sphone_audio_directsound.h"
#include "sphone_audio_directsound_play.h"

struct audio_directsound_play{
    /* Private fields */
    rcv_session_t              *ap_rs; /* Backpointer to receive session */

    /* Specialized for directsound  */
    LPDIRECTSOUND       ap_ds; 
    LPDIRECTSOUNDBUFFER ap_dsbuf;
    unsigned int                ap_playbuflen; 
};

#ifdef newtry
/*--------------------------------------------------------------------------
  CreatePlayBuffer
  Create DirectSound writable primary audio buffer. This code was
  taken from MS VC++ documentation but modified quite thoroughly.
  In short:
     1. Set cooperative level of directsound device to WRITEPRIMARY.
        This fails in NT4.
     2. Create primary sound buffer. A handle to it is return. There is
        no way one can control its creation, you just accept it as it is.
     3. SetFormat of the buffer to appropriate coding, ie pcm.
     4. Call GetCaps to retrieve the size of the buffer: and check the
        DSBCAPS_LOCHARDWARE flag.
 *-------------------------------------------------------------------------*/
static int 
CreatePlayBuffer(struct IDirectSound *dirsound, 
		 struct IDirectSoundBuffer **dirsoundbufp,
		 int *dirsoundbuflen, 
		 HWND hwnd, 
		 int samp_sec,
		 int bits_samp) 
{ 
    DSBUFFERDESC dsbdesc; 
    DSBCAPS dsbcaps; 
    DSCAPS dscaps; 
    HRESULT hr; 
    PCMWAVEFORMAT pcmwf;
    WAVEFORMATEX wf;
    int primary = 0;

    /* Set up wave format structure. */
    memset(&wf, 0, sizeof(wf)); 
    wf.wFormatTag       = WAVE_FORMAT_PCM; 
    wf.nChannels        = 1; 
    wf.nSamplesPerSec   = samp_sec; 
    wf.wBitsPerSample   = bits_samp; 
    wf.nBlockAlign      = wf.wBitsPerSample*wf.nChannels/8; 
    wf.nAvgBytesPerSec  = wf.nSamplesPerSec * wf.nBlockAlign; 
    wf.cbSize           = 0;

    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
    dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
    dsbdesc.dwFlags =  DSBCAPS_CTRLFX | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;

    /* Buffer size is determined by sound hardware. */
    dsbdesc.dwBufferBytes = 0; /* XXX */
    dsbdesc.lpwfxFormat = NULL; /* &wf; */

    /* Create soundbuffer. A handle (dirsoundbufp) to it is returned.
       Use Query interface to transform to IDirectSoundBuffer8 */ 
    if ((hr = IDirectSound_Initialize(dirsound, NULL)) < 0){
      directsound_error(hr, "CreatePlayBuffer: Initialize"); 
      goto done; 
    }
    if ((hr = IDirectSound_CreateSoundBuffer(dirsound, 
					     &dsbdesc, 
					     dirsoundbufp, 
					     NULL)) != 0){
      fprintf(stderr, "%x %x\n", hr, DS_OK);
      /*      directsound_error(hr, "CreatePlayBuffer: CreateSoundBuffer"); */
      /*      goto done; */
    }
    /* XXX: see http://www1.obi.ne.jp/~fenrir24/vc/class/CDirectSoundBuffer.cpp.txt */
    memset(&dsbcaps, 0, sizeof(DSBCAPS));
    dsbcaps.dwSize = sizeof(DSBCAPS); 
    if ((hr = IDirectSoundBuffer_GetCaps(*dirsoundbufp, &dsbcaps)) < 0){
      directsound_error(hr, "CreatePlayBuffer: IDirectSoundBuffer_GetCaps");
      goto done;
    }
    primary = ((dsbcaps.dwFlags & DSBCAPS_LOCHARDWARE) != 0);
    if (primary)
      fprintf(stderr, "CreatePlayBuffer: using primary buffer\n");
    else
      fprintf(stderr, "CreatePlayBuffer: using secondary buffer\n");

    /* For primary buffer, the cooperative level shall be WRITEPRIMARY */
    if ((hr = IDirectSound_SetCooperativeLevel(dirsound, 
			       hwnd, 
			       primary ? DSSCL_WRITEPRIMARY: DSSCL_EXCLUSIVE
			       )) < 0){
	directsound_error(hr, "CreatePlayBuffer: SetCooperativeLevel");
	goto done;
    }

    memset(&dsbcaps, 0, sizeof(DSBCAPS));
    dsbcaps.dwSize = sizeof(DSBCAPS); 

    /* Call GetCaps to retrieve the size of the buffer; check LOCHARDWARE flag */
    if ((hr = IDirectSound_GetCaps(*dirsoundbufp, &dsbcaps)) < 0){
	directsound_error(hr, "CreatePlayBuffer: GetCaps");
	goto done;
    }
    *dirsoundbuflen = dsbcaps.dwBufferBytes; 
    return 0;  /* OK */
  done:
    dirsoundbufp = NULL; 
    *dirsoundbuflen = 0; 
    return -1; 
} 

/*--------------------------------------------------------------------------
 CheckAndRestoreSoundBuffer
 This function restores the soundbuffer when lost
 Happens when running win98
 *-------------------------------------------------------------------------*/
static HRESULT 
CheckAndRestoreSoundBuffer(struct IDirectSoundBuffer *dirsoundbuf, HRESULT hr)
{
    HRESULT hr2;
    int nr = 0;

    if(hr != DSERR_BUFFERLOST) 
	return hr;
    while((hr2 = IDirectSoundBuffer_Restore(dirsoundbuf))==DSERR_BUFFERLOST){
	if(++nr > MAX_NO_RESTORES){
	    sphone_warning("CheckAndRestoreSoundBuffer:"
			 "RestoreSoundBuffer Timeout!\n");
	    break;
	}
	Sleep(10); /* XXX */
    }
    return hr2; /* XXX: is this negative on error or what? */
}


int 
audio_directsound_play_start(play_api *pa)
{
    struct audio_directsound_play *ap = 
	(struct audio_directsound_play *)pa->pa_private; 
    coding_params *cp = ap->ap_rs->rs_cp_audio;
    int hr;

    /* Open DirectSound device */
    if ((hr = DirectSoundCreate(NULL, &ap->ap_ds, NULL)) < 0){
	directsound_error(hr, "audio_directsound_play_start: DirectSoundCreate");
	return -1;
    }
    /* Create Primary Buffer for writing */
    if (CreatePlayBuffer(ap->ap_ds, 
			 &ap->ap_dsbuf, 
			 &ap->ap_playbuflen, 
			 directsound->ds_hwnd, 
			 cp->cp_samp_sec, 
			 cp->cp_precision) < 0)
	return -1;
    hr = IDirectSoundBuffer_Stop(ap->ap_dsbuf);
    /* Restore buffer */
    if ((hr = CheckAndRestoreSoundBuffer(ap->ap_dsbuf, hr)) < 0){
	directsound_error(hr, "audio_directsound_start: DS_Stop failed");
	return -1;
    }

    /* Start the device */  
    hr = IDirectSoundBuffer_Play(ap->ap_dsbuf, 0, 0, DSBPLAY_LOOPING);
    /* Restore buffer */
    if ((hr = CheckAndRestoreSoundBuffer(ap->ap_dsbuf, hr)) < 0){
	directsound_error(hr, "audio_directsound_start: CheckAndRestoreSoundBuffer");
	return -1;
    }
    return 0;
}
#else /* newtry */

int 
audio_directsound_play_start(play_api *pa)
{
    struct audio_directsound_play *ap = 
	(struct audio_directsound_play *)pa->pa_private; 
    coding_params *cp = ap->ap_rs->rs_cp_audio;
    int hr;
    WAVEFORMATEX wf;

    DSBUFFERDESC dsbd; 

    /* Open DirectSound device */
    if ((hr = DirectSoundCreate(NULL, &ap->ap_ds, NULL)) < 0){
	directsound_error(hr, "audio_directsound_play_start: DirectSoundCreate");
	return -1;
    }
    if ((hr = IDirectSound_SetCooperativeLevel(ap->ap_ds, directsound->ds_hwnd,
					     DSSCL_NORMAL)) < 0){
      	directsound_error(hr, "audio_directsound_play_start: SetCooperativeLevel");
	return -1;
    }

    memset(&wf, 0, sizeof(wf));
    wf.wFormatTag		= WAVE_FORMAT_PCM;
    wf.nChannels		= cp->cp_channels;		
    wf.nSamplesPerSec	        = cp->cp_samp_sec; 
    wf.wBitsPerSample   	= cp->cp_precision;
    wf.nBlockAlign		= wf.wBitsPerSample*wf.nChannels/8; 
    wf.nAvgBytesPerSec	        = wf.nSamplesPerSec * wf.nBlockAlign;
    wf.cbSize		        = 0;
    memset(&dsbd, 0, sizeof(DSBUFFERDESC)); 
    dsbd.dwSize = sizeof(DSBUFFERDESC); 
    dsbd.dwFlags		= DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
    dsbd.lpwfxFormat		= &wf;
    dsbd.dwBufferBytes		= ap->ap_playbuflen;
    ap->ap_dsbuf                = NULL;
    if((hr = IDirectSound_CreateSoundBuffer(ap->ap_ds, &dsbd, &ap->ap_dsbuf, NULL)) < 0){
      	directsound_error(hr, "audio_directsound_play_start: CreateSoundBuffer");
	return -1;
    }

    /* Start the device */  
    hr = IDirectSoundBuffer_Play(ap->ap_dsbuf, 0, 0, DSBPLAY_LOOPING);
}
#endif /* newtry */


/* Called when settings change */
int 
audio_directsound_play_settings(play_api *pa, coding_params *cp)
{
    return 0;
}


int 
audio_directsound_play_ioctl(play_api *pa, int op, void *oparg)
{
    struct audio_directsound_play *ap = 
	(struct audio_directsound_play *)pa->pa_private; 

    switch (op){
    case SPHONE_AUDIO_IOCTL_PLAYOUT_GET:
	*(int*)oparg = PLAYOUT_SHMEM;
	break;
    case SPHONE_AUDIO_IOCTL_PLAYBUFLEN:
	*(int*)oparg = ap->ap_playbuflen;
	break;
    default:
	sphone_warning("audio_directsound_play_ioctl: No such op: %d", op);
	break;
    }
    return 0;
}

int 
audio_directsound_play_open(play_api *pa, void *arg)
{
  /*    struct audio_directsound_play *ap = 
	(struct audio_directsound_play *)pa->pa_private; */

    return 0;
}


int 
audio_directsound_play_play(play_api *pa, char *buf, size_t len, void* writecursor)
{
    struct audio_directsound_play *ap = 
	(struct audio_directsound_play *)pa->pa_private;
    struct rcv_stats *rst = &ap->ap_rs->rs_stats;
    int hr;
    void *pos1;
    int len1;
    void *pos2;
    int len2;

    if ((hr = IDirectSoundBuffer_Lock(ap->ap_dsbuf, 
				      (int)writecursor, /* write cursor */
				      len,      /* # bytes to lock */
				      &pos1,    /* address start of bytes */
				      (LPDWORD)&len1, 
				      &pos2, 
				      (LPDWORD)&len2, 
				      0x0/* DSBLOCK_FROMWRITECURSOR */)) < 0){
	directsound_error(hr, "audio_directsound_play_play: DS_Lock failed");
	return -1;
    } 
    assert(len == len1 + len2);

    /* 
     * Write the samples into the primary buffer, possibly wrap around to 
     * the beginning 
     */
    dbg_print(DBG_PLAY, "play: %d %d\n",   (int)pos1,
	      len1);
    memcpy(pos1, buf, len1);
    if (pos2){
      dbg_print(DBG_PLAY, "play: %d %d (2)\n", pos2, len2);
      memcpy(pos2, buf+len1, len2);
    }
    rst->rst_pbytes += len;
    rst->rst_ppkt++;
    if ((hr = IDirectSoundBuffer_Unlock(ap->ap_dsbuf, 
					pos1, len1, 
					pos2, len2)) < 0){
	directsound_error(hr, "audio_directsound_play_play: UnLock failed");
	return -1;
    } 
    return 0;
}

int 
audio_directsound_play_pos(play_api *pa, uint32_t *play_pos, uint32_t *write_pos)
{
    struct audio_directsound_play *ap = 
	(struct audio_directsound_play *)pa->pa_private; 
    int hr;
    if ((hr = IDirectSoundBuffer_GetCurrentPosition(ap->ap_dsbuf, 
						    play_pos, write_pos)) < 0){
	directsound_error(hr, "audio_directsound_play_pos:"
			  " GetCurrentPosition failed");
	return -1;
    }
    return 0;
}

int 
audio_directsound_play_stop(play_api *pa)
{
    struct audio_directsound_play *ap = 
	(struct audio_directsound_play *)pa->pa_private; 
    int hr;

    if ((hr = IDirectSoundBuffer_Stop(ap->ap_dsbuf)) < 0){
	directsound_error(hr, "audio_directsound_play_stop:"
			  "DS_Stop failed");
	return -1;
    }
    return 0;
}


int
audio_directsound_play_exit(play_api *pa)
{
    struct audio_directsound_play *ap = 
	(struct audio_directsound_play *)pa->pa_private;

    audio_directsound_play_stop(pa);
    /* Release sound buffer */
    if (ap->ap_dsbuf){
	IDirectSoundBuffer_Release(ap->ap_dsbuf);
	ap->ap_dsbuf = NULL;
    }
    if (ap->ap_ds) {
	/* Release DirectSound object */
	IDirectSound_Release(ap->ap_ds);
	ap->ap_ds = NULL;
    }
    if (ap)
	sfree(ap);
    sfree(pa);
    return 0;
}


int
audio_directsound_play_init(play_api *pa, rcv_session_t *rs)
{
    struct audio_directsound_play *ap;

    pa->pa_start     = audio_directsound_play_start;
    pa->pa_open      = audio_directsound_play_open;
    pa->pa_play      = audio_directsound_play_play;
    pa->pa_play_pos  = audio_directsound_play_pos;
    pa->pa_play_stop = audio_directsound_play_stop;
    pa->pa_settings  = audio_directsound_play_settings;
    pa->pa_ioctl     = audio_directsound_play_ioctl;
    pa->pa_exit      = audio_directsound_play_exit;

    if ((ap = (void*)smalloc(sizeof(struct audio_directsound_play))) == NULL){
	sphone_error("audio_directsound_play_init: malloc: %s", strerror(errno));
	return -1;
    }
    memset(ap, 0, sizeof(struct audio_directsound_play));
    pa->pa_private   = ap;
    ap->ap_rs        = rs;
    ap->ap_playbuflen = DEFAULT_PLAYBUF_LEN; /* XXX */

    if (audio_directsound_init() < 0)
	return -1;
    dbg_print(DBG_PLAY, "audio_directsound_play_init\n");
    return 0;
} 

#endif /* HAVE_DIRECTSOUND */
