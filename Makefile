# Makefile for osh
#
# @(#)$Id$
#
# Begin CONFIGURATION
#
# See the INSTALL file for build and install instructions.
#

#
# Choose where and how to install the binaries and manual pages.
#
DESTDIR?=
PREFIX?=	/usr/local
BINDIR?=	$(PREFIX)/bin
MANDIR?=	$(PREFIX)/man/man1
SYSCONFDIR?=	$(PREFIX)/etc
#BINGRP=		-g bin
BINMODE=	-m 0555
#MANGRP=		-g bin
MANMODE=	-m 0444

#
# Build utilities (SHELL must be POSIX-compliant)
#
INSTALL?=	/usr/bin/install
SHELL=		/bin/sh

#
# Preprocessor, compiler, and linker flags
#
#	If the compiler gives errors about any of flags specified
#	by `OPTIONS' or `WARNINGS' below, comment the appropriate
#	line(s) with a `#' character to fix the compiler errors.
#	Then, try to build again by doing a `make clean ; make'.
#
#CPPFLAGS=
OPTIONS=	-std=c99 -pedantic
#OPTIONS+=	-fstack-protector
WARNINGS=	-Wall -W
#WARNINGS+=	-Wstack-protector
#CFLAGS+=	-g
CFLAGS+=	-O2 $(OPTIONS) $(WARNINGS)
#LDFLAGS+=	-static

#
# End CONFIGURATION
#
# !!! ================= Developer stuff below... ================= !!!

#
# Adjust CFLAGS and LDFLAGS for MOXARCH if needed.
#
MOXARCH?=
CFLAGS+=	$(MOXARCH)
LDFLAGS+=	$(MOXARCH)

#
# The following specifies the osh date and version:
#
#	osh-YYYYMMDD-current == development snapshot
#	osh-YYYYMMDD         == official release
#
OSH_DATE=	April 18, 2009
OSH_VERSION=	osh-20090418-current

OSH=	osh
SH6=	sh6 glob6
UBIN=	fd2 goto if
PEXSRC=	pexec.h pexec.c
SIGSRC=	sasignal.h sasignal.c
OBJ=	fd2.o glob6.o goto.o if.o osh.o pexec.o sasignal.o sh6.o util.o v.o
OSHMAN=	osh.1.out
SH6MAN=	sh6.1.out glob6.1.out
UMAN=	fd2.1.out goto.1.out if.1.out
MANALL=	$(OSHMAN) $(SH6MAN) $(UMAN)

DEFS=	-DOSH_VERSION='"$(OSH_VERSION)"' -DSYSCONFDIR='"$(SYSCONFDIR)"'

.SUFFIXES: .1 .1.out .c .o
.1.1.out:
	sed 's|@OSH_DATE@|$(OSH_DATE)| ; s|@OSH_VERSION@|$(OSH_VERSION)| ; \
	     s|@SYSCONFDIR@|$(SYSCONFDIR)|' <$< >$@

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(DEFS) $<

#
# Build targets
#
all: oshall sh6all

oshall: $(OSH) $(OSHMAN) $(UMAN)

sh6all: $(SH6) $(SH6MAN) utils

utils: $(UBIN) $(UMAN)

osh: config.h defs.h rcsid.h sh.h v.c util.c $(PEXSRC) $(SIGSRC) osh.c
	@$(MAKE) $@bin

sh6: config.h defs.h rcsid.h sh.h v.c $(PEXSRC) $(SIGSRC) sh6.c
	@$(MAKE) $@bin

glob6: config.h defs.h rcsid.h v.c $(PEXSRC) glob6.c
	@$(MAKE) $@bin

if: config.h defs.h rcsid.h v.c $(PEXSRC) $(SIGSRC) if.c
	@$(MAKE) $@bin

goto: config.h defs.h rcsid.h v.c goto.c
	@$(MAKE) $@bin

fd2: config.h defs.h rcsid.h v.c $(PEXSRC) fd2.c
	@$(MAKE) $@bin

