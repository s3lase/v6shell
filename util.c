/*
 * util.c - special built-in versions of the shell utilities for osh
 */
/*-
 * Copyright (c) 2004-2008
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
/*
 *	Derived from:
 *		- osh-20080629/if.c   (r465 2008-06-25 22:11:58Z jneitzel)
 *		- osh-20080629/goto.c (r465 2008-06-25 22:11:58Z jneitzel)
 *		- osh-20080629/fd2.c  (r404 2008-05-09 16:52:15Z jneitzel)
 */
/*-
 * Copyright (C) Caldera International Inc.  2001-2002.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code and documentation must retain the above
 *    copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 * 4. Neither the name of Caldera International, Inc. nor the names of other
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE FOR ANY DIRECT,
 * INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	lint
#include "rcsid.h"
OSH_RCSID("@(#)$Id$");
#endif	/* !lint */

#include "config.h"

#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "osh.h"
#include "pexec.h"

static	const char	*argv0;

static	void	uerr(int, /*@null@*/ const char *, /*@printflike@*/ ...);
static	int	imain(int, char **);
static	int	gmain(int, char **);
static	int	fmain(int, char **);

/* ==== if ==== */
#define	F_GZ		1	/* for the `-s' primary           */
#define	F_OT		2	/* for the `-ot' primary          */
#define	F_NT		3	/* for the `-nt' primary          */
#define	F_EF		4	/* for the `-ef' primary          */

#define	FORKED		true
#define	RETERR		true

static	int		ac;
static	int		ap;
static	char		**av;

/*@noreturn@*/
static	void	doex(bool);
static	bool	e1(void);
static	bool	e2(void);
static	bool	e3(void);
static	bool	equal(/*@null@*/ const char *, /*@null@*/ const char *);
/*@noreturn@*/
static	bool	expr(void);
static	bool	ifaccess(/*@null@*/ const char *, int);
static	bool	ifstat1(/*@null@*/ const char *, mode_t);
static	bool	ifstat2(/*@null@*/ const char *, /*@null@*/ const char *, int);
/*@null@*/
static	char	*nxtarg(bool);

/* ==== goto ==== */
#define	LABELSIZE	64	/* size of the label buffer */

static	off_t	offset;

static	bool	getlabel(/*@out@*/ char *, int, size_t);
static	int	xgetc(void);

/* ==== fd2 ==== */
/*@noreturn@*/
static	void	usage(void);

int
uexec(enum sbikey key, int ac, char **av)
{
	static int ccnt, lcnt, ss;
	int r;

	lcnt = ccnt++;

	switch (key) {
	case SBI_IF: case SBI_GOTO: case SBI_FD2:
		(void)fprintf(stderr, "%2d, %2d) uexec(%d, %d, %p);\n",
			ccnt, lcnt, key, ac, (void *)av);
		break;
	default:
		break;
	}

	switch (key) {
	case SBI_IF:	r = imain(ac, av);	break;
	case SBI_GOTO:	r = gmain(ac, av);	break;
	case SBI_FD2:	r = fmain(ac, av);	break;
	default:	r = SH_FALSE;
	}

	if (ccnt > lcnt)
		ss = r;
	(void)fprintf(stderr,"%2d, %2d) r == %d, ss = %d\n",ccnt,lcnt,r,ss);
	ccnt--;

	return ss;
}

static void
uerr(int es, const char *msgfmt, ...)
{
	va_list va;

	if (msgfmt != NULL) {
		va_start(va, msgfmt);
		omsg(FD2, msgfmt, va);
		va_end(va);
	}
	_exit(es);
}

/*
 * NAME
 *	if - conditional command
 *
 * SYNOPSIS
 *	if expr [command [arg ...]]
 *
 * DESCRIPTION
 *	See the if(1) manual page for full details.
 */
static int
imain(int argc, char **argv)
{
	bool re;		/* return value of expr() */

	argv0 = argv[0];

	if (argc > 1) {
		ac = argc;
		av = argv;
		ap = 1;
		re = expr();
		if (re && ap < ac)
			doex(!FORKED);
	} else
		re = false;

	return re ? 0 : 1;
}

/*
 * Evaluate the expression.
 * Return true (1) or false (0).
 */
