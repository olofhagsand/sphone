/*-----------------------------------------------------------------------------
  File:   phone.c
  Description: Main Sicsophone application
  Author: Olof Hagsand
  CVS Version: $Id: phone.c,v 1.18 2005/01/28 11:06:26 olof Exp $
 
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
#include <fcntl.h>

#include "sphone.h"

static void usage(char *argv0)
{
    fprintf(stderr, "usage:%s \n"
	    "\t[-h]\t\tPrint this text\n"
	    "\t[-v]\t\tPrint version\n"
	    "\t[-f filename]\tEmulated audio record filename\n"
	    "\t[-F filename]\tEmulated audio play filename\n"
	    "\t[-C 0-4]\tAudio encoding type 0:L16, 1:ulaw, 2:alaw, 3:L8, 4:iLBC\n\t\t\t(default: L16)\n"
	    "\t[-c 0-4]\tNetw encoding type 0:L16, 1:ulaw, 2:alaw, 3:L8, 4:iLBC\n\t\t\t(default: ulaw)\n" 
	    "\t[-S samples]\tSamples per second (default:%d)\n"
	    "\t[-B bits]\tBits per sample(default:%d)\n"
	    "\t[-r port]\tRTP receive port (default: %d)\n"
	    "\t[-p]\t\tTurn on RTCP\n"
	    "\t[-R port]\tRTCP receive port (default: %d)\n"
	    "\t[-T port]\tRTCP sender port (default: %d)\n"
	    "\t[-d addr]\tDestination address\n"
	    "\t[-s size]\tPacket size in ms \n\t\t\t(must match with -S and -B, default:%d)\n" 
	    "\t[-a 0-2]\tAudio module 0:file, 1:directsound, 2:devaudio\n"
	    "\t\t\t(default: %d)\n" 
	    "\t[-D mask]\tDebug mask (1:rcv, 2:send, 4:event, 8:inet\n"
            "\t\t\t10:rtp, 20:rtcp, 40:coding, 80:play, 100:record,"
	    "\n\t\t\t200:malloc, 400: exit, 800:detailed)\n",
	    argv0,
	    SPHONE_SAMPLES,
	    SPHONE_BITS,
	    SPHONE_RTP_PORT,
	    SPHONE_RTP_PORT+1,
	    SPHONE_RTP_PORT+1,
	    SPHONE_SIZE_MS,
	    SPHONE_DEFAULT_AUDIO
	); 
    
    exit(0);
}

int
main(int argc, char *argv[])
{
    int c;
    play_api *pa; 
    record_api *ra; 
    char record_filename[64];        
    char play_filename[64];        
    char dst_hostname[64];
    rcv_session_t *rs;
    send_session_t *ss;
    coding_api *ca_r; 
    coding_api *ca_s;
    int use_rtcp = 0;
    uint16_t rtcp_rport, rtcp_sport, rtp_port;
    int rtcp_rport_given = 0;
    int rtcp_sport_given = 0;
    enum audio_type audio_module;
    struct rtp_session *rtp_s;
    struct rtp_session *rtp_r;
    enum playout_type playout;
    coding_params *cp_audio_s; /* coding of audio sender */
    coding_params *cp_audio_r; /* coding of audio receiver */
    coding_params *cp_nw_s; /* coding on network */
    coding_params *cp_nw_r; /* coding on network */

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
    record_filename[0] = '\0';
    play_filename[0] = '\0';
    if ((cp_audio_r = coding_param_init()) == NULL)
	exit(0);
    if ((cp_audio_s = coding_param_init()) == NULL)
	exit(0);
    cp_audio_r->cp_type       = CODING_LINEAR16;
    cp_audio_r->cp_size_ms    = SPHONE_SIZE_MS;
    cp_audio_r->cp_samp_sec   = SPHONE_SAMPLES;
    cp_audio_r->cp_precision  = SPHONE_BITS;
    cp_audio_r->cp_channels   = 1;

    if ((cp_nw_r = coding_param_init()) == NULL)
	exit(0);
    if ((cp_nw_s = coding_param_init()) == NULL)
	exit(0);
    cp_nw_r->cp_type          = CODING_ULAW;
    audio_module              = SPHONE_DEFAULT_AUDIO;
    rtp_port                  = SPHONE_RTP_PORT;
    strcpy(dst_hostname, "127.0.0.1");

    /*
     * Command line args
     */
    while ((c = getopt(argc, argv, "hvf:F:C:c:S:B:d:r:pR:T:s:a:D:")) != -1){
	switch (c) {
	case 'h' : /* help */
	    usage(argv[0]);
	    break;
	case 'v' : /* version */
	    fprintf(stdout, "Sicsophone version %s\n", SPHONE_VERSION);
	    exit(0);
	    break;
	case 'f' : /* record filename */
	    if (!optarg || sscanf(optarg, "%s", record_filename) != 1)
		usage(argv[0]);
	    break;
	case 'F' : /* play filename */
	    if (!optarg || sscanf(optarg, "%s", play_filename) != 1)
		usage(argv[0]);
	    break;
	case 'C': /* Audio coding */
	    if (!optarg || sscanf(optarg, "%u", (int*)&cp_audio_r->cp_type) != 1)
		usage(argv[0]);
	    break;
	case 'c': /* coding on network */
	    if (!optarg || sscanf(optarg, "%u", (int*)&cp_nw_r->cp_type) != 1)
		usage(argv[0]);
	    break;
	case 'S': /* samples per second */
	    if (!optarg || sscanf(optarg, "%lu", 
			     (long unsigned*)&cp_audio_r->cp_samp_sec) != 1)
		usage(argv[0]);
	    break;
	case 'B': /* bits per sample */
	    if (!optarg || sscanf(optarg, "%lu", 
			   (long unsigned*)&cp_audio_r->cp_precision) != 1)
		usage(argv[0]);
	    break;
	case 'd': /* destination */
	    strcpy(dst_hostname, optarg);
	    break;
	case 'r': /* receive port */
	    if (!optarg || sscanf(optarg, "%hu", &rtp_port) != 1)
		usage(argv[0]);
	    break;
	case 'p':
	    use_rtcp = 1;
	    break;
	case 'R': /* receiver (own) RTCP port */
	    if (!optarg || sscanf(optarg, "%hu", &rtcp_rport) != 1)
		usage(argv[0]);
	    rtcp_rport_given = 1;
	    break;
	case 'T': /* Sender's (other) RTCP port */
	    if (!optarg || sscanf(optarg, "%hu", &rtcp_sport) != 1)
		usage(argv[0]);
	    rtcp_sport_given = 1;
	    break;
	case 's': /* packet size in ms */
	    if (!optarg || sscanf(optarg, "%lu", 
			      (long unsigned*)&cp_audio_r->cp_size_ms) != 1)
		usage(argv[0]);
	    break;
	case 'a': /* audio module */
	    if (!optarg || sscanf(optarg, "%x", (int*)&audio_module) != 1)
		usage(argv[0]);
	    break;
	case 'D': /* debugging */
	    if (!optarg || sscanf(optarg, "%x", (int*)&debug) != 1)
		usage(argv[0]);
	    break;
	default:
	    usage(argv[0]);
	    break;
	} /* switch */
    } /* while */
    
    if (audio_module == AUDIO_FILE && (strlen(record_filename) == 0) && (strlen(play_filename) == 0)){
	sphone_error("-a 0 requires -f <filename> -F <filename> to be specified");
	exit(0);
    }
