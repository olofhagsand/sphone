/*-----------------------------------------------------------------------------
  File:   sphone_eventloop.c
  Description: Contains the event loop (select)
  Author: Olof Hagsand
  CVS Version: $Id: sphone_eventloop.c,v 1.21 2005/01/31 18:26:18 olof Exp $
 
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "sphone.h"

/*
 * Internal types to handle eventloop
 */
struct event_data{
    struct event_data *e_next;     /* next in list */
    int (*e_fn)(int, void*);            /* function */
    enum {EVENT_FD, EVENT_TIME} e_type;        /* type of event */
    int e_fd;                      /* File descriptor */
    struct timeval e_time;         /* Timeout */
    void *e_arg;                   /* function argument */
    char e_string[EVENT_STRLEN];             /* string for debugging */
#ifdef WINSOCK
    int  e_msg_type;               /* broken windose eventhandling need int */ 
#endif
};

#ifdef WIN32
HWND E_hwnd = 0;      /* Broken windoze event handling needs window */
#endif /* WIN32 */

/*
 * Internal variables
 */
static struct event_data *ee = NULL;
static struct event_data *ee_timers = NULL;

/*
 * event_init
 * Initiziaze event handling (for some platforms)
 */
int
event_init(void *arg)
{

#ifdef WIN32
  HWND h = (void*)arg;

  if (h == NULL){
    if ((E_hwnd = CreateWindow("BUTTON", "Sicsophone client",WS_MINIMIZE,0,0,0,0,0,0,0,0)) == NULL){
      sphone_error("event_init: CreateWindow %d", GetLastError());
      return -1;
    }
  }
  else
    E_hwnd = h;
#endif /* WIN32 */
  return 0;
}

/*
 * Sort into internal event list
 * Given an absolute timestamp, register function to call.
 */
int
eventloop_reg_timeout(struct timeval t,  int (*fn)(int, void*), 
		      void *arg, char *str)
{
    struct event_data *e, *e1, **e_prev;

    e = (struct event_data *)smalloc(sizeof(struct event_data));
    if (e==NULL){
	sphone_error("eventloop_reg_timeout: %s", strerror(errno));
	return -1;
    }
    memset(e, 0, sizeof(struct event_data));
    strncpy(e->e_string, str, EVENT_STRLEN);
    e->e_fn = fn;
    e->e_arg = arg;
    e->e_type = EVENT_TIME;
    e->e_time = t;
    /* Sort into right place */
    e_prev = &ee_timers;
    for (e1=ee_timers; e1; e1=e1->e_next){
	if (timercmp(&e->e_time, &e1->e_time, <))
	    break;
	e_prev = &e1->e_next;
    }
    e->e_next = e1;
    *e_prev = e;
    dbg_print(DBG_EVENT, "eventloop_reg_timeout: %s\n", timevalprint(t)); 
    return 0;
}


/*
 * Register a callback function when something occurs on a file descriptor.
 * When an input event occurs on file desriptor <fd>, 
 * the function <fn> shall be called  with argument <arg>.
 * <str> is a debug string for logging.
 */
int
eventloop_reg_fd(int fd, int (*fn)(int, void*), void *arg, char *str)
{
    struct event_data *e;

    e = (struct event_data *)smalloc(sizeof(struct event_data));
    if (e==NULL){
	sphone_error("eventloop_reg_fd: %s", strerror(errno));
	return -1;
    }
    memset(e, 0, sizeof(struct event_data));
    strncpy(e->e_string, str, EVENT_STRLEN);
    e->e_fd = fd;
    e->e_fn = fn;
    e->e_arg = arg;
    e->e_type = EVENT_FD;
    e->e_next = ee;
    ee = e;
#ifdef WIN32
    {
      static int msg_type = WM_USER+10;

      assert(E_hwnd);
      e->e_msg_type = msg_type++;
      if (WSAAsyncSelect(fd, E_hwnd, e->e_msg_type, FD_READ) < 0){
	sphone_error("eventloop_reg_fd: WSAAsyncSelect %d", WSAGetLastError());
	return -1;
      }
    }
#endif /* WIN32 */
    dbg_print(DBG_EVENT, "eventloop_reg_fd, registering %d: %s\n", 
	      e->e_fd, 
	      e->e_string);
    return 0;
}

/*
 * Deregister a sphone event.
 * If the function and argument match, deregister.
 * XXX: it could happen that a timeout and a _fd share the same
 * event.
 */