static bool
expr(void)
{
	bool re;

	re = e1();
	if (equal(nxtarg(RETERR), "-o"))
		return re | expr();
	ap--;
	return re;
}

static bool
e1(void)
{
	bool re;

	re = e2();
	if (equal(nxtarg(RETERR), "-a"))
		return re & e1();
	ap--;
	return re;
}

static bool
e2(void)
{

	if (equal(nxtarg(RETERR), "!"))
		return !e3();
	ap--;
	return e3();
}

static bool
e3(void)
{
	bool re;
	pid_t cpid, tpid;
	int cstat, d;
	char *a, *b;

	if ((a = nxtarg(RETERR)) == NULL)
		uerr(FC_ERR, FMT3S, argv0, av[ap - 2], "expression expected");

	/*
	 * Deal w/ parentheses for grouping.
	 */
	if (equal(a, "(")) {
		re = expr();
		if (!equal(nxtarg(RETERR), ")"))
			uerr(FC_ERR, FMT3S, argv0, a, ") expected");
		return re;
	}

	/*
	 * Execute command within braces to obtain its exit status.
	 */
	if (equal(a, "{")) {
		if ((cpid = fork()) == -1)
			uerr(FC_ERR, FMT2S, argv0, "Cannot fork - try again");
		if (cpid == 0)
			/**** Child! ****/
			doex(FORKED);
		else {
			/**** Parent! ****/
			tpid = wait(&cstat);
			while ((a = nxtarg(RETERR)) != NULL && !equal(a, "}"))
				;	/* nothing */
			if (a == NULL)
				ap--;
			return (tpid == cpid && cstat == 0) ? true : false;
		}
	}

	/*
	 * file existence/permission tests
	 */
	if (equal(a, "-e"))
		return ifaccess(nxtarg(!RETERR), F_OK);
	if (equal(a, "-r"))
		return ifaccess(nxtarg(!RETERR), R_OK);
	if (equal(a, "-w"))
		return ifaccess(nxtarg(!RETERR), W_OK);
	if (equal(a, "-x"))
		return ifaccess(nxtarg(!RETERR), X_OK);

	/*
	 * file existence/type tests
	 */
	if (equal(a, "-d"))
		return ifstat1(nxtarg(!RETERR), S_IFDIR);
	if (equal(a, "-f"))
		return ifstat1(nxtarg(!RETERR), S_IFREG);
	if (equal(a, "-h"))
		return ifstat1(nxtarg(!RETERR), S_IFLNK);
	if (equal(a, "-s"))
		return ifstat1(nxtarg(!RETERR), F_GZ);
	if (equal(a, "-t")) {
		/* Does the descriptor refer to a terminal device? */
		b = nxtarg(RETERR);
		if (b == NULL || *b == '\0')
			uerr(FC_ERR, FMT3S, argv0, a, "digit expected");
		if (*b >= '0' && *b <= '9' && *(b + 1) == '\0') {
			d = *b - '0';
			if (d >= 0 && d <= 9 && "0123456789"[d % 10] == *b)
				return isatty(d) != 0;
		}
		uerr(FC_ERR, FMT3S, argv0, b, "not a digit");
	}

	/*
	 * binary comparisons
	 */
	if ((b = nxtarg(RETERR)) == NULL)
		uerr(FC_ERR, FMT3S, argv0, a, "operator expected");
	if (equal(b,  "="))
		return  equal(a, nxtarg(!RETERR));
	if (equal(b, "!="))
		return !equal(a, nxtarg(!RETERR));
	if (equal(b, "-ot"))
		return ifstat2(a, nxtarg(!RETERR), F_OT);
	if (equal(b, "-nt"))
		return ifstat2(a, nxtarg(!RETERR), F_NT);
	if (equal(b, "-ef"))
		return ifstat2(a, nxtarg(!RETERR), F_EF);
	uerr(FC_ERR, FMT3S, argv0, b, "unknown operator");
	/*NOTREACHED*/
	return false;
}

