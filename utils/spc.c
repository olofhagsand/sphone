/*-----------------------------------------------------------------------------
  File:   spc.c
  Description: Sicsophone client application: spawn sphone senders & rcvers
  Author: Olof Hagsand
  CVS Version: $Id: spc.c,v 1.60 2005/01/31 18:26:19 olof Exp $
 
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
/*
  State machine for sicsophone protocol client:

            ----------------> SS_INIT <--------------- 
	  /                      ^                     \
         /     	                 |                      \
	/		         v                       \
       /-------------------->  SS_SERVER <--------------- \
      /                          ^   ^                     \
     /                          /     \                     \
  SS_DATA_RCV<-----------------        --> SS_DATA_SEND--> SS_DATA_SEND_CLOSING

SS_INIT:           Looking for server
SS_SERVER:         Server found
SS_DATA_RCV:       Engaged in data rcv
SS_DATA_SEND:      Engaged in data send
SS_DATA_SEND_CLOSING: Stopped sending and waiting for all replies

 */
#ifdef HAVE_CONFIG_H
#include "config.h" /* generated by config & autoconf */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> /* open */
#include <math.h>
#include <signal.h>
#include <time.h>

#include "sphone.h" 
#include "sphone_lib.h" /* debug */
#include "sphone_error.h" /* exit */
#include "sphone_eventloop.h"

#ifdef WIN32
#define WIN32_WINMAIN /* No console: use real windows */

#include <windows.h>
#include <process.h>
#include <mmsystem.h>

#include "sphone_win32.h"
#else /* WIN32 */
#include <netdb.h>
#endif /* WIN32 */

#include "sp.h"
#include "spc.h"
#include "spc_rcv.h"
#include "spc_send.h"



/* forw decl */
static int spc_exit(void *arg);

char*
state2str(enum spc_state state)
{
    switch (state){
    case SS_INIT: return "INIT";
	break;
    case SS_SERVER: return "SERVER";
	break;
    case SS_DATA_RCV: return "DATA_RCV";
	break;
    case SS_DATA_SEND: return "DATA_SEND";
	break;
    case SS_DATA_SEND_CLOSING: return "DATA_SEND_CLOSING";
	break;
    }
    return "<null>";
}

static int
udp_socket_input(int s, void *arg)
{
    struct spc_info *spc = (struct spc_info *)arg;
    int retval = 0;

    assert(s == spc->sp_ds);
    switch (spc->sp_state){
    case SS_DATA_RCV:
	retval = rcv_data_input(s, spc, spc->sp_data);
	break;
    case SS_DATA_SEND:
    case SS_DATA_SEND_CLOSING:
	retval = send_data_input(s, spc, spc->sp_data);
	break;
    case SS_SERVER:
    case SS_INIT:{
	static char buf[1500];
	struct sockaddr_in from;
	int len = 1500;

	fprintf(stderr, "Warning: udp_socket_input: illegal state: %s\n", 
		state2str(spc->sp_state));
	if (inet_input(s, buf, &len, &from) < 0) /* dummy read */
	    return 0;
	break;
    }
    }; /* switch */
    return retval;
}


