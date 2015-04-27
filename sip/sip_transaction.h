/*-----------------------------------------------------------------------------
  File:   sip_transaction.h
  Description: SIP transaction.
  Author: Olof Hagsand
  CVS Version: $Id: sip_transaction.h,v 1.2 2005/01/11 08:45:58 olof Exp $
 
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
 * From rfc 3261:
 Each transaction consists of a request that invokes a particular
   method, or function, on the server and at least one response. 

      Final Response: A response that terminates a SIP transaction, as
         opposed to a provisional response that does not.  All 2xx, 3xx,
         4xx, 5xx and 6xx responses are final.

*/
#ifndef _SIP_TRANSACTION_H_
#define _SIP_TRANSACTION_H_

/*
 * Constants
 */

/*
 * Types
 */

struct sip_transaction{
    struct sip_msg    *st_msg;      /* messages */
    struct sip_dialog *st_dialog;   /* backpointer */
    char               st_branch[BRANCH_LEN];
#ifdef notyet
    struct sip_msg    *st_request;  /* Original request */
#endif

};


/*
 * Prototypes
 */ 
struct sip_transaction* sip_transaction_create(struct sip_dialog *);
int sip_transaction_free(struct sip_transaction *st);

#endif  /* _SIP_TRANSACTION_H_ */


