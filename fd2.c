/*
 * fd2.c - redirect file descriptor 2
 */
/*-
 * Copyright (c) 2005, 2006
 *	Jeffrey Allen Neitzel <jneitzel (at) sdf1 (dot) org>.
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
 *	@(#)fd2.c	1.5 (jneitzel) 2006/08/20
 */

#ifndef	lint
#include "version.h"
OSH_SOURCEID("fd2.c	1.5 (jneitzel) 2006/08/20");
#endif	/* !lint */

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pexec.h"

#define	FD0	STDIN_FILENO
#define	FD1	STDOUT_FILENO
#define	FD2	STDERR_FILENO

#define	FD2_ERR	124

/*@noreturn@*/ static
	void	error(int, /*@null@*/ const char *, const char *);
static	int	fdisopen(int);
/*@noreturn@*/ static
	void	usage(void);

/*
 * NAME
 *	fd2 - redirect file descriptor 2
 *
 * SYNOPSIS
 *	fd2 [-f file] command [arg ...]
 *
 * DESCRIPTION
 *	See the fd2(1) manual page for full details.
 */
int
main(int argc, char **argv)
{
	int nfd, opt;
	char *file;

	/*
	 * Set-ID execution is not supported.
	 */
	if (geteuid() != getuid() || getegid() != getgid())
		error(FD2_ERR, NULL, "Set-ID execution denied");

	/*
	 * File descriptors 0, 1, and 2 must be open.
	 */
	if (!fdisopen(FD0) || !fdisopen(FD1) || !fdisopen(FD2))
		error(FD2_ERR, NULL, strerror(errno));

	/*
	 * If the `-f' option is specified, file descriptor 2 is
	 * redirected to the specified file.  Otherwise, it is
	 * redirected to file descriptor 1 by default.
	 */
	file = NULL;
	while ((opt = getopt(argc, argv, "f:")) != -1) {
		switch (opt) {
		case 'f':
			file = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc < 1)
		usage();

	if (file != NULL) {
		if ((nfd = open(file, O_WRONLY|O_APPEND|O_CREAT, 0666)) == -1)
			error(FD2_ERR, file, "cannot create");
		if (dup2(nfd, FD2) == -1)
			error(FD2_ERR, NULL, strerror(errno));
		(void)close(nfd);
	} else
		if (dup2(FD1, FD2) == -1)
			error(FD2_ERR, NULL, strerror(errno));

	/*
	 * Try to execute the specified command.
	 */
	(void)pexec(argv[0], argv);
	if (errno == ENOEXEC)
		error(125, argv[0], "No shell!");
	if (errno != ENOENT && errno != ENOTDIR)
		error(126, argv[0], "cannot execute");
	error(127, argv[0], "not found");
	/*NOTREACHED*/
	return FD2_ERR;
}

static void
error(int es, const char *msg1, const char *msg2)
{

	if (msg1 != NULL)
		(void)fprintf(stderr, "fd2: %s: %s\n", msg1, msg2);
	else
		(void)fprintf(stderr, "fd2: %s\n", msg2);
	exit(es);
}

static int
fdisopen(int fd)
{
	struct stat sb;

	return fstat(fd, &sb) == 0;
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: fd2 [-f file] command [arg ...]\n");
	exit(FD2_ERR);
}
