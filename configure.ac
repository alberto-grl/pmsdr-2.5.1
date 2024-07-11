#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(libpmsdr, m4_esyscmd([./get_release.sh]), andrew@montefusco.com)
AM_INIT_AUTOMAKE(pmsdr, m4_esyscmd([./get_release.sh]))

# check if the source folder is correct
AC_CONFIG_SRCDIR([pmsdr.h])
##########AC_CONFIG_HEADER([config.h])
AC_CONFIG_HEADERS([config.h])


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

#
# configure platform
# On a x86 PC with linux running $host is: i686-pc-linux-gnu
#
#
case $host in

  *linux*)
    AC_MSG_NOTICE([Platform LINUX: $host])
    XXX_PRFX_DEF="/usr/local"
    export XXX_PRFX_DEF 
    ;;

  *darwin*)
    AC_MSG_NOTICE([Platform MAC OSX: $host])
    OSX_LDFLAGS=" -L/opt/local/lib"
    OSX_CFLAGS="-I/opt/local/include"
    LDFLAGS="$LDFLAGS $OSX_LDFLAGS"
    CFLAGS="$CFLAGS $OSX_CFLAGS"
    export LDFLAGS
    export CFLAGS
    XXX_PRFX_DEF="/opt/local"
    export XXX_PRFX_DEF 
    ;;

 *-*-*-*)
    AC_MSG_NOTICE([Platform unknown: $host])
    ;;

esac

AS_IF([test "x$XXX_PRFX_DEF" = x], AC_PREFIX_DEFAULT([`pwd`]), AC_PREFIX_DEFAULT(["$XXX_PRFX_DEF"]))


#
# Configure options
#
# --enable-debug[=no]
#
AC_ARG_ENABLE([debug], 
              [AS_HELP_STRING([--enable-debug], [enable debug code (default is disabled)])], 
	      [debug=yes], 
	      [debug=no])

#
# Checks for libraries.
#
AC_CHECK_LIB([m], [floor, ceil])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([readline], [main])
AC_CHECK_LIB([ncurses], [initscr])
AC_CHECK_LIB([usb-1.0], [libusb_init], [],
             [
              echo "Error! You need to have libusb-1.0 around."
              exit -1
             ])
AC_CHECK_LIB([dl], [dlopen])


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h stdbool.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday])

#
# Configure _LOG_DEB
#
AS_IF([test "x$debug" = xyes], 
   [AC_DEFINE([_LOG_DEB],[1],[Define to enable PMSDR debug feature])])





# files to be generated via autotools (.am or .in source files)
AC_OUTPUT(Makefile)


echo "
    PMSDR library $PACKAGE_NAME version $PACKAGE_VERSION
    Prefix......... ($prefix)
    C Compiler..... ($CPPFLAGS)
    Linker......... ($LD) ($LDFLAGS) ($LIBS)
    Debug build.... ($debug)
"
