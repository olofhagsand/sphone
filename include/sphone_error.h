/*-----------------------------------------------------------------------------
  File:   sphone_error.h
  Description: Useful functions
  Author: Olof Hagsand
  CVS Version: $Id: sphone_error.h,v 1.4 2004/02/08 07:52:17 olofh Exp $
 
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


#ifndef _SPHONE_ERROR_H_
#define _SPHONE_ERROR_H_

/*
 * Types
 */
typedef int (exit_fn)(void*);


/*
 * Variables
 */
extern unsigned int sphone_exit;

/*
 * Prototypes
 */ 
int exit_register(exit_fn *fn, void *arg, char *string);
int sphone_verror(char *buffer, va_list args);
int sphone_error(char *template, ...);
int sphone_warning(char *template, ...);
void sphone_signal_exit(int arg);

#endif  /* _SPHONE_ERROR_H_ */
