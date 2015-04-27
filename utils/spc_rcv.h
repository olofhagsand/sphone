/*-----------------------------------------------------------------------------
  File:   spc_rcv.h
  Description: sphone server - client side - receive data
  this file.
  Author: Olof Hagsand
  CVS Version: $Id: spc_rcv.h,v 1.7 2004/12/27 17:34:05 olof Exp $
 
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


#ifndef _SPC_RCV_H_
#define _SPC_RCV_H_

/*
 * Types
 */
struct sp_rcv_session{
    uint32_t             rs_testid;
    struct timeval       rs_duration; /* How long to rcv */
    struct timeval       rs_start;    /* When started */
    struct sockaddr_in   rs_srcaddr;  /* From where */
    struct spc_info     *rs_spc;     /* backpointer to spc struct */
    struct coding_params rs_coding; /* audio coding parameters */
    uint32_t             rs_rtp_ssrc; /* sending src id */
    struct sdhdr_start   rs_hdr;      /* sp hdr used for report */
};

/*
 * Prototypes
 */
int start_receiver(struct spc_info *spc, struct sp_proto *sp);
int rcv_data_input(int s, struct spc_info *spc, struct sp_rcv_session *rs);
int send_rcv_report(struct spc_info *spc, struct sdhdr_start *hdr_orig);

#endif /* _SPC_RCV_H_ */


