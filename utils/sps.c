/*-----------------------------------------------------------------------------
  File:   sps.c
  Description: Sicsophone server application
  Author: Olof Hagsand
  CVS Version: $Id: sps.c,v 1.71 2005/01/31 18:37:58 olof Exp $
 
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

#ifdef HAVE_CONFIG_H
#include "config.h" /* generated by config & autoconf */
#endif

#include <math.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#ifndef WIN32
#include <dirent.h>
#endif /* WIN32 */
#include "sphone.h" 
#include "sphone_lib.h" /* debug */
#include "sphone_error.h" /* exit */
#include "sphone_eventloop.h"

#include "sp.h"

/*
 * Server does not run on windows: problem: the random() function.
 */
#define SOCKADDR_IN_EQ(sin1, sin2) \
            (((sin1)->sin_family == (sin2)->sin_family) && \
             ((sin1)->sin_port == (sin2)->sin_port) && \
             ((sin1)->sin_addr.s_addr == (sin2)->sin_addr.s_addr))


/* Forward decl */
static int client_input(int s, void *arg);
static int client_timeout(int s, void* arg);
static int sps_exit(void *arg);

enum client_state{
    CS_INIT,  /* client is in an init state: dont know pub/udp address */
    CS_IDLE,  /* client is waitin for calls */
    CS_RCV,   /* client is busy with receiving */
    CS_SEND  /* client is busy with sending */
};

/*
 * Client entry
 */
struct cf_entry{
    struct cf_entry   *cf_next;  /* The clients linked list */
    struct sockaddr_in cf_addr;  /* The clients (TCP) address */
    struct sockaddr_in cf_addr_pub;  /* The clients public UDP address */
    uint32_t           cf_id;    /* Unique client id */
    int                cf_s;     /* The clients socket */
    enum client_state  cf_state; /* Which state of this client */
    int                cf_peer;  /* If blocked: who am I talking to? */
    int                cf_timeout; /* When 0, treat as disconnected */
};

/*
 * Global variable
 */
static int server_socket = -1;
static    struct cf_entry *cf_list = NULL;
static double mean_call_arrival = SP_MEAN_CALL_ARRIVAL;
static double mean_call_duration = SP_MEAN_CALL_DURATION;
static char logdir[64] = SPS_LOGDIR;
static char logprefix[64] = SPS_LOGPREFIX;        
/* For dbg and filenaming, every meas has a uid */
static uint32_t test_id = 0; 

static struct cf_entry*
cfe_find(struct cf_entry *cf, struct sockaddr_in *addr)
{
    struct cf_entry *cfe;

    for (cfe=cf; cfe; cfe=cfe->cf_next)
	if (SOCKADDR_IN_EQ(&cfe->cf_addr, addr))
	    return cfe;
    return NULL;
}

static struct cf_entry*
cfe_find_byid(struct cf_entry *cf, uint32_t id)
{
    struct cf_entry *cfe;

    for (cfe=cf; cfe; cfe=cfe->cf_next)
	if (cfe->cf_id == id)
	    return cfe;
    return NULL;
}

static struct cf_entry*
cfe_add(struct cf_entry **cfp, struct sockaddr_in *addr)
{
    struct cf_entry *cfe;

    if ((cfe = (struct cf_entry *)smalloc(sizeof(*cfe))) == NULL){
	perror("cfe_add: malloc");
	return NULL;
    }
    memset(cfe, 0, sizeof(*cfe));
    memcpy(&cfe->cf_addr, addr, sizeof(struct sockaddr_in));
    cfe->cf_timeout = SP_TIMEOUT_REG_NR;

    /* 
     * Data addresses should be set correctly by register message.
     */
    cfe->cf_next = *cfp;
    *cfp = cfe;
    return cfe;
}

static int
cfe_rm(struct cf_entry **cfp, struct cf_entry *cfe)
{
    struct cf_entry *c;
    struct cf_entry **cf_prev;

    cf_prev = cfp;
    for (c = *cf_prev; c; c = c->cf_next){
	if (c == cfe){
	    *cf_prev = c->cf_next;
	    if (cfe->cf_s)
		close(cfe->cf_s);
	    sfree(c);
	    break;
	}
	cf_prev = &c->cf_next;
    }
    return 0;
}

