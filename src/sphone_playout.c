/*-----------------------------------------------------------------------------
  File:   sphone_audio.c
  Description: audio indirection module
  Author: Olof Hagsand
  CVS Version: $Id: sphone_playout.c,v 1.7 2004/02/15 22:21:15 olofh Exp $
 
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

#include "sphone.h"
#include "sphone_playout_shbuf.h"

struct playout_private{
    int stupid_win32_dont_allow_empty_struct;/* Common to all playouts */
};


int
playout_fifo_play(rcv_session_t *rs, char* buf, size_t len, int seqtype)
{
    struct playout_private *pp = (struct playout_private*)rs->rs_playout;
    play_api *pa = rs->rs_play_api;
    int retval;

    dbg_print(DBG_PLAY, "playout_fifo_play\n");
    assert(pp);
    assert(pa);
    switch (seqtype){
    case SEQ_DUP:          /* Duplicate */
    case SEQ_BAD:          /* Very large difference in sequence number */
	break; /* shouldnt get here */
    case SEQ_RESET:        /* First time */
    case SEQ_TALK_START:   /* Silence period ended */
    case SEQ_GAP:          /* Lost packet */
    case SEQ_NORMAL:       /* Normal */
	retval = (*pa->pa_play)(pa, buf, len, 0);
    }
    return retval;
}

int
playout_play(rcv_session_t *rs, char* buf, size_t len, int seqtype)
{
    switch(rs->rs_playout_type){
    case PLAYOUT_FIFO:
	return playout_fifo_play(rs, buf, len, seqtype);
	break;
    case PLAYOUT_SHMEM:
	return playout_shbuf_play(rs, buf, len, seqtype);
	break;
    }
    return 0;
}

int
playout_exit(void *arg)
{
    if (arg)
	sfree(arg);
    return 0;
}


int
playout_init(enum playout_type type, rcv_session_t *rs)
{
    int retval;
    struct playout_private *pp;

    rs->rs_playout_type = type;
    switch (type){
    case PLAYOUT_FIFO:
	if ((pp = (struct playout_private *)
	     smalloc(sizeof(struct playout_private))) == NULL){
	    sphone_error("playout_init: malloc: %s", strerror(errno));
	    return -1;
	}
	rs->rs_playout = pp;
	break;
    case PLAYOUT_SHMEM:
	retval = playout_shbuf_init(rs);
	if (retval < 0)
	    return -1;
	break;
    }
    return 0;
}

