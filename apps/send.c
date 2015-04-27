/*-----------------------------------------------------------------------------
  File:   send.c
  Description: Main Sicsophone sender application
  Author: Olof Hagsand
  CVS Version: $Id: send.c,v 1.17 2005/01/28 11:06:27 olof Exp $
 
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
#include <signal.h>

#include "sphone.h"

/*
 * Ultimate timeout handler - quit when this fires
 */
static int 
duration_done(int arg1, void* arg2)
{
    sphone_error("Duration done"); /* Actually not an error */
    return -1;
}

static void usage(char *argv0)
{
    fprintf(stderr, "usage:%s \n"
	    "\t[-h]\t\tPrint this text\n"
	    "\t[-v]\t\tPrint version\n"
	    "\t[-f filename]\tEmulated audio filename\n"
	    "\t[-C 0-4]\tAudio encoding type 0:L16, 1:ulaw, 2:alaw, 3:L8, 4:iLBC\n\t\t\t(default: L16)\n"
	    "\t[-c 0-4]\tNetw encoding type 0:L16, 1:ulaw, 2:alaw, 3:L8, 4:iLBC\n\t\t\t(default: ulaw)\n" 
	    "\t[-S samples]\tSamples per second (default:%d)\n"
	    "\t[-B bits]\tBits per sample(default:%d)\n"
	    "\t[-r port]\tRTP receive port (default: %d)\n"
	    "\t[-t port]\tRTP sender port (default: 0)\n"
	    "\t[-p]   \t\tTurn on RTCP\n"
	    "\t[-R port]\tRTCP receive port (default: %d)\n"
	    "\t[-T port]\tRTCP sender (own) port (default: %d)\n"
	    "\t[-d addr]\tDestination address\n"
	    "\t[-s size]\tPacket size in ms \n\t\t\t(must match with -S and -B, default:%d)\n" 
	    "\t[-u duration]\tSession duration in seconds (default: unlimited:0)\n"
	    "\t[-a 0-2]\tAudio module 0:file, 1:directsound, 2:devaudio\n"
	    "\t\t\t(default: %d)\n" 
	    "\t[-D mask]\tDebug mask (1:rcv, 2:send, 4:event, 8:inet\n"
            "\t\t\t10:rtp, 20:rtcp, 40:coding, 80:play, 100:record,"
	    "\n\t\t\t200:malloc, 400: exit, 800:detailed)\n",
	    argv0,
	    SPHONE_SAMPLES,
	    SPHONE_BITS,
	    SPHONE_RTP_PORT,
	    SPHONE_RTP_PORT + 1,
	    SPHONE_RTP_PORT + 1,
	    SPHONE_SIZE_MS,
	    SPHONE_DEFAULT_AUDIO
	); 
    exit(0);
}

