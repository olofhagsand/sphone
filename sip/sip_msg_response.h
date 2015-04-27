/*-----------------------------------------------------------------------------
  File:   sip_msg_response.h
  Description: SIP responses
  Author: Olof Hagsand
  CVS Version: $Id: sip_msg_response.h,v 1.1 2005/01/11 08:52:08 olof Exp $
 
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
#ifndef _SIP_MSG_RESPONSE_H_
#define _SIP_MSG_RESPONSE_H_

/*
 * Constants
 */

/*
 * Types
 */


/*
 * Prototypes
 */ 
int sip_parse_response(struct sip_msg *sm, char *line);

#endif  /* _SIP_MSG_RESPONSE_H_ */