static void
doex(bool forked)
{
	enum sbikey xak;
	char **xap, **xav;

	if (ap < 2 || ap > ac)	/* should never be true */
		uerr(FC_ERR, FMT2S, argv0, "Invalid argv index");

	xav = xap = &av[ap];
	while (*xap != NULL) {
		if (forked && equal(*xap, "}"))
			break;
		xap++;
	}
	if (forked && xap - xav > 0 && !equal(*xap, "}"))
		uerr(FC_ERR, FMT3S, argv0, av[ap - 1], "} expected");
	*xap = NULL;
	if (xav[0] == NULL)
		uerr(FC_ERR, FMT3S, argv0, av[ap - 1], "command expected");

	/* Use a built-in exit since there is no external exit utility. */
	if (equal(xav[0], "exit")) {
		(void)lseek(FD0, (off_t)0, SEEK_END);
		_exit(0);
	}

	xak = cmd_key_lookup(xav[0]);
	if (xak == SBI_IF || xak == SBI_GOTO || xak == SBI_FD2) {
		if (forked)
			_exit(uexec(xak, xap - xav, xav));
		else
			(void)uexec(xak, xap - xav, xav);
		return;
	}

	(void)pexec(xav[0], xav);
	if (errno == ENOEXEC)
		uerr(125, FMT3S, argv0, xav[0], "No shell!");
	if (errno != ENOENT && errno != ENOTDIR)
		uerr(126, FMT3S, argv0, xav[0], "cannot execute");
	uerr(127, FMT3S, argv0, xav[0], "not found");
}

/*
 * Check access permission for file according to mode while
 * dealing w/ special cases for the superuser and `-x':
 *	- Always grant search access on directories.
 *	- If not a directory, require at least one execute bit.
 * Return true  (1) if access is granted.
 * Return false (0) if access is denied.
 */
static bool
ifaccess(const char *file, int mode)
{
	struct stat sb;
	bool ra;

	if (file == NULL || *file == '\0')
		return false;

	ra = access(file, mode) == 0;

	if (ra && mode == X_OK && euid == 0) {
		if (stat(file, &sb) < 0)
			ra = false;
		else if (S_ISDIR(sb.st_mode))
			ra = true;
		else
			ra = (sb.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) != 0;
	}

	return ra;
}

static bool
ifstat1(const char *file, mode_t type)
{
	struct stat sb;
	bool rs;

	if (file == NULL || *file == '\0')
		return false;

	if (type == S_IFLNK) {
		if (lstat(file, &sb) < 0)
			rs = false;
		else
			rs = (sb.st_mode & S_IFMT) == type;
	} else if (stat(file, &sb) < 0)
		rs = false;
	else if (type == S_IFDIR || type == S_IFREG)
		rs = (sb.st_mode & S_IFMT) == type;
	else if (type == F_GZ)
		rs = sb.st_size > (off_t)0;
	else
		rs = false;

	return rs;
}

static bool
ifstat2(const char *file1, const char *file2, int act)
{
	struct stat sb1, sb2;
	bool rs;

	if (file1 == NULL || *file1 == '\0')
		return false;
	if (file2 == NULL || *file2 == '\0')
		return false;

	if (stat(file1, &sb1) < 0)
		return false;
	if (stat(file2, &sb2) < 0)
		return false;

	if (act == F_OT)
		rs = sb1.st_mtime < sb2.st_mtime;
	else if (act == F_NT)
		rs = sb1.st_mtime > sb2.st_mtime;
	else if (act == F_EF)
		rs = sb1.st_dev == sb2.st_dev && sb1.st_ino == sb2.st_ino;
	else
		rs = false;

	return rs;
}

static char *
nxtarg(bool reterr)
{
	char *nap;

	if (ap < 1 || ap > ac)	/* should never be true */
		uerr(FC_ERR, FMT2S, argv0, "Invalid argv index");

	if (ap == ac) {
		if (reterr) {
			ap++;
			return NULL;
		}
		uerr(FC_ERR, FMT3S, argv0, av[ap - 1], "argument expected");
	}
	nap = av[ap];
	ap++;
	return nap;
}

static bool
equal(const char *a, const char *b)
{

	if (a == NULL || b == NULL)
		return false;
	return EQUAL(a, b);
}

/*
 * NAME
 *	goto - transfer command
 *
 * SYNOPSIS
 *	goto label
 *
 * DESCRIPTION
 *	See the goto(1) manual page for full details.
 */
