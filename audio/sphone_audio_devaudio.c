/*-----------------------------------------------------------------------------
  File:   sphone_audio_devaudio.c
  Description: audio module for unix /dev/audio - common part
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
/*
 * See also audioctl -a (1)
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> /* open */

#include "sphone.h"

#ifdef HAVE_SYS_AUDIOIO_H
#include <sys/ioctl.h>
#include <sys/audioio.h>

#include "sphone_audio_devaudio.h"



/*
 * Variables
 */
/* Global variable for unique process resources */
static struct devaudio_device *Devaudio_devicelist = NULL;

/*
 * For some strange reason, everything is muted by default on /dev/audio
 * One way (the only?) to unmute is to access the mixer device and
 * just set everything to "off" that rhymes with mute,...
 */
static int
devaudio_unmute(int devnr)
{
    int fd;
    mixer_devinfo_t md;
    mixer_ctrl_t mc;
    char buf[32];

    snprintf(buf, sizeof buf, "%s%d", DEVAUDIO_MIXERDEVICE, devnr);
    if ((fd = open(buf, O_RDWR)) < 0){
	sphone_error("devaudio_unmute open(/dev/mixer): %s", 
		     strerror(errno));
	return -1;
    }
    md.index = 0;
    while (ioctl(fd, AUDIO_MIXER_DEVINFO, &md) == 0){
	if (md.type == AUDIO_MIXER_ENUM && strcmp(md.label.name, "mute")==0){
	    mc.dev = md.index;
	    mc.type =  AUDIO_MIXER_ENUM;
	    mc.un.ord = 0;
	    if (ioctl(fd, AUDIO_MIXER_WRITE, &mc) < 0){
		sphone_error("devaudio_unmute ioctl AUDIO_MIXER_WRITE: %s", 
			     strerror(errno));
		close(fd);
		return -1;
	    }
	}
	md.index++;
    }
    close(fd);
    return 0;
}


/*
 * Common initializtion for both recording and playing of /dev/audio
 * Try to open /dev/audio0,..,/dev/audio9: first try to find USB audio,
 * if that fails, open first.
 */
struct devaudio_device *
audio_devaudio_init(char *devname)
{
    int fullduplex = 1;
    int fd = -1;
    int devnr;
    struct devaudio_device *dd;
    int info;

    devnr = atoi(devname + strlen(devname)-1);
    for (dd=Devaudio_devicelist; dd; dd=dd->dd_next)
	if (strcmp(dd->dd_name, devname) == 0)
	    break;
    if (dd != NULL){ /* Init only once: can be shared between threads */
	dd->dd_refcnt++;
	return dd;
    }
    if ((fd = open(devname, O_RDWR)) < 0){
	sphone_error("audio_devaudio_init: open %s: %s", 
		     devname, strerror(errno));
	return NULL;
    }
    dbg_print(DBG_ALWAYS, "audio_devaudio_init: Opening %s\n", devname);

    /* 
     * Check hardware properties
     * Sanity check: we require the device to support full duplex and 
     * independent setting of play and receive.
     */
    if (ioctl(fd, AUDIO_GETPROPS, &info) < 0){
	sphone_error("audio_devaudio_init ioctl GETPROPS: /dev/audio: %s", 
		     strerror(errno));
	return NULL;
    }
    if (((info & AUDIO_PROP_FULLDUPLEX) == 0) ||
	((info & AUDIO_PROP_FULLDUPLEX) == 0)){
	close(fd);
	sphone_error("audio_devaudio_init: audio device not full duplex/ independent capable");
	return NULL;
    }
    if ((dd = (void*)smalloc(sizeof(struct devaudio_device))) == NULL){
	sphone_error("audio_devaudio_init: malloc: %s\n", 
		     strerror(errno));
	return NULL;
    }
    dd->dd_next   = Devaudio_devicelist;
    Devaudio_devicelist = dd;
    strcpy(dd->dd_name, devname);
    dd->dd_fd     = fd;
    dd->dd_refcnt = 1;
    dd->dd_devnr  = devnr;
    dd->dd_shmem  = (info & AUDIO_PROP_MMAP)? 1 : 0;

    /* 
     * Unmute /dev/audio* (by using /dev/mixer*)
     */
    if (devaudio_unmute(devnr) < 0)
	return NULL;

#if 0
    /* Set stuff common to both play and record */
    if (ioctl(fd, AUDIO_FLUSH, NULL) < 0){
	sphone_error("audio_devaudio_init ioctl AUDIO_FLUSH: %s", 
		     strerror(errno));
	return NULL;
    }
#endif    
    if (ioctl(fd, AUDIO_SETFD, &fullduplex) < 0){
	sphone_error("audio_devaudio_init ioctl AUDIO_SETFD: %s", 
		     strerror(errno));
	return NULL;
    }
    return dd;
}