static int
cf_dump(FILE *f, struct cf_entry *cf)
{
    struct cf_entry *cfe;

    for (cfe=cf; cfe; cfe=cfe->cf_next) {
	fprintf(f, "client %d, tcpaddr: %s:%d,",
		(int)cfe->cf_id,
		inet_ntoa(cfe->cf_addr.sin_addr), 
		ntohs(cfe->cf_addr.sin_port));
	fprintf(f, " pubaddr: %s:%d\n", 
		inet_ntoa(cfe->cf_addr_pub.sin_addr), 
		ntohs(cfe->cf_addr_pub.sin_port));
	dbg_print(DBG_APP, "client %d, tcpaddr: %s:%d,",
		  (int)cfe->cf_id,
		  inet_ntoa(cfe->cf_addr.sin_addr), 
		  ntohs(cfe->cf_addr.sin_port));
	dbg_print(DBG_APP, " pubaddr: %s:%d\n", 
		  inet_ntoa(cfe->cf_addr_pub.sin_addr), 
		  ntohs(cfe->cf_addr_pub.sin_port));
    }
    return 0;
}

/*
 * Invoked as USR1 signal handler:
 * dump all clients data to file (default .sps)
 */
static void
dump_wrapper(int arg)
{
    dbg_print(DBG_APP, "Dumping client list to file\n");
    cf_dump(stderr, cf_list);
    dbg_print(DBG_APP, "Done\n");
}

static void
kill_all_clients(int arg)
{
    struct cf_entry *cfe, *cfe_next;
    struct sp_proto sp;

    for (cfe=cf_list; cfe; cfe=cfe_next) {
	cfe_next = cfe->cf_next;
	sp.sp_magic = SP_MAGIC;
	sp.sp_type  = SP_TYPE_EXIT;
	sp.sp_version = htons(SP_VERSION_NR);
	sp.sp_len   = htonl(sizeof(sp));
	sp.sp_id    = htonl(cfe->cf_id);
	if (send(cfe->cf_s, (void*)&sp, sizeof(sp), 0x0) < 0){
	    perror("kill_all_clients: send");
	    return;
	}
	if (eventloop_unreg(cfe->cf_s, client_input, cfe) < 0){
	    perror("kill_all_clients: client not found");
	    return;
	}
	cfe_rm(&cf_list, cfe);
    }
    dbg_print(DBG_APP, "kill_all_clients\n");
}


/*
 * client_output
 * Send a start message to a client.
 */
static int
client_output(struct cf_entry *cfe, struct cf_entry *cfe_peer, 
	      enum sp_type type, struct timeval *duration, 
	      uint32_t test_id)
{
    int s = cfe->cf_s;
    struct sp_proto sp;
    struct timeval now;

    now = gettimestamp();
    switch (type){
    case SP_TYPE_START_SEND:
	sp.sp_reporthdr.ss_testid = htonl(test_id);
	sp.sp_reporthdr.ss_clientid_src = htonl(cfe->cf_id);
	sp.sp_reporthdr.ss_clientid_dst = htonl(cfe_peer->cf_id);
	sp_addr_encode(&cfe->cf_addr_pub, 
		       (char*)&sp.sp_starthdr.ss_addr_src_pub);
	assert(cfe->cf_addr_pub.sin_addr.s_addr);
	sp_addr_encode(&cfe_peer->cf_addr_pub, 
		       (char*)&sp.sp_starthdr.ss_addr_dst_pub);
	sp.sp_starthdr.ss_time_start.tv_sec = htonl(now.tv_sec);
	sp.sp_starthdr.ss_time_start.tv_usec = htonl(now.tv_usec);
	sp.sp_starthdr.ss_time_duration.tv_sec = htonl(duration->tv_sec);
	sp.sp_starthdr.ss_time_duration.tv_usec = htonl(duration->tv_usec);
	break;
    case SP_TYPE_START_RCV:
	sp.sp_reporthdr.ss_testid = htonl(test_id);
	sp.sp_reporthdr.ss_clientid_src = htonl(cfe->cf_id);
	sp.sp_reporthdr.ss_clientid_dst = htonl(cfe_peer->cf_id);
	sp_addr_encode(&cfe_peer->cf_addr_pub, 
		       (char*)&sp.sp_starthdr.ss_addr_src_pub);
	assert(cfe->cf_addr_pub.sin_addr.s_addr);
	sp_addr_encode(&cfe->cf_addr_pub, 
		       (char*)&sp.sp_starthdr.ss_addr_dst_pub);
	sp.sp_starthdr.ss_time_start.tv_sec = htonl(now.tv_sec);
	sp.sp_starthdr.ss_time_start.tv_usec = htonl(now.tv_usec);
	sp.sp_starthdr.ss_time_duration.tv_sec = htonl(duration->tv_sec);
	sp.sp_starthdr.ss_time_duration.tv_usec = htonl(duration->tv_usec);
	break;
    default:
	break;
    }
    sp.sp_magic = SP_MAGIC;
    sp.sp_type  = type;
    sp.sp_version = htons(SP_VERSION_NR);
    sp.sp_len   = htonl(sizeof(sp));
    sp.sp_id    = htonl(cfe->cf_id);

    dbg_print_pkt(DBG_SEND|DBG_DETAIL, "client_output", (char*)&sp, sizeof(sp));
    if (send(s, (void*)&sp, sizeof(sp), 0x0) < 0){
	perror("client_output: send");
	close(s);
	return -1;
    }
    return 0;
}

