/*-----------------------------------------------------------------------------
  File:   sphone_audio_shbuf.c
  Description: for audio devices supporting shared playout buffers
  Author: Olof Hagsand
  CVS Version: $Id: sphone_playout_shbuf.c,v 1.6 2005/02/13 17:19:31 olof Exp $
 
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

#define SMOOTHER 0.1 /* SMOOTH FACTOR */

struct playout_shbuf{
    /* Common to all playouts */
    uint32_t ps_bits_samp;               

    /* Specialized to shbuf playout */
    size_t ps_len;    /* Length of current packet */

    uint32_t ps_cursor_r;                /* Read cursor */
    uint32_t ps_cursor_w;                /* Write cursor */
    uint32_t ps_cursor_w_prev;           /* Previous Write cursor */
    uint32_t ps_cursor_w_s;              /* Weighted average of write cursor */

    size_t   ps_playbuflen;
    uint32_t ps_readpointer;
    uint32_t ps_write;
    uint32_t ps_playoutpointer;
    uint32_t ps_c;                       /* Position to write next pkt in buf */
    uint32_t ps_c_old;                   /* Previous position of c */

    uint32_t ps_qmin;                    /* Lower bound in bytes for d_s */
    uint32_t ps_qmean;                   /* Position to write next pkt in buf */
    uint32_t ps_qmax;                    /* Upper bound in bytes for d_s */

    uint32_t ps_d;                        /* length of queue at one instant */
    double ps_d_s;                        /* running mean of ps_d */

    void *ps_algorithm;      /* algoritm-specific data */

    size_t ps_drop;   /* Drop this packet */

    uint32_t ps_pkt;   /* Number of packets played XXX: duplicates */
    uint32_t ps_bytes; /* Number of bytes played XXX: duplicates */

    /* silence buffer: fill buffer with this regularly */
    char    *ps_silentbuf; /* Buffer with background noice to fill playbuf */
    size_t   ps_silentlen; /* Length of silentbuf */
    int      ps_nextsilence; /* Track position where silence was played last */

    /* Specializations for emulate */
    char *ps_playbuf;
    size_t ps_playbufsize;
};


/*
 * Reset corridor.
 */
#ifdef NOTYET
static int 
queue_reset(struct audioRparam_t *r)
{
  /* do frequent check of queue short after reset */
  r->adaptive_period_max = ADAPTIVE_PERIOD_INIT;  
  r->adaptive_period_min = ADAPTIVE_PERIOD_INIT/2;

  return 0;
}
#else
static int 
queue_reset(void)
{
  return 0;
}

#endif

/*
 * Wrap a *difference* pointer (a-b =) d, so that it is correct even if the
 * two terms (a, b) yielding the diff are out of phase.
 * Eg:
 *      |---------------------------|
 *       a                         b  then d = a-b should be (a+len)-b
 *       b                         a  then d = a-b should be a-(b+len)
 */
static int 
wrap(int d, unsigned int len)
{
  if(d < -(int)(len/2))
    d += len;
  else
    if(d > (int)(len/2))
      d -= len; 
  return d;
}

/*
 * incremental monotonic pointer diff (a-b=d) where we know
 * that a>b (or a+len>b).
 * Eg:
 *      |---------------------------|
 *       a                         b  then d = a-b should be (a+len)-b
 *
 */
static int 
wrap1(int d, unsigned int len)
{
  if (d < 0)
    return d+len;
  return d;
}



static int
shbuf_reset(struct playout_shbuf *ps)
{
    /*  
     * Adjust audio position in queue to qmean (reset)
     */
    ps->ps_c = (ps->ps_cursor_w + ps->ps_qmean)%ps->ps_playbuflen; 
    ps->ps_d = ps->ps_qmean; 
    ps->ps_d_s = (double)ps->ps_d; 
    ps->ps_cursor_w_prev = ps->ps_cursor_w; 
    return 0;
}

static int
shbuf_gap(struct playout_shbuf *ps, seq_t seq_gap)
{
    int delta;

    /* 
     * Do not add time for lost packet into running mean.
     * Save nr of lost packets
     */
    ps->ps_c = (ps->ps_cursor_w + ps->ps_qmean)%ps->ps_playbuflen; 
    ps->ps_d = wrap(ps->ps_c + (seq_gap) * ps->ps_len - ps->ps_cursor_w,
		ps->ps_playbuflen);
    ps->ps_d_s = SMOOTHER * (ps->ps_d) + (1.0 - SMOOTHER) * (ps->ps_d_s); 
    ps->ps_cursor_w_prev += 
	((seq_gap + 1) * (ps->ps_len))%ps->ps_playbuflen;
    delta = wrap1(ps->ps_cursor_w - ps->ps_cursor_w_prev, ps->ps_playbuflen);

    ps->ps_cursor_w_prev = ps->ps_cursor_w;
#if 0
    ps->ps_cursor_w_s = ps->ps_cursor_w_s + (abs(delta - ps->ps_len) - ps->ps_cursor_w_s)* RTCP_SMOOTHER;  
#endif
    return 0;
}