static int
gmain(int argc, char **argv)
{
	size_t siz;
	char label[LABELSIZE];

	argv0 = argv[0];

	if (argc < 2 || *argv[1] == '\0' || isatty(STDIN_FILENO) != 0) {
		uerr(FC_ERR, "%s: error\n", argv0);
	}
	if ((siz = strlen(argv[1]) + 1) > sizeof(label)) {
		uerr(FC_ERR, "%s: %s: label too long\n", argv0, argv[1]);
	}
	if (lseek(STDIN_FILENO, (off_t)0, SEEK_SET) == -1) {
		uerr(FC_ERR, "%s: cannot seek\n", argv0);
	}

	while (getlabel(label, *argv[1] & 0377, siz))
		if (strcmp(label, argv[1]) == 0) {
			(void)lseek(STDIN_FILENO, offset, SEEK_SET);
			return SH_TRUE;
		}

	uerr(SH_FALSE, "%s: %s: label not found\n", argv0, argv[1]);
	/*NOTREACHED*/
	return SH_FALSE;
}

/*
 * Search for the first occurrence of a possible label with both
 * the same first character (fc) and the same length (siz - 1)
 * as argv[1], and copy this possible label to buf.
 * Return true  (1) if possible label found.
 * Return false (0) at end-of-file.
 */
static bool
getlabel(char *buf, int fc, size_t siz)
{
	int c;
	char *b;

	while ((c = xgetc()) != EOF) {
		/* `:' may be preceded by blanks. */
		while (c == ' ' || c == '\t')
			c = xgetc();
		if (c != ':') {
			while (c != '\n' && c != EOF)
				c = xgetc();
			continue;
		}

		/* Prepare for possible label. */
		while ((c = xgetc()) == ' ' || c == '\t')
			;	/* nothing   */
		if (c != fc)	/* not label */
			continue;

		/*
		 * Try to copy possible label (first word only)
		 * to buf, ignoring it if it becomes too long.
		 */
		b = buf;
		do {
			if (c == '\n' || c == ' ' || c == '\t' || c == EOF) {
				*b = '\0';
				break;
			}
			*b = c;
			c = xgetc();
		} while (++b < &buf[siz]);

		/* Ignore any remaining characters on labelled line. */
		while (c != '\n' && c != EOF)
			c = xgetc();
		if (c == EOF)
			break;

		if ((size_t)(b - buf) != siz - 1)	/* not label */
			continue;
		return true;
	}

	*buf = '\0';
	return false;
}

/*
 * If not at end-of-file, return the next character from the standard
 * input as an unsigned char converted to an int while incrementing
 * the global offset.  Otherwise, return EOF at end-of-file.
 */
static int
xgetc(void)
{
	int nc;

	offset++;
	nc = getchar();
	return (nc != EOF) ? nc & 0377 : EOF;
}

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
static int
fmain(int argc, char **argv)
{
	enum sbikey key;
	int nfd, opt;
	char *file;

	argv0 = argv[0];

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
			uerr(FC_ERR, FMT3S, argv0, file, "cannot create");
		if (dup2(nfd, FD2) == -1)
			uerr(FC_ERR, FMT2S, argv0, strerror(errno));
		(void)close(nfd);
	} else
		if (dup2(FD1, FD2) == -1)
			uerr(FC_ERR, FMT2S, argv0, strerror(errno));

	/*
	 * Try to execute the specified command.
	 */
	key = cmd_key_lookup(argv[0]);
	if (key == SBI_IF || key == SBI_GOTO || key == SBI_FD2)
		return uexec(key, argc, argv);

	(void)pexec(argv[0], argv);
	if (errno == ENOEXEC)
		uerr(125, FMT3S, argv0, argv[0], "No shell!");
	if (errno != ENOENT && errno != ENOTDIR)
		uerr(126, FMT3S, argv0, argv[0], "cannot execute");
	uerr(127, FMT3S, argv0, argv[0], "not found");
	/*NOTREACHED*/
	return FC_ERR;
}

static void
usage(void)
{

	uerr(FC_ERR, "usage: %s [-f file] command [arg ...]\n", argv0);
}