/*
 * Returns random exponential distribution function
 * If fn(x) = l*exp(-l*x)
 * The if a = 1/l is the average arrival time, and r() is a random function [0,1]
 * Then the random exponential distribution function is -a ln r().
 */
static int
exprand(double alpha, struct timeval *delta)
{
    uint32_t r;
    double d;
    double sec;
    
    r = random();
    d = ((double)r)/((double)0x7fffffffL);
    sec = -alpha*log(d);
    delta->tv_sec = (uint32_t)sec;
    delta->tv_usec = (uint32_t)1000000*(sec - (uint32_t)sec); 
    return 0;
}

/*
 * Select two clients and send start message to them
 */
static int
client_startcalls(int s, void* arg)
{
    struct timeval t, delta;
    struct timeval duration; /* duration of call */
    struct cf_entry **cfp = (struct cf_entry**)arg;
    struct cf_entry *cf_send, *cf_rcv, *cfe;
    int i, index1, index2, nr = 0;
    int retval = 0;
    uint32_t this_test_id;
    /* 
     * Select two clients: first count nr of clients.
     */

    for (cfe=*cfp; cfe; cfe=cfe->cf_next)  nr++;
    if (nr < 2)
	goto done;
    index2=-1;
    index1 = random()%nr;
    while (index1 == (index2 = random()%nr));
    cf_send = cf_rcv = NULL;
    for (cfe=*cfp, i=0; cfe; cfe=cfe->cf_next, i++){
	if (i==index1)
	    cf_send = cfe;
	if (i==index2)
	    cf_rcv = cfe;
    }
    exprand(mean_call_duration, &duration);
    t = gettimestamp();
    this_test_id = test_id; /* tentative */
    dbg_print(DBG_APP, "Test %d: Client %d -> client %d\n",
	      test_id,
	      cf_send->cf_id, 
	      cf_rcv->cf_id);
    if (cf_send->cf_state == CS_IDLE && cf_rcv->cf_state == CS_IDLE){
	test_id++; /* commit test */
	if (client_output(cf_send, cf_rcv, SP_TYPE_START_SEND, &duration, 
			  this_test_id) < 0){
	    retval = -1;
	    goto done;
	}
	if (client_output(cf_rcv, cf_send, SP_TYPE_START_RCV, &duration,
			  this_test_id) < 0){
	    retval = -1;
	    goto done;
	}
	cf_send->cf_state = CS_SEND;
	cf_rcv->cf_state = CS_RCV;
	cf_rcv->cf_peer = cf_send->cf_id;
	cf_send->cf_peer = cf_rcv->cf_id;
    }
    else{
	dbg_print(DBG_APP, " (blocked: ");
	if (cf_send->cf_state != CS_IDLE)
	    dbg_print(DBG_APP, "%d ", cf_send->cf_id);
	if (cf_rcv->cf_state != CS_IDLE)
	    dbg_print(DBG_APP, "%d ", cf_rcv->cf_id);
	dbg_print(DBG_APP, ")\n");
    }
  done:
    exprand(mean_call_arrival/max(1,nr-1), &delta);
    t = gettimestamp();
    timeradd(&t, &delta, &t);
    if (eventloop_reg_timeout(t,  client_startcalls, cfp, "client startcalls") < 0)
	return -1;
    return retval;
}

/*
 * Set the timer for starting client timeout checks
 */
static int
start_client_timeout(struct cf_entry **cfp)
{
    struct timeval t, delta;

    t = gettimestamp();
    delta.tv_usec = 0;
    delta.tv_sec = SPC_SERVER_INTERVAL;
    timeradd(&t, &delta, &t);
    if (eventloop_reg_timeout(t,  client_timeout, cfp, "client timeouts") < 0)
	return -1;
    return 0;
}

