/*-----------------------------------------------------------------------------
  File:   sphone_silence.c
  Description: Code for silence detection
  Author: Kjell Hansson and Olof Hagsand
  CVS Version: $Id: sphone_silence.c,v 1.3 2004/12/27 17:34:05 olof Exp $
 
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

#include "sphone.h"

/* number of (not consecutive) samples to check for silence */
#define NR_SAMPLES 4 

int 
silence8(char *buf, int len, int threshold)
{
    int i, index;
    uint8_t value=0;
    uint8_t min, max;

    min = *buf;
    max = *buf;
    for (i=1; i<= NR_SAMPLES; i++) {
	index = len*i/NR_SAMPLES - 1;
	value = buf[index];
	if (value < min) 
	    min = value;
	if (value > max) 
	    max = value;
    }
    if((max-min) < threshold)
	return 1;
    return 0;
}

int 
silence16(char *buf, int len, int threshold)
{
    int i, index;
    uint16_t value=0; 
    uint16_t min, max; 

    min = ((uint16_t *)buf)[0];
    max = ((uint16_t *)buf)[0];
    for (i=1; i<= NR_SAMPLES; i++){
	index = len*i/(2*NR_SAMPLES) - 2; /* index */
	value = ((uint16_t *)buf)[index];
	if (value < min) 
	    min = value;
	if (value > max) 
	    max = value;
    }
    if((max-min) < threshold)
	return 1;
    return 0;
}


/*
 *  Silence (Voice Activity Detection )
 * Take data (samples) and length as input and check if it's a silence packet
 * return 1 if silence, 0 if no silence, -1 if error
 */
int 
silence(char *buf, int len, uint32_t bits_samp, int *silence_nr, 
	int silence_threshold)
{
    int retval;

    switch(bits_samp){
    case 8:
	retval = silence8(buf, len, SILENCE_LEVEL_8);
	break;
    case 16:
	retval = silence16(buf, len, SILENCE_LEVEL_16);
	break;
    default:
	sphone_error("silence, unsupported bits_samp: %d\n", bits_samp);
	return -1;
	break;
    }
    if (retval == 1)
	(*silence_nr)++;	 
    else
	*silence_nr = 0;
    if (*silence_nr > silence_threshold)
	return 1; /* really silent */
    return 0;
}
