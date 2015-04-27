/*-----------------------------------------------------------------------------
  File:   sphone_playout_shbuf.h
  Description: for audio devices supporting shared playout buffers
  Author: Olof Hagsand
  CVS Version: $Id: sphone_playout_shbuf.h,v 1.4 2005/02/13 17:19:31 olof Exp $
 
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
#ifndef _SPHONE_PLAYOUT_SHBUF_H_
#define _SPHONE_PLAYOUT_SHBUF_H_

/*
 *
 Playout is quite complex. There is a playoutpointer that the audio
 module updates as samples are played on the audio device. There is
 also a writepointer that the user updates where he has written the
 latest packet.

 +------------------------------------------------------+
 |           |           |            |                 |
 +------------------------------------------------------+
             ^writepointer           ^playoutpointer  --->

 *
 */ 

/*
 * Constants
 */ 
#define QMIN_DEFAULT_MS 50 /* 20 */
#define Q_DELTA_MS      100 /* 40 */
#define SILENT_INTERVAL_MS 200
/*
 * IOCTL's
 */ 

/*
 * Prototypes
 */ 
int playout_shbuf_play(rcv_session_t *rs, char* buf, size_t len, int seqtype);
int playout_shbuf_init(rcv_session_t *rs);

#endif  /* _SPHONE_PLAYOUT_SHBUF_H_ */