#ifdef HAVE_SYS_AUDIOIO_H
    if (audio_module == AUDIO_DEVAUDIO && (strlen(play_filename) == 0))
	strcpy(play_filename, DEVAUDIO_DEVICE);

    if (audio_module == AUDIO_DEVAUDIO && (strlen(record_filename) == 0))
	strcpy(record_filename, DEVAUDIO_DEVICE);
#endif /* HAVE_SYS_AUDIOIO_H */

#ifdef HAVE_ILBC
    if (cp_nw_r->cp_type == CODING_iLBC && cp_audio_r->cp_size_ms != 30){
	/* Force to 30ms - this is all the iLBC coder supports */
	fprintf(stderr, "iLBC coding: forcing to 30ms packets\n");
	cp_audio_r->cp_size_ms = 30;
    }
#endif /* HAVE_ILBC */
    /* (Uncoded) Stream Settings */
    cp_audio_r->cp_size_bytes = ms2bytes(cp_audio_r->cp_samp_sec, 
				       cp_audio_r->cp_precision,
				       cp_audio_r->cp_channels,
				       cp_audio_r->cp_size_ms);
    *cp_audio_s = *cp_audio_r;
    *cp_nw_s = *cp_nw_r;

    /* 
     * Init audio coding 
     */
    if ((ca_r = coding_init(cp_audio_r, cp_nw_r)) == NULL)
	exit(0);
    if ((ca_s = coding_init(cp_audio_s, cp_nw_s)) == NULL)
	exit(0);

    /* 
     * Init rtp/rtcp, init network: create rtp socket rtcp sockets
     */
    if ((rtp_r = rtp_session_new()) == NULL)
	exit(0);
    if ((rtp_s = rtp_session_new()) == NULL)
	exit(0);
    if (rtp_init(NULL) < 0)
	exit(0);
    rtp_r->rtp_port = rtp_port;
    rtp_r->rtcp_rport = rtcp_rport_given ? rtcp_rport : rtp_port + 1;
    rtp_r->rtcp_sport = rtcp_sport_given ? rtcp_sport : rtp_port + 1;
    rtp_r->rtp_payload = coding2rtp(cp_nw_r->cp_type);
    if (rtp_rcv_session_init(rtp_r) < 0)
	exit(0);
    memcpy(rtp_s, rtp_r, sizeof(*rtp_r));
    rtp_r->rtp_payload = coding2rtp(cp_nw_s->cp_type);
    if (rtp_send_session_init(rtp_s, dst_hostname) < 0)
	exit(0);

    /*
     * Init receive struct
     */
    if ((rs = rcv_session_new()) == NULL)
	exit(0);
    rs->rs_cp_audio = cp_audio_r;
    rs->rs_cp_nw = cp_nw_r;
    rs->rs_rtp = rtp_r;
    if ((rs->rs_rtcp = rtcp_rcv_stats_new()) == NULL)
	exit(0);
    rs->rs_coding_api = ca_r;

    /*
     * Init sender struct
     */
    if ((ss = send_session_new()) == NULL)
	exit(0);
    ss->ss_silence_detection = 0;  /* does not seem to work */
    ss->ss_cp_audio = cp_audio_s;
    ss->ss_cp_nw = cp_nw_s;
    ss->ss_rtp = rtp_s;
    if ((ss->ss_rtcp = rtcp_send_stats_new()) == NULL)
	exit(0);
    ss->ss_coding_api = ca_s;

    /* 
     * Init Audio - Play
     */
    if ((pa = audio_play_init(audio_module, rs)) == NULL)
	exit(0);
    if ((*pa->pa_settings)(pa, cp_audio_r) < 0)
	exit(0);
    if ((*pa->pa_open)(pa, play_filename) < 0)
	exit(0);
    rs->rs_play_api = pa;

    /* 
     * Init Audio - Record
     */
    if ((ra = audio_record_init(audio_module, ss)) == NULL)
	exit(0);
    if ((*ra->ra_settings)(ra, cp_audio_s) < 0)
	exit(0);
    ss->ss_record_api = ra;
    if (audio_module == AUDIO_FILE)
	if ((*ra->ra_ioctl)(ra, SPHONE_AUDIO_IOCTL_LOOP, (void*)1) < 0)
	    exit(0);

    if ((*ra->ra_open)(ra, record_filename) < 0)
	exit(0);

    /* 
     * Init Playout
     */
    if ((*pa->pa_ioctl)(pa, SPHONE_AUDIO_IOCTL_PLAYOUT_GET, &playout) < 0)
	exit(0);
    if (playout_init(playout, rs) < 0)
	exit(0);

    /* 
     * Init RTCP event handling 
     * XXX: send/rcv
     */
    if (use_rtcp && rtcp_rcv_init(rtp_r->rtcp_s, rs) < 0)
	exit(0);
    if (use_rtcp && rtcp_send_init(rtp_s->rtcp_s, ss) < 0)     
	exit(0);

    /* Start receiving: register rtp socket and dispatch to audio play module */
    if ((*pa->pa_start)(pa) < 0)
	exit(0);

    /* 
     * Register send info (after netw initialized) 
     * and start recording.
     */
    if ((*ra->ra_callback)(ra, record_and_send, ss) < 0)
	exit(0);

    if ((*ra->ra_start)(ra) < 0)
	exit(0);

    if (rcv_start(rs) < 0)
	exit(0);

    eventloop();
    dbg_print(DBG_EXIT, "Normal exit\n");
    return 0;
}