int
eventloop_unreg(int s, int (*fn)(int, void*), void *arg)
{
    struct event_data *e, **e_prev;
    int found = 0;

    e_prev = &ee_timers;
    for (e = ee_timers; e; e = e->e_next){
	if (fn == e->e_fn && arg == e->e_arg) {
	    found++;
	    dbg_print(DBG_EVENT, "eventloop_unreg timeout: %s\n",e->e_string); 
	    *e_prev = e->e_next;
	    sfree(e);
	    break;
	}
	e_prev = &e->e_next;
    }
    e_prev = &ee;
    for (e = ee; e; e = e->e_next){
	if (fn == e->e_fn && arg == e->e_arg && s == e->e_fd) {
	    found++;
	    dbg_print(DBG_EVENT, "eventloop_unreg_fd, unregistering %d\n", 
		      e->e_fd);
	    *e_prev = e->e_next;
#ifdef WIN32
	    if (WSAAsyncSelect(e->e_fd, E_hwnd, 0, 0) < 0){
		sphone_error("eventloop_reg_fd: WSAAsyncSelect %d", 
			     WSAGetLastError());
		return -1;
	    }
#endif /* WIN32 */
	    sfree(e);
	    break;
	}
	e_prev = &e->e_next;
    }
    return found?0:-1;
}


/*
 * Main event loop.
 * Dispatch file descriptor events (and timeouts) by invoking callbacks.
 * There is an issue with fairness that timeouts may take over all events
 * One could try to poll the file descriptors after a timeout?
 */
int
eventloop()
{
    struct event_data *e, *e_next;
    int n;
    struct timeval t, t0;
#ifdef WIN32
    int wst;
    MSG msg;
#else /* WIN32 */
    fd_set fdset;
#endif /* WIN32 */

    if (!sphone_exit)
	while (1){
#ifdef WIN32
	    wst = 0;
#else /* WIN32 */	    
	    FD_ZERO(&fdset);
	    for (e=ee; e; e=e->e_next){
		if (e->e_type == EVENT_FD)
		    FD_SET(e->e_fd, &fdset);
	    }
#endif /* WIN32 */	    
	    if (ee_timers != NULL){
		t0 = gettimestamp();
		timersub(&ee_timers->e_time, &t0, &t); 
	      if (t.tv_sec < 0){
		    n = 0;
#ifdef WIN32
		    msg.message = WM_TIMER;
#endif
	      }
		else{
#ifdef WIN32
		    wst = SetTimer(NULL,0, t.tv_sec*1000+t.tv_usec/1000, NULL);
		    if (wst == 0){
			sphone_error("eventloop: SetTimer: %d",GetLastError());
			break;
		    }
		    n = GetMessage(&msg, NULL, 0, 0);
		    if (KillTimer(NULL, wst) == 0){
			sphone_error("eventloop: KillTimer: %d", GetLastError());
			break;
		    }
#else /* WIN32 */
		    n = select(FD_SETSIZE, &fdset, NULL, NULL, &t); 
#endif /* WIN32 */
		}
	    }
	    else{
#ifdef WIN32
	        n = GetMessage(&msg, NULL, 0, 0);
#else /* WIN32 */
		n = select(FD_SETSIZE, &fdset, NULL, NULL, NULL); 
#endif /* WIN32 */
	    }
	    if (n == -1) {
		if (errno == EINTR){
		    if (sphone_exit){
			dbg_print(DBG_EXIT, "eventloop: Interrupted system call\n");
			break;
		    }
		    else
			continue;
		}
		sphone_error("eventloop: select: %s", strerror(errno));
		break;
	    }
#ifdef WIN32
	    if (msg.message == WM_TIMER){ /* Timeout */
#else /* WIN32 */
		if (n==0){ /* Timeout */
#endif /* WIN32 */
			e = ee_timers;
			ee_timers = ee_timers->e_next;
			dbg_print(DBG_EVENT, "event timeout: %s: %s[%x]\n", 
				  timevalprint(gettimestamp()), 
				  e->e_string, 
				  (int)e->e_arg);
			if ((*e->e_fn)(0, e->e_arg) < 0)
			    sphone_exit++;
			sfree(e);
			if (sphone_exit)
			    break;
			continue; 
		}
		assert(n > 0);
		for (e=ee; e; e=e_next){
		    e_next = e->e_next;
#ifdef WIN32
  	        if (e->e_type == EVENT_FD && msg.message == e->e_msg_type)
#else /* WIN32 */
		    if (e->e_type == EVENT_FD && FD_ISSET(e->e_fd, &fdset))
#endif /* WIN32 */
			{
			dbg_print(DBG_EVENT, "event fd: %s: %s[%x]\n", 
				  timevalprint(gettimestamp()), 
				  e->e_string, 
				  (int)e->e_arg);
			if ((*e->e_fn)(e->e_fd, e->e_arg) < 0)
			    sphone_exit++;
			if (sphone_exit)
			    break;
#ifdef WIN32
			msg.message = 0; /* for safety */
			break;
#endif /* WIN32 */	
			}
		}
	    if (sphone_exit)
		break;
	}
    return 0;
}
