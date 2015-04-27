/*-----------------------------------------------------------------------------
  File:   sip_dialog.h
  Description: SIP dialog.
  Author: Olof Hagsand
  CVS Version: $Id: sip_dialog.h,v 1.3 2005/01/11 08:45:58 olof Exp $
 
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
 * A dialog is identified by a call identifier, local tag, and a remote tag.
*/
#ifndef _SIP_DIALOG_H_
#define _SIP_DIALOG_H_

/*
 * Constants
 */
#define SIP_CSEQ_START 101

/*
 * Types
 */
/* Order is significant, see coding_mapping variable in sphone_coding.c */
enum sip_dialog_state{
    SD_INIT
}; 

struct sip_dialog{
    struct sip_transaction *sd_transaction; /* transactions (only one) */
    struct sip_client      *sd_client;      /* backpointer */
    enum sip_dialog_state   sd_state;
    char                    sd_peer_disp[DISP_LEN]; /* src display */
    char                    sd_peer_uri[URI_LEN]; /* dst URI */
    char                    sd_tag[TAG_LEN]; /* src tag */
    char                    sd_peer_tag[TAG_LEN]; /* dst tag */
    char                    sd_call_id[TAG_LEN]; /* call id */
    uint32_t                sd_cseq[SM_RESPONSE];/* command sequence */
};


/*
 * Prototypes
 */ 
struct sip_dialog* sip_dialog_create(struct sip_client *sc, char *peer_uri);
int sip_dialog_free(struct sip_dialog *sd);

#endif  /* _SIP_DIALOG_H_ */


