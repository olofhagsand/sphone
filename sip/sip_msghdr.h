/*-----------------------------------------------------------------------------
  File:   sip_msghdr.h
  Description: SIP messages headers.
  Author: Olof Hagsand
  CVS Version: $Id: sip_msghdr.h,v 1.4 2005/01/31 18:37:25 olof Exp $
 
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
#ifndef _SIP_MSGHDR_H_
#define _SIP_MSGHDR_H_

/*
 * Constants
 */

/*
 * Types
 */
enum msghdr_type{
    MH_Accept,
    MH_Accept_Encoding,
    MH_Accept_Language,
    MH_Alert_Info,
    MH_Allow,
    MH_Authentication_Info,
    MH_Authorization,
    MH_Call_ID,
    MH_Call_Info,
    MH_Contact,
    MH_Content_Disposition,
    MH_Content_Encoding,
    MH_Content_Language,
    MH_Content_Length,
    MH_Content_Type,
    MH_CSeq,
    MH_Date,
    MH_Error_Info,
    MH_Expires,
    MH_From,
    MH_In_Reply_To,
    MH_Max_Forwards,
    MH_MIME_Version,
    MH_Min_Expires,
    MH_Organization,
    MH_Priority,
    MH_Proxy_Authenticate,
    MH_Proxy_Authorization,
    MH_Proxy_Require,
    MH_Record_Route,
    MH_Reply_To,
    MH_Require,
    MH_Retry_After,
    MH_Route,
    MH_Server,
    MH_Subject,
    MH_Supported,
    MH_Timestamp,
    MH_To,
    MH_Unsupported,
    MH_User_Agent,
    MH_Via,
    MH_Warning,
    MH_WWW_Authenticate
};

struct msghdr_Accept{
};

struct msghdr_Accept_Encoding{
};

struct msghdr_Accept_Language{
};

struct msghdr_Alert_Info{
};

struct msghdr_Allow{
};

struct msghdr_Authentication_Info{
};

struct msghdr_Authorization{
};

struct msghdr_Call_ID{
    char               mci_call_id[TAG_LEN];
    struct in_addr     mci_in_addr;
};

struct msghdr_Call_Info{
};


struct msghdr_Contact{
    char               mc_disp[DISP_LEN];
    struct sockaddr_in mc_addr;
};

struct msghdr_Content_Disposition{
};

struct msghdr_Content_Encoding{
};

struct msghdr_Content_Language{
};

struct msghdr_Content_Length{
    size_t             mcl_len;
};

struct msghdr_Content_Type{
};

struct msghdr_CSeq{
    uint32_t           mc_cseq;
    char               mc_type[SIP_FLEN];
};
struct msghdr_Date{
};

struct msghdr_Error_Info{
};

struct msghdr_Expires{
};

struct msghdr_From{
    char               mf_disp[DISP_LEN];
    char               mf_uri[URI_LEN];
    char               mf_tag[TAG_LEN];
};
struct msghdr_In_Reply_To{
};

struct msghdr_Max_Forwards{
};

struct msghdr_MIME_Version{
};

struct msghdr_Min_Expires{
};

struct msghdr_Organization{
};

struct msghdr_Priority{
};

struct msghdr_Proxy_Authenticate{
};

struct msghdr_Proxy_Authorization{
};

struct msghdr_Proxy_Require{
};

struct msghdr_Record_Route{
};

struct msghdr_Reply_To{
};

struct msghdr_Require{
};

struct msghdr_Retry_After{
};

struct msghdr_Route{
};

struct msghdr_Server{
};

struct msghdr_Subject{
};

struct msghdr_Supported{
};

struct msghdr_Timestamp{
};

struct msghdr_To{
    char               mt_uri[URI_LEN];
};

struct msghdr_Unsupported{
};

struct msghdr_User_Agent{
};

struct msghdr_Warning{
};

struct msghdr_WWW_Authenticate{
};

struct msghdr_Via{
    struct sockaddr_in mv_addr;
    char               mv_branch[BRANCH_LEN];
};

