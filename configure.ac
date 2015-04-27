AC_INIT(include/sphone.h)
AC_CONFIG_HEADER(include/config.h)
AC_CANONICAL_TARGET
AC_DEFINE(AUTOCONF)
AC_DEFINE(DEBUG)
AC_SUBST(INCLUDES)
AC_SUBST(SUBDIRS, "src coding audio apps utils")
AC_SUBST(RES) # Resource files (windoze only) (hack)
#AC_PROG_CC()
# OBJECT should be defined AC_PROG_CC but not on linserv
# (autoconf -v 13 and linux 2.2.14-5.0)
AC_SUBST(OBJEXT)  # XXX
OBJEXT="o"	  # XXX

AC_ARG_ENABLE(vc, 
[  --enable-vc             Enable vc (Microsoft Visual C Compiler)], VC=$enableval)
if test "$VC" = "yes"; then
   # Microsoft windose cl compiler
   # Set environment variable PATH to where cl.exe is, 
   # INCLUDE and LIB to where include files and libs are, respectively
    CC=cl
    CFLAGS="/nologo"
    LDFLAGS="/nologo"
    AC_MSG_RESULT(compiler is $CC)	
    AC_PROG_RANLIB
    #  CFLAGS="${CFLAGS} -wccc-"    # Suppress "Cond. always true/false"
    OBJEXT=obj
    AC_SUBST(AREXT, lib)
    AC_SUBST(EXEEXT, ".exe")
    AC_DEFINE(WIN32)	
    AC_DEFINE(WINSOCK,2)
    LIBS=" user32.lib" # For GetMessage()
    LIBS="${LIBS} ws2_32.lib" # Winsock2
    LIBS="${LIBS} gdi32.lib ole32.lib" # Windows opening stuff
    RES="${RES} spc.res" # Windows opening stuff
    # If dsound.h is found, compile directsound module and add to libs.
#    AC_CHECK_HEADER(dsound.h, AC_DEFINE(HAVE_DIRECTSOUND) LIBS="${LIBS} dsound.lib")
    AC_DEFINE(HAVE_DIRECTSOUND) 
    LIBS="${LIBS} dsound.lib"		 
#    AC_CHECK_LIB(dsound, XXX)
    #      LDFLAGS="-g"
    AC_SUBST(AR, "rm \$@; tlib")
    AC_SUBST(ARCRFLAGS, "/P64")
    AC_SUBST(AR_USEOBJS,"+\`echo \$^ | sed -e 's/ / +/g' | sed -e 's/+ //g'\`")
else
    AC_PROG_CC()
    AC_PROG_CPP
    AC_SUBST(AREXT,a)
    AC_PROG_RANLIB
    if test "$GCC" = "yes"; then
       AC_MSG_RESULT(compiler is $CC)	
       CFLAGS="-g -Wall -O2 -x c"
       AC_CHECK_LIB(m, main)
       AR="ar"
       ARCRFLAGS="cr"
       AR_USEOBJS="\$^"
    else
       AC_MSG_RESULT(-- $CC is unknown)	
    fi
fi

# Extra codecs
AC_SUBST(CODECS, "")
ilbc=yes
AC_MSG_CHECKING(whether ilbc codec is enabled)
AC_ARG_ENABLE(ilbc, 
[  --disable-ilbc             disable iLBC coding], ilbc=$enableval, ilbc=yes)
AC_MSG_RESULT($ilbc)
if test "$ilbc" = "yes"; then
   CODECS="${CODECS} iLBC_1.1.0"
   AC_DEFINE(HAVE_ILBC)
   if test "$VC" = "yes"; then
      LIBS="${LIBS} \${top_srcdir}/coding/iLBC_1.1.0/libiLBC.lib"
   else
      LIBS="${LIBS} -L \${top_srcdir}/coding/iLBC_1.1.0 -liLBC"
   fi
   INCLUDES="${INCLUDES} -I\${top_srcdir}/coding/iLBC_1.1.0"
fi

AC_MSG_CHECKING(whether sip is enabled)
AC_ARG_ENABLE(sip, 
[  --enable-sip             enable sip signaling], sip=$enableval, sip=no)
AC_MSG_RESULT($sip)
if test "$sip" = "yes"; then
   AC_DEFINE(HAVE_SIP)
   INCLUDES="${INCLUDES} -I\${top_srcdir}/sip"
   if test "$VC" = "yes"; then
      LIBS="${LIBS} \${top_srcdir}/coding/sip/libsip.lib"
   else
      LIBS="${LIBS} -L \${top_srcdir}/sip -lsip"
   fi
   SUBDIRS="${SUBDIRS} sip"
fi


#AC_ARG_ENABLE(gtk2, [  --enable-gtk2             Enable GTK2], GTK2=$enableval)
#if test "$GTK2" = "yes"; then
#   AC_CHECK_LIB(gtk, gtk_main)
#fi


dnl ----------------------------
dnl check sa_len of sockaddr
dnl ----------------------------
AC_MSG_CHECKING(whether struct sockaddr has a sa_len field)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
],[static struct sockaddr ac_i;int ac_j = sizeof (ac_i.sa_len);],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SA_LEN)],
 AC_MSG_RESULT(no))

dnl ----------------------------
dnl check sin_len of sockaddr_in
dnl ----------------------------
AC_MSG_CHECKING(whether struct sockaddr_in has a sin_len field)
AC_TRY_COMPILE([#include <sys/types.h>
#include <netinet/in.h>
],[static struct sockaddr_in ac_i;int ac_j = sizeof (ac_i.sin_len);],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SIN_LEN)],
 AC_MSG_RESULT(no))

AC_CHECK_FILES(/dev/audio)
AC_CHECK_HEADERS(sys/audioio.h)

# does not work on win
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, xdr_char)

AC_CHECK_FUNCS(inet_aton strsep sigaction sigvec bcopy memset)

# Doesnt work yet: How do you check for SIOCCIFCONF/ifconf?
AC_CHECK_TYPES(struct ifconf)

AC_OUTPUT(Makefile src/Makefile coding/Makefile audio/Makefile apps/Makefile utils/Makefile coding/iLBC_1.1.0/Makefile sip/Makefile)

