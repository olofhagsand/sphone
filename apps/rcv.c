/*-----------------------------------------------------------------------------
  File:   rcv.c
  Description: Main Sicsophone receiver application
  Author: Olof Hagsand
 
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

static int
napt_probing(struct rtp_session *rtp, char *src_hostname)
{
    char buf[32];
    struct sockaddr_in addr;

    if (inet_host2addr(src_hostname, htons(rtp->rtp_sport), &addr) < 0)
	return -1;
    if (sendto(rtp->rtp_s, buf, sizeof(buf), 0x0,
	       (struct sockaddr *)&addr,
	       sizeof(struct sockaddr_in))< 0){
	sphone_error("napt_probing: sendto: %s\n", strerror(errno));
    }
    return 0;
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
	    "\t[-p]\t\tTurn on RTCP\n"
	    "\t[-R port]\tRTCP receive port (default: %d)\n"
	    "\t[-T port]\tRTCP sender port (default: %d)\n"
	    "\t[-s size]\tPacket size in ms \n\t\t\t(must match with -S and -B, default:%d)\n" 
	    "\t[-u duration]\tSession duration in seconds (default: unlimited:0)\n"
	    "\t[-a 0-2]\tAudio module 0:file, 1:directsound, 2:devaudio\n"
	    "\t\t\t(default: %d)\n" 
	    "\t[-L logfile]\tLog filename\n"
	    "\t[-x addr]\tSenders address (for NAPT probing)\n"
	    "\t[-X port]\tSenders port (for NAPT probing)\n"
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
    char filename[64];        
    char src_hostname[64];        
    char logfilename[64];        
    rcv_session_t *rs;
    coding_api *ca;
    int use_rtcp = 0;
    uint16_t rtcp_rport, rtcp_sport, rtp_port, rtp_sport;
    int rtcp_rport_given = 0;
    int rtcp_sport_given = 0;
    enum audio_type audio_module;
    struct rtp_session *rtp;
    enum playout_type playout;
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
    logfilename[0] = '\0';
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
    *src_hostname            = '\0';
    duration.tv_usec = duration.tv_sec = 0; /* indefinite */

    /*
     * Command line args
     */
    while ((c = getopt(argc, argv, "hvf:C:c:S:B:r:pR:T:X:x:s:a:L:u:D:")) != -1){
	switch (c) {
	case 'h' : /* help */
	    usage(argv[0]);
	    break;
	case 'v' : /* version */
	    fprintf(stdout, "Sicsophone version %s\n", SPHONE_VERSION);
	    exit(0);
	    break;
	case 'f' : /* filename */
	    if (!optarg || sscanf(optarg, "%s", filename) != 1)
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
	    if (!optarg || 
		sscanf(optarg, "%lu", 
		       (long unsigned*)&cp_audio->cp_samp_sec) != 1)
		usage(argv[0]);
	    break;
	case 'B': /* bits per sample */
	    if (!optarg || sscanf(optarg, "%lu",
				  (long unsigned*)&cp_audio->cp_precision) != 1)
		usage(argv[0]);
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
	case 'x': /* Senders address (for NAPT probing) */
	    strcpy(src_hostname, optarg);
	    break;
	case 'X': /* Senders port (for NAPT probing) */
	    if (!optarg || sscanf(optarg, "%hu", &rtp_sport) != 1)
		usage(argv[0]);
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
	case 'L': /* log file name */
	    if (!optarg || sscanf(optarg, "%s", logfilename) != 1)
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
     * Init rtp/rtcp, init network: create rtp socket rtcp sockets
     */
    if ((rtp = rtp_session_new()) == NULL)
	exit(0);
    if (rtp_init(NULL) < 0)
	exit(0);
    rtp->rtp_port = rtp_port;
    rtp->rtp_sport = rtp_sport;
    rtp->rtcp_rport = rtcp_rport_given ? rtcp_rport : rtp_port + 1;
    rtp->rtcp_sport = rtcp_sport_given ? rtcp_sport : rtp_port + 1;
    rtp->rtp_payload = coding2rtp(cp_nw->cp_type);
    if (rtp_rcv_session_init(rtp) < 0)
	exit(0);

    /*
     * Init receive struct
     */
    if ((rs = rcv_session_new()) == NULL)
	exit(0);
    rs->rs_cp_audio = cp_audio;
    rs->rs_cp_nw = cp_nw;
    rs->rs_rtp = rtp;
    if ((rs->rs_rtcp = rtcp_rcv_stats_new()) == NULL)
	exit(0);
    rs->rs_coding_api = ca;
    if (strlen(logfilename)){
	if ((rs->rs_logf = fopen(logfilename, "w")) == NULL){
	    perror("fopen");
	    exit(0);
	}
	now = gettimestamp();
	fprintf(rs->rs_logf, "I %ld %06ld\n", now.tv_sec, now.tv_usec);
    }

    /* 
     * Init Audio 
     */
    if ((pa = audio_play_init(audio_module, rs)) == NULL)
	exit(0);
    if ((*pa->pa_settings)(pa, cp_audio) < 0)
	exit(0);
    if ((*pa->pa_open)(pa, filename) < 0)
	exit(0);
    rs->rs_play_api = pa;

    /* 
     * Init Playout
     */
    if ((*pa->pa_ioctl)(pa, SPHONE_AUDIO_IOCTL_PLAYOUT_GET, &playout) < 0)
	exit(0);
    if (playout_init(playout, rs) < 0)
	exit(0);

    /* Init RTCP event handling */
    if (use_rtcp && rtcp_rcv_init(rtp->rtcp_s, rs) < 0)
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
     * Start receiving: 
     * register rtp socket and dispatch to audio play module 
     */
    if ((*pa->pa_start)(pa) < 0)
	exit(0);

    if (rcv_start(rs) < 0)
	exit(0);

    if (strlen(src_hostname) && rtp_sport)
	if (napt_probing(rtp, src_hostname) < 0)
	    exit(0);
    eventloop();
    dbg_print(DBG_EXIT, "Normal exit\n");
    return 0;
}
