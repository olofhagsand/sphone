/*-----------------------------------------------------------------------------
  File:   sphone_error.c
  Description: Useful functions
  Author: Olof Hagsand
  CVS Version: $Id: sphone_error.c,v 1.5 2004/06/16 08:38:47 olofh Exp $
 
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
/*
     The error handling is made in the following way:
     When a module is started, it registers an exit function.
     When sphone exits, it goes through the exit functions (in reverse order 
     from how they were registered).
     Exiting sphone is made either via a signal (^C) or via the sphone_error 
     function.  
     The sphone_error function prints an error message, then calls 
     all exit functions, sets the sphone_exit variable, and then returns. 
     The signal error function call all exit functions, sets the sphone_exit
     variable and returns.
     The returns from both methods should ultimately lead to an exit in the
     top-level function, such as eventloop. If not, sphone will (hopefully) 
     exit again, and this time, the two exit functions unconditionally exits.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>

#include "sphone.h"

/*
 * Types
 */
struct err_handler{
    struct err_handler *eh_next;  /* Next error function */
    exit_fn            *eh_fn;    /* Exit function */
    void               *eh_arg;   /* Argument to Exit function */
    char               eh_string[32]; /* string for debugging */
};

/*
 * Exported Variables
 */
unsigned int sphone_exit = 0;

/*
 * Local Variables
 */
static struct err_handler *Err_handler = NULL;

static int
exit_handler()
{
    struct err_handler *eh;

    for (eh=Err_handler; eh; eh=eh->eh_next){
	dbg_print(DBG_EXIT, "exit_handler: %s\n", eh->eh_string);
	(*eh->eh_fn)(eh->eh_arg);
    }
    dbg_print(DBG_EXIT, "exit_handler done\n");
    return 0;
}


/*
 * Register an exit handler.
 * The exit handlers will be called in reverse order as they were called:
 * that is, last registered, first called.
 */
int
exit_register(exit_fn *fn, void *arg, char *string)
{
    struct err_handler *eh;

    dbg_print(DBG_EXIT, "exit_register: %s\n", string);
    eh = (struct err_handler *)smalloc(sizeof(struct err_handler));
    if (eh==NULL){
	sphone_error("exit_register: %s", strerror(errno));
	return -1;
    }
    memset(eh, 0, sizeof(struct err_handler));
    eh->eh_next = Err_handler;
    Err_handler = eh;
    eh->eh_fn = fn;
    eh->eh_arg = arg;
    strcpy(eh->eh_string, string);
    return 0;
}

/*
 * Error handling.
 * First time: 
 *    try to call all exit functions, 
 *    then we just return. This should go all the way to the top and exit.
 * Second time:
 *    just exit - we have been here before.
 */
int
sphone_verror(char *template, va_list args)
{
    char buffer[512];

    if (sphone_exit > 1){ /* We have been twice before */
	dbg_print(DBG_EXIT, "sphone_error: Exiting again\n");
	exit(0);
    }
    vsprintf(buffer, template, args);
    fprintf(stderr,"Sphone error: %s\n", buffer);
    if (sphone_exit){ /* We have been once before */
	dbg_print(DBG_EXIT, "sphone_error: Exiting again\n");
	exit(0);
    }
    sphone_exit++;
    exit_handler();
    return 0;
}

/* Error function with variable argument list */
int
sphone_error(char *template, ...)
{
    va_list args;

    va_start(args,template);
    sphone_verror(template, args);
    va_end(args);
    return 0;
}

/*
 * User has typed ^C or similar. 
 * Remember: on interrupt stack.
 * Alternatively: set a variable and let eventloop exit on normal stack.
 */
void
sphone_signal_exit(int arg)
{

    if (sphone_exit){ /* We have been here before */
	dbg_print(DBG_EXIT, "sphone_signal_exit: Exiting again\n");
	exit(0);
    }
    dbg_print(DBG_EXIT, "sphone_signal_exit: Exiting sphone\n");
    exit_handler();
    sphone_exit++;
    exit(0); /* Alternatively: return, check for EINTR in sphone_error, etc */
}

int
sphone_warning(char *template, ...)
{
    va_list args;
    char buffer[512];

    va_start(args,template);
    vsprintf(buffer, template, args);
    fprintf(stderr,"Sphone warning: %s\n", buffer);
    va_end(args);
    return 0;
}
