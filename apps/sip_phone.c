/*-----------------------------------------------------------------------------
  File:   sip_phone.c
  Description: Application using sip signaling 
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
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "sphone.h"
#ifdef HAVE_SIP
#include "sip.h"

int 
sip_rcv(int s, void *arg)
{
    struct sip_client *sc = (struct sip_client *)arg;
    size_t len = SIP_MSG_LEN;
    struct sip_msg *sm;
    struct sockaddr_in from_addr;

    if ((sm = sip_msg_new()) == NULL)
	exit(0);
    if (inet_input(s, sm->sm_msg, &len, &from_addr) < 0)
	return 0;
    if (memcmp(&sc->sc_server_addr, &from_addr, sizeof(struct sockaddr_in))){
	fprintf(stderr, "sip_rcv: Not expected input from: %s\n",
		inet_ntoa(from_addr.sin_addr));
	return -1;
    }
    if (sip_msg_parse(sm) < 0)
	return -1;
    /* Now we have the message -> analyze its contents */
    /* XXX */
    return 0;
}


static void 
usage(char *argv0)
{
    fprintf(stderr, "usage:%s \n"
	    "\t[-h]\t\tPrint this text\n"
	    "\t[-v]\t\tPrint version\n"
	    "\t[-D mask]\tDebug mask (1:rcv, 2:send, 4:event, 8:inet\n"
            "\t\t\t10:rtp, 20:rtcp, 40:coding, 80:play, 100:record,"
	    "\n\t\t\t200:malloc, 400: exit, 800:detailed)\n"
	    "\t[-a <ipaddr>]\tMy address\n"
	    "\t[-s <ipaddr>]\tServer address\n",
	    argv0
	); 
    exit(0);
}

int
main(int argc, char *argv[])
{
    int c;
    int ret;
    struct sip_dialog *sd;
    struct sip_transaction *st;
    struct sip_client *sc;
    struct sip_msg *sm;
    char sdp_data[256];
    struct in_addr my_addr;
    struct in_addr server_addr;

    /* 
     * Handle control-C 
     */
    set_signal(SIGINT, sphone_signal_exit);
    set_signal(SIGTERM, sphone_signal_exit);

    dbg_init(stderr);

    /*
     * Defaults and inits
     */
    memset(&my_addr, 0, sizeof(struct in_addr));

    /*
     * Command line args
     */
    while ((c = getopt(argc, argv, "hvD:a:s:")) != -1){
	switch (c) {
	case 'h' : /* help */
	    usage(argv[0]);
	    break;
	case 'v' : /* version */
	    fprintf(stdout, "Sicsophone version %s\n", SPHONE_VERSION);
	    exit(0);
	    break;
	case 'D': /* debugging */
	    if (!optarg || sscanf(optarg, "%x", (int*)&debug) != 1)
		usage(argv[0]);
	    dbg_sip = 1;
	    break;
	case 'a': /* my address */
	    my_addr.s_addr = inet_addr(optarg);
	    break;
	case 's': /* server address */
	    server_addr.s_addr = inet_addr(optarg);
	    break;
	default:
	    usage(argv[0]);
	    break;
	} /* switch */
    } /* while */
    if (my_addr.s_addr == 0){
	ret = inet_get_default_addr(&my_addr);
	if (ret < 0)
	    exit(0);
	if (ret == 0){
	    fprintf(stderr, "No default address found: try -a option\n");
	    exit(0);
	}
    }
    
    /* XXX: hardcoded: should be softphone not cisco */
    strncpy(sdp_data, "v=0\n"
	   "o=Cisco-SIPUA 15829 11606 IN IP4 192.36.125.167\n"
	   "s=SIP Call\n"
	   "c=IN IP4 192.36.125.167\n"
	   "t=0 0\n"
	   "m=audio 28798 RTP/AVP 8 0 18 101\n"
	   "a=rtpmap:8 PCMA/8000\n"
	   "a=rtpmap:0 PCMU/8000\n"
	    "a=fmtp:101 0-15\n",
	    256);
    
    if ((sc = sip_client_create(my_addr, 
				SIP_PORT, 
				server_addr, 
				SIP_PORT, 
				"6534",
				"sip:6534@kth.se")) == NULL)
	exit(0);

    /* Bind sip rcv port */
    if ((sc->sc_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	sphone_error("inet_init: socket: %s\n", strerror(errno)); 
	return -1;
    }
    if (inet_bind(sc->sc_s, htons(SIP_PORT)) < 0)
	return -1;
    if (eventloop_reg_fd(sc->sc_s, 
			 sip_rcv, 
			 (void*)sc,
			 "sip receive") < 0)
	return -1;
    if ((sd = sip_dialog_create(sc, "sip:000730631661@kth.se")) == NULL)
	exit(0);
    if ((st = sip_transaction_create(sd)) == NULL)
	exit(0);
    if ((sm = sip_msg_create(st, SM_INVITE)) == NULL)
	exit(0);
    sm->sm_body = strdup(sdp_data);
    if (sip_msg_invite_construct(sm, st, sd, sc) < 0)
	exit(0);
    if (sip_msg_pack(sm) < 0)
	exit(0);
    if (debug){
	printf("sip_msg:\n");
	printf(sm->sm_msg);
	printf("---\n");
    }
    if (sendto(sc->sc_s, sm->sm_msg, strlen(sm->sm_msg)+1, 0x0, 
	       (struct sockaddr *)&sc->sc_server_addr, 
	       sizeof(sc->sc_server_addr)) < 0){
	sphone_error(" sendto: %s\n", strerror(errno));
	return -1;
    }
    eventloop();
    dbg_print(DBG_EXIT, "Normal exit\n");

    sip_client_free(sc);
    return 0;
}
#else /* HAVE_SIP */
int
main()
{
    return 0;
}
#endif /* HAVE_SIP */