union sip_msghdr1{
    struct msghdr_Accept           _mh_Accept;
    struct msghdr_Accept_Encoding  _mh_Accept_Encoding;
    struct msghdr_Accept_Language  _mh_Accept_Language;
    struct msghdr_Alert_Info       _mh_Alert_Info;
    struct msghdr_Allow            _mh_Allow;
    struct msghdr_Authentication_Info _mh_Authentication_Info;
    struct msghdr_Authorization    _mh_Authorization;
    struct msghdr_Call_ID          _mh_Call_ID;
    struct msghdr_Call_Info        _mh_Call_Info;
    struct msghdr_Contact          _mh_Contact;
    struct msghdr_Content_Disposition _mh_Content_Disposition;
    struct msghdr_Content_Encoding _mh_Content_Encoding;
    struct msghdr_Content_Language _mh_Content_Language;
    struct msghdr_Content_Length   _mh_Content_Length;
    struct msghdr_Content_Type     _mh_Content_Type;
    struct msghdr_CSeq             _mh_CSeq;
    struct msghdr_Date             _mh_Date;
    struct msghdr_Error_Info       _mh_Error_Info;
    struct msghdr_Expires          _mh_Expires;
    struct msghdr_From             _mh_From;
    struct msghdr_In_Reply_To      _mh_In_Reply_To;
    struct msghdr_Max_Forwards     _mh_Max_Forwards;
    struct msghdr_MIME_Version     _mh_MIME_Version;
    struct msghdr_Min_Expires      _mh_Min_Expires;
    struct msghdr_Organization     _mh_Organization;
    struct msghdr_Priority         _mh_Priority;
    struct msghdr_Proxy_Authenticate _mh_Proxy_Authenticate;
    struct msghdr_Proxy_Authorization _mh_Proxy_Authorization;
    struct msghdr_Proxy_Require    _mh_Proxy_Require;
    struct msghdr_Record_Route     _mh_Record_Route;
    struct msghdr_Reply_To         _mh_Reply_To;
    struct msghdr_Require          _mh_Require;
    struct msghdr_Retry_After      _mh_Retry_After;
    struct msghdr_Route            _mh_Route;
    struct msghdr_Server           _mh_Server;
    struct msghdr_Subject          _mh_Subject;
    struct msghdr_Supported        _mh_Supported;
    struct msghdr_Timestamp        _mh_Timestamp;
    struct msghdr_To               _mh_To;
    struct msghdr_Unsupported      _mh_Unsupported;
    struct msghdr_User_Agent       _mh_User_Agent;
    struct msghdr_Via              _mh_Via;
    struct msghdr_Warning          _mh_Warning;
    struct msghdr_WWW_Authenticate  _mh_WWW_Authenticate;
};

struct sip_msghdr{
    struct sip_msghdr *smh_next;  /* all msghdr in linked list */
    enum msghdr_type   smh_type;  /* type of the msghdr below */
    union sip_msghdr1  _smh_msghdr;
};

/* access macros, one for each union element */

#define mh_Accept             _smh_msghdr._mh_Accept
#define mh_Accept_Encoding    _smh_msghdr._mh_Accept_Encoding
#define mh_Accept_Language    _smh_msghdr._mh_Accept_Language
#define mh_Alert_Info         _smh_msghdr._mh_Alert_Info
#define mh_Allow              _smh_msghdr._mh_Allow
#define mh_Authentication_Info  _smh_msghdr._mh_Authentication_Info
#define mh_Authorization      _smh_msghdr._mh_Authorization
#define mh_Call_ID            _smh_msghdr._mh_Call_ID
#define mh_Call_Info          _smh_msghdr._mh_Call_Info
#define mh_Contact            _smh_msghdr._mh_Contact
#define mh_Content_Disposition _smh_msghdr._mh_Content_Disposition
#define mh_Content_Encoding   _smh_msghdr._mh_Content_Encoding
#define mh_Content_Language   _smh_msghdr._mh_Content_Language
#define mh_Content_Length     _smh_msghdr._mh_Content_Length
#define mh_Content_Type       _smh_msghdr._mh_Content_Type
#define mh_CSeq               _smh_msghdr._mh_CSeq
#define mh_Date               _smh_msghdr._mh_Date
#define mh_Error_Info         _smh_msghdr._mh_Error_Info
#define mh_Expires            _smh_msghdr._mh_Expires
#define mh_From               _smh_msghdr._mh_From
#define mh_In_Reply_To        _smh_msghdr._mh_In_Reply_To
#define mh_Max_Forwards       _smh_msghdr._mh_Max_Forwards
#define mh_MIME_Version       _smh_msghdr._mh_MIME_Version
#define mh_Min_Expires        _smh_msghdr._mh_Min_Expires
#define mh_Organization       _smh_msghdr._mh_Organization
#define mh_Priority           _smh_msghdr._mh_Priority
#define mh_Proxy_Authenticate _smh_msghdr._mh_Proxy_Authenticate
#define mh_Proxy_Authorization _smh_msghdr._mh_Proxy_Authorization
#define mh_Proxy_Require      _smh_msghdr._mh_Proxy_Require
#define mh_Record_Route       _smh_msghdr._mh_Record_Route
#define mh_Reply_To           _smh_msghdr._mh_Reply_To
#define mh_Require            _smh_msghdr._mh_Require
#define mh_Retry_After        _smh_msghdr._mh_Retry_After
#define mh_Route              _smh_msghdr._mh_Route
#define mh_Server             _smh_msghdr._mh_Server
#define mh_Subject            _smh_msghdr._mh_Subject
#define mh_Supported          _smh_msghdr._mh_Supported
#define mh_Timestamp          _smh_msghdr._mh_Timestamp
#define mh_To                 _smh_msghdr._mh_To
#define mh_Unsupported        _smh_msghdr._mh_Unsupported
#define mh_User_Agent         _smh_msghdr._mh_User_Agent
#define mh_Via                _smh_msghdr._mh_Via
#define mh_Warning            _smh_msghdr._mh_Warning
#define mh_WWW_Authenticate   _smh_msghdr._mh_WWW_Authenticate



/*
 * Prototypes
 */ 
enum msghdr_type msghdr2enum(char *str);
struct sip_msghdr *msghdr_add(struct sip_msg *, enum msghdr_type type);
/* In msghdr_pack.c */
char *msghdr_pack(char *line, size_t *len, struct sip_msghdr *smh);
/* In msghdr_parse.c */
int msghdr_parse(struct sip_msg *sm, char *line);

#endif  /* _SIP_MSGHDR_H_ */