int
audio_devaudio_exit(struct devaudio_device *dd0)
{
    struct devaudio_device **ddp = &Devaudio_devicelist;
    struct devaudio_device *dd;

    dd0->dd_refcnt--;
    if (dd0->dd_refcnt > 0)
	return 0;
    for (dd = *ddp; dd; dd=dd->dd_next){
	if (dd == dd0){
	    *ddp = dd->dd_next;
	    break;
	}
	ddp = &dd->dd_next;
    }
    dbg_print(DBG_ALWAYS, "Closing %s,...\n", dd->dd_name);
    close(dd0->dd_fd);
    sfree(dd0);

    return 0;
}



/*
 * Translate from our coding to devaudio coding
 */
int
coding2devaudio(enum coding_type type)
{
    int type2;

    switch(type){
	case CODING_LINEAR16: 
	    /* Signed 16-bit two's complement and network byte order:big end */
	    type2 = AUDIO_ENCODING_SLINEAR_LE;  /* XXX: Should be BE */
	    break;
	case CODING_ULAW:
	    type2 = AUDIO_ENCODING_ULAW;
	    break;
	case CODING_ALAW:
	    type2 = AUDIO_ENCODING_ALAW;
	    break;
	case CODING_LINEAR8:
	    type2 = AUDIO_ENCODING_SLINEAR; /* XXX: wrong */
	    break;
#ifdef HAVE_ILBC
	case CODING_iLBC:
	    type2 = AUDIO_ENCODING_SLINEAR; /* XXX: wrong */
	    break;
#endif /* HAVE_ILBC */
    }
    return type2;
}


static char *
encoding2string(int encoding)
{
    switch (encoding){
    case AUDIO_ENCODING_ULAW:
	return "ULAW";
    case AUDIO_ENCODING_ALAW:
	return "ALAW";
    case AUDIO_ENCODING_SLINEAR:
	return "SLINEAR";
    case AUDIO_ENCODING_ULINEAR:
	return "ULINEAR";
    case AUDIO_ENCODING_ADPCM:
	return "ADPCM";
    case AUDIO_ENCODING_SLINEAR_LE:
	return "SLINEAR_LE";
    case AUDIO_ENCODING_SLINEAR_BE:
	return "SLINEAR_BE";
    case AUDIO_ENCODING_ULINEAR_LE:
	return "ULINEAR_LE";
    case AUDIO_ENCODING_ULINEAR_BE:
	return "ULINEAR_BE";
    default:
	return "<unknown>";
	break;
    }
    return NULL;
}