/*
 * Go through clients and delete passive clients.
 */
static int
client_timeout(int s, void* arg)
{
    struct cf_entry **cfp = (struct cf_entry**)arg;
    struct cf_entry *cfe, *cfe_next;
    struct timeval t;
    char timestr[26];

    /* 
     * Check for timeouts
     */

    t = gettimestamp();
    strcpy(timestr, ctime((time_t*)&t.tv_sec));
    timestr[strlen(timestr)-1] = '\0';
    dbg_print(DBG_APP, "%s: ", timestr);

    if (*cfp == NULL)
	dbg_print(DBG_APP, "No clients\n");
    else{
	dbg_print(DBG_APP, "clients:");
	for (cfe=*cfp; cfe; cfe=cfe_next){
	    cfe_next = cfe->cf_next;
	    cfe->cf_timeout--;
	    if (cfe->cf_timeout == 0){
		dbg_print(DBG_APP, "(%d removed) ", cfe->cf_id);
		if (eventloop_unreg(cfe->cf_s, client_input, cfe) < 0){
		  sphone_error("client_input unreg: client not found");
		  return -1;
		}
		close(cfe->cf_s);
		cfe_rm(&cf_list, cfe);
	    }
	    else
		dbg_print(DBG_APP, "%d ", cfe->cf_id);
	}
	dbg_print(DBG_APP, "\n");
    }
    return start_client_timeout(cfp);
}

/*
 * receive log from sending client.
 * Create a log file name and dump all data there.
 * Filename is constructed as follows:
 * <prefix>_<testid>
 * The beginning of the filelooks like this:
 * testid:<testid>
 * srcid: <src client id>
 * dstid: <dst client id>
 * src:   <srcaddr>/<srcport>
 * dst:   <dstaddr>/<dstport>
 * start: <date>
 * dur:   <duration>
 */
static int
receive_send_report(struct sp_proto *sp, struct cf_entry *cfe)
{
    char buf[SP_BUFLEN];
    int len;
    int restlen;
    char filename[128];
    int fd;
    time_t start_t;

    start_t = ntohl(sp->sp_reporthdr.ss_time_start.tv_sec);
    restlen = ntohl(sp->sp_len) - sizeof(*sp);
    dbg_print(DBG_APP, "Client %d test %d ready send report: %d bytes\n", 
	      cfe->cf_id,
	      ntohl(sp->sp_reporthdr.ss_testid),
	      restlen);
    if (restlen == 0)
	goto done;
    sprintf(filename, "%s/%s_%u",
	    logdir, logprefix, (u_int)ntohl(sp->sp_reporthdr.ss_testid));
    if ((fd = open(filename, O_WRONLY|O_CREAT, 00666)) < 0){
	fprintf(stderr, "receive_send_report: open(%s): %s\n",
		filename,
		strerror(errno));
	return -1;
    }
    sprintf(buf, "testid:\t%u\n", (u_int)ntohl(sp->sp_reporthdr.ss_testid));
    if (write(fd, buf, strlen(buf)) < 0){
	perror("receive_send_report: write 1");
	close(fd);
	return -1;
    }
    sprintf(buf, "srcid:\t%u\n", (u_int)ntohl(sp->sp_reporthdr.ss_clientid_src));
    if (write(fd, buf, strlen(buf)) < 0){
	perror("receive_send_report: write 1.2");
	close(fd);
	return -1;
    }
    sprintf(buf, "dstid:\t%u\n", (u_int)ntohl(sp->sp_reporthdr.ss_clientid_dst));
    if (write(fd, buf, strlen(buf)) < 0){
	perror("receive_send_report: write 1.5");
	close(fd);
	return -1;
    }

    sprintf(buf, "src:\t%s/%hu\n", 
	    inet_ntoa(sp->sp_reporthdr.ss_addr_src_pub.sin_addr),
	    ntohs(sp->sp_reporthdr.ss_addr_src_pub.sin_port));
    if (write(fd, buf, strlen(buf)) < 0){
	perror("receive_send_report: write 2");
	close(fd);
	return -1;
    }
    sprintf(buf, "dst:\t%s/%hu\n", 
	    inet_ntoa(sp->sp_reporthdr.ss_addr_dst_pub.sin_addr),
	    ntohs(sp->sp_reporthdr.ss_addr_dst_pub.sin_port));
    if (write(fd, buf, strlen(buf)) < 0){
	perror("receive_send_report: write 3");
	close(fd);
	return -1;
    }
    sprintf(buf, "start:\t%u\ndur:\t%u\n", 
	    (u_int)start_t, 
	    (u_int)ntohl(sp->sp_reporthdr.ss_time_duration.tv_sec)+1);
    if (write(fd, buf, strlen(buf)) < 0){
	perror("receive_send_report: write 4");
	close(fd);
	return -1;
    }
    len = SP_BUFLEN;
    while (len > 0 && restlen > 0){
	if ((len = recv(cfe->cf_s, buf, min(SP_BUFLEN, restlen), 0x0)) < 0){
#ifdef WINSOCK
	    if (errno == WSAECONNRESET)
#else /* WINSOCK */
		/* TCP closed or timedout in the middle of a transfer */
	    if (errno == ECONNRESET || errno == ETIMEDOUT)
#endif /* WINSOCK */
		{
		if (eventloop_unreg(cfe->cf_s, client_input, cfe) < 0){
		    sphone_error("receive_send_report unreg: client not found");
		    return -1;
		}
		close(cfe->cf_s);
		dbg_print(DBG_APP, "receive_send_report: closing client socket %d\n", 
			  cfe->cf_s);
		cfe_rm(&cf_list, cfe);
		close(fd);
		return 0;
	    }
	    perror("receive_send_report: recv");
	    close(fd);
	    return -1;
	}
	restlen -= len;
	if (len > 0)
	    if (write(fd, buf, len) < 0){
		perror("receive_send_report: write");
		close(fd);
		return -1;
	    }
    }
    close(fd);
  done:
    return 0;
}