static int
server_input(int s, void* arg)
{
    struct spc_info *spc = (struct spc_info *)arg;
    struct sp_proto sp;
    int len;

    assert(s == spc->sp_s);     /* Sanity */
    /* OK Got a message from a server. Now what? */
    if ((len = recv(spc->sp_s, (void*)&sp, sizeof(struct sp_proto), 0x0)) < 0){
	perror("server_input: recv");
	return 0;
    }
    if (len == 0){ /* closedown */
	spc_state2init(spc);
	return 0;
    }
    dbg_print_pkt(DBG_RCV|DBG_DETAIL, "server_input", (char*)&sp, len);
    if (sp.sp_magic != SP_MAGIC){     /* sanity */
	fprintf(stderr, "Warning: server_input: wrong magic cookie: %x\n", 
		sp.sp_magic);
	return 0;
    }
    if (ntohs(sp.sp_version) != SP_VERSION_NR){
	fprintf(stderr, "Warning: server_input: wrong protocol version: %hu\n", 
		(int)ntohs(sp.sp_version));
	return 0;
    }
    if ((htonl(sp.sp_id) != spc->sp_id)){
	fprintf(stderr, "Warning: server_input: wrong client id: %d\n", 
		(int)ntohl(sp.sp_id));
	return 0;
    }
    switch(sp.sp_type){
    case SP_TYPE_START_SEND:
	if (spc->sp_state != SS_SERVER)
	    break;
	if (start_sender(spc, &sp)< 0)
	    return -1;
#if 0 /* dont think these are necessary */
	if (send_register(spc->sp_s, spc, &spc->sp_addr) < 0) 
	    return -1;
#endif
	break;
    case SP_TYPE_START_RCV:{
	if (spc->sp_state != SS_SERVER)
	    break;
	if (start_receiver(spc, &sp)< 0)
	    return -1;
#if 0
	if (send_register(spc->sp_s, spc, &spc->sp_addr) < 0) 
	    return -1;
#endif
	break;
    }
    case SP_TYPE_EXIT:
	spc_exit(spc);
	break;
    case SP_TYPE_REPORT_RCV: /* getting data */
    case SP_TYPE_REPORT_SEND: 
    case SP_TYPE_DEREGISTER:
    case SP_TYPE_REGISTER:
    default:
	fprintf(stderr, "sanity: wrong protocol type: %d\n", sp.sp_type);
	break;
    }
    return 0;
}

int
spc_state2init(struct spc_info *spc)
{
    if (eventloop_unreg(spc->sp_s, server_input, spc) < 0){
	sphone_error("spc_state2init unreg: client not found");
	return -1;
    }
    close(spc->sp_s);
    dbg_print(DBG_APP, "spc_state2init: closing socket %d\n", spc->sp_s);
    spc->sp_s = -1;
    if (spc->sp_data){
	sfree(spc->sp_data);
	spc->sp_data = NULL;
    }
    dbg_print(DBG_APP, "State: %s -> ", state2str(spc->sp_state));
    spc->sp_state = SS_INIT;
    dbg_print(DBG_APP, "%s\n", state2str(spc->sp_state));
    return 0;
}


/*
 * connect_server
 * Tries to connect to server
 * return values: -1: error, 0: did not succedd in connect, 1: OK 
 *
 */