$(OBJ)                               : config.h defs.h rcsid.h
osh.o sh6.o util.o                   : sh.h
fd2.o glob6.o if.o osh.o sh6.o util.o: pexec.h
if.o osh.o sh6.o                     : sasignal.h
pexec.o                              : $(PEXSRC)
sasignal.o                           : $(SIGSRC)

config.h: mkconfig
	$(SHELL) ./mkconfig

oshbin: v.o util.o pexec.o sasignal.o osh.o
	$(CC) $(LDFLAGS) -o osh v.o osh.o util.o pexec.o sasignal.o $(LIBS)

sh6bin: v.o pexec.o sasignal.o sh6.o
	$(CC) $(LDFLAGS) -o sh6 v.o sh6.o pexec.o sasignal.o $(LIBS)

glob6bin: v.o pexec.o glob6.o
	$(CC) $(LDFLAGS) -o glob6 v.o glob6.o pexec.o $(LIBS)

ifbin: v.o pexec.o sasignal.o if.o
	$(CC) $(LDFLAGS) -o if v.o if.o pexec.o sasignal.o $(LIBS)

gotobin: v.o goto.o
	$(CC) $(LDFLAGS) -o goto v.o goto.o $(LIBS)

fd2bin: v.o pexec.o fd2.o
	$(CC) $(LDFLAGS) -o fd2 v.o fd2.o pexec.o $(LIBS)

#
# Install targets
#
DESTBINDIR=	$(DESTDIR)$(BINDIR)
DESTMANDIR=	$(DESTDIR)$(MANDIR)
install: install-oshall install-sh6all

install-oshall: oshall install-osh install-uman

install-sh6all: sh6all install-sh6 install-utils

install-utils: install-ubin install-uman

install-osh: $(OSH) $(OSHMAN) install-dest
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) osh         $(DESTBINDIR)/osh
	$(INSTALL) -c    $(MANGRP) $(MANMODE) osh.1.out   $(DESTMANDIR)/osh.1

install-sh6: $(SH6) $(SH6MAN) install-dest
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) sh6         $(DESTBINDIR)/sh6
	$(INSTALL) -c    $(MANGRP) $(MANMODE) sh6.1.out   $(DESTMANDIR)/sh6.1
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) glob6       $(DESTBINDIR)/glob6
	$(INSTALL) -c    $(MANGRP) $(MANMODE) glob6.1.out $(DESTMANDIR)/glob6.1

install-ubin: $(UBIN) install-dest
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) fd2         $(DESTBINDIR)/fd2
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) goto        $(DESTBINDIR)/goto
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) if          $(DESTBINDIR)/if

install-uman: $(UMAN) install-dest
	$(INSTALL) -c    $(MANGRP) $(MANMODE) fd2.1.out   $(DESTMANDIR)/fd2.1
	$(INSTALL) -c    $(MANGRP) $(MANMODE) goto.1.out  $(DESTMANDIR)/goto.1
	$(INSTALL) -c    $(MANGRP) $(MANMODE) if.1.out    $(DESTMANDIR)/if.1

install-dest:
	test -d $(DESTBINDIR) || { umask 0022 && mkdir -p $(DESTBINDIR) ; }
	test -d $(DESTMANDIR) || { umask 0022 && mkdir -p $(DESTMANDIR) ; }

#
# Cleanup targets
#
clean-obj:
	rm -f $(OBJ)

clean: clean-obj
	rm -f $(OSH) $(SH6) $(UBIN) $(MANALL) config.h

#
# Create osh source package tarball and checksums.
#
OSP=	$(OSH_VERSION)
OSPROOT=./.osp
OSPDIR=	$(OSPROOT)/$(OSP)
OSPSUMS=$(OSP).sums
OSPTAR=	$(OSP).tar
OSPTGZ=	$(OSPTAR).gz
osp: clean
	rm -rf $(OSPROOT)
	umask 0022 && mkdir -p $(OSPDIR) && cp -p * $(OSPDIR)/ && \
	chmod 0644 $(OSPDIR)/* && cd $(OSPROOT) && tar cf $(OSPTAR) $(OSP) && \
	gzip -9 $(OSPTAR) && mksums $(OSPTGZ) > $(OSPSUMS)
	rm -rf $(OSPDIR)