static int
shbuf_normal(struct playout_shbuf *ps)
{
    int delta;

    ps->ps_c = (ps->ps_c_old + ps->ps_len)%ps->ps_playbuflen; 
    ps->ps_d = wrap(ps->ps_c - ps->ps_cursor_w, ps->ps_playbuflen);
    ps->ps_d_s = SMOOTHER*ps->ps_d + (1.0 - SMOOTHER)*ps->ps_d_s;
    /*
     * Arrival variance.
     */
    delta = wrap1(ps->ps_cursor_w - ps->ps_cursor_w_prev, ps->ps_playbuflen);

#if 0
    ps->ps_cursor_w_s = ps->ps_cursor_w_s + (abs(delta - ps->ps_len) - ps->ps_cursor_w_s)* RTCP_SMOOTHER;  
#endif
    ps->ps_cursor_w_prev = ps->ps_cursor_w; 

    /*
     * Break corridor bounds (late or early packets)
     */
    if(ps->ps_d_s < ps->ps_qmin) 
	queue_reset();
    if(ps->ps_d_s > ps->ps_qmax) 
	queue_reset();

    if (ps->ps_d < 0){  /* Late packet */
	if(abs(ps->ps_d) >= ps->ps_len){ /* w before c , drop packet */
#ifdef XXX
	    dropit++; 
#endif
	    return 0;
	}
    } 


    return 0;
}

/*
  audioout_play_shmem
  Called with buffer to play out.
  Create audio out buffer, prepare it and write it to audio out device.

  +-----------------------------------------------------------------+
  |             |////////////|                                      |
  +-----------------------------------------------------------------+
    | <-- q --> | <-- len--> |
    w(i)        c(i)         c(i+1)
  
    where
    w is queried device's CurrentWriteCursor.
    c is where packet is placed in buffer.
    T is the "sending time" of a packet, T(i)-T(i-1) is the length of a packet.
    d is length of queue - playout point at one particular moment
    D is running mean of d
    s is d's deviation from D
    Then we have:

    c(0) = w(0) + qmean
    c(i) = c(i-1) + [T(i) - T(i-1)]      if D(i) in [qmin, qmax]
    c(i) = w(i) + qmean                  if D(i) not in [qmin, qmax] (reset)
    d(i) = [w(i) - w(j)] - [T(i) - T(j)] + qmean
                                         where j is the latest reset.
    D(i) = g*c(i) + (1-g)D(i-1), where g < 1.
    s(i) = d(i) - D(i) 

    The bounds on the queue length is a corridor determined by qmin and qmax. 
    qmean is a starting value between qmin and qmax. D(i) is allowed to
    vary in the corridor, but as soon as D(i) breaks either barrier,
    reset the playout point to qmean:
          ^
    qmax  |-----------------------------
          |
          |  
    qmean |...                 ....
          |   ........             ..... D
          |           ........   
    qmin  |-------------------.---------
          |


  There are two sequence numbers. audioseq that increments for every timeslice,
  and pktseq that increments for every packet sent. Audioseq may go faster
  than pktseq when silence detection is enabled.
  ----------------------------------------------------------------------------*/
int
playout_shbuf_play(rcv_session_t *rs, char* buf, size_t len, int seqtype)
{
    struct playout_shbuf *ps = (struct playout_shbuf*)rs->rs_playout;
    int retval = 0;
    play_api *pa = rs->rs_play_api;
    seq_t seq_gap = rs->seq_cur - rs->seq_prev;

    ps->ps_len = len;
    /* 
     * Get current playback pointers.
     * w is 10 ms before CurrentPlayCursor 
     */
    assert(pa && pa->pa_play_pos);

    if ((*pa->pa_play_pos)(pa, &ps->ps_cursor_r, &ps->ps_cursor_w) < 0){
	retval = -1;
	goto done;
    }
    /*
     * To compensate for some audio drivers that only count bytes without
     * wrap-around.
     */
    ps->ps_cursor_r %= ps->ps_playbuflen;
    ps->ps_cursor_w %= ps->ps_playbuflen;
    /* For 16-bit samples: no odd byte values */
    if ((ps->ps_bits_samp > 8) && (ps->ps_cursor_w % 2))
	ps->ps_cursor_w++;
    switch (seqtype){
    case SEQ_RESET:
    case SEQ_TALK_START:
	shbuf_reset(ps);
	break;
    case SEQ_GAP:
	shbuf_gap(ps, seq_gap);
	break;
    case SEQ_NORMAL:
	shbuf_normal(ps);
	break;
    default:
	sphone_error("audio_play_shbuf: %d illegal seqtype", seqtype);
    }

    /*
     * Now we (finally) play the audio packet.
     * Get pointers to memory: where to write audio data.     
     * You get two pointers in case the circular buffer is at 
     * the end-point.                                         
     */
    if (ps->ps_drop)
	retval = 0;
    else
      retval = (*pa->pa_play)(pa, buf, ps->ps_len, (void*)ps->ps_c);
  done:

    ps->ps_c_old = ps->ps_c; 
    ps->ps_c_old = ps->ps_c; 
    ps->ps_drop = 0;

    return retval;
}