static int
connect_server(struct spc_info *spc)
{
    if ((spc->sp_s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
#ifdef WINSOCK
	errno = WSAGetLastError();
	printf("socket: %d\n", errno);
#else /* WINSOCK */
	perror("connect_server: socket");
#endif /* WINSOCK */
	return -1;
    }
    if (connect(spc->sp_s, (struct sockaddr*)&spc->sp_addr, 
		sizeof(struct sockaddr_in)) < 0){
#ifdef WINSOCK
	errno = WSAGetLastError();
	if (errno == WSAECONNREFUSED || errno == WSAETIMEDOUT || 
	    errno == WSAENETUNREACH){
	    close(spc->sp_s);
	    spc->sp_s = -1;
	    dbg_print(DBG_APP, "spc: connect: failed to connect to server %s:%d keep trying\n",
		      inet_ntoa(spc->sp_addr.sin_addr), 
		      ntohs(spc->sp_addr.sin_port));
	    return 0; /* try again */	    
	}
	if (errno == WSAEADDRNOTAVAIL)
	  fprintf(stderr, "WSAEADDRNOTAVAIL\n");
	fprintf(stderr, "connect_server: connect: %d\n", errno);
#else /* WINSOCK */
	if (errno == ECONNREFUSED || errno == ETIMEDOUT || errno==ENETUNREACH){
	    /* maybe there are more "legal" reasons */
	    close(spc->sp_s);
	    spc->sp_s = -1;
	    dbg_print(DBG_APP, "spc: connect: failed to connect to server %s:%d keep trying\n",
		      inet_ntoa(spc->sp_addr.sin_addr), 
		      ntohs(spc->sp_addr.sin_port));	    
	    return 0; /* try again */	    
	}
	perror("connect_server: connect");
#endif /* WINSOCK */
	close(spc->sp_s);
	spc->sp_s = -1;
	return -1;
    }
    dbg_print(DBG_APP, "Connected to server %s:%d\n", 
	      inet_ntoa(spc->sp_addr.sin_addr), 
	      ntohs(spc->sp_addr.sin_port));

    return 1; /* 1: proceed */
}


int
send_register(int s, struct spc_info *spc, struct sockaddr_in *addr)
{
    struct sp_proto sp;

    memset(&sp, 0 , sizeof(sp));
    sp.sp_magic = SP_MAGIC;
    sp.sp_type = SP_TYPE_REGISTER;
    sp.sp_version = htons(SP_VERSION_NR);
    sp.sp_len   = htonl(sizeof(sp)); 
    sp.sp_id = htonl(spc->sp_id);
    if (spc->sp_state == SS_SERVER || spc->sp_state == SS_INIT)
	sp.sp_reghdr.sr_busy = 0;
    else
	sp.sp_reghdr.sr_busy = 1;
    dbg_print(DBG_SEND, "send_register to %s:%d\n",
	      inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    dbg_print_pkt(DBG_SEND|DBG_DETAIL, "send_register", (char*)&sp, sizeof(sp));
    if (sendto(s, (void*)&sp, sizeof(sp), 0x0, 
	       (struct sockaddr *)addr, 
	       sizeof(struct sockaddr_in)) < 0){
	perror("send_register: sendto");
	return -1;
    }

    return 0;
}

static int
udp_socket_init(struct spc_info *spc, uint32_t port)
{
    int one = 1;

    /* Make a test socket and send a datagram to the server */
    if ((spc->sp_ds = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("udp_socket_init: socket");
	return -1;
    }
#ifdef SO_REUSEPORT
    if (setsockopt(spc->sp_ds, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) < 0)
	perror("udp_socket_init: setsockopt(SO_REUSEPORT)");
#endif
    if (setsockopt(spc->sp_ds, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one)) < 0)
	perror("udp_socket_init: setsockopt(SO_REUSEADDR)");
    /* All input to udp_socket */
    memset(&spc->sp_mydaddr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
    spc->sp_mydaddr.sin_len    = sizeof(struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
    spc->sp_mydaddr.sin_family = AF_INET;
    spc->sp_mydaddr.sin_addr.s_addr = INADDR_ANY; 
    spc->sp_mydaddr.sin_port   = htons(port);

    if (bind(spc->sp_ds, 
	     (struct sockaddr *)&spc->sp_mydaddr, 
	     sizeof(spc->sp_mydaddr)) < 0){
	perror("udp_socket_init: bind");
	close(spc->sp_ds);
	spc->sp_ds = 0;
	return -1;
    }
    if (eventloop_reg_fd(spc->sp_ds, udp_socket_input, spc, 
			 "sicsophone data receiver") < 0)
	return -1;
    return 0;
}


static int 
sps_wrapper(int dummy, void *arg)
{
    struct timeval t, delta;
    struct spc_info *spc = (struct spc_info *)arg;
    int retval;

    if (spc->sp_state == SS_INIT){
	retval = connect_server(spc);
	switch (retval){
	case -1:
	    return -1;
	    break;
	case 0:
	    break;
	case 1:
	    dbg_print(DBG_APP, "State: %s -> ", state2str(spc->sp_state));
	    spc->sp_state = SS_SERVER;
	    dbg_print(DBG_APP, "%s\n", state2str(spc->sp_state));
	    assert(spc->sp_s != -1);
	    if (eventloop_reg_fd(spc->sp_s, server_input, 
				 spc, "sicsophone client input") < 0)
		return -1;
	    break;
	default:
	    fprintf(stderr, "sps_wrapper: state: SS_INIT: Illegal return value: %d\n",
		    retval);
	    return -1;
	    break;
	}
    }
    /* If state change from INIT->CONNECTED, we send an immediate register */
    if (spc->sp_state != SS_INIT){
	retval = send_register(spc->sp_s, spc, &spc->sp_addr);
	switch (retval){
	case -1:
	    return -1;
	    break;
	case 0:{
	    /* Also send one probe to server via udp port */
	    if (send_register(spc->sp_ds, spc, &spc->sp_addr) < 0)
		return -1;
	    break;
	}
	default:
	    fprintf(stderr, "sps_wrapper: state: %s: Illegal return value: %d\n", 
		    state2str(spc->sp_state), retval);
	    return -1;
	    break;
	}
	delta.tv_sec = SPC_SERVER_INTERVAL; 
    }
    else
	delta.tv_sec = SPC_INIT_INTERVAL; 
    delta.tv_usec = 0;
    t = gettimestamp();
    timeradd(&t, &delta, &t);
    if (eventloop_reg_timeout(t,  sps_wrapper, arg, 
			      "sicsophone client sps_wrapper") < 0)
	return -1;
    return 0;
}

static int
spc_exit(void *arg)
{
    struct spc_info *spc = (struct spc_info *)arg;

    dbg_print(DBG_APP, "spc_exit\n");
    /* Close and deallocate server socket */
    if (spc->sp_s != -1){
	eventloop_unreg(spc->sp_s, server_input, spc);
	eventloop_unreg(spc->sp_ds, udp_socket_input, spc);
	close(spc->sp_s);
	dbg_print(DBG_APP, "spc_exit: closing socket %d\n", spc->sp_s);
    }
    exit(0);
    return 0;
}

static int
sp_inet_init()
{
#ifdef WINSOCK
    int err;
    WSADATA wsaData; 
    WORD wVersionRequested;
      
#if (WINSOCK == 1)
    wVersionRequested = MAKEWORD( 1, 1 ); 
#elif (WINSOCK == 2)
    wVersionRequested = MAKEWORD( 2, 0 ); 
#endif
    err = WSAStartup( wVersionRequested, &wsaData ); 
    if (err != 0) {
	sphone_error("inet_init: WSAStartup: %d\n", err); 
	return -1;
    }
#endif /* WINSOCK */
    return 0;
}

static void
usage(char *argv0)
{
    fprintf(stderr, "usage:%s \n"
	    "\t[-h]\t\tPrint this text\n"
	    "\t[-v]\t\tPrint version\n"
	    "\t[-a <ipv4>]\tServer addr (default: %s)\n"
	    "\t[-c <ipv4>]\tClient addr (overrides default)\n"
	    "\t[-p <port>]\tServer port (default: %d)\n"
	    "\t[-r <port>]\tRTP data port (default: %d)\n"
	    "\t[-l <logfmt>]\tLog format (default: 1 (TIMESTAMP))\n"
	    "\t[-D <mask>]\tDebug mask (rcv: 1, send: 2, sp: 1000)\n",
	    argv0,
	    SP_DEFAULT_SERVER,
	    SPS_PORT,
	    SPHONE_RTP_PORT); 
    exit(0);
}

#ifdef WIN32_WINMAIN
int
WinMain(HINSTANCE hInstC, HINSTANCE hInstP, LPSTR lpCmdLine, int nCmdShow)
#else /* WIN32_WINMAIN */
int
main(int argc, char *argv[])
#endif /* WIN32_WINMAIN */
{
    int c;
    struct spc_info spc;
    uint16_t rtp_port;
    struct timeval t;
    struct coding_params *cp = &spc.sp_coding;

#ifdef WIN32_WINMAIN
    /* XXX: CreateMutex() is a better way to ensure uniqueness */
    if (FindWindow("Sicsophone", "Sicsophone") != NULL){
      fprintf(stderr, "spc: sicsophone window already found\n");
      return -1;
    }
    /* If no console -> redirect output to temp file */
    {
      char filename[SPC_LOGFILE_LEN];
      FILE *f;

      if (GetTempPath(SPC_LOGFILE_LEN, filename) < 0){
	perror("spc:GetTempPath");
	return -1;
      }
      strcat(filename, "/spc.log");
      fprintf(stderr, "Opening spc logfile: %s\n", filename);
      if ((f = fopen(filename, "w")) == NULL){
	perror("spc: fopen");
	return -1;
      }
#if 0
      fprintf(f, "Starting spc:\nlpCmdLine:%s\nnCmdShow:%d\n", 
	      lpCmdLine, nCmdShow);
#endif
      dbg_init(f);
    }

    {
      HWND h;
      if ((h = create_dumb_window(hInstC)) == NULL)
	exit(0);
      assert(h);
      event_init((void*)h);
    }
#else /* WIN32_WINMAIN */
    dbg_init(stderr);
    event_init(NULL);
#endif /* WIN32_WINMAIN */

    set_signal(SIGINT, sphone_signal_exit);
    set_signal(SIGTERM, sphone_signal_exit);

    /*
     * Defaults and inits
     */
    t = gettimestamp();
    srandom(t.tv_usec);

    debug = DBG_APP;
    memset(&spc, 0, sizeof(spc));
    spc.sp_id = random()%100000; 
    spc.sp_port = SPS_PORT;
    spc.sp_logformat  = LOGFMT_TIME;
    cp->cp_size_ms    = SPHONE_SIZE_MS;
    cp->cp_samp_sec   = SPHONE_SAMPLES;
    cp->cp_precision  = SPHONE_BITS;
    cp->cp_channels   = 1;

    strcpy(spc.sp_hostname, SP_DEFAULT_SERVER);
    *spc.sp_myhostname = '\0';
    rtp_port                = SPHONE_RTP_PORT;
    sp_inet_init();

    /*
     * Command line args
     * XXX: If WIN32_WINMAIN, lpCmdLine contains a single string of args
     */
#ifndef WIN32_WINMAIN
    while ((c = getopt(argc, argv, "hvc:a:r:p:l:D:")) != -1){
	switch (c) {
	case 'h' : /* help */
	    usage(argv[0]);
	    break;
	case 'v' : /* version */
	    fprintf(stdout, "Sicsophone version %s\n", SP_VERSION);
	    exit(0);
	    break;
	case 'c': /* client rtp address */
	    strncpy(spc.sp_myhostname, optarg, 64);
	    break;
	case 'a': /* server address */
	    strncpy(spc.sp_hostname, optarg, 64);
	    break;
	case 'r': /* receiver data port */
	    if (!optarg || sscanf(optarg, "%hu", &rtp_port) != 1)
		usage(argv[0]);
	    break;
	case 'p': /* server port */
	    if (!optarg || sscanf(optarg, "%hu", &spc.sp_port) != 1)
		usage(argv[0]);
	    break;
	case 'l': /* log format */
	    if (!optarg || sscanf(optarg, "%d", (int*)&spc.sp_logformat) != 1)
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
#endif /* WIN32_WINMAIN */

    if (exit_register(spc_exit, &spc, "spc_exit") < 0)
	exit(0);


    dbg_print(DBG_APP, "Starting client %d version %s at: %s", 
	      spc.sp_id,
	      SP_VERSION,
	      ctime((time_t*)&t.tv_sec));

    cp->cp_size_bytes = ms2bytes(cp->cp_samp_sec, 
				 cp->cp_precision, 
				 cp->cp_channels,
				 cp->cp_size_ms);

    /* Server control address */
    memset(&spc.sp_addr, 0, sizeof(struct sockaddr_in));
    spc.sp_addr.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
    spc.sp_addr.sin_len = sizeof(struct sockaddr_in);
#endif
    if ((spc.sp_addr.sin_addr.s_addr = inet_addr(spc.sp_hostname)) == -1){
	fprintf(stderr, "Illegal IPv4 address: %s\n", spc.sp_hostname);
	exit(0);
    }
    spc.sp_addr.sin_port = htons(spc.sp_port);
    if (udp_socket_init(&spc, 0 /*spc.sp_port */) < 0)
	exit(0);
    
    /* There is a state machine here: first try to *connect* to server.
       When done, it proceeds to send a register message. */
    if (sps_wrapper(0, &spc) < 0)
	exit(0);

    eventloop();
    spc_exit(&spc);
    return 0;
}