int
dbg_devaudio(int level, int fd)
{
    audio_device_t ad;
    audio_encoding_t ae;
    audio_info_t ai;
    int info;

    dbg_print(level, "Debug info: /dev/audio \n");
    dbg_print(level, "======================\n"); 

    /* Device name and version */
    if (ioctl(fd, AUDIO_GETDEV, &ad) < 0){
	sphone_error("audio_devaudio_play_init ioctl: /dev/audio: %s", 
		     strerror(errno));
	return -1;
    }
    dbg_print(level, "\tdevice name: %s\n\tversion:%s\n\tconfig:%s\n", 
	      ad.name, 
	      ad.version, 
	      ad.config);

    /* List of supported encodings */
    ae.index = 0;
    while (ioctl(fd, AUDIO_GETENC, &ae) == 0){
	dbg_print(level, "\tEncoding: index: %d: name %s, encoding: %d, precision: %d flags: %x\n", 
		  ae.index++, 
		  ae.name, 
		  ae.encoding, 
		  ae.precision,
		  ae.flags);
    }

    /* Check hardware properties */
    if (ioctl(fd, AUDIO_GETPROPS, &info) < 0){
	sphone_error("audio_devaudio_play_init ioctl: /dev/audio: %s", 
		     strerror(errno));
	return -1;
    }
    dbg_print(level, "\thw props: ");
    if (info & AUDIO_PROP_FULLDUPLEX)
	dbg_print(level, "admits full duplex, ");
    if (info & AUDIO_PROP_MMAP)
	dbg_print(level, "can be used with mmap, ");
    if (info & AUDIO_PROP_INDEPENDENT)
	dbg_print(level, "set playing and recording independently");
    dbg_print(level, "\n");

    /* Check setting of full-duplex mode */
    if (ioctl(fd, AUDIO_GETFD, &info) < 0){
	sphone_error("audio_devaudio_play_init ioctl: /dev/audio: %s", 
		     strerror(errno));
	return -1;
    }
    if (info)
	dbg_print(level, "\tdevice is in full duplex mode\n");
    else
	dbg_print(level, "\tdevice is in half duplex mode\n");

    /* Get audio information */
    if (ioctl(fd, AUDIO_GETINFO, &ai) < 0){
	sphone_error("audio_devaudio_play_init ioctl: /dev/audio: %s", 
		     strerror(errno));
	return -1;
    }
    dbg_print(level, "\tblocksize: %d\n", ai.blocksize);
    dbg_print(level, "\tlowat: %d\n", ai.lowat);
    dbg_print(level, "\thiwat: %d\n", ai.hiwat);

    dbg_print(level, "\tmode[0x%x]: ", ai.mode);
    if (ai.mode & AUMODE_PLAY)
	dbg_print(level, "playing, ");
    if (ai.mode & AUMODE_RECORD)
	dbg_print(level, "recording, ");
    if (ai.mode & AUMODE_PLAY_ALL)
	dbg_print(level, "play all");
    dbg_print(level, "\n");

    /* Print play information */
    dbg_print(level, "\tplay sample_rate: %d\n\tplay precision: %d\n",
	      ai.play.sample_rate,
	      ai.play.precision);
    dbg_print(level, "\tplay encoding: %s\n", 
	      encoding2string(ai.play.encoding));
    dbg_print(level, "\tplay gain: %d [min:%d, max:%d]\n",
	      ai.play.gain, AUDIO_MIN_GAIN, AUDIO_MAX_GAIN);

    dbg_print(level, "\tplay port: [0x%x]", ai.play.port);
    if (ai.play.port & AUDIO_SPEAKER)
	dbg_print(level, "speaker, ");
    if (ai.play.port & AUDIO_HEADPHONE)
	dbg_print(level, "headphone, ");
    if (ai.play.port & AUDIO_LINE_OUT)
	dbg_print(level, "line out");
    dbg_print(level, "\n");

    dbg_print(level, "\tAvailable play ports: [0x%x]", ai.play.avail_ports);
    if (ai.play.avail_ports & AUDIO_SPEAKER)
	dbg_print(level, "speaker, ");
    if (ai.play.avail_ports & AUDIO_HEADPHONE)
	dbg_print(level, "headphone, ");
    if (ai.play.avail_ports & AUDIO_LINE_OUT)
	dbg_print(level, "line out");
    dbg_print(level, "\n");


    dbg_print(level, "\tplay buffer size: %d\n\tplay samples: %d\n",
	      ai.play.buffer_size,
	      ai.play.samples);
    dbg_print(level, "\tplay pause: %d\n\tplay error: %d\n",
	      ai.play.pause,
	      ai.play.error);
    dbg_print(level, "\tplay waiting: %d\n\tplay open: %d\n",
	      ai.play.waiting,
	      ai.play.open);
    dbg_print(level, "\tplay active: %d\n", ai.play.active);

    /* Print record information */
    dbg_print(level, "\trecord sample_rate: %d\n\trecord precision: %d\n",
	      ai.record.sample_rate,
	      ai.record.precision);
    dbg_print(level, "\trecord encoding: %s\n", 
	      encoding2string(ai.record.encoding));
    dbg_print(level, "\trecord gain: %d [min:%d, max:%d]\n",
	      ai.record.gain, AUDIO_MIN_GAIN, AUDIO_MAX_GAIN);

    dbg_print(level, "\trecord port: [0x%x]", ai.record.port);
    if (ai.record.port & AUDIO_MICROPHONE)
	dbg_print(level, "microphone, ");
    if (ai.record.port & AUDIO_LINE_IN)
	dbg_print(level, "headphone, ");
    if (ai.record.port & AUDIO_CD)
	dbg_print(level, "CD input");
    dbg_print(level, "\n");

    dbg_print(level, "\tAvailable record ports: [0x%x]", ai.record.avail_ports);
    if (ai.record.avail_ports & AUDIO_MICROPHONE)
	dbg_print(level, "microphone, ");
    if (ai.record.avail_ports & AUDIO_LINE_IN)
	dbg_print(level, "headphone, ");
    if (ai.record.avail_ports & AUDIO_CD)
	dbg_print(level, "CD input");
    dbg_print(level, "\n");

    dbg_print(level, "\trecord buffer size: %d\n\trecord samples: %d\n",
	      ai.record.buffer_size,
	      ai.record.samples);
    dbg_print(level, "\trecord pause: %d\n\trecord error: %d\n",
	      ai.record.pause,
	      ai.record.error);
    dbg_print(level, "\trecord waiting: %d\n\trecord open: %d\n",
	      ai.record.waiting,
	      ai.record.open);
    dbg_print(level, "\trecord active: %d\n", ai.record.active);

    return 0;
}


#endif /* HAVE_SYS_AUDIOIO_H */