static int
rcv_sp_proto(struct cf_entry *cfe, struct sp_proto *sp)
{
    int firsttime = (cfe->cf_id == 0);

    switch(sp->sp_type){
    case SP_TYPE_REGISTER:
	cfe->cf_timeout = SP_TIMEOUT_REG_NR;
	cfe->cf_id = ntohl(sp->sp_id);
	if (sp->sp_reghdr.sr_busy == 0)
	    cfe->cf_peer = 0;
	if (sp->sp_reghdr.sr_addr_pub.sin_addr.s_addr)
	    sp_addr_decode(&cfe->cf_addr_pub, 
			   (char*)&sp->sp_reghdr.sr_addr_pub);
	if (firsttime)
	    dbg_print(DBG_APP, "New client %d at %s:%d\n", 
		      ntohl(sp->sp_id), 
		      inet_ntoa(cfe->cf_addr.sin_addr), 
		      ntohs(cfe->cf_addr.sin_port));
	break;
    case SP_TYPE_DEREGISTER:
	dbg_print(DBG_APP, "Deregister client %d at %s:%d\n", 
		  ntohl(sp->sp_id), 
		  inet_ntoa(cfe->cf_addr.sin_addr), 
		  ntohs(cfe->cf_addr.sin_port));
	if (eventloop_unreg(cfe->cf_s, client_input, cfe) < 0){
	    sphone_error("rcv_sp_proto unreg: client not found");
	    return -1;
	}
	close(cfe->cf_s);
	cfe_rm(&cf_list, cfe);
	break;
    case SP_TYPE_REPORT_SEND:
	if (receive_send_report(sp, cfe) < 0)
	    return -1;
	cfe->cf_state = CS_IDLE;
	cfe->cf_peer = 0;
	break;
    case SP_TYPE_REPORT_RCV: /* getting data */
	dbg_print(DBG_APP, "Client %d test %d ready rcv report\n", 
		  cfe->cf_id,
		  ntohl(sp->sp_reporthdr.ss_testid));
	cfe->cf_state = CS_IDLE;
	cfe->cf_peer = 0;
	break;
    case SP_TYPE_EXIT:
	sps_exit(&cf_list);
	break;
    case SP_TYPE_START_RCV:
    case SP_TYPE_START_SEND:
    default:
	fprintf(stderr, "sanity: wrong protocol type: %d\n", sp->sp_type);
	break;
    }
    return 0;
}

