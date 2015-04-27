/*-----------------------------------------------------------------------------
  File:   sphone_queue.c
  Description: Adaptive queue handling
  Author: Olof Hagsand
  CVS Version: $Id: sphone_queue.c,v 1.3 2004/02/01 21:42:47 olofh Exp $
 
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

/*--------------------------------------------------------------------------
  adapt_queue()
  Called periodically to control and adjust queue length and corridor.
  First, Calculate the standard deviation and the running mean of standard 
  deviation. The input to running mean is twice the standard deviation  and 
  the result is the value set to corridor qmax - qmin
  Secondly, calculate the queue length to qmin
  The d_s - d (d = c - w) is logged in quantiles for every packet handled.
  If d is negative means that current write cursor to audio device
  has passed the position where new audio data should be placed, thus
  we have a late packet. 
  1)If c_dev = d_s - d for a specified quantile is negative means that we 
    have a packet coming early in relation to running mean of d.
        If c_dev is smaller than previous s_s, s_s is decreasing and thus qmin is 
        set to lower value and the corridor is pushed down so delay is minimized.
  2)If c_dev = d_s - d for a specified quantile is positive means that we 
        have a packet coming late in relation to running mean of d (and in
        certain case too late, see above).
  s_s holds the running mean for qmin and is the setting we do for 
  qmin plus some xtra millisecond.     
  Called periodically to check current drop rates. Here is where an adaptive
  algorithm can adapt queue lengths.  
  
 *------------------------------------------------------------------------*/

#ifdef NOTYET
void 
adapt_queue(struct audioRparam_t * r, int buflen)
{
    int i;

    if(r->adaptive_period_max < ADAPTIVE_PERIOD_NORMAL) {
	r->adaptive_period_max += 20; 
	r->adaptive_period_min += 10; 
    }

    /* 
     * sort the incoming packet variance data vector (used in rtcp
     * reports)
     */
 
    /*  printf("(%d, %d)\n", r->adaptive_period_min, r->adaptive_period_max); */
    for (i=0; i < r->deltaIx; i++) {
	printf("%d \t %f\n", i, r->deltav[i] / 8.0);
    }
    printf("\n\n");
    qsort(r->deltav, r->deltaIx, sizeof(int), compareInt); 
    calcLen_corridor(r, Q_SMOOTHER);
    calcLen_queue(r, Q_SMOOTHER); 
    adjust_queue_corridor(r);

    /*  printf("%d: qmin: %d, corridor: (%d) %d\n", 
	r->curr_audioSeq, r->qmin, MAX(r->size_byte, r->deltav[r->deltaIx-1]-r->deltav[0]), r->qmax-r->qmin); */

    /*
     * Here we detect that the smoothed mean of the queue length has
     * drifted from queue mean, indicate a soft reset of the queue, to
     * be performed next time we play a packet.
     */
    if(r->d_s > r->qmean) 
	r->queue_reset_early = 1;
    if(r->d_s < r->qmean) 
	r->queue_reset_late = 1;

    queuedata_print(r); 


#ifdef NOTUSED
    r->index++;
    r->rout = 0;
    r->misses = 0;
    r->quantileIx = 0;
    r->deltaIx = 0;
#endif
}
#endif
