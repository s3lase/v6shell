/*
 * if.c - a port of the Sixth Edition (V6) UNIX conditional command
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
 *		- Sixth Edition UNIX	/usr/source/s1/if.c
 *		- Seventh Edition UNIX	/usr/src/cmd/test.c
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pexec.h"

#define	IF_ERR		124	/* default exit status for errors */
#define	F_GZ		1	/* for the `-s' primary           */
#define	F_OT		2	/* for the `-ot' primary          */
#define	F_NT		3	/* for the `-nt' primary          */
#define	F_EF		4	/* for the `-ef' primary          */

#define	EXIT(s)		((getpid() == ifpid) ? exit((s)) : _exit((s)))
#define	FORKED		true
#define	RETERR		true

static	int	ac;
static	int	ap;
static	char	**av;
static	uid_t	ifeuid;
static	pid_t	ifpid;

/*@noreturn@*/
static	void	doex(bool);
static	bool	e1(void);
static	bool	e2(void);
static	bool	e3(void);
static	bool	equal(/*@null@*/ const char *, /*@null@*/ const char *);
/*@noreturn@*/
static	void	err(int, /*@null@*/ const char *, const char *);
static	bool	expr(void);
static	bool	ifaccess(/*@null@*/ const char *, int);
static	bool	ifstat1(/*@null@*/ const char *, mode_t);
static	bool	ifstat2(/*@null@*/ const char *, /*@null@*/ const char *, int);
/*@null@*/
static	char	*nxtarg(bool);

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
int
main(int argc, char **argv)
{
	bool re;		/* return value of expr() */

	ifpid = getpid();
	ifeuid = geteuid();

	/*
	 * Set-ID execution is not supported.
	 */
	if (ifeuid != getuid() || getegid() != getgid())
		err(IF_ERR, NULL, "Set-ID execution denied");

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
		err(IF_ERR, av[ap - 2], "expression expected");

	/*
	 * Deal w/ parentheses for grouping.
	 */
	if (equal(a, "(")) {
		re = expr();
		if (!equal(nxtarg(RETERR), ")"))
			err(IF_ERR, a, ") expected");
		return re;
	}

	/*
	 * Execute command within braces to obtain its exit status.
	 */
	if (equal(a, "{")) {
		if ((cpid = fork()) == -1)
			err(IF_ERR, NULL, "Cannot fork - try again");
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
			err(IF_ERR, a, "digit expected");
		if (*b >= '0' && *b <= '9' && *(b + 1) == '\0') {
			d = *b - '0';
			if (d >= 0 && d <= 9 && "0123456789"[d % 10] == *b)
				return isatty(d) != 0;
		}
		err(IF_ERR, b, "not a digit");
	}

	/*
	 * binary comparisons
	 */
	if ((b = nxtarg(RETERR)) == NULL)
		err(IF_ERR, a, "operator expected");
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
	err(IF_ERR, b, "unknown operator");
	/*NOTREACHED*/
	return false;
}

static void
doex(bool forked)
{
	char **xap, **xav;

	if (ap < 2 || ap > ac)	/* should never be true */
		err(IF_ERR, NULL, "Invalid argv index");

	xav = xap = &av[ap];
	while (*xap != NULL) {
		if (forked && equal(*xap, "}"))
			break;
		xap++;
	}
	if (forked && xap - xav > 0 && !equal(*xap, "}"))
		err(IF_ERR, av[ap - 1], "} expected");
	*xap = NULL;
	if (xav[0] == NULL)
		err(IF_ERR, forked ? av[ap - 1] : NULL, "command expected");

	/* Use a built-in exit since there is no external exit utility. */
	if (equal(xav[0], "exit")) {
		(void)lseek(STDIN_FILENO, (off_t)0, SEEK_END);
		EXIT(0);
	}

	(void)pexec(xav[0], xav);
	if (errno == ENOEXEC)
		err(125, xav[0], "No shell!");
	if (errno != ENOENT && errno != ENOTDIR)
		err(126, xav[0], "cannot execute");
	err(127, xav[0], "not found");
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

	if (ra && mode == X_OK && ifeuid == 0) {
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
		err(IF_ERR, NULL, "Invalid argv index");

	if (ap == ac) {
		if (reterr) {
			ap++;
			return NULL;
		}
		err(IF_ERR, av[ap - 1], "argument expected");
	}
	nap = av[ap];
	ap++;
	return nap;
}

static bool
equal(const char *a, const char *b)
{

	if (a == NULL || b == NULL || *a != *b)
		return false;
	return strcmp(a, b) == 0;
}

static void
err(int es, const char *msg1, const char *msg2)
{

	if (msg1 != NULL)
		(void)fprintf(stderr, "if: %s: %s\n", msg1, msg2);
	else
		(void)fprintf(stderr, "if: %s\n", msg2);
	EXIT(es);
}