int
playout_shbuf_exit(rcv_session_t *rs)
{
    struct playout_shbuf *ps = (struct playout_shbuf*)rs->rs_playout;

    if (ps){
      if (ps->ps_silentbuf)
	sfree(ps->ps_silentbuf);
      sfree(ps);
    }
    return 0;
}

/*--------------------------------------------------------------------------
  silentbuf_init
  Make buffers with "silent" audio for silent suppression use
 *------------------------------------------------------------------------*/
static void 
silentbuf_init(char *buf, int len, int bits_samp)
{
  int i;
  struct timeval t;

  for (i = 0; i < len; i++){
    t=gettimestamp(); /* XXX: random */
    buf[i] = (1<<(bits_samp-1)) + t.tv_usec%10; /* white noise */
  }
}


/*---------------------------------------------------------------------
  silentout_play
 ---------------------------------------------------------------------*/
static int 
silentout_play(int dummy, void *arg)
{
  rcv_session_t *rs = (rcv_session_t *)arg;
  struct playout_shbuf *ps = (struct playout_shbuf*)rs->rs_playout;
  play_api *pa = rs->rs_play_api;
  int w = 0, p=0; 
  int plen;
  int tmp_len; 
  char *buf = ps->ps_silentbuf;
  int len = ps->ps_silentlen;
  struct timeval now, t;

  plen = ps->ps_playbuflen;
  /* Get current playback pointers.*/
  /* w 10 ms before CurrentPlayCursor */
  if ((*pa->pa_play_pos)(pa, &p, &w) < 0)
    return -1;

  /*-----------------------------------------------------------------------*/
  /* här bestämma var tystnad  skall in, skulle kunna ha flera bufferdtrl. */
  /*-----------------------------------------------------------------------*/
  if(ps->ps_nextsilence)
    tmp_len = (p - ps->ps_nextsilence + plen)%plen;
  else { /* first time */
    ps->ps_nextsilence = (p - len + plen)%plen; 
    tmp_len = len;
  }
  if(ps->ps_nextsilence%2)
    ps->ps_nextsilence -= 1;

  tmp_len -= len;
  while (tmp_len > 0) {
    dbg_print(DBG_PLAY, "silence: len:%d\n", len);
    if ((*pa->pa_play)(pa, buf, len, (void*)ps->ps_nextsilence) < 0)
      return -1;
    ps->ps_nextsilence = (ps->ps_nextsilence + len)%plen;
    tmp_len -= len;
  }
  now = gettimestamp();
  t.tv_sec = 0;
  t.tv_usec = SILENT_INTERVAL_MS*1000;
  timeradd(&now, &t, &t);
  if (eventloop_reg_timeout(t, silentout_play, rs, "background audio") < 0)
    return -1;

 return 0;
}

int
playout_shbuf_init(rcv_session_t *rs)
{
    struct playout_shbuf *ps;
    play_api  *pa = (play_api*)rs->rs_play_api;
    struct timeval now, t;

    if ((ps = (void*)smalloc(sizeof(struct playout_shbuf))) == NULL){
	sphone_error("playout_shbuf_init: malloc: %s", strerror(errno));
	return -1;
    }
    memset(ps, 0, sizeof(struct playout_shbuf));
    rs->rs_playout   = ps;
    ps->ps_qmin = QMIN_DEFAULT_MS;
    ps->ps_qmax = ps->ps_qmin + Q_DELTA_MS;
    ps->ps_qmean = ps->ps_qmin + (ps->ps_qmax - ps->ps_qmin)/2;

    if ((*pa->pa_ioctl)(pa, SPHONE_AUDIO_IOCTL_PLAYBUFLEN, 
			&ps->ps_playbuflen) < 0)
	return -1;

    ps->ps_silentlen = ps->ps_playbuflen/3;
    if ((ps->ps_silentbuf = malloc(ps->ps_silentlen)) == NULL){
      perror("playout_shbuf_init: malloc\n");
      return -1;
    }
    silentbuf_init(ps->ps_silentbuf, ps->ps_silentlen, ps->ps_bits_samp); 
    /* XXX: Can be tuned better */
    now = gettimestamp();
    t.tv_sec = 0;
    t.tv_usec = SILENT_INTERVAL_MS*1000;
    timeradd(&now, &t, &t);
    if (eventloop_reg_timeout(t, silentout_play, rs, "background audio") < 0)
      return -1;
    return 0;
}


