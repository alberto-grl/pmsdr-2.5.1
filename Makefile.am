# flags you want to pass to the C compiler & linker
AM_CFLAGS	= -Wall -O3 -D__USE_GNU -std=gnu99 -fgnu89-inline 
AM_LDFLAGS	=
AM_LIBS		= 



bin_PROGRAMS = pmsdr pmsdr_dyn

pmsdr_SOURCES = cmdparser.c pmsdr.c pmsdr_ctrl.c pmsdr_err.c Si570.c udp_cmd.c \
		cmdparser.h log.h pmsdr.h pmsdr_ctrl.h Si570.h udp_cmd.h

pmsdr_dyn_SOURCES = pmsdr.c udp_cmd.c cmdparser.c Si570.c pmsdr.h pmsdr_ctrl.h Si570.h 
# the additional libraries needed to link pmsdr_dyn
pmsdr_dyn_LDADD   = ./libpmsdr.la


lib_LTLIBRARIES = libpmsdr.la

libpmsdr_la_CFLAGS = $(AM_CFLAGS) 
libpmsdr_la_LDFLAGS = -version-info 2:0:1


libpmsdr_la_SOURCES = pmsdr_ctrl.c pmsdr_err.c Si570.c pmsdr.h pmsdr_ctrl.h

# the list of header files that belong to the library (to be installed later)
hdr_HEADERS = pmsdr.h

# where to install the headers on the system
hdrdir = $(includedir)

# man pages
man1_MANS = pmsdr.man pmsdr_dyn.man
dist_man_MANS = pmsdr.pod
#
# Generate man pages via pod2man utility
#
pmsdr.man: $(srcdir)/pmsdr.pod
	pod2man -r pmsdr  -c PMSDR $(srcdir)/pmsdr.pod > $@

pmsdr_dyn.man: $(srcdir)/pmsdr.pod
	pod2man -r pmsdr -c PMSDR  $(srcdir)/pmsdr.pod > $@



CLEANFILES = pmsdr.man pmsdr_dyn.man
