/*
 * lib.c - a library of common functions
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
#include <sys/stat.h>
#include <sys/wait.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "lib.h"

/*@noreturn@*/
static	void	omsg(int, const char *, va_list);

/*
 * Exit the shell utility child process on error w/ the
 * specified exit status and any specified message.
 */
void
uerr(pid_t pid, int es, const char *msgfmt, ...)
{
	va_list va;

	va_start(va, msgfmt);
	omsg(FD2, msgfmt, va);
	va_end(va);
	(getpid() == pid) ? exit(es) : _exit(es);
}

/*
 * Print any specified message to the file descriptor pfd.
 */
void
fd_print(int pfd, const char *msgfmt, ...)
{
	va_list va;

	va_start(va, msgfmt);
	omsg(pfd, msgfmt, va);
	va_end(va);
}

/*
 * Create and output the message specified by err() or fd_print() to
 * the file descriptor ofd.  A diagnostic is written to FD2 on error.
 */
static void
omsg(int ofd, const char *msgfmt, va_list va)
{
	int r;
	char fmt[FMTSIZE];
	char msg[MSGSIZE];
	const char *e;

	e = "omsg: Internal error\n";
	r = snprintf(fmt, sizeof(fmt), "%s", msgfmt);
	if (r > 0 && r < (int)sizeof(fmt)) {
		r = vsnprintf(msg, sizeof(msg), fmt, va);
		if (r > 0 && r < (int)sizeof(msg)) {
			if (write(ofd, msg, strlen(msg)) == -1)
				(void)write(FD2, e, strlen(e));
		} else
			(void)write(FD2, e, strlen(e));
	} else
		(void)write(FD2, e, strlen(e));
}
