/*-----------------------------------------------------------------------------
  File:   sphone_win32.h
  Description: Windoze support functions
  Author: Olof Hagsand
  CVS Version: $Id: sphone_win32.h,v 1.1 2005/01/31 18:43:30 olof Exp $
 
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

#ifndef _SPHONE_WIN32_H_
#define _SPHONE_WIN32_H_

/*
 * Prototypes
 */
#ifdef WIN32
LRESULT CALLBACK WndMainProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND create_dumb_window(HINSTANCE hInstC);
#endif /* WIN32 */

#endif /* _SPHONE_WIN32_H_ */
