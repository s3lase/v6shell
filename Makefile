# Makefile for osh-current (20071125)

#
# Begin CONFIGURATION
#
# See the INSTALL file for build and install instructions.
#

#
# Specify the path name of the login(1) utility.
#
PATH_LOGIN?=	/usr/bin/login

#
# Specify the path name of the newgrp(1) utility.
#
PATH_NEWGRP?=

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
# Build utilities
#
#CC=		/usr/bin/cc
INSTALL=	/usr/bin/install

#
# Compiler and linker flags
#
#CFLAGS+=	-g
CFLAGS+=	-O2
CFLAGS+=	-std=c99
CFLAGS+=	-pedantic
CFLAGS+=	-Wall -W
#LDFLAGS+=	-static

#
# End CONFIGURATION
#

# !!! ================= Developer stuff below... ================= !!!

#
# Further adjust CFLAGS and LDFLAGS as needed according to MOX*.
#
MOXARCH?=
MOXSHELLARCH?=	$(MOXARCH)
MOXUTILSARCH?=	$(MOXARCH)
SCFLAGS=	$(CFLAGS)	$(MOXSHELLARCH)
SLDFLAGS=	$(LDFLAGS)	$(MOXSHELLARCH)
UCFLAGS=	$(CFLAGS)	$(MOXUTILSARCH)
ULDFLAGS=	$(LDFLAGS)	$(MOXUTILSARCH)

#
# X/Open System Interfaces Extension (NOTES: POSIX, required)
#
XSIE=		-D_XOPEN_SOURCE=600

#
# The following specifies the osh version:
#	osh-YYYYMMDD		== official release
#	osh-current (YYYYMMDD)	== development snapshot
#
OSH_VERSION=	osh-current (20071125)

OSH=	osh
SH6=	sh6 glob6
UTILS=	if goto fd2
PEXSRC=	pexec.h pexec.c
OBJ=	pexec.o osh.o sh6.o glob6.o if.o goto.o fd2.o
MANSRC=	osh.1.in sh6.1.in glob6.1.in if.1.in goto.1.in fd2.1.in
MANDST=	osh.1 sh6.1 glob6.1 if.1 goto.1 fd2.1

DEFS=	-DPATH_LOGIN='"$(PATH_LOGIN)"'	-DPATH_NEWGRP='"$(PATH_NEWGRP)"'
DEFS+=	-DSYSCONFDIR='"$(SYSCONFDIR)"'	-DOSH_VERSION='"$(OSH_VERSION)"'

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(XSIE) $(DEFS) $<

#
# Build targets
#
all: oshall sh6all

oshall: $(OSH) utils man

sh6all: $(SH6) utils man

utils: $(UTILS)

osh: rcsid.h $(PEXSRC) osh.c
	@rm -f pexec.o
	@$(MAKE) CFLAGS='$(SCFLAGS)' LDFLAGS='$(SLDFLAGS)' $@bin

sh6: rcsid.h $(PEXSRC) sh6.c
	@rm -f pexec.o
	@$(MAKE) CFLAGS='$(SCFLAGS)' LDFLAGS='$(SLDFLAGS)' $@bin

glob6: rcsid.h $(PEXSRC) glob6.c
	@rm -f pexec.o
	@$(MAKE) CFLAGS='$(SCFLAGS)' LDFLAGS='$(SLDFLAGS)' $@bin

if: rcsid.h $(PEXSRC) if.c
	@rm -f pexec.o
	@$(MAKE) CFLAGS='$(UCFLAGS)' LDFLAGS='$(ULDFLAGS)' $@bin

goto: rcsid.h goto.c
	@$(MAKE) CFLAGS='$(UCFLAGS)' LDFLAGS='$(ULDFLAGS)' $@bin

fd2: rcsid.h $(PEXSRC) fd2.c
	@rm -f pexec.o
	@$(MAKE) CFLAGS='$(UCFLAGS)' LDFLAGS='$(ULDFLAGS)' $@bin

$(OBJ): rcsid.h
pexec.o: $(PEXSRC)
osh.o sh6.o glob6.o if.o fd2.o: pexec.h

oshbin: pexec.o osh.o
	$(CC) $(LDFLAGS) -o osh osh.o pexec.o $(LIBS)

sh6bin: pexec.o sh6.o
	$(CC) $(LDFLAGS) -o sh6 sh6.o pexec.o $(LIBS)

glob6bin: pexec.o glob6.o
	$(CC) $(LDFLAGS) -o glob6 glob6.o pexec.o $(LIBS)

ifbin: pexec.o if.o
	$(CC) $(LDFLAGS) -o if if.o pexec.o $(LIBS)

gotobin: goto.o
	$(CC) $(LDFLAGS) -o goto goto.o $(LIBS)

fd2bin: pexec.o fd2.o
	$(CC) $(LDFLAGS) -o fd2 fd2.o pexec.o $(LIBS)

#
# Manual-page targets
#
man: $(MANDST)
$(MANDST): $(MANSRC)
	@for file in $(MANSRC) ; do \
		sed -e 's,@OSH_VERSION@,$(OSH_VERSION),' \
			-e 's,@SYSCONFDIR@,$(SYSCONFDIR),' \
				<$$file >$${file%.in} ; \
	done

#
# Install targets
#
install: install-oshall install-sh6all

install-oshall: $(OSH) utils man install-osh install-utils

install-sh6all: $(SH6) utils man install-sh6 install-utils

install-osh: $(OSH) man install-dest
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) osh     $(DESTDIR)$(BINDIR)
	$(INSTALL) -c    $(MANGRP) $(MANMODE) osh.1   $(DESTDIR)$(MANDIR)

install-sh6: $(SH6) man install-dest
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) sh6     $(DESTDIR)$(BINDIR)
	$(INSTALL) -c    $(MANGRP) $(MANMODE) sh6.1   $(DESTDIR)$(MANDIR)
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) glob6   $(DESTDIR)$(BINDIR)
	$(INSTALL) -c    $(MANGRP) $(MANMODE) glob6.1 $(DESTDIR)$(MANDIR)

install-utils: utils man install-dest
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) if      $(DESTDIR)$(BINDIR)
	$(INSTALL) -c    $(MANGRP) $(MANMODE) if.1    $(DESTDIR)$(MANDIR)
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) goto    $(DESTDIR)$(BINDIR)
	$(INSTALL) -c    $(MANGRP) $(MANMODE) goto.1  $(DESTDIR)$(MANDIR)
	$(INSTALL) -c -s $(BINGRP) $(BINMODE) fd2     $(DESTDIR)$(BINDIR)
	$(INSTALL) -c    $(MANGRP) $(MANMODE) fd2.1   $(DESTDIR)$(MANDIR)

install-dest:
	test -d $(DESTDIR)$(BINDIR) || { \
		umask 0022 ; mkdir -p $(DESTDIR)$(BINDIR) ; \
	}
	test -d $(DESTDIR)$(MANDIR) || { \
		umask 0022 ; mkdir -p $(DESTDIR)$(MANDIR) ; \
	}

#
# Cleanup targets
#
clean-obj:
	rm -f $(OBJ)

clean: clean-obj
	rm -f $(OSH) $(SH6) $(UTILS) $(MANDST)
