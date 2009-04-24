/*
 * lib.c - a common library for osh and utilities
 */
/*-
 * Copyright (c) 2004-2009
 *	Jeffrey Allen Neitzel <jan (at) v6shell (dot) org>.
 *	All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY JEFFREY ALLEN NEITZEL ``AS IS'', AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL JEFFREY ALLEN NEITZEL BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	@(#)$Id$
 */

#ifndef	lint
#include "rcsid.h"
OSH_RCSID("@(#)$Id$");
#endif	/* !lint */

#include "config.h"

#include <sys/types.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "lib.h"

#define	FMTSIZE		64
#define	MSGSIZE		(FMTSIZE + LINEMAX)

#define	UTILNAME	"unknown"	/* used in setmyname() */

/*@observer@*/
static	const char	*myname = (char *)-1;
static	pid_t		mypid   = -1;

/*
 * Print any specified message to the file descriptor pfd.
 */
void
fd_print(int pfd, const char *msgfmt, ...)
{
	va_list va;

	va_start(va, msgfmt);
	wmsg(pfd, msgfmt, va);
	va_end(va);
}

/*
 * Return the global myname for use in diagnostics.
 * Must call the setmyname() function first.
 */
const char *
getmyname(void)
{

	if (myname == (char *)-1) {
		fd_print(FD2, "getmyname: Must call setmyname()\n");
		return "(null)";
	}

	return myname;
}

/*
 * Set the global myname from the string pointed to by s.
 */
void
setmyname(const char *s)
{
	const char *p;

	if (myname != (char *)-1)
		return;

	if (s != NULL && *s != '\0') {
		if ((p = strrchr(s, '/')) != NULL)
			p++;
		else
			p = s;
	} else
		p = UTILNAME;

	myname = p;
}

/*
 * Return the global mypid for use in diagnostic processing.
 * Must call the setmypid() function first.
 */
pid_t
getmypid(void)
{

	if (mypid == -1)
		fd_print(FD2, "getmypid: Must call setmypid()\n");

	return mypid;
}

/*
 * Set the global mypid from the process ID p.
 */
void
setmypid(const pid_t p)
{

	if (mypid != -1)
		return;

	mypid = p;
}

/*
 * Exit the shell utility process on error w/ the
 * specified exit status and any specified message.
 */
void
uerr(int es, const char *msgfmt, ...)
{
	va_list va;

	va_start(va, msgfmt);
	wmsg(FD2, msgfmt, va);
	va_end(va);

	fd_print(FD2, "uerr: getpid() == %u, mypid == %u, %s(%d)\n",
	    (unsigned)getpid(), (unsigned)mypid,
	    (getpid() == mypid) ? "exit" : "_exit", es);

	(getpid() == mypid) ? exit(es) : _exit(es);
}

/*
 * Write the specified message to the file descriptor wfd.
 * A diagnostic is written to FD2 on error.
 */
void
wmsg(int wfd, const char *msgfmt, va_list va)
{
	int r;
	char fmt[FMTSIZE];
	char msg[MSGSIZE];
	const char *e;

	e = "wmsg: Internal error\n";
	r = snprintf(fmt, sizeof(fmt), "%s", msgfmt);
	if (r > 0 && r < (int)sizeof(fmt)) {
		r = vsnprintf(msg, sizeof(msg), fmt, va);
		if (r > 0 && r < (int)sizeof(msg)) {
			if (write(wfd, msg, strlen(msg)) == -1)
				(void)write(FD2, e, strlen(e));
		} else
			(void)write(FD2, e, strlen(e));
	} else
		(void)write(FD2, e, strlen(e));
}
