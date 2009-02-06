/*
 * util.c - special built-in shell utilities for osh
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
/*
 *	Derived from:
 *		- osh-20080629:
 *			fd2.c  (r404 2008-05-09 16:52:15Z jneitzel)
 *			goto.c (r465 2008-06-25 22:11:58Z jneitzel)
 *			if.c   (r465 2008-06-25 22:11:58Z jneitzel)
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

#define	OSH_SHELL

#include "sh.h"
#include "defs.h"
#include "pexec.h"

#define	IS_SBI(k)	\
	((k) == SBI_ECHO || (k) == SBI_FD2 || (k) == SBI_GOTO || (k) == SBI_IF)

static	const char	*utilname;

/*@noreturn@*/
static	void	uerr(int, const char *, /*@printflike@*/ ...);
static	int	(*util)(int, char **);
static	int	sbi_echo(int, char **);
static	int	sbi_fd2(int, char **);
static	int	sbi_goto(int, char **);
static	int	sbi_if(int, char **);

/*
 * Execute the shell utility specified by key w/ the argument
 * count ac and the argument vector pointed to by av.
 * Return status s of the last call in the chain.
 */
int
uexec(enum sbikey key, int ac, char **av)
{
	int r;
	static int cnt, cnt1, s;

	switch (key) {
	case SBI_ECHO:	util = sbi_echo;	break;
	case SBI_FD2:	util = sbi_fd2;		break;
	case SBI_GOTO:	util = sbi_goto;	break;
	case SBI_IF:	util = sbi_if;		break;
	default:
		uerr(FC_ERR, "uexec: Invalid utility\n");
	}

	cnt1 = cnt++;

	r = util(ac, av);

	if (cnt-- > cnt1)
		s = r;

	return s;
}

/*
 * Exit the shell utility child process on error w/ the
 * specified exit status and any specified message.
 */
static void
uerr(int es, const char *msgfmt, ...)
{
	va_list va;

	va_start(va, msgfmt);
	omsg(FD2, msgfmt, va);
	va_end(va);
	_exit(es);
}

/*
 * NAME
 *	echo - write arguments to standard output
 *
 * SYNOPSIS
 *	echo [-n] [string ...]
 *
 * DESCRIPTION
 *	Echo writes its string arguments (if any) separated by
 *	blanks and terminated by a newline to the standard output.
 *	If `-n' is specified, the terminating newline is not written.
 */
static int
sbi_echo(int argc, char **argv)
{
	bool nopt;
	char **avp, **ave;

	argc--, argv++;
	if (*argv != NULL && EQUAL(*argv, "-n")) {
		argc--, argv++;
		nopt = true;
	} else
		nopt = false;

	for (avp = argv, ave = &argv[argc]; avp < ave; avp++)
		fd_print(FD1, "%s%s", *avp, (avp + 1 < ave) ? " " : "");
	if (!nopt)
		fd_print(FD1, "\n");

	return SH_TRUE;
}

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
static int
sbi_fd2(int argc, char **argv)
{
	bool eopt;
	enum sbikey key;
	int efd, nfd, ofd, opt;
	char *file;

	utilname = argv[0];

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
			uerr(FC_ERR, FMT3S, utilname, file, ERR_CREATE);
		if (dup2(nfd, efd) == -1)
			uerr(FC_ERR, FMT2S, utilname, strerror(errno));
		if (eopt && dup2(efd, ofd) == -1)
			uerr(FC_ERR, FMT2S, utilname, strerror(errno));
		(void)close(nfd);
	} else {
		if (eopt)
			ofd = FD2, efd = FD1;
		if (dup2(ofd, efd) == -1)
			uerr(FC_ERR, FMT2S, utilname, strerror(errno));
	}

	/*
	 * Try to execute the specified command.
	 */
	key = cmd_lookup(argv[0]);
	if (IS_SBI(key))
		return uexec(key, argc, argv);

	(void)pexec(argv[0], argv);
	if (errno == ENOEXEC)
		uerr(125, FMT3S, utilname, argv[0], ERR_NOSHELL);
	if (errno != ENOENT && errno != ENOTDIR)
		uerr(126, FMT3S, utilname, argv[0], ERR_EXEC);
	uerr(127, FMT3S, utilname, argv[0], ERR_NOTFOUND);
	/*NOTREACHED*/
	return FC_ERR;
}

static void
usage(void)
{

	uerr(FC_ERR, "usage: %s [-e] [-f file] command [arg ...]\n", utilname);
}

static	off_t	offset;