static int
client_input(int s, void *arg)
{
    struct cf_entry *cfe = (struct cf_entry *)arg, *cfe1;
    struct sp_proto sp;
    int l, len = 0;

    /* Sanity: cfe still exists. */
    for (cfe1=cf_list; cfe1; cfe1=cfe1->cf_next)
	if (cfe == cfe1 && cfe->cf_s == s)
	    break;
    assert(cfe1 != NULL); /* means we have event on non-existing client */

    dbg_print(DBG_RCV, "client_input from: %s:%d,",
	      inet_ntoa(cfe->cf_addr.sin_addr), 
	      ntohs(cfe->cf_addr.sin_port));

    /* OK Got a message from a client. Now what? */
    while (len < sizeof(sp)){
	if ((l = recv(cfe->cf_s, ((char*)&sp)+len, 
		      sizeof(struct sp_proto)-len, 0x0)) < 0){
#ifdef WINSOCK
	    if (errno == WSAECONNRESET)
#else /* WINSOCK */
		/* TCP closed or timedout in the middle of a transfer */
	    if (errno == ECONNRESET || errno == ETIMEDOUT)
#endif /* WINSOCK */
		{
		    if (eventloop_unreg(cfe->cf_s, client_input, cfe) < 0){
			sphone_error("client_input unreg: client not found");
			return -1;
		    }
		    close(cfe->cf_s);
		dbg_print(DBG_APP, "client_input: closing client socket %d\n", 
			  cfe->cf_s);
		cfe_rm(&cf_list, cfe);
		return 0;
	    }
	    perror("client_input: recv");
	    return -1;
	}
	dbg_print(DBG_RCV, "client_input: rcv: (l:%d)\n", l);
	dbg_print_pkt(DBG_RCV|DBG_DETAIL, "client_input", ((char*)&sp)+len, l);
	if (l == 0){ /* closedown? */
	    if (eventloop_unreg(cfe->cf_s, client_input, cfe) < 0){
		sphone_error("client_input unreg: client not found");
		return -1;
	    }
	    close(cfe->cf_s);
	    dbg_print(DBG_APP, "client_input: closing client socket %d\n", 
		      cfe->cf_s);
	    cfe_rm(&cf_list, cfe);
	    return 0;
	}
	len += l;
	if (sp.sp_magic != SP_MAGIC){     /* sanity */
	    fprintf(stderr, "Warning: client_input: wrong magic cookie: 0x%x from %d\n",
		    sp.sp_magic,
		    cfe->cf_id);
	    return 0;
	}
	if (ntohs(sp.sp_version) != SP_VERSION_NR){
	    fprintf(stderr, "Error: client_input: wrong protocol version: %hu from %s/%hu, expected %hu\n",
		    ntohs(sp.sp_version),
		    inet_ntoa(cfe->cf_addr.sin_addr), 
		    ntohs(cfe->cf_addr.sin_port),
		    SP_VERSION_NR);
	    return 0;
	}
    }
    /* If client id is set (it might not be) its should match */
    if (cfe->cf_id && htonl(sp.sp_id) != cfe->cf_id){
	fprintf(stderr, "Warning: client_input: wrong client id: %d\n", 
		(int)ntohl(sp.sp_id));
	return 0;
    }
    if (rcv_sp_proto(cfe, &sp) < 0)
	return -1;

    return 0;
}

static int
connect_input(int s, void* arg)
{
    int s1;
    struct sockaddr_in from;
    struct cf_entry **cfp = (struct cf_entry **)arg;
    struct cf_entry *cfe;
    int len = sizeof(struct sockaddr_in);

    memset(&from, 0, sizeof(struct sockaddr_in));
    if ((s1 = accept(s, (struct sockaddr*)&from, &len)) < 0){
	perror("connect_input: accept");
	return -1;
    }
    /* sanity */
    if (from.sin_family != AF_INET || len != sizeof(struct sockaddr_in)){
	fprintf(stderr, "sanity: dropping accept\n");
	close(s1);
	return 0;
    }
    /* This should be from a client we do not know of */
    if ((cfe = cfe_find(*cfp, &from)) != NULL){
	/* Somewhat strange: we know of this fellow: close the old and 
	   use the new */
	fprintf(stderr, "Warning: connect_input: connect from already connecting client: reopening socket\n");
	if (eventloop_unreg(cfe->cf_s, client_input, cfe) < 0){
	    sphone_error("connect_input unreg: client not found");
	    return -1;
	}
	close(cfe->cf_s);
	cfe->cf_s = s1;
	if (eventloop_reg_fd(cfe->cf_s, client_input, cfe, 
			     "sicsophone server input") < 0)

	return 0;
    }
    /* So far, we dont know anything about this client since only an accept */
    if ((cfe = cfe_add(cfp, &from)) == NULL)
	return -1;
    cfe->cf_s = s1;
    if (eventloop_reg_fd(cfe->cf_s, client_input, cfe, 
			 "sicsophone server input") < 0)
	return -1;
    return 0;
}

