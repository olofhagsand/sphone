/*-----------------------------------------------------------------------------
  File:   sphone_audio_record.h
  Description: audio recording indirection module
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
#ifndef _SPHONE_AUDIO_RECORD_H_
#define _SPHONE_AUDIO_RECORD_H_

/*
 * Generic audio interface function. 
 *

 Recording is based on a polling function and a sending function. There is a
 recordpointer that the audio device updates, and there is a readpointer
 that keeps track of how far the user has read.  The user
 calls the polling function, and the polling function makes a call to the 
 sending function for each packet it records: all packets between the 
 readpointer and the recordpointer. Thereafter, the readpointer is updated.

               send_one      send_one
 +------------------------------------------------------+
 |           |   pkt1     |  pkt2       |               |
 +------------------------------------------------------+
            ^readpointer                   ^recordpointer  -->
 *
 * A structure describing an indirection interface as follows:
 *
   ra_init       Initialization 
   ra_start      Start
   ra_poll       Get (record) audio samples from audio module.  
                        Read all ready samples, and 
			use the function send_one for sending each packet.
			Return -1 on error, return 0 on OK, 
			return 1 if "ready" or eof.
   ra_ioctl      Set a variable in audio recording module.		
 */ 
typedef struct audio_record_api record_api;
struct audio_record_api{
    /* Public fields */
    int (*ra_start)(record_api *);        
    int (*ra_open)(record_api *, void *);        /* Open file/device */
    int (*ra_poll)(record_api *);
    int (*ra_settings)(record_api *, coding_params *); /* Called when settings change */
    int (*ra_callback)(record_api *, int (*fn)(char*, int, void*), void *arg);        
    int (*ra_ioctl)(record_api *, int op, void *arg);        
    int (*ra_exit)(record_api *);        
    void *ra_private;                             /* private data */
};

/*
 * IOCTL's
 */ 

/*
 * Prototypes
 */ 
record_api *audio_record_init(enum audio_type type, void *ss);

#endif  /* _SPHONE_AUDIO_RECORD_H_ */