static	bool	getlabel(/*@out@*/ char *, int, size_t);
static	int	xgetc(void);

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
sbi_goto(int argc, char **argv)
{
	size_t siz;
	char label[LABELSIZE];

	utilname = argv[0];

	if (argc < 2 || *argv[1] == '\0' || isatty(FD0) != 0)
		uerr(FC_ERR, FMT2S, utilname, ERR_GENERIC);
	if ((siz = strlen(argv[1]) + 1) > sizeof(label))
		uerr(FC_ERR, FMT3S, utilname, argv[1], ERR_LABTOOLONG);
	if (lseek(FD0, (off_t)0, SEEK_SET) == -1)
		uerr(FC_ERR, FMT2S, utilname, ERR_SEEK);

	while (getlabel(label, *argv[1] & 0377, siz))
		if (strcmp(label, argv[1]) == 0) {
			(void)lseek(FD0, offset, SEEK_SET);
			return SH_TRUE;
		}

	fd_print(FD2, FMT3S, utilname, argv[1], ERR_LABNOTFOUND);
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

static	int		iac;
static	int		iap;
static	char		**iav;

/*@maynotreturn@*/
static	void	doex(bool);
static	bool	e1(void);
static	bool	e2(void);
static	bool	e3(void);
static	bool	equal(/*@null@*/ const char *, /*@null@*/ const char *);
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
 *	if expression [command [arg ...]]
 *
 * DESCRIPTION
 *	See the if(1) manual page for full details.
 */
static int
sbi_if(int argc, char **argv)
{
	bool re;		/* return value of expr() */

	utilname = argv[0];

	if (argc > 1) {
		iac = argc;
		iav = argv;
		iap = 1;
		re  = expr();
		if (re && iap < iac)
			doex(!FORKED);
	} else
		re = false;

	return re ? SH_TRUE : SH_FALSE;
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
	iap--;
	return re;
}

static bool
e1(void)
{
	bool re;

	re = e2();
	if (equal(nxtarg(RETERR), "-a"))
		return re & e1();
	iap--;
	return re;
}

static bool
e2(void)
{

	if (equal(nxtarg(RETERR), "!"))
		return !e3();
	iap--;
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
		uerr(FC_ERR, FMT3S, utilname, iav[iap - 2], ERR_EXPR);

	/*
	 * Deal w/ parentheses for grouping.
	 */
	if (equal(a, "(")) {
		re = expr();
		if (!equal(nxtarg(RETERR), ")"))
			uerr(FC_ERR, FMT3S, utilname, a, ERR_PAREN);
		return re;
	}

	/*
	 * Execute { command [arg ...] } to obtain its exit status.
	 */
	if (equal(a, "{")) {
		if ((cpid = fork()) == -1)
			uerr(FC_ERR, FMT2S, utilname, ERR_FORK);
		if (cpid == 0)
			/**** Child! ****/
			doex(FORKED);
		else {
			/**** Parent! ****/
			tpid = wait(&cstat);
			while ((a = nxtarg(RETERR)) != NULL && !equal(a, "}"))
				;	/* nothing */
			if (a == NULL)
				iap--;
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
			uerr(FC_ERR, FMT3S, utilname, a, ERR_DIGIT);
		if (*b >= '0' && *b <= '9' && *(b + 1) == '\0') {
			d = *b - '0';
			if (IS_DIGIT(d, *b))
				return isatty(d) != 0;
		}
		uerr(FC_ERR, FMT3S, utilname, b, ERR_NOTDIGIT);
	}

	/*
	 * binary comparisons
	 */
	if ((b = nxtarg(RETERR)) == NULL)
		uerr(FC_ERR, FMT3S, utilname, a, ERR_OPERATOR);
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
	uerr(FC_ERR, FMT3S, utilname, b, ERR_OPUNKNOWN);
	/*NOTREACHED*/
	return false;
}

static void
doex(bool forked)
{
	enum sbikey xak;
	char **xap, **xav;

	if (iap < 2 || iap > iac)	/* should never be true */
		uerr(FC_ERR, FMT2S, utilname, ERR_AVIINVAL);

	xav = xap = &iav[iap];
	while (*xap != NULL) {
		if (forked && equal(*xap, "}"))
			break;
		xap++;
	}
	if (forked && xap - xav > 0 && !equal(*xap, "}"))
		uerr(FC_ERR, FMT3S, utilname, iav[iap - 1], ERR_BRACE);
	*xap = NULL;
	if (xav[0] == NULL)
		uerr(FC_ERR, FMT3S, utilname, iav[iap - 1], ERR_COMMAND);

	/* Invoke a special "exit" utility in this case. */
	if (equal(xav[0], "exit")) {
		(void)lseek(FD0, (off_t)0, SEEK_END);
		_exit(SH_TRUE);
	}

	xak = cmd_lookup(xav[0]);
	if (IS_SBI(xak)) {
		if (forked)
			_exit(uexec(xak, xap - xav, xav));
		else
			(void)uexec(xak, xap - xav, xav);
		return;
	}

	(void)pexec(xav[0], xav);
	if (errno == ENOEXEC)
		uerr(125, FMT3S, utilname, xav[0], ERR_NOSHELL);
	if (errno != ENOENT && errno != ENOTDIR)
		uerr(126, FMT3S, utilname, xav[0], ERR_EXEC);
	uerr(127, FMT3S, utilname, xav[0], ERR_NOTFOUND);
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

	if (iap < 1 || iap > iac)	/* should never be true */
		uerr(FC_ERR, FMT2S, utilname, ERR_AVIINVAL);

	if (iap == iac) {
		if (reterr) {
			iap++;
			return NULL;
		}
		uerr(FC_ERR, FMT3S, utilname, iav[iap - 1], ERR_ARGUMENT);
	}
	nap = iav[iap];
	iap++;
	return nap;
}

static bool
equal(const char *a, const char *b)
{

	if (a == NULL || b == NULL)
		return false;
	return EQUAL(a, b);
}