/*
 * A probe message sent from client.
 * We assume we know the client already, but the private address is not unique 
 * eg can be a 10 address, and the public address is learnt by this message.
 * Therefore, we need to rely on the client id.
 */
static int
udp_input(int s, void* arg)
{
    struct sockaddr_in from;
    struct cf_entry **cfp = (struct cf_entry **)arg;
    struct cf_entry *cfe;
    int len, fromlen;
    struct sp_proto sp;

    fromlen = sizeof(from);
    if ((len = recvfrom(s, (void*)&sp, sizeof(struct sp_proto), 0x0,
			(struct sockaddr *)&from, &fromlen)) < 0){
	perror("udp_input: recvfrom");
	return -1;
    }
    dbg_print_pkt(DBG_RCV|DBG_DETAIL, "udp_input", (char*)&sp, len);
    if (sp.sp_magic != SP_MAGIC){     /* sanity */
	fprintf(stderr, "Warning: udp_input: wrong magic cookie: 0x%x\n",
		sp.sp_magic);
	return 0;
    }
    if (ntohs(sp.sp_version) != SP_VERSION_NR){
    fprintf(stderr, "Error: udp_input: wrong protocol version: %hu from %s/%hu, expected %hu\n",
	    ntohs(sp.sp_version),
	    inet_ntoa(from.sin_addr), 
	    ntohs(from.sin_port),
	    SP_VERSION_NR);

	fprintf(stderr, "Error: udp_input: wrong protocol version: %hu\n",
		ntohs(sp.sp_version));
	return 0;
    }

    if (len != sizeof(sp)){
	fprintf(stderr, "Warning: udp_input: len != sizeof(sp)\n");
	return 0;
    }
    if (sp.sp_type == SP_TYPE_EXIT){
	sps_exit(&cf_list);
	return -1;
    }
    if (sp.sp_type != SP_TYPE_REGISTER){
	fprintf(stderr, "udp_input: wrong protocol type: %d\n", sp.sp_type);
	return -1;
    }
    /* This can be from a client we do not know of */
    if ((cfe = cfe_find_byid(*cfp, ntohl(sp.sp_id))) == NULL){
	fprintf(stderr, "Warning: udp_input: clientid not found: %d\n", 
		(int)ntohl(sp.sp_id));
	return 0;
    }
    if (memcmp(&from, &cfe->cf_addr_pub, sizeof(struct sockaddr_in))){
	dbg_print(DBG_APP, "Client %d registered public address: %s:%d\n", 
		  cfe->cf_id,
		  inet_ntoa(from.sin_addr), 
		  ntohs(from.sin_port));
	cfe->cf_timeout = SP_TIMEOUT_REG_NR;
	memcpy(&cfe->cf_addr_pub, &from, sizeof(struct sockaddr_in));
	if (cfe->cf_state == CS_INIT)
	    cfe->cf_state = CS_IDLE;
    }
    return 0;
}


static int
sps_exit(void *arg)
{
    struct cf_entry **cfp = (struct cf_entry **)arg;
    struct cf_entry *cfe, *cfe_next;
    struct timeval t = gettimestamp();
    
    dbg_print(DBG_APP, "sps_exit at: %s\n", 
	      ctime((time_t*)&t.tv_sec));
    /* Close and deallocate server socket */
    if (server_socket != -1){
	close(server_socket);
	eventloop_unreg(server_socket, connect_input, &cf_list);
    }

    /* Close and deallocate all clients */
    for (cfe=*cfp; cfe; cfe=cfe_next){
	cfe_next = cfe->cf_next;
	eventloop_unreg(cfe->cf_s, client_input, cfe);
	close(cfe->cf_s);
	dbg_print(DBG_APP, "sps_exit: closing client %d socket %d\n", 
		  cfe->cf_id,
		  cfe->cf_s);
	sfree(cfe);
	break;
    }
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
	    "\t[-f <prefix>]\tLogfile prefix (default: %s)\n"
	    "\t[-p <port>]\tServer port (default: %d)\n"
	    "\t[-l <logdir>]\tlogdir (default: %s)\n"
	    "\t[-m <sec>]\tMean call arrival (/#clients) (default: %d)\n"
	    "\t[-M <sec>]\tMean call duration (default: %d)\n"
	    "\t[-o <nr>]\t testid offset (default: 0)\n"
	    "\t[-D <mask>]\tDebug mask (rcv: 1, send: 2, sp: 1000)\n"
	    "kill -USR1 lists all clients, and -USR2 kills all clients\n",
	    argv0, 
	    SPS_LOGPREFIX,
	    SPS_PORT,
	    SPS_LOGDIR,
	    SP_MEAN_CALL_ARRIVAL,
	    SP_MEAN_CALL_DURATION); 
    exit(0);
}

