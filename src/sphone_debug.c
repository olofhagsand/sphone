/*-----------------------------------------------------------------------------
  File:   sphone_debug.c
  Description: Debugging functions
  Author: Olof Hagsand
  CVS Version: $Id: sphone_debug.c,v 1.4 2005/01/28 11:06:27 olof Exp $
 
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
#include <stdarg.h>
#include <stdlib.h>

#include "sphone.h"

#ifndef min
#define  min(a,b)        ((a) < (b) ? (a) : (b))
#endif /* min */

/*
 * Constants
 */
#define DEBUG_STR_LEN 512

/*
 * External Variables
 */
unsigned int debug = 0;

/*
 * Internal Variables
 */
static FILE *dbg_fd = NULL;

int 
dbg_init(FILE *fd)
{
    dbg_fd = fd;
    return 0;
}

void 
dbg_print(int level, char *format, ...)
{
    static char dbg_str[DEBUG_STR_LEN];
    va_list args;

    assert(dbg_fd != NULL);
    va_start(args, format);
    /* debug should have exact match with level, or level should be always */

    if ((level!=0 && ((level&debug)==level)) || (level & DBG_ALWAYS)){
#ifdef HAVE_VSNPRINTF /* not defined */
	if (vsnprintf(dbg_str, DEBUG_STR_LEN, format, args) > 0)
#else
	    if (vsprintf(dbg_str, format, args) > 0)
#endif
		    fprintf(dbg_fd, "%s", dbg_str);
    }
    va_end(args);
    fflush(dbg_fd);
}

void 
dbg_print_pkt(int level, char *str, char *buf, size_t len)
{
    int i;

    dbg_print(level, "%s:\n", str);
    for (i=0;i<min(len,64);i++){
	dbg_print(level, "%02x", buf[i]&0xff);
	if (i%2==1)
	    dbg_print(level, " ");
    }
    dbg_print(level, "\n");
}
