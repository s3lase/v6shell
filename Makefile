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
CFLAGS+=	-Os $(OPTIONS) $(WARNINGS)
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
OSH_DATE=	May 21, 2009
OSH_VERSION=	osh-20090521-current

OSH=	osh
SH6=	sh6 glob6
UBIN=	fd2 goto if
GHEAD=	config.h defs.h rcsid.h
ERRSRC=	err.h err.c
PEXSRC=	pexec.h pexec.c
SIGSRC=	sasignal.h sasignal.c
OBJ=	err.o fd2.o glob6.o goto.o if.o osh.o pexec.o sasignal.o sh6.o util.o v.o
OSHMAN=	osh.1.out
SH6MAN=	sh6.1.out glob6.1.out
UMAN=	fd2.1.out goto.1.out if.1.out
MANALL=	$(OSHMAN) $(SH6MAN) $(UMAN)
SEDSUB=	-e 's|@OSH_DATE@|$(OSH_DATE)|' \
	-e 's|@OSH_VERSION@|$(OSH_VERSION)|' \
	-e 's|@SYSCONFDIR@|$(SYSCONFDIR)|'

DEFS=	-DOSH_VERSION='"$(OSH_VERSION)"' -DSYSCONFDIR='"$(SYSCONFDIR)"'

.SUFFIXES: .1 .1.out .c .o

.1.1.out:
	sed $(SEDSUB) <$< >$@

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(DEFS) $<

#
# Build targets
#
all: oshall sh6all

oshall: $(OSH) $(OSHMAN) $(UMAN)

sh6all: $(SH6) $(SH6MAN) utils

utils: $(UBIN) $(UMAN)

man: $(MANALL)

osh: sh.h v.c osh.c util.c $(GHEAD) $(ERRSRC) $(PEXSRC) $(SIGSRC)
	@$(MAKE) $@bin

sh6: sh.h v.c sh6.c $(GHEAD) $(ERRSRC) $(PEXSRC) $(SIGSRC)
	@$(MAKE) $@bin

glob6: v.c glob6.c $(GHEAD) $(ERRSRC) $(PEXSRC)
	@$(MAKE) $@bin

if: v.c if.c $(GHEAD) $(ERRSRC) $(PEXSRC) $(SIGSRC)
	@$(MAKE) $@bin

goto: v.c goto.c $(GHEAD) $(ERRSRC)
	@$(MAKE) $@bin

fd2: v.c fd2.c $(GHEAD) $(ERRSRC) $(PEXSRC)
	@$(MAKE) $@bin

$(OBJ)                                      : $(GHEAD)
fd2.o glob6.o goto.o if.o osh.o sh6.o util.o: err.h
fd2.o glob6.o if.o osh.o sh6.o util.o       : pexec.h
if.o osh.o sh6.o                            : sasignal.h
osh.o sh6.o util.o                          : sh.h
err.o                                       : $(ERRSRC)
pexec.o                                     : $(PEXSRC)
sasignal.o                                  : $(SIGSRC)

config.h: mkconfig
	$(SHELL) ./mkconfig

oshbin: v.o osh.o err.o util.o pexec.o sasignal.o
	$(CC) $(LDFLAGS) -o osh v.o osh.o err.o util.o pexec.o sasignal.o $(LIBS)

sh6bin: v.o sh6.o err.o pexec.o sasignal.o
	$(CC) $(LDFLAGS) -o sh6 v.o sh6.o err.o pexec.o sasignal.o $(LIBS)

glob6bin: v.o glob6.o err.o pexec.o
	$(CC) $(LDFLAGS) -o glob6 v.o glob6.o err.o pexec.o $(LIBS)

ifbin: v.o if.o err.o pexec.o sasignal.o
	$(CC) $(LDFLAGS) -o if v.o if.o err.o pexec.o sasignal.o $(LIBS)

gotobin: v.o goto.o err.o
	$(CC) $(LDFLAGS) -o goto v.o goto.o err.o $(LIBS)

fd2bin: v.o fd2.o err.o pexec.o
	$(CC) $(LDFLAGS) -o fd2 v.o fd2.o err.o pexec.o $(LIBS)

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
# Release target
#
release: clean
	rm -rf .release
	oshrelease .release $(OSH_VERSION)