int
main(int argc, char *argv[])
{
    int c;
    int s;
    int udp_s;
    struct sockaddr_in addr;
    uint16_t server_port;
    struct timeval tm;
#ifndef WIN32
    DIR *dirp;
#endif
#ifdef SO_REUSEPORT
    int one=1;
#endif

    /* 
     * Handle control-C 
     */
    set_signal(SIGINT, sphone_signal_exit);
    set_signal(SIGTERM, sphone_signal_exit);
#ifndef WIN32
    set_signal(SIGUSR1, dump_wrapper);
    set_signal(SIGUSR2, kill_all_clients);
#endif /* WIN32 */
    /*
     * Defaults and inits
     */
    dbg_init(stderr);
    event_init(NULL);
    debug = DBG_APP; /* XXX */
    server_port = SPS_PORT;
    srandom(1);
    sp_inet_init();

    /*
     * Command line args
     */
    while ((c = getopt(argc, argv, "hvf:o:p:l:m:M:D:")) != -1){
	switch (c) {
	case 'h' : /* help */
	    usage(argv[0]);
	    break;
	case 'v' : /* version */
	    fprintf(stdout, "Sicsophone version %s\n", SP_VERSION);
	    exit(0);
	    break;
	case 'f' : /* logfile prefix */
	    if (!optarg || (sscanf(optarg, "%s", (char*)logprefix) != 1))
		usage(argv[0]);
	    break;
	case 'o': /* testid offset */
	    if (!optarg || sscanf(optarg, "%u", &test_id) != 1)
		usage(argv[0]);
	    break;
	case 'p': /* server port */
	    if (!optarg || sscanf(optarg, "%hu", &server_port) != 1)
		usage(argv[0]);
	    break;
	case 'l': /* log directory */
	    if (!optarg || (sscanf(optarg, "%s", (char*)logdir) != 1))
		usage(argv[0]);
	    break;
	case 'm': /* mean call arrival */
	    if (!optarg || sscanf(optarg, "%lf", 
				  (double*)&mean_call_arrival) != 1)
		usage(argv[0]);
	    break;
	case 'M': /* mean call duration */
	    if (!optarg || sscanf(optarg, "%lf", 
				  (double*)&mean_call_duration) != 1)
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

#ifndef WIN32
    /* Sanity check: check that logdir is accessible, so we dont fail later */
    if ((dirp = opendir(logdir)) == NULL){
	fprintf(stderr, "Error when accessing logdir: opendir(%s): %s\n"
		"(Set logdir with the -l option)\n",
		logdir, strerror(errno));
	return -1;
    }
#endif /* WIN32 */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	perror("sps: socket");
	exit(0);
    }
    tm = gettimestamp();
    dbg_print(DBG_APP, "Starting sps version %s at: %s", 
	      SP_VERSION,
	      ctime((time_t*)&tm.tv_sec));
    server_socket = s; /* XXX: global for exit handler */
    if (exit_register(sps_exit, &cf_list, "sps_exit") < 0)
	exit(0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
    addr.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server_port);
#ifdef SO_REUSEPORT
    setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
	perror("sps: bind");
	exit(0);
    }
    if (listen(s, 5) < 0){
	perror("sps: listen");
	exit(0);
    }
    if (eventloop_reg_fd(s, connect_input, &cf_list, 
			 "sicsophone connect server input") < 0)
	exit(0);
    if ((udp_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
	perror("sps: socket");
	exit(0);
    }
    if (bind(udp_s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
	perror("sps: bind");
	exit(0);
    }
    if (eventloop_reg_fd(udp_s, udp_input, &cf_list, 
			 "sicsophone udp server input") < 0)
	exit(0);
    if (client_startcalls(0, &cf_list) < 0)
	exit(0);
    if (start_client_timeout(&cf_list) < 0)
	exit(0);

    eventloop();
    sps_exit(&cf_list);
    return 0;
}
