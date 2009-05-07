/*
 * err.c - shell and utility error-handling routines
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

#include "defs.h"
#include "err.h"

#define	FMTSIZE		64
#define	MSGSIZE		(FMTSIZE + LINEMAX)

#define	UTILNAME	"unknown"

static	void		(*myerrexit)(int)	= NULL;
/*@observer@*/
static	const char	*myname			= (char *)-1;
static	pid_t		mypid			= -1;

static	void		wmsg(int, const char *, va_list);

/*
 * Handle all errors for the calling process.  This includes printing
 * any specified non-NULL message to the standard error and calling
 * the error exit function pointed to by the global myerrexit.
 * This function may or may not return.
 */
void
err(int es, const char *msgfmt, ...)
{
	va_list va;

	if (msgfmt != NULL) {
		va_start(va, msgfmt);
		wmsg(FD2, msgfmt, va);
		va_end(va);
	}
	if (myerrexit == NULL) {
		fd_print(FD2, FMT1S, "err: Invalid myerrexit function pointer");
		abort();
	}

#ifdef	DEBUG
	fd_print(FD2, "err: Call (*myerrexit)(%d);\n", es);
#endif

	(*myerrexit)(es);
}

/*
 * Print any specified non-NULL message to the file descriptor pfd.
 */
void
fd_print(int pfd, const char *msgfmt, ...)
{
	va_list va;

	if (msgfmt != NULL) {
		va_start(va, msgfmt);
		wmsg(pfd, msgfmt, va);
		va_end(va);
	}
}

/*
 * Return a pointer to the global myname on success.
 * Return a pointer to "(null)" on error.
 *
 * NOTE: Must call setmyname(argv[0]) first.
 */
const char *
getmyname(void)
{

	if (myname == (char *)-1)
		return "(null)";

	return myname;
}

/*
 * Return the global mypid on success.
 * Return 0 on error.
 *
 * NOTE: Must call setmypid(getpid()) first.
 */
pid_t
getmypid(void)
{

	if (mypid == -1)
		return 0;

	return mypid;
}

/*
 * Set the global myerrexit to the function pointed to by func.
 */
void
setmyerrexit(void (*func)(int))
{

	if (func == NULL)
		return;

	myerrexit = func;
}

/*
 * Set the global myname to the basename of the string pointed to by s.
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
 * Set the global mypid to the process ID p.
 */
void
setmypid(const pid_t p)
{

	if (mypid != -1)
		return;

	mypid = p;
}

/*
 * Cause all shell utility processes to exit appropriately on error.
 * This function is called by err() and never returns.
 */
void
util_errexit(int es)
{

#ifdef	DEBUG
	fd_print(FD2, "util_errexit: es == %d: Call %s(%d);\n",
	    es, (getpid() == getmypid()) ? "exit" : "_exit", es);
#endif

	EXIT(es);
}

/*
 * Write the specified message to the file descriptor wfd.
 * A diagnostic is written to FD2 on error.
 */
static void
wmsg(int wfd, const char *msgfmt, va_list va)
{
	int r;
	char fmt[FMTSIZE];
	char msg[MSGSIZE];
	const char *e;

	e = "wmsg: Invalid message\n";
	r = snprintf(fmt, sizeof(fmt), "%s", msgfmt);
	if (r >= 1 && r < (int)sizeof(fmt)) {
		r = vsnprintf(msg, sizeof(msg), fmt, va);
		if (r >= 0 && r < (int)sizeof(msg)) {
			if (write(wfd, msg, strlen(msg)) == -1)
				(void)write(FD2, e, strlen(e));
		} else
			(void)write(FD2, e, strlen(e));
	} else
		(void)write(FD2, e, strlen(e));
}