int
main(int argc, char *argv[])
{
    int c;
    char dst_hostname[64];
    record_api *ra; 
    char filename[64];        
    send_session_t *ss;
    coding_api *ca;
    int use_rtcp = 0;
    uint16_t rtcp_rport, rtcp_sport, rtp_port, rtp_sport;
    int rtcp_rport_given = 0;
    int rtcp_sport_given = 0;
    enum audio_type audio_module;
    struct rtp_session *rtp;
    coding_params *cp_audio; /* coding of audio */
    coding_params *cp_nw; /* coding on network */
    struct timeval now; 
    struct timeval duration; 

    /* 
     * Handle control-C 
     */
    set_signal(SIGINT, sphone_signal_exit);
    set_signal(SIGTERM, sphone_signal_exit);

    dbg_init(stderr);
    event_init(NULL);

    /*
     * Defaults and inits
     */
    filename[0] = '\0';
    
    if ((cp_audio = coding_param_init()) == NULL)
	exit(0);
    cp_audio->cp_type       = CODING_LINEAR16;
    cp_audio->cp_size_ms    = SPHONE_SIZE_MS;
    cp_audio->cp_samp_sec   = SPHONE_SAMPLES;
    cp_audio->cp_precision  = SPHONE_BITS;
    cp_audio->cp_channels   = 1;

    if ((cp_nw = coding_param_init()) == NULL)
	exit(0);
    cp_nw->cp_type          = CODING_ULAW;
    audio_module            = SPHONE_DEFAULT_AUDIO;
    rtp_port                = SPHONE_RTP_PORT;
    rtp_sport               = 0;
    strcpy(dst_hostname, "127.0.0.1");
    duration.tv_usec = duration.tv_sec = 0; /* indefinite */

    /*
     * Command line args
     */
    while ((c = getopt(argc, argv, "hvf:C:c:S:B:d:r:t:pR:T:s:a:f:u:D:")) != -1){
	switch (c) {
	case 'h' : /* help */
	    usage(argv[0]);
	    break;
	case 'v' : /* version */
	    fprintf(stdout, "Sicsophone version %s\n", SPHONE_VERSION);
	    exit(0);
	    break;
	case 'f' : /* filename */
	    if (!optarg || (sscanf(optarg, "%s", (char*)filename) != 1))
		usage(argv[0]);
	    break;
	case 'C': /* Audio coding */
	    if (!optarg || sscanf(optarg, "%u", (int*)&cp_audio->cp_type) != 1)
		usage(argv[0]);
	    break;
	case 'c': /* coding on network */
	    if (!optarg || sscanf(optarg, "%u", (int*)&cp_nw->cp_type) != 1)
		usage(argv[0]);
	    break;
	case 'S': /* samples per second */
	    if (!optarg || sscanf(optarg, "%lu", 
				  (long unsigned*)&cp_audio->cp_samp_sec) != 1)
		usage(argv[0]);
	    break;
	case 'B': /* bits per sample */
	    if (!optarg || sscanf(optarg, "%lu", 
			      (long unsigned*)&cp_audio->cp_precision) != 1)
		usage(argv[0]);
	    break;
	case 'd': /* destination */
	    strcpy(dst_hostname, optarg);
	    break;
	case 'r': /* receive (dst) port */
	    if (!optarg || sscanf(optarg, "%hu", &rtp_port) != 1)
		usage(argv[0]);
	    break;
	case 't': /* sender (src) rtp port (NAPT) */
	    if (!optarg || sscanf(optarg, "%hu", &rtp_sport) != 1)
		usage(argv[0]);
	    break;
	case 'p':
	    use_rtcp = 1;
	    break;
	case 'R': /* receiver (dst) RTCP port */
	    if (!optarg || sscanf(optarg, "%hu", &rtcp_rport) != 1)
		usage(argv[0]);
	    rtcp_rport_given = 1;
	    break;
	case 'T': /* Sender's (own) RTCP port */
	    if (!optarg || sscanf(optarg, "%hu", &rtcp_sport) != 1)
		usage(argv[0]);
	    rtcp_sport_given = 1;
	    break;
	case 's': /* packet size in ms */
	    if (!optarg || sscanf(optarg, "%lu", 
				  (long unsigned*)&cp_audio->cp_size_ms) != 1)
		usage(argv[0]);
	    break;
	case 'a': /* audio module */
	    if (!optarg || sscanf(optarg, "%x", (int*)&audio_module) != 1)
		usage(argv[0]);
	    break;
	case 'u':{ /* dUration in seconds */
	    char dummy;
	    if (!optarg || sscanf(optarg, "%lu%c%lu", 
				  &duration.tv_sec, &dummy,
				  &duration.tv_usec) < 1)
		usage(argv[0]);
	    break;
	  }
	case 'D': /* debugging */
	    if (!optarg || sscanf(optarg, "%x", (int*)&debug) != 1)
		usage(argv[0]);
	    break;
	default:
	    usage(argv[0]);
	    break;
	} /* switch */
    } /* while */

    if (audio_module == AUDIO_FILE && (strlen(filename) == 0)){
	sphone_error("-a 0 requires -f <filename> to be specified");
	exit(0);
    }
#ifdef HAVE_SYS_AUDIOIO_H
    if (audio_module == AUDIO_DEVAUDIO && (strlen(filename) == 0))
	strcpy(filename, DEVAUDIO_DEVICE);
#endif /* HAVE_SYS_AUDIOIO_H */
#ifdef HAVE_ILBC
    if (cp_nw->cp_type == CODING_iLBC && cp_audio->cp_size_ms != 30){
	/* Force to 30ms - this is all the iLBC coder supports */
	fprintf(stderr, "iLBC coding: forcing to 30ms packets\n");
	cp_audio->cp_size_ms = 30;
    }
#endif /* HAVE_ILBC */

    /* (Uncoded) Stream Settings */
    cp_audio->cp_size_bytes = ms2bytes(cp_audio->cp_samp_sec, 
				       cp_audio->cp_precision, 
				       cp_audio->cp_channels,
				       cp_audio->cp_size_ms);
    /* 
     * Init audio coding 
     */
    if ((ca = coding_init(cp_audio, cp_nw)) == NULL)
	exit(0);

    /* 
     * Init rtp/rtcp, init network: create rtp and rtcp sockets
     */
    if ((rtp = rtp_session_new()) == NULL)
	exit(0);
    if (rtp_init(&rtp->rtp_ssrc) < 0)
	exit(0);

    rtp->rtp_port = rtp_port;
    rtp->rtp_sport = rtp_sport;
    rtp->rtcp_rport = rtcp_rport_given ? rtcp_rport : rtp_port + 1;
    rtp->rtcp_sport = rtcp_sport_given ? rtcp_sport : rtp_port + 1;
    rtp->rtp_payload = coding2rtp(cp_nw->cp_type);
    if (rtp_send_session_init(rtp, dst_hostname) < 0)
	exit(0);

    /*
     * Init sender struct
     */
    if ((ss = send_session_new()) == NULL)
	exit(0);
    ss->ss_cp_audio = cp_audio;
    ss->ss_cp_nw = cp_nw;
    ss->ss_rtp = rtp;
    if ((ss->ss_rtcp = rtcp_send_stats_new()) == NULL)
	exit(0);
    ss->ss_coding_api = ca;

    /* 
     * Init Audio 
     */
    if ((ra = audio_record_init(audio_module, ss)) == NULL)
	exit(0);

    if ((*ra->ra_settings)(ra, cp_audio) < 0)
	exit(0);

    if (audio_module == AUDIO_FILE)
	if ((*ra->ra_ioctl)(ra, SPHONE_AUDIO_IOCTL_LOOP, (void*)0) < 0)
	    exit(0);

    if ((*ra->ra_open)(ra, filename) < 0)
	exit(0);

    /* Init RTCP event handling */
    if (use_rtcp && rtcp_send_init(rtp->rtcp_s, ss) < 0)     
	exit(0);

    /* 
     * Register send info (after netw initialized) 
     * and start recording.
     */
    if ((*ra->ra_callback)(ra, record_and_send, ss) < 0)
	exit(0);

    /*
     * If duration parameter: quit hard after timeperiod.
     */
    if (duration.tv_sec!=0 || duration.tv_usec!=0){
	now=gettimestamp();
	timeradd(&now, &duration, &now);
	if (eventloop_reg_timeout(now,  duration_done,
				  NULL, "duration") < 0)
	    exit(0);
    }

    /* 
     * Start system
     */
    if ((*ra->ra_start)(ra) < 0)
	exit(0);
    eventloop();
    dbg_print(DBG_EXIT, "Normal exit\n");
    return 0;
}
