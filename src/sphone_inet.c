/*-----------------------------------------------------------------------------
  File:   sphone_inet.c
  Description:  sphone internet network code
  Author: Olof Hagsand
  CVS Version: $Id: sphone_inet.c,v 1.26 2005/01/31 18:26:18 olof Exp $
 
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


#include "sphone.h"

#ifndef WIN32
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#endif /* WIN32 */

/*
 * Variables
 */ 


/* 
 * inet_host2addr
 * Translate from hostname as string to inet addr
 * port in network byte order */ 
int 
inet_host2addr(char *hostname, uint16_t port, struct sockaddr_in *addr)
{
    struct hostent *hp;
    
    assert(addr);
    addr->sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
    addr->sin_len = sizeof(struct sockaddr_in);
#endif
    addr->sin_port = port;
    addr->sin_addr.s_addr = inet_addr(hostname);
    if (addr->sin_addr.s_addr == -1) {
	/* symbolic name */
	hp = gethostbyname(hostname);
	if (hp == 0){
            sphone_error("inet_host2addr: gethostbyname: %s\n", strerror(errno)); 
	    return -1;
	}
	memcpy((char*)&addr->sin_addr, (char*)hp->h_addr_list[0],hp->h_length);
    }
    return 0; /* OK */
}

/*
 * Init internet
 * Output: udp socket.
 */
int
inet_init(int *s0, int *s1)
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
    assert(s0); assert(s1);
    /* Open the sockets (data (rtp) and control (rtcp)). */
    if ((*s0 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {

	sphone_error("inet_init: socket: %s\n", strerror(errno)); 
	return -1;
    }
    if ((*s1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	sphone_error("inet_init: socket: %s\n", strerror(errno)); 
	return -1;
    }

    return 0;
}

/*
 * bind a udp socket
 * Input/Output: port number of receiving socket (netw byte order)
 */
int
inet_bind(int s, uint16_t port)
{
    struct sockaddr_in addr;
    int one = 1;

    /* All input to udp_socket */
    memset(&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
    addr.sin_len    = sizeof(struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port   = port;

    /*
     * Reuse - useful when debugging on own machine - gdb, etc
     */
#ifdef SO_REUSEPORT
    setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void*)&one, sizeof(one));

    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0){
	sphone_error("inet_init: bind: %s", strerror(errno)); 
	return -1;
    }
    return 0;
}

/*
 * bind a udp socket
 * Input/Output: port number of receiving socket (netw byte order)
 */
int
inet_input(int s, char *buf, int *len, struct sockaddr_in *from)
{
    int fromlen=sizeof(struct sockaddr_in);
    int retval;

    assert(len && from);
    retval = recvfrom(s, buf, *len, 0x0, (struct sockaddr *)from, &fromlen);
    if (retval < 0){
#ifdef WIN32 /* This is done by the -X option */
      int errno = WSAGetLastError();
      if (errno == WSAECONNRESET){
	fprintf(stderr, "inet_input: WSAECONNRESET\n");
	return -1;
      }
	fprintf(stderr, "inet_input: recvfrom: %d\n", errno);
#else /* WIN32 */
	perror("inet_input: recvfrom");
#endif /* WIN32 */
	return -1;
    }
    *len = retval;
    dbg_print_pkt(DBG_RCV|DBG_DETAIL, "sphone_inet", buf, 32);
    if (*len == 0){
	sphone_error("inet_input: Connection closed");
	return -1;
    }
    if (fromlen != sizeof(struct sockaddr_in)){
	sphone_error("inet_input: Wrong addr len %d", fromlen);
	return -1;
    }
    return 0;
}

/*
 * get_default_address
 * Go through all interfaces and find first non-local address
 * If not found retrun 0, if found -1 on error
 */
int
inet_get_default_addr(struct in_addr *addr)
{
    int found = 0;
/* XXX: I'd rather have HAVE_IFCONF, or something similar, 
 * but I dont know how to do 
 */
#ifndef WIN32  
    int s;
    int n;
    struct ifreq *ifreq;
    struct ifconf ifconf;
    int ifnum = 32; /* start value: # interfaces */
    int lastlen = 0;


    if ((s = socket (AF_INET, SOCK_DGRAM, 0)) < 0){
	fprintf(stderr, "init_get_default_addr: socket: %s", strerror (errno));
	return -1;
    }
    ifconf.ifc_buf = NULL;
    /* Loop until SIOCGIFCONF success. */
    for (;;) {
	ifconf.ifc_len = sizeof (struct ifreq) * ifnum;
	if ((ifconf.ifc_buf = realloc(ifconf.ifc_buf, ifconf.ifc_len)) == NULL){
	    perror("init_get_default_addr: realloc");
	    return -1;
	}
	if (ioctl(s, SIOCGIFCONF, &ifconf) < 0){
	    perror("init_get_default_addr: ioctl(SIOCGIFCONF)");
	    goto end;
	}
	/* Repeatedly get info til buffer fails to grow. */
	if (ifconf.ifc_len > lastlen){
	    lastlen = ifconf.ifc_len;
	    ifnum += 10;
	    continue;
	}
	/* Success. */
	break;
    }
#ifdef __OpenBSD__ /* magic code ffrom zebra */
    for (n = 0; n < ifconf.ifc_len; ){
	int size;

	ifreq = (struct ifreq *)((caddr_t) ifconf.ifc_req + n);
	if (ifreq->ifr_addr.sa_family == AF_INET){
	    memcpy(addr, &((struct sockaddr_in*)&ifreq->ifr_addr)->sin_addr, 
		   sizeof(struct in_addr));
	    if (ntohl(addr->s_addr) != INADDR_LOOPBACK &&
		ntohl(addr->s_addr) != INADDR_ANY){
		fprintf(stderr, "Found addr %s on interface: %s\n", 
			inet_ntoa(*addr),
			ifreq->ifr_name);
		found = 1;
		break;
	    }
	}
	size = ifreq->ifr_addr.sa_len;
	if (size < sizeof (ifreq->ifr_addr))
	    size = sizeof (ifreq->ifr_addr);
	size += sizeof (ifreq->ifr_name);
	n += size;
    }
#else /* __OpenBSD__ */
    ifreq = ifconf.ifc_req;     /* Allocate interface. */
    for (n = 0; n < ifconf.ifc_len; n += sizeof(struct ifreq)) {
	printf("ifconf: %s\n", ifreq->ifr_name);
	if (ifreq->ifr_addr.sa_family == AF_INET){
	    memcpy(addr, &((struct sockaddr_in*)&ifreq->ifr_addr)->sin_addr, 
		   sizeof(struct in_addr));
	    if (ntohl(addr->s_addr) != INADDR_LOOPBACK &&
		ntohl(addr->s_addr) != INADDR_ANY){
		fprintf(stderr, "Found addr %s on interface: %s\n", 
			inet_ntoa(*addr),
			ifreq->ifr_name);
		found = 1;
		break;
	    }
	}

	ifreq++;
    }
#endif /* __OpenBSD__ */    
  end:
    close(s);
    if (ifconf.ifc_buf)
	sfree(ifconf.ifc_buf);
#endif /* WIN32 */
    return found;
}

