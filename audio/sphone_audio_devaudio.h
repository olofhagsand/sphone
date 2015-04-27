/*-----------------------------------------------------------------------------
  File:   sphone_audio_devaudio.h
  Description: audio module for unix /dev/audio - common part
  Author: Olof Hagsand
  CVS Version: $Id: sphone_audio_devaudio.h,v 1.9 2004/12/27 17:34:04 olof Exp $
 
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
#ifndef _SPHONE_AUDIO_DEVAUDIO_H_
#define _SPHONE_AUDIO_DEVAUDIO_H_

/*
 * Types
 */
struct devaudio_device{
    struct devaudio_device *dd_next;     /* Next audio device descriptor */
    char                   dd_name[16]; /* Name of audio device */
    int                    dd_fd;       /* file descriptor */
    int                    dd_refcnt;   /* ref count. remove if 0 */
    int                    dd_devnr;    /* Device number, eg if /dev/audio5 is used, this is 5 */
    int                    dd_shmem;    /* Device supports shared mem */
};

/*
 * Prototypes
 */ 
int dbg_devaudio(int level, int fd);
struct devaudio_device *audio_devaudio_init(char *devname);
int audio_devaudio_exit(struct devaudio_device *);
int coding2devaudio(enum coding_type type);


#endif  /* _SPHONE_AUDIO_DEVAUDIO_H_ */


