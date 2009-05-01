/*
 * fd2.c - redirect from/to file descriptor 2
 */
/*-
 * Copyright (c) 2005-2009
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
#include "pexec.h"

static	bool	fd_isopen(int);
/*@noreturn@*/
static	void	usage(void);

/*
 * NAME
 *	fd2 - redirect from/to file descriptor 2
 *
 * SYNOPSIS
 *	fd2 [-e] [-f file] command [arg ...]
 *
 * DESCRIPTION
 *	See the fd2(1) manual page for full details.
 */
int
main(int argc, char **argv)
{
	bool eopt;
	int efd, fd, nfd, ofd, opt;
	char *file;

	setmyerrexit(util_errexit);
	setmyname(argv[0]);
	setmypid(getpid());

	/*
	 * Set-ID execution is not supported.
	 */
	if (geteuid() != getuid() || getegid() != getgid())
		err(FC_ERR, FMT2S, getmyname(), ERR_SETID);

	/*
	 * File descriptors 0, 1, and 2 must be open.
	 */
	for (fd = 0; fd < 3; fd++)
		if (!fd_isopen(fd))
			err(FC_ERR, "%s: %u: %s\n",
			     getmyname(), (unsigned)fd, strerror(errno));

	ofd = FD1, efd = FD2;
	eopt = false, file = NULL;
	while ((opt = getopt(argc, argv, ":ef:")) != -1)
		switch (opt) {
		case 'e':
			eopt = true;
			break;
		case 'f':
			file = optarg;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (argc < 1)
		usage();

	if (file != NULL) {
		if ((nfd = open(file, O_WRONLY|O_APPEND|O_CREAT, 0666)) == -1)
			err(FC_ERR, FMT3S, getmyname(), file, ERR_CREATE);
		if (dup2(nfd, efd) == -1)
			err(FC_ERR, FMT2S, getmyname(), strerror(errno));
		if (eopt && dup2(efd, ofd) == -1)
			err(FC_ERR, FMT2S, getmyname(), strerror(errno));
		(void)close(nfd);
	} else {
		if (eopt)
			ofd = FD2, efd = FD1;
		if (dup2(ofd, efd) == -1)
			err(FC_ERR, FMT2S, getmyname(), strerror(errno));
	}

	/*
	 * Try to execute the specified command.
	 */
	(void)pexec(argv[0], argv);
	if (errno == ENOEXEC)
		err(125, FMT3S, getmyname(), argv[0], ERR_NOSHELL);
	if (errno != ENOENT && errno != ENOTDIR)
		err(126, FMT3S, getmyname(), argv[0], ERR_EXEC);
	err(127, FMT3S, getmyname(), argv[0], ERR_NOTFOUND);
	/*NOTREACHED*/
	return FC_ERR;
}

static bool
fd_isopen(int fd)
{
	struct stat sb;

	return fstat(fd, &sb) == 0;
}

static void
usage(void)
{

	err(FC_ERR,"usage: %s [-e] [-f file] command [arg ...]\n",getmyname());
}
