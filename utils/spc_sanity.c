/*-----------------------------------------------------------------------------
  File:   spc_sanity.c
  Description: Utility that checks sanity of log files
  Author: Olof Hagsand
  CVS Version: $Id: spc_sanity.c,v 1.1 2005/03/16 10:38:03 olof Exp $
 
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int
main(int argc, char *argv[])
{
    struct stat sb;
    FILE *f;
    char keyword[64];
    char ipaddr[64];
    int i, i1, i2, i3, i4;
    int seq, seq_prev;
    char c0, c1;
    int retval;
    int line=0;

    if (argc != 2){
	fprintf(stderr, "Usage: %s filename (argc:%d)\n", argv[0], argc);
	exit(1);
    }
    if (stat(argv[1], &sb) < 0){
	perror("spc_sanity: stat");
	exit(1);
    }
    if ((f = fopen(argv[1], "r")) == NULL){
	perror("spc_sanity:fopen");
	exit(1);
    }

    /*
     * Line 1: testid: <int>
     */
    retval = fscanf(f, "%s %d", keyword, &i);
    if (retval != 2){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if (strcmp(keyword, "testid:") != 0){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    line++;

    /*
     * Line 2: srcid: <int>
     */
    retval = fscanf(f, "%s %d", keyword, &i);
    if (retval != 2){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if (strcmp(keyword, "srcid:") != 0){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    line++;

    /*
     * Line 3: dstid: <int>
     */
    retval = fscanf(f, "%s %d", keyword, &i);
    if (retval != 2){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if (strcmp(keyword, "dstid:") != 0){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    line++;

    /*
     * Line 4: src: <ipaddr>/<port>
     */
    retval = fscanf(f, "%s %s", keyword, ipaddr);
    if (retval != 2){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if (strcmp(keyword, "src:") != 0){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if ((strchr(ipaddr, '/')) == NULL){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    line++;

    /*
     * Line 5: dst: <ipaddr>/<port>
     */
    retval = fscanf(f, "%s %s", keyword, ipaddr);
    if (retval != 2){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if (strcmp(keyword, "dst:") != 0){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if ((strchr(ipaddr, '/')) == NULL){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    line++;

    /*
     * Line 6: start: <int>
     */
    retval = fscanf(f, "%s %d", keyword, &i);
    if (retval != 2){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if (strcmp(keyword, "start:") != 0){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    line++;

    /*
     * Line 7: dur: <int>
     */
    retval = fscanf(f, "%s %d", keyword, &i);
    if (retval != 2){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    if (strcmp(keyword, "dur:") != 0){
	fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	exit(1);
    }
    line++;

    seq_prev = 0;
    while (!feof(f)){
	retval = fscanf(f, "%d %d%c%d %d%c%d", &seq, &i1, &c0, &i2, &i3, &c1, &i4);
	if (feof(f))
	    break;
	if (retval != 7){
	    fprintf(stderr, "spc_sanity: %s: Error on line %d (retval:%d)\n", argv[1], line, retval);
	    exit(1);
	}
	if (c0 != '.' && c1 != '.'){
	    fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	    exit(1);
	}
	if (seq - seq_prev > 32000 || seq - seq_prev < -32000){
	    fprintf(stderr, "spc_sanity: %s: Error on line %d\n", argv[1], line);
	    exit(1);
	}
	seq_prev = seq;
	line++;
    }
    exit(0);
}
