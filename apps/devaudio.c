/*-----------------------------------------------------------------------------
  File:   devaudio.c
  Description: /dev/audio test application - check status of /dev/audio
  Author: Olof Hagsand
  CVS Version: $Id: devaudio.c,v 1.6 2004/12/27 17:34:03 olof Exp $
 
  This software is a part of SICSPHONE, a real-time, IP-based system for 
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
#include <string.h>
#include <fcntl.h>

#include "sphone.h"

#ifdef HAVE_SYS_AUDIOIO_H
#include "audio/sphone_audio_devaudio.h"


static void usage(char *argv0)
{
    fprintf(stderr, "usage:%s \n"
	    "\t[-h]\t\tPrint this text\n"
	    "\t[-v]\t\tPrint version\n"
	    "\t[-f <devnr>]\tAudio device (default: %s)\n",
	    argv0, DEVAUDIO_DEVICE
	); 
    
    exit(0);
}

int
main(int argc, char *argv[])
{
    int c;
    int fd;
    char device[32];

    dbg_init(stderr);

    /*
     * Command line args
     */
    while ((c = getopt(argc, argv, "hvf:")) != -1){
	switch (c) {
	case 'h' : /* help */
	    usage(argv[0]);
	    break;
	case 'v' : /* version */
	    fprintf(stdout, "Sicsophone version %s\n", SPHONE_VERSION);
	    usage(argv[0]);
	    break;
	case 'f' : /* devaudio device */
	    if (!optarg || sscanf(optarg, "%s", device) != 1)
		usage(argv[0]);
	    break;
	default:
	    usage(argv[0]);
	    break;
	} /* switch */
    } /* while */

    if ((fd = open(device, O_RDONLY)) < 0){
	sphone_error("devaudio open(/dev/audioctl): %s", 
		     strerror(errno));
	return -1;
    }
    debug = 1;
    dbg_devaudio(debug, fd);
    close(fd);
    exit(0);
}
#else /* HAVE_SYS_AUDIOIO_H */
int
main(int argc, char *argv[])
{
  exit(0);
}
#endif /* HAVE_SYS_AUDIOIO_H */
