/*
 * osh.c - an enhanced port of the Sixth Edition (V6) Unix Thompson shell
 */
/*-
 * Copyright (c) 2004, 2005, 2006, 2007
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
 *	@(#)osh.c	1.2 (jneitzel) 2007/01/14
 */
/*
 *	Derived from:
 *		- osh-20061230:
 *			@(#)sh6.c	1.3 (jneitzel) 2006/09/15
 *		- osh-20061230:
 *			@(#)glob6.c	1.3 (jneitzel) 2006/09/23
 *		- osh-060124:
 *			@(#)osh.h	1.8 (jneitzel) 2006/01/20
 *			@(#)osh.c	1.108 (jneitzel) 2006/01/20
 *			@(#)osh-parse.c	1.8 (jneitzel) 2006/01/20
 *			@(#)osh-exec.c	1.7 (jneitzel) 2006/01/20
 *			@(#)osh-misc.c	1.5 (jneitzel) 2006/01/20
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
#include "version.h"
OSH_SOURCEID("osh.c	1.2 (jneitzel) 2007/01/14");
#endif	/* !lint */

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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pexec.h"

/*
 * Constants
 */
#define	LINEMAX		2048	/* 1000 in the original Sixth Edition shell */
#define	WORDMAX		1024	/*   50 ...                                 */

#ifdef	ARG_MAX
#define	GAVMAX		ARG_MAX
#else
#define	GAVMAX		1048576
#endif
#define	GAVNEW		128	/* # of new arguments per gav allocation */

#ifdef	PATH_MAX
#define	PATHMAX		PATH_MAX
#else
#define	PATHMAX		1024
#endif

#define	FMTSIZE		64
#if LINEMAX >= PATHMAX
#define	MSGSIZE		(FMTSIZE + LINEMAX)
#else
#define	MSGSIZE		(FMTSIZE + PATHMAX)
#endif

#ifdef	_POSIX_OPEN_MAX
#define	FDFREEMIN	_POSIX_OPEN_MAX
#else
#define	FDFREEMIN	20	/* Value is the same as _POSIX_OPEN_MAX. */
#endif
#define	FDFREEMAX	65536	/* Arbitrary maximum value for fdfree(). */

/*
 * Following standard conventions, file descriptors 0, 1, and 2 are used
 * for standard input, standard output, and standard error respectively.
 */
#define	FD0		STDIN_FILENO
#define	FD1		STDOUT_FILENO
#define	FD2		STDERR_FILENO

/*
 * The following file descriptors are reserved for special use by osh.
 */
#define	DUPFD0		7	/* used for input redirection w/ `<-' */
#define	PWD		8	/* see do_chdir()                     */
#define	SAVFD0		9	/* see do_source()                    */

/*
 * These are the initialization files used by osh.
 * The `_PATH_DOT_*' files are in the user's HOME directory.
 */
#define	_PATH_SYSTEM_LOGIN	"/etc/osh.login"
#define	_PATH_DOT_LOGIN		".osh.login"
#define	_PATH_DOT_OSHRC		".oshrc"

/*
 * These are the nonstandard symbolic names for some of the type values
 * used by the fdtype() function.
 */
#define	FD_OPEN		1	/* Is descriptor any type of open file? */
#define	FD_DIROK	2	/* Is descriptor an existent directory? */

/*
 * Child interrupt flags (see chintr)
 */
#define	CH_SIGINT	01
#define	CH_SIGQUIT	02

/*
 * Signal state flags (see sig_state)
 */
#define	SS_SIGINT	01
#define	SS_SIGQUIT	02
#define	SS_SIGTERM	04

/*
 * (NSIG - 1) is the maximum signal number value accepted by `sigign'.
 */
#ifndef	NSIG
#define	NSIG		32
#endif

/*
 * Shell type flags (see stype)
 */
#define	ONELINE		001
#define	COMMANDFILE	002
#define	INTERACTIVE	004
#define	RCFILE		010
#define	SOURCE		020

/*
 * Non-zero exit status values
 */
#define	FC_ERR		124	/* fatal child error (changed in pwait()) */
#define	SH_ERR		2	/* shell-detected error (default value)   */

/*
 * Diagnostics
 */
#define	ERR_ALTOOLONG	"Arg list too long"
#define	ERR_FORK	"Cannot fork - try again"
#define	ERR_PIPE	"Cannot pipe - try again"
#define	ERR_TRIM	"Cannot trim"
#define	ERR_ALINVAL	"Invalid argument list"
#define	ERR_NODIR	"No directory"
#define	ERR_NOHOMEDIR	"No home directory"
#define	ERR_NOMATCH	"No match"
#define	ERR_NOPWD	"No previous directory"
#define	ERR_NOSHELL	"No shell!"
#define	ERR_NOMEM	"Out of memory"
#define	ERR_SETID	"Set-ID execution denied"
#define	ERR_TMARGS	"Too many args"
#define	ERR_TMCHARS	"Too many characters"
#define	ERR_ARGCOUNT	"arg count"
#define	ERR_BADDIR	"bad directory"
#define	ERR_BADMASK	"bad mask"
#define	ERR_BADNAME	"bad name"
#define	ERR_BADSIGNAL	"bad signal"
#define	ERR_CREATE	"cannot create"
#define	ERR_EXEC	"cannot execute"
#define	ERR_OPEN	"cannot open"
#define	ERR_GENERIC	"error"
#define	ERR_NOARGS	"no args"
#define	ERR_NOTFOUND	"not found"
#define	ERR_SYNTAX	"syntax error"
#define	FMT1S		"%s"
#define	FMT2S		"%s: %s"
#define	FMT3S		"%s: %s: %s"

/*
 * Macros
 */
#define	DOLDIGIT(d, c)	((d) >= 0 && (d) <= 9 && "0123456789"[(d) % 10] == (c))
#define	EQUAL(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)
#define	ESTATUS		((getpid() == spid) ? SH_ERR : FC_ERR)
#define	EXIT(s)		((getpid() == spid) ? exit((s)) : _exit((s)))
#define	PROMPT		((stype & 037) == INTERACTIVE)
#define	STYPE(f)	((stype & (f)) != 0)

/*
 * WCOREDUMP may be undefined when compiled w/ -D_XOPEN_SOURCE=600.
 */
#ifndef	WCOREDUMP
# if defined(_W_INT)
#  define	SH_W_INT(s)	_W_INT(s)	/* [FNO]..*BSD, Mac OS X */
# elif defined(__WAIT_INT)
#  define	SH_W_INT(s)	__WAIT_INT(s)	/* GNU/Linux w/ glibc */
# else
#  define	SH_W_INT(s)	(s)		/* default to int */
# endif
# ifndef	WCOREFLAG
#  define	WCOREFLAG	0200
# endif
#define		WCOREDUMP(s)	((SH_W_INT(s) & WCOREFLAG) != 0)
#endif	/* !WCOREDUMP */

/*
 * Shell command-tree structure
 */
struct tnode {
/*@null@*/struct tnode	 *nleft;	/* Pointer to left node.           */
/*@null@*/struct tnode	 *nright;	/* Pointer to right node.          */
/*@null@*/struct tnode	 *nsub;		/* Pointer to TSUBSHELL node.      */
	  int		  ntype;	/* Node type (see below).          */
	  int		  nflags;	/* Node/command flags (see below). */
	  int		  nidx;		/* Node/command index (see sbi[]). */
/*@null@*/char		**nav;		/* Argument vector for TCOMMAND.   */
/*@null@*/char		 *nfin;		/* Pointer to input file (<).      */
/*@null@*/char		 *nfout;	/* Pointer to output file (>, >>). */
};

/*
 * Node type
 */
#define	TLIST		1	/* pipelines separated by `;', `&', or `\n' */
#define	TPIPE		2	/* commands separated by `|' or `^'         */
#define	TCOMMAND	3	/* command [ arg ... ] [ < in ] [ > out ]   */
#define	TSUBSHELL	4	/* ( list ) [ < in ] [ > out ]              */

/*
 * Node/command flags
 */
#define	FAND		0001	/* A `&'  designates asynchronous execution.  */
#define	FCAT		0002	/* A `>>' appends output to file.             */
#define	FFIN		0004	/* A `<'  redirects input from file.          */
#define	FPIN		0010	/* A `|' or `^' redirects input from pipe.    */
#define	FPOUT		0020	/* A `|' or `^' redirects output to pipe.     */
#define	FNOFORK		0040	/* No fork(2) for last command in `( list )'. */
#define	FINTR		0100	/* Child process ignores SIGINT and SIGQUIT.  */
#define	FPRS		0200	/* Print process ID of child as a string.     */

/*
 * Global variable declarations
 */
#define	XNSIG		14
static	const char *const sig[XNSIG] = {
	NULL,
	"Hangup",
	NULL,
	"Quit",
	"Illegal instruction",
	"Trace/BPT trap",
	"IOT trap",
	"EMT trap",
	"Floating exception",
	"Killed",
	"Bus error",
	"Memory fault",
	"Bad system call",
	NULL
};

/*
 * Special built-in commands
 */
static const char *const sbi[] = {
#define	SBINULL		0
	":",
#define	SBICHDIR	1
	"chdir",
#define	SBIEXIT		2
	"exit",
#define	SBILOGIN	3
	"login",
#define	SBINEWGRP	4
	"newgrp",
#define	SBISHIFT	5
	"shift",
#define	SBIWAIT		6
	"wait",
#define	SBISIGIGN	7
	"sigign",
#define	SBISETENV	8
	"setenv",
#define	SBIUNSETENV	9
	"unsetenv",
#define	SBIUMASK	10
	"umask",
#define	SBISOURCE	11
	"source",
#define	SBIEXEC		12
	"exec",
	NULL
};

/*@null@*/ static
	const char	*arginp;	/* string for `-c' option           */
static	int		chintr;		/* SIGINT / SIGQUIT flag for child  */
static	char		dolbuf[32];	/* dollar buffer for $$, $n, $s     */
static	int		dolc;		/* $N dollar-argument count         */
/*@null@*/ /*@only@*/ static
	const char	*dolp;		/* $ dollar-value pointer           */
static	char	*const	*dolv;		/* $N dollar-argument value list    */
static	int		dupfd0;		/* duplicate of the standard input  */
static	int		error;		/* error flag for read/parse errors */
static	int		err_source;	/* error flag for `source' command  */
static	uid_t		euid;		/* effective shell user ID          */
static	const char	*name;		/* $0 - shell command name          */
static	int		nulcnt;		/* `\0'-character count (per line)  */
static	int		peekc;		/* just-read, pushed-back character */
static	int		sig_state;	/* SIGINT / SIGQUIT / SIGTERM state */
static	pid_t		spid;		/* shell process ID                 */
static	int		status;		/* shell exit status                */
static	int		stype;		/* shell type (determines behavior) */
/*@null@*/ /*@only@*/ static
	char		*tty;		/* $t - terminal name               */
/*@null@*/ /*@only@*/ static
	char		*user;		/* $u - effective user name         */

static	char		line[LINEMAX];	/* command-line buffer              */
static	char		*linep;
static	char		*words[WORDMAX];/* argument/word pointer array      */
static	char		**wordp;
/*@only@*/ static
	struct tnode	*treep;		/* shell command-tree pointer       */

/*
 * Function prototypes
 */
static	void		cmd_loop(int);
static	int		rpxline(void);
static	int		getword(void);
static	int		xgetc(int);
/*@null@*/ /*@only@*/ static
	const char	*getdolp(int);
static	int		readc(void);
static	struct tnode	*talloc(void);
static	void		tfree(/*@null@*/ /*@only@*/ struct tnode *);
static	void		xfree(/*@null@*/ /*@only@*/ void *);
/*@null@*/ static
	struct tnode	*syntax(char **, char **);
/*@null@*/ static
	struct tnode	*syn1(char **, char **);
/*@null@*/ static
	struct tnode	*syn2(char **, char **);
/*@null@*/ static
	struct tnode	*syn3(char **, char **);
static	int		any(int, const char *);
static	int		which(const char *);
static	int		vtglob(char **);
static	void		vtrim(char **);
static	void		execute(/*@null@*/ struct tnode *,
				/*@null@*/ int *, /*@null@*/ int *);
static	void		exec1(struct tnode *);
static	void		exec2(struct tnode *,
			      /*@null@*/ int *, /*@null@*/ int *);
static	void		do_chdir(char **);
static	void		do_sig(char **);
static	void		sigflags(void (*)(int), int);
static	void		do_source(char **);
static	void		pwait(pid_t);
static	int		prsig(int, pid_t, pid_t);
static	void		sh_init(void);
static	void		fdfree(void);
static	void		sh_magic(void);
static	void		rcfile(int *);
static	void		atrim(char *);
static	char		*gchar(/*@returned@*/ const char *);
static	char		*gtrim(/*@returned@*/ char *);
/*@out@*/ static
	void		*xmalloc(size_t);
static	void		*xrealloc(/*@only@*/ void *, size_t);
static	char		*xstrdup(const char *);
/*@maynotreturn@*/ static
	void		err(int, /*@null@*/ const char *, /*@printflike@*/ ...);
static	void		fdprint(int, const char *, /*@printflike@*/ ...);
static	void		omsg(int, const char *, va_list);
static	int		fdtype(int, mode_t);
static	char		**glob(char **);

/*
 * NAME
 *	osh - old shell (command interpreter)
 *
 * SYNOPSIS
 *	osh [- | -c string | -t | file [arg1 ...]]
 *
 * DESCRIPTION
 *	See the osh(1) manual page for full details.
 */
int
main(int argc, char **argv)
{
	int rcflag = 0;

	sh_init();
	if (argv[0] == NULL || *argv[0] == '\0')
		err(SH_ERR, FMT1S, ERR_ALINVAL);
	if (fdtype(FD0, S_IFDIR))
		goto done;

	if (argc > 1) {
		name = argv[1];
		dolv = &argv[1];
		dolc = argc - 1;
		if (*argv[1] == '-') {
			chintr = 1;
			if (argv[1][1] == 'c' && argc > 2) {
				stype  = ONELINE;
				dolv  += 1;
				dolc  -= 1;
				arginp = argv[2];
			} else if (argv[1][1] == 't')
				stype = ONELINE;
		} else {
			stype = COMMANDFILE;
			(void)close(FD0);
			if (open(argv[1], O_RDONLY) != FD0)
				err(SH_ERR, FMT2S, argv[1], ERR_OPEN);
			if (fdtype(FD0, S_IFDIR))
				goto done;
		}
	} else {
		chintr = 1;
		if (isatty(FD0) && isatty(FD2)) {
			stype = INTERACTIVE;
			rcflag = (*argv[0] == '-') ? 1 : 3;
			rcfile(&rcflag);
		}
	}
	if (chintr) {
		chintr = 0;
		if (signal(SIGINT, SIG_IGN) == SIG_DFL)
			chintr |= CH_SIGINT;
		if (signal(SIGQUIT, SIG_IGN) == SIG_DFL)
			chintr |= CH_SIGQUIT;
		if (PROMPT)
			(void)signal(SIGTERM, SIG_IGN);
	}
	fdfree();

	if (STYPE(ONELINE))
		(void)rpxline();
	else {
		while (STYPE(RCFILE)) {
			cmd_loop(0);
			rcfile(&rcflag);
		}
		cmd_loop(0);
	}

done:
	xfree(tty);
	tty = NULL;
	xfree(user);
	user = NULL;
	return status;
}

/*
 * Read and execute command lines forever, or until one of the
 * following conditions is true.  If halt == 1, return on EOF
 * or when err_source == 1.  If halt == 0, return on EOF.
 */
static void
cmd_loop(int halt)
{
	int gz;

	sh_magic();

	for (err_source = gz = 0; ; ) {
		if (PROMPT)
			(void)write(FD2, (euid != 0) ? "% " : "# ", (size_t)2);
		if (rpxline() == EOF) {
			if (!gz)
				status = 0;
			break;
		}
		if (halt && err_source)
			break;
		gz = 1;
	}
}

/*
 * Read, parse, and execute a command line.
 */
static int
rpxline(void)
{
	struct tnode *t;
	sigset_t nmask, omask;
	char *wp;

	linep = line;
	wordp = words;
	error = nulcnt = 0;
	do {
		wp = linep;
		if (getword() == EOF)
			return EOF;
	} while (*wp != '\n');

	if (!error && wordp - words > 1) {
		(void)sigfillset(&nmask);
		(void)sigprocmask(SIG_SETMASK, &nmask, &omask);
		t = treep;
		treep = NULL;
		treep = syntax(words, wordp);
		(void)sigprocmask(SIG_SETMASK, &omask, NULL);
		if (error)
			err(-1, FMT1S, ERR_SYNTAX);
		else
			execute(treep, NULL, NULL);
		tfree(treep);
		treep = NULL;
		treep = t;
	}
	return 1;
}

/*
 * Copy a word from the standard input into the line buffer,
 * and point to it w/ *wordp.  Each copied word is represented
 * in line as an individual `\0'-terminated string.
 */
static int
getword(void)
{
	int c, c1;

	*wordp++ = linep;

	for (;;) {
		switch (c = xgetc(1)) {
		case EOF:
			return EOF;

		case ' ':
		case '\t':
			continue;

		case '"':
		case '\'':
			c1 = c;
			*linep++ = c;
			while ((c = xgetc(0)) != c1) {
				if (c == EOF)
					return EOF;
				if (c == '\n') {
					if (!error)
						err(-1, FMT1S, ERR_SYNTAX);
					peekc = c;
					*linep++ = '\0';
					error = 1;
					return 1;
				}
				if (c == '\\') {
					if ((c = xgetc(0)) == EOF)
						return EOF;
					if (c == '\n')
						c = ' ';
					else {
						peekc = c;
						c = '\\';
					}
				}
				*linep++ = c;
			}
			*linep++ = c;
			break;

		case '\\':
			if ((c = xgetc(0)) == EOF)
				return EOF;
			if (c == '\n')
				continue;
			*linep++ = '\\';
			*linep++ = c;
			break;

		case '(': case ')':
		case ';': case '&':
		case '|': case '^':
		case '<': case '>':
		case '\n':
			*linep++ = c;
			*linep++ = '\0';
			return 1;

		default:
			peekc = c;
		}

		for (;;) {
			if ((c = xgetc(1)) == EOF)
				return EOF;
			if (c == '\\') {
				if ((c = xgetc(0)) == EOF)
					return EOF;
				if (c == '\n')
					c = ' ';
				else {
					*linep++ = '\\';
					*linep++ = c;
					continue;
				}
			}
			if (any(c, " \t\"'();&|^<>\n")) {
				peekc = c;
				if (any(c, "\"'"))
					break;
				*linep++ = '\0';
				return 1;
			}
			*linep++ = c;
		}
	}
	/*NOTREACHED*/
}

/*
 * If dolsub == 0, get the next character from the standard input.
 * Otherwise, if dolsub == 1, get either the next character from the
 * standard input or substitute the current $ dollar w/ the next
 * character of its value, which is pointed to by dolp.
 */
static int
xgetc(int dolsub)
{
	int c;

	if (peekc != '\0') {
		c = peekc;
		peekc = '\0';
		return c;
	}

	if (wordp >= &words[WORDMAX - 2]) {
		wordp -= 4;
		while ((c = xgetc(0)) != EOF && c != '\n')
			;	/* nothing */
		wordp += 4;
		err(-1, FMT1S, ERR_TMARGS);
		goto geterr;
	}
	if (linep >= &line[LINEMAX - 5]) {
		linep -= 10;
		while ((c = xgetc(0)) != EOF && c != '\n')
			;	/* nothing */
		linep += 10;
		err(-1, FMT1S, ERR_TMCHARS);
		goto geterr;
	}

getdol:
	if (dolp != NULL) {
		c = *dolp++;
		if (c != '\0')
			return c;
		dolp = NULL;
	}
	c = readc();
	if (c == '$' && dolsub) {
		c = readc();
		if ((dolp = getdolp(c)) != NULL)
			goto getdol;
	}
	while (c == '\0') {
		if (++nulcnt >= LINEMAX) {
			err(-1, FMT1S, ERR_TMCHARS);
			goto geterr;
		}
		c = readc();
	}
	return c;

geterr:
	error = 1;
	return '\n';
}

/*
 * Return a pointer to the appropriate $ dollar value on success.
 * Return a NULL pointer on error.
 */
static const char *
getdolp(int c)
{
	int n, r;
	const char *v;

	*dolbuf = '\0';
	switch (c) {
	case '$':
		r = snprintf(dolbuf, sizeof(dolbuf), "%05u", (unsigned)spid);
		v = (r < 0 || r >= (int)sizeof(dolbuf)) ? NULL : dolbuf;
		break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		n = c - '0';
		if (DOLDIGIT(n, c) && n < dolc)
			v = (n > 0) ? dolv[n] : name;
		else
			v = dolbuf;
		break;
	case 'd':
		if ((v = getenv("OSHDIR")) == NULL)
			v = dolbuf;
		break;
	case 'e':
		if ((v = getenv("EXECSHELL")) == NULL)
			v = dolbuf;
		break;
	case 'h':
		if ((v = getenv("HOME")) == NULL)
			v = dolbuf;
		break;
	case 'n':
		n = (dolc > 1) ? dolc - 1 : 0;
		r = snprintf(dolbuf, sizeof(dolbuf), "%u", (unsigned)n);
		v = (r < 0 || r >= (int)sizeof(dolbuf)) ? NULL : dolbuf;
		break;
	case 'p':
		if ((v = getenv("PATH")) == NULL)
			v = dolbuf;
		break;
	case 's':
		r = snprintf(dolbuf, sizeof(dolbuf), "%u", (unsigned)status);
		v = (r < 0 || r >= (int)sizeof(dolbuf)) ? NULL : dolbuf;
		break;
	case 't':
		v = tty;
		break;
	case 'u':
		v = user;
		break;
	default:
		v = NULL;
	}
	return v;
}

/*
 * Read and return a character from the string pointed to by
 * arginp or from the standard input.  When reading from arginp,
 * return the character or `\n' at end-of-string.  When reading
 * from the standard input, return the character or EOF.  EOF is
 * returned both on end-of-file and on read(2) error.
 */
static int
readc(void)
{
	unsigned char c;

	if (arginp != NULL) {
		if ((c = (*arginp++ & 0377)) == '\0') {
			arginp = NULL;
			c = '\n';
		}
		return c;
	}
	if (read(FD0, &c, (size_t)1) != 1)
		return EOF;
	return c;
}

/*
 * Allocate and initialize memory for a new tree node.
 * Return a pointer to the new node on success.
 * Do not return on error (ENOMEM).
 */
static struct tnode *
talloc(void)
{
	struct tnode *t;

	t = xmalloc(sizeof(struct tnode));
	t->nleft  = NULL;
	t->nright = NULL;
	t->nsub   = NULL;
	t->ntype  = 0;
	t->nflags = 0;
	t->nidx   = 0;
	t->nav    = NULL;
	t->nfin   = NULL;
	t->nfout  = NULL;
	return t;
}

/*
 * Deallocate memory for the shell command tree pointed to by t.
 */
static void
tfree(struct tnode *t)
{
	char **p;

	if (t == NULL)
		return;
	switch (t->ntype) {
	case TLIST:
	case TPIPE:
		tfree(t->nleft);
		tfree(t->nright);
		break;
	case TCOMMAND:
		if (t->nav != NULL) {
			for (p = t->nav; *p != NULL; p++)
				free(*p);
			free(t->nav);
		}
		xfree(t->nfin);
		xfree(t->nfout);
		break;
	case TSUBSHELL:
		tfree(t->nsub);
		xfree(t->nfin);
		xfree(t->nfout);
		break;
	}
	xfree(t);
}

static void
xfree(void *p)
{

	if (p != NULL)
		free(p);
}

/*
 * syntax:
 *	empty
 *	syn1
 */
static struct tnode *
syntax(char **p1, char **p2)
{

	while (p1 < p2)
		if (any(**p1, ";&\n"))
			p1++;
		else
			return syn1(p1, p2);
	return NULL;
}

/*
 * syn1:
 *	syn2
 *	syn2 ; syntax
 *	syn2 & syntax
 */
static struct tnode *
syn1(char **p1, char **p2)
{
	struct tnode *t;
	int c, subcnt;
	char **p;

	subcnt = 0;
	for (p = p1; p < p2; p++)
		switch (**p) {
		case '(':
			subcnt++;
			continue;

		case ')':
			subcnt--;
			if (subcnt < 0)
				goto synerr;
			continue;

		case ';':
		case '&':
		case '\n':
			if (subcnt == 0) {
				c = **p;
				t = talloc();
				t->ntype  = TLIST;
				t->nleft  = syn2(p1, p);
				if (c == '&' && t->nleft != NULL)
					t->nleft->nflags |= FAND | FINTR | FPRS;
				t->nright = syntax(p + 1, p2);
				t->nflags = 0;
				return t;
			}
			break;
		}

	if (subcnt == 0)
		return syn2(p1, p2);

synerr:
	error = 1;
	return NULL;
}

/*
 * syn2:
 *	syn3
 *	syn3 | syn2
 *	syn3 ^ syn2
 */
static struct tnode *
syn2(char **p1, char **p2)
{
	struct tnode *t;
	int subcnt;
	char **p;

	subcnt = 0;
	for (p = p1; p < p2; p++)
		switch (**p) {
		case '(':
			subcnt++;
			continue;

		case ')':
			subcnt--;
			continue;

		case '|':
		case '^':
			if (subcnt == 0) {
				t = talloc();
				t->ntype  = TPIPE;
				t->nleft  = syn3(p1, p);
				t->nright = syn2(p + 1, p2);
				t->nflags = 0;
				return t;
			}
			break;
		}

	return syn3(p1, p2);
}

/*
 * syn3:
 *	command [ arg ... ] [ < in ] [ > out ]
 *	( syn1 ) [ < in ] [ > out ]
 */
static struct tnode *
syn3(char **p1, char **p2)
{
	struct tnode *t;
	int ac, c, flags, n, subcnt;
	char **p, **lp, **rp;
	char *fin, *fout;

	flags = 0;
	if (**p2 == ')')
		flags |= FNOFORK;

	fin    = NULL;
	fout   = NULL;
	lp     = NULL;
	rp     = NULL;
	n      = 0;
	subcnt = 0;
	for (p = p1; p < p2; p++)
		switch (c = **p) {
		case '(':
			if (subcnt == 0) {
				if (lp != NULL)
					goto synerr;
				lp = p + 1;
			}
			subcnt++;
			continue;

		case ')':
			subcnt--;
			if (subcnt == 0)
				rp = p;
			continue;

		case '>':
			p++;
			if (p < p2 && **p == '>')
				flags |= FCAT;
			else
				p--;
			/*FALLTHROUGH*/

		case '<':
			if (subcnt == 0) {
				p++;
				if (p == p2 || any(**p, "(<>"))
					goto synerr;
				if (c == '<') {
					if (fin != NULL)
						goto synerr;
					fin = xstrdup(*p);
				} else {
					if (fout != NULL)
						goto synerr;
					fout = xstrdup(*p);
				}
			}
			continue;

		default:
			if (subcnt == 0)
				p1[n++] = *p;
		}

	if (lp == NULL) {
		if (n == 0)
			goto synerr;
		t = talloc();
		t->ntype = TCOMMAND;
		t->nav   = xmalloc((n + 1) * sizeof(char *));
		for (ac = 0; ac < n; ac++)
			t->nav[ac] = xstrdup(p1[ac]);
		t->nav[ac] = NULL;
		t->nidx = which(t->nav[0]);
	} else {
		if (n != 0)
			goto synerr;
		t = talloc();
		t->ntype = TSUBSHELL;
		t->nsub  = syn1(lp, rp);
	}
	t->nfin   = fin;
	t->nfout  = fout;
	t->nflags = flags;
	return t;

synerr:
	xfree(fin);
	xfree(fout);
	error = 1;
	return NULL;
}

/*
 * Return 1 if the character c matches any character in the string
 * pointed to by as.  Otherwise, return 0.
 */
static int
any(int c, const char *as)
{
	const char *s;

	s = as;
	while (*s != '\0')
		if (*s++ == c)
			return 1;
	return 0;
}

/*
 * Determine whether cmd is a special built-in command.
 * If so, return its index value.  Otherwise, return -1
 * as name is either external, or not a command at all.
 */
static int
which(const char *cmd)
{
	int i;

	for (i = 0; sbi[i] != NULL; i++)
		if (EQUAL(cmd, sbi[i]))
			return i;
	return -1;
}

/*
 * Return 1 if any argument in the argument vector pointed to by vp
 * contains any unquoted glob characters.  Otherwise, return 0.
 */
static int
vtglob(char **vp)
{
	char **p;

	for (p = vp; *p != NULL; p++)
		if (gchar(*p) != NULL)
			return 1;
	return 0;
}

/*
 * Remove any unquoted quote characters from each argument
 * in the argument vector pointed to by vp.
 */
static void
vtrim(char **vp)
{
	char **p;

	for (p = vp; *p != NULL; p++)
		atrim(*p);
}

/*
 * Try to execute the shell command tree pointed to by t.
 */
static void
execute(struct tnode *t, int *pin, int *pout)
{
	struct tnode *t1;
	pid_t cpid;
	int f, pfd[2];

	if (t == NULL)
		return;
	switch (t->ntype) {
	case TLIST:
		f = t->nflags & (FFIN | FPIN | FINTR);
		if ((t1 = t->nleft) != NULL)
			t1->nflags |= f;
		execute(t1, NULL, NULL);
		if ((t1 = t->nright) != NULL)
			t1->nflags |= f;
		execute(t1, NULL, NULL);
		return;

	case TPIPE:
		if (pipe(pfd) == -1) {
			err(-1, FMT1S, ERR_PIPE);
			if (pin != NULL) {
				(void)close(pin[0]);
				(void)close(pin[1]);
			}
			return;
		}
		f = t->nflags;
		if ((t1 = t->nleft) != NULL)
			t1->nflags |= FPOUT | (f & (FFIN|FPIN|FINTR|FPRS));
		execute(t1, pin, pfd);
		if ((t1 = t->nright) != NULL)
			t1->nflags |= FPIN | (f & (FAND|FPOUT|FINTR|FPRS));
		execute(t1, pfd, pout);
		(void)close(pfd[0]);
		(void)close(pfd[1]);
		return;

	case TCOMMAND:
		if (t->nav == NULL || t->nav[0] == NULL) {
			/* should never be true */
			err(-1, "execute: Invalid command");
			return;
		}
		if (t->nidx != -1 && t->nidx != SBIEXEC) {
			exec1(t);
			return;
		}
		if (t->nidx == SBIEXEC) {
			/*
			 * Replace the current shell w/ an instance of
			 * the specified command.
			 *
			 * usage: exec command [arg ...]
			 */
			if (t->nav[1] == NULL) {
				err(-1, FMT2S, t->nav[0], ERR_ARGCOUNT);
				return;
			}
			t->nflags |= FNOFORK;
			t->nav     = &t->nav[1];
			(void)signal(SIGCHLD, SIG_IGN);
		}
		/*FALLTHROUGH*/

	case TSUBSHELL:
		f = t->nflags;
		if ((cpid = ((f & FNOFORK) != 0) ? 0 : fork()) == -1) {
			err(-1, FMT1S, ERR_FORK);
			return;
		}
		/**** Parent! ****/
		if (cpid != 0) {
			if (pin != NULL && (f & FPIN) != 0) {
				(void)close(pin[0]);
				(void)close(pin[1]);
			}
			if ((f & FPRS) != 0)
				fdprint(FD2, "%u", (unsigned)cpid);
			if ((f & FAND) != 0)
				return;
			if ((f & FPOUT) == 0)
				pwait(cpid);
			return;
		}
		/**** Child! ****/
		exec2(t, pin, pout);
		/*NOTREACHED*/
	}
}

/*
 * Try to execute the special built-in command which is specified by the
 * t->nidx and t->nav fields in the shell command tree pointed to by t.
 */
static void
exec1(struct tnode *t)
{
	mode_t m;
	const char *emsg, *p;

	if (t->nav == NULL || t->nav[0] == NULL) {
		/* should never be true */
		err(-1, "execute: Invalid command");
		return;
	}
	switch (t->nidx) {
	case SBINULL:
		/*
		 * Do nothing and set the exit status to zero.
		 */
		status = 0;
		return;

	case SBICHDIR:
		/*
		 * Change the shell's working directory.
		 */
		do_chdir(t->nav);
		return;

	case SBIEXIT:
		/*
		 * If the shell is invoked w/ the `-c' or `-t' option, or is
		 * executing an initialization file, exit the shell outright
		 * if it is not sourcing another file in the current context.
		 * Otherwise, cause the shell to cease execution of a file of
		 * commands by seeking to the end of the file (and explicitly
		 * exiting the shell only if the file is not being sourced).
		 */
		if (!PROMPT) {
			if (STYPE(ONELINE|RCFILE) && !STYPE(SOURCE))
				EXIT(status);
			(void)lseek(FD0, (off_t)0, SEEK_END);
			if (!STYPE(SOURCE))
				EXIT(status);
		}
		return;

	case SBILOGIN:
	case SBINEWGRP:
		/*
		 * Replace the current interactive shell w/ an
		 * instance of login(1) or newgrp(1).
		 */
		if (PROMPT) {
			p = (t->nidx == SBILOGIN) ? _PATH_LOGIN : _PATH_NEWGRP;
			vtrim(t->nav);
			(void)signal(SIGINT, SIG_DFL);
			(void)signal(SIGQUIT, SIG_DFL);
			(void)pexec(p, (char *const *)t->nav);
			(void)signal(SIGINT, SIG_IGN);
			(void)signal(SIGQUIT, SIG_IGN);
		}
		emsg = ERR_EXEC;
		break;

	case SBISHIFT:
		/*
		 * Shift all positional-parameter values to the left by 1.
		 * The value of $0 does not shift.
		 */
		if (dolc > 1) {
			dolv = &dolv[1];
			dolc--;
			status = 0;
			return;
		}
		emsg = ERR_NOARGS;
		break;

	case SBIWAIT:
		/*
		 * Wait for all asynchronous processes to terminate,
		 * reporting on abnormal terminations.
		 */
		pwait(-1);
		return;

	case SBISIGIGN:
		/*
		 * Ignore (or unignore) the specified signals, or
		 * print a list of those signals which are ignored
		 * because of a previous invocation of `sigign' in
		 * the current shell.
		 *
		 * usage: sigign [+ | - signal_number ...]
		 */
		vtrim(t->nav);
		do_sig(t->nav);
		return;

	case SBISETENV:
		/*
		 * Set the specified environment variable.
		 *
		 * usage: setenv name [value]
		 */
		if (t->nav[1] != NULL &&
		    (t->nav[2] == NULL || t->nav[3] == NULL)) {

			vtrim(t->nav);
			for (p = t->nav[1]; *p != '=' && *p != '\0'; p++)
				;	/* nothing */
			if (*t->nav[1] == '\0' || *p == '=') {
				err(-1,FMT3S,t->nav[0],t->nav[1],ERR_BADNAME);
				return;
			}
			p = (t->nav[2] != NULL) ? t->nav[2] : "";
			if (setenv(t->nav[1], p, 1) == -1)
				err(ESTATUS, FMT1S, ERR_NOMEM);

			status = 0;
			return;

		}
		emsg = ERR_ARGCOUNT;
		break;

	case SBIUNSETENV:
		/*
		 * Unset the specified environment variable.
		 *
		 * usage: unsetenv name
		 */
		if (t->nav[1] != NULL && t->nav[2] == NULL) {

			atrim(t->nav[1]);
			for (p = t->nav[1]; *p != '=' && *p != '\0'; p++)
				;	/* nothing */
			if (*t->nav[1] == '\0' || *p == '=') {
				err(-1,FMT3S,t->nav[0],t->nav[1],ERR_BADNAME);
				return;
			}
			unsetenv(t->nav[1]);

			status = 0;
			return;

		}
		emsg = ERR_ARGCOUNT;
		break;

	case SBIUMASK:
		/*
		 * Set the file creation mask to the specified
		 * octal value, or print its current value.
		 *
		 * usage: umask [mask]
		 */
		if (t->nav[1] != NULL && t->nav[2] != NULL) {
			emsg = ERR_ARGCOUNT;
			break;
		}
		if (t->nav[1] == NULL) {
			(void)umask(m = umask(0));
			fdprint(FD1, "%04o", (unsigned)m);
		} else {
			atrim(t->nav[1]);
			for (m = 0, p = t->nav[1]; *p >= '0' && *p <= '7'; p++)
				m = m * 8 + (*p - '0');
			if (*t->nav[1] == '\0' || *p != '\0' || m > 0777) {
				err(-1,FMT3S,t->nav[0],t->nav[1],ERR_BADMASK);
				return;
			}
			(void)umask(m);
		}
		status = 0;
		return;

	case SBISOURCE:
		/*
		 * Read and execute commands from file and return.
		 *
		 * usage: source file [arg1 ...]
		 */
		if (t->nav[1] != NULL) {
			vtrim(t->nav);
			do_source(t->nav);
			return;
		}
		emsg = ERR_ARGCOUNT;
		break;

	default:
		emsg = ERR_EXEC;
	}
	err(-1, FMT2S, t->nav[0], emsg);
}

/*
 * Try to execute the child process which is specified by combination
 * of the t->nsub, t->nflags, t->nav, t->nfin, and t->nfout fields in
 * the shell command tree pointed to by t.
 */
static void
exec2(struct tnode *t, int *pin, int *pout)
{
	struct tnode *t1;
	int f, i;
	const char *cmd;
	char **glob_av;

	f = t->nflags;

	/*
	 * Redirect (read) input from pipe.
	 */
	if (pin != NULL && (f & FPIN) != 0) {
		if (dup2(pin[0], FD0) == -1)
			err(FC_ERR, FMT1S, strerror(errno));
		(void)close(pin[0]);
		(void)close(pin[1]);
	}
	/*
	 * Redirect (write) output to pipe.
	 */
	if (pout != NULL && (f & FPOUT) != 0) {
		if (dup2(pout[1], FD1) == -1)
			err(FC_ERR, FMT1S, strerror(errno));
		(void)close(pout[0]);
		(void)close(pout[1]);
	}
	/*
	 * Redirect (read) input from file.
	 */
	if (t->nfin != NULL && (f & FPIN) == 0) {
		f |= FFIN;
		i  = 0;
		if (*t->nfin == '-' && *(t->nfin + 1) == '\0')
			i = 1;
		atrim(t->nfin);
		if (i)
			i = dup(dupfd0);
		else
			i = open(t->nfin, O_RDONLY);
		if (i == -1)
			err(FC_ERR, FMT2S, t->nfin, ERR_OPEN);
		if (dup2(i, FD0) == -1)
			err(FC_ERR, FMT1S, strerror(errno));
		(void)close(i);
	}
	/*
	 * Redirect (write) output to file.
	 */
	if (t->nfout != NULL && (f & FPOUT) == 0) {
		if ((f & FCAT) != 0)
			i = O_WRONLY | O_APPEND | O_CREAT;
		else
			i = O_WRONLY | O_TRUNC | O_CREAT;
		atrim(t->nfout);
		if ((i = open(t->nfout, i, 0666)) == -1)
			err(FC_ERR, FMT2S, t->nfout, ERR_CREATE);
		if (dup2(i, FD1) == -1)
			err(FC_ERR, FMT1S, strerror(errno));
		(void)close(i);
	}
	/*
	 * Set the action for the SIGINT and SIGQUIT signals, and
	 * redirect input for `&' commands from `/dev/null' if needed.
	 */
	if ((f & FINTR) != 0) {
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGQUIT, SIG_IGN);
		if (t->nfin == NULL && (f & (FFIN|FPIN|FPRS)) == FPRS) {
			(void)close(FD0);
			if (open("/dev/null", O_RDONLY) != FD0)
				err(FC_ERR, FMT2S, "/dev/null", ERR_OPEN);
		}
	} else if ((f & FINTR) == 0) {
		if ((sig_state & SS_SIGINT) == 0 && (chintr & CH_SIGINT) != 0)
			(void)signal(SIGINT, SIG_DFL);
		if ((sig_state & SS_SIGQUIT) == 0 && (chintr & CH_SIGQUIT) != 0)
			(void)signal(SIGQUIT, SIG_DFL);
	}
	if ((sig_state & SS_SIGTERM) == 0)
		(void)signal(SIGTERM, SIG_DFL);
	if (t->ntype == TSUBSHELL) {
		if ((t1 = t->nsub) != NULL)
			t1->nflags |= (f & (FFIN | FPIN | FINTR));
		execute(t1, NULL, NULL);
		_exit(status);
	}
	if (t->nav == NULL || t->nav[0] == NULL) {
		/* should never be true */
		err(FC_ERR, "execute: Invalid command");
		/*NOTREACHED*/
	}
	if (vtglob(t->nav)) {
		glob_av = glob(t->nav);
		cmd = glob_av[0];
		(void)pexec(cmd, (char *const *)glob_av);
	} else {
		vtrim(t->nav);
		cmd = t->nav[0];
		(void)pexec(cmd, (char *const *)t->nav);
	}
	if (errno == ENOEXEC)
		err(125, FMT2S, cmd, ERR_NOSHELL);
	if (errno == E2BIG)
		err(126, FMT2S, cmd, ERR_ALTOOLONG);
	if (errno != ENOENT && errno != ENOTDIR)
		err(126, FMT2S, cmd, ERR_EXEC);
	err(127, FMT2S, cmd, ERR_NOTFOUND);
	/*NOTREACHED*/
}

/*
 * Change the shell's working directory.
 *
 *	`chdir'		changes to user's home directory
 *	`chdir -'	changes to previous working directory
 *	`chdir dir'	changes to `dir'
 *
 * NOTE: The user must have both read and search permission on
 *	 a directory in order for `chdir -' to be effective.
 */
static void
do_chdir(char **av)
{
	int cwd;
	static int pwd = -1;
	const char *emsg, *home;

	emsg = ERR_BADDIR;

	cwd = open(".", O_RDONLY | O_NONBLOCK);

	if (av[1] == NULL) {
		/*
		 * Change to the user's home directory.
		 */
		home = getenv("HOME");
		if (home == NULL || *home == '\0') {
			emsg = ERR_NOHOMEDIR;
			goto chdirerr;
		}
		if (chdir(home) == -1)
			goto chdirerr;
	} else if (EQUAL(av[1], "-")) {
		/*
		 * Change to the previous working directory.
		 */
		if (pwd == -1) {
			emsg = ERR_NOPWD;
			goto chdirerr;
		}
		if (!fdtype(pwd, FD_DIROK) || fchdir(pwd) == -1) {
			if (close(pwd) != -1)
				pwd = -1;
			goto chdirerr;
		}
	} else {
		/*
		 * Change to any other directory.
		 */
		atrim(av[1]);
		if (chdir(av[1]) == -1)
			goto chdirerr;
	}

	/* success - clean up if needed and return */
	if (cwd != -1) {
		if (cwd < PWD && (pwd = dup2(cwd, PWD)) == PWD)
			(void)fcntl(pwd, F_SETFD, FD_CLOEXEC);
		(void)close(cwd);
	}
	status = 0;
	return;

chdirerr:
	if (cwd != -1)
		(void)close(cwd);
	err(-1, FMT2S, av[0], emsg);
}

/*
 * Ignore (or unignore) the specified signals, or print a list of
 * those signals which are ignored because of a previous invocation
 * of `sigign' in the current shell.
 */
static void
do_sig(char **av)
{
	struct sigaction act, oact;
	sigset_t new_mask, old_mask;
	long lsigno;
	static int ignlst[NSIG], gotlst;
	int i, sigerr, signo;
	char *sigbad;

	/* Temporarily block all signals in this function. */
	(void)sigfillset(&new_mask);
	(void)sigprocmask(SIG_SETMASK, &new_mask, &old_mask);

	if (!gotlst) {
		/* Initialize the list of already ignored signals. */
		for (i = 1; i < NSIG; i++) {
			ignlst[i - 1] = 0;
			if (sigaction(i, NULL, &oact) < 0)
				continue;
			if (oact.sa_handler == SIG_IGN)
				ignlst[i - 1] = 1;
		}
		gotlst = 1;
	}

	sigerr = 0;
	if (av[1] != NULL) {
		if (av[2] == NULL) {
			sigerr = 1;
			goto sigdone;
		}

		(void)memset(&act, 0, sizeof(act));
		if (EQUAL(av[1], "+"))
			act.sa_handler = SIG_IGN;
		else if (EQUAL(av[1], "-"))
			act.sa_handler = SIG_DFL;
		else {
			sigerr = 1;
			goto sigdone;
		}
		(void)sigemptyset(&act.sa_mask);
		act.sa_flags = 0;

		for (i = 2; av[i] != NULL; i++) {
			errno = 0;
			lsigno = strtol(av[i], &sigbad, 10);
			if (errno != 0 || *av[i] == '\0' || *sigbad != '\0' ||
			    lsigno <= 0 || lsigno >= NSIG) {
				sigerr = i;
				goto sigdone;
			}
			signo = lsigno;

			/* Does anything need to be done? */
			if (sigaction(signo, NULL, &oact) < 0 ||
			    (act.sa_handler == SIG_DFL &&
			    oact.sa_handler == SIG_DFL))
				continue;

			if (ignlst[signo - 1]) {
				sigflags(act.sa_handler, signo);
				continue;
			}

			/*
			 * Trying to ignore SIGKILL, SIGSTOP, and/or SIGCHLD
			 * has no effect.
			 */
			if (signo == SIGKILL ||
			    signo == SIGSTOP || signo == SIGCHLD ||
			    sigaction(signo, &act, NULL) < 0)
				continue;

			sigflags(act.sa_handler, signo);
		}
	} else {
		/* Print signals currently ignored because of `sigign'. */
		for (i = 1; i < NSIG; i++) {
			if (sigaction(i, NULL, &oact) < 0 ||
			    oact.sa_handler != SIG_IGN)
				continue;
			if (!ignlst[i - 1] ||
			    (i == SIGINT && (sig_state & SS_SIGINT) != 0) ||
			    (i == SIGQUIT && (sig_state & SS_SIGQUIT) != 0) ||
			    (i == SIGTERM && (sig_state & SS_SIGTERM) != 0))
				fdprint(FD1, "%s + %u", av[0], (unsigned)i);
		}
	}

sigdone:
	(void)sigprocmask(SIG_SETMASK, &old_mask, NULL);
	if (sigerr == 0)
		status = 0;
	else if (sigerr == 1)
		err(-1, FMT2S, av[0], ERR_GENERIC);
	else
		err(-1, FMT3S, av[0], av[sigerr], ERR_BADSIGNAL);
}

static void
sigflags(void (*act)(int), int s)
{

	if (act == SIG_IGN) {
		if (s == SIGINT)
			sig_state |= SS_SIGINT;
		else if (s == SIGQUIT)
			sig_state |= SS_SIGQUIT;
		else if (s == SIGTERM)
			sig_state |= SS_SIGTERM;
	} else if (act == SIG_DFL) {
		if (s == SIGINT)
			sig_state &= ~SS_SIGINT;
		else if (s == SIGQUIT)
			sig_state &= ~SS_SIGQUIT;
		else if (s == SIGTERM)
			sig_state &= ~SS_SIGTERM;
	}
}

/*
 * Read and execute commands from the file av[1] and return.
 * Calls to this function can be nested to the point imposed by
 * any limits in the shell's environment, such as running out of
 * file descriptors or hitting a limit on the size of the stack.
 */
static void
do_source(char **av)
{
	char *const *sdolv;
	int nfd, sfd, sdolc;
	static int cnt;

	if ((nfd = open(av[1], O_RDONLY | O_NONBLOCK)) == -1) {
		err(-1, FMT3S, av[0], av[1], ERR_OPEN);
		return;
	}
	if (nfd == SAVFD0 || !fdtype(nfd, S_IFREG)) {
		(void)close(nfd);
		err(-1, FMT3S, av[0], av[1], ERR_EXEC);
		return;
	}
	sfd = dup2(FD0, SAVFD0 + cnt);
	if (sfd == -1 || dup2(nfd, FD0) == -1 ||
	    fcntl(FD0, F_SETFL, O_RDONLY & ~O_NONBLOCK) == -1) {
		(void)close(sfd);
		(void)close(nfd);
		err(-1, FMT3S, av[0], av[1], strerror(EMFILE));
		return;
	}
	(void)fcntl(sfd, F_SETFD, FD_CLOEXEC);
	(void)close(nfd);

	if (!STYPE(SOURCE))
		stype |= SOURCE;

	/* Save and initialize any positional parameters if needed. */
	sdolv = dolv;
	sdolc = dolc;
	if (av[2] != NULL)
		for (dolv = &av[1], dolc = 2; dolv[dolc] != NULL; dolc++)
			;	/* nothing */

	cnt++;
	cmd_loop(1);
	cnt--;

	/* Restore any saved positional parameters. */
	dolv = sdolv;
	dolc = sdolc;

	if (err_source) {
		/*
		 * The shell has detected an error (e.g., syntax error).
		 * Terminate any and all source commands (nested or not).
		 * Restore original standard input before returning for
		 * the final time, and call err() if needed.
		 */
		if (cnt == 0) {
			/* Restore original standard input or die trying. */
			if (dup2(SAVFD0, FD0) == -1)
				err(SH_ERR,FMT3S,av[0],av[1],strerror(errno));
			(void)close(SAVFD0);
			stype &= ~SOURCE;
			if (!STYPE(RCFILE))
				err(0, NULL);
			return;
		}
		(void)close(sfd);
		return;
	}

	/* Restore previous standard input or die trying. */
	if (dup2(sfd, FD0) == -1) {
		(void)close(sfd);
		err(SH_ERR, FMT3S, av[0], av[1], strerror(errno));
		return;
	}
	(void)close(sfd);

	if (cnt == 0)
		stype &= ~SOURCE;
}

/*
 * If cp > 0, wait for the child process cp to terminate.
 * While doing so, exorcise any zombies for which the shell has not
 * yet waited, and wait for any other child processes which terminate
 * during this time.  Otherwise, if cp < 0, block and wait for all
 * child processes to terminate.
 */
static void
pwait(pid_t cp)
{
	pid_t tp;
	int s;

	if (cp == 0)
		return;
	for (;;) {
		tp = wait(&s);
		if (tp == -1)
			break;
		if (s != 0) {
			if (WIFSIGNALED(s))
				status = prsig(s, tp, cp);
			else if (WIFEXITED(s))
				status = WEXITSTATUS(s);
			if (status >= FC_ERR) {
				if (status == FC_ERR)
					status = SH_ERR;
				err(0, NULL);
			}
		} else
			status = 0;
		if (tp == cp)
			break;
	}
}

/*
 * Print termination report for process tp according to signal s.
 * Return a value of 128 + signal s (e) for exit status of tp.
 */
static int
prsig(int s, pid_t tp, pid_t cp)
{
	int e, r;
	char buf[32];
	const char *c, *m;

	e = WTERMSIG(s);
	if (e >= XNSIG || (e >= 0 && sig[e] != NULL)) {
		if (e < XNSIG)
			m = sig[e];
		else {
			r = snprintf(buf, sizeof(buf), "Sig %u", (unsigned)e);
			m = (r < 0 || r >= (int)sizeof(buf)) ?
			    "prsig: snprintf: Internal error" :
			    buf;
		}
		c = "";
		if (WCOREDUMP(s))
			c = " -- Core dumped";
		if (tp != cp)
			fdprint(FD2, "%u: %s%s", (unsigned)tp, m, c);
		else
			fdprint(FD2, "%s%s", m, c);
	} else
		fdprint(FD2, "");
	return 128 + e;
}

/*
 * Initialize the shell.
 */
static void
sh_init(void)
{
	struct passwd *pwu;
	int fd;
	const char *p;

	spid = getpid();
	euid = geteuid();

	/*
	 * Set-ID execution is not supported.
	 */
	if (euid != getuid() || getegid() != getgid())
		err(SH_ERR, FMT1S, ERR_SETID);

	/*
	 * Fail if any of the descriptors 0, 1, or 2 is not open,
	 * or if dupfd0 cannot be set up properly.
	 */
	for (fd = 0; fd < 3; fd++)
		if (!fdtype(fd, FD_OPEN))
			err(SH_ERR, "%u: %s", (unsigned)fd, strerror(errno));
	if ((dupfd0 = dup2(FD0, DUPFD0)) == -1 ||
	    fcntl(dupfd0, F_SETFD, FD_CLOEXEC) == -1)
		err(SH_ERR, "%u: %s", DUPFD0, strerror(errno));

	/* Try to get the terminal name for $t. */
	p   = ttyname(dupfd0);
	tty = xstrdup((p != NULL) ? p : "");

	/* Try to get the effective user name for $u. */
	pwu  = getpwuid(euid);
	user = xstrdup((pwu != NULL) ? pwu->pw_name : "");

	/*
	 * Set the SIGCHLD signal to its default action.
	 * Correct operation of the shell requires that zombies
	 * be created for its children when they terminate.
	 */
	(void)signal(SIGCHLD, SIG_DFL);
}

/*
 * Attempt to free or release all of the file descriptors in the range
 * from (fdmax - 1) through (FD2 + 1), skipping DUPFD0; the value of
 * fdmax may fall between FDFREEMIN and FDFREEMAX, inclusive.
 */
static void
fdfree(void)
{
	long fdmax;
	int fd;

	fdmax = sysconf(_SC_OPEN_MAX);
	if (fdmax < FDFREEMIN || fdmax > FDFREEMAX)
		fdmax = FDFREEMIN;
	for (fd = fdmax - 1; fd > DUPFD0; fd--)
		(void)close(fd);
	for (fd--; fd > FD2; fd--)
		(void)close(fd);
}

/*
 * Ignore any `#!shell' sequence as the first line of a regular file.
 * The length of this line is limited to (LINEMAX - 1) characters.
 */
static void
sh_magic(void)
{
	size_t len;
	int c;

	if (fdtype(FD0, S_IFREG) && lseek(FD0, (off_t)0, SEEK_CUR) == 0) {
		if (readc() == '#' && readc() == '!') {
			for (len = 2; len < LINEMAX; len++)
				if ((c = readc()) == EOF || c == '\n')
					return;
			err(-1, FMT1S, ERR_TMCHARS);
		} else
			(void)lseek(FD0, (off_t)0, SEEK_SET);
	}
}

/*
 * Try to open each rc file for shell initialization.
 * If successful, temporarily assign the shell's standard input to
 * come from said file and return.  Restore original standard input
 * after all files have been tried.
 *
 * NOTE: The shell places no restrictions on the ownership/permissions
 *	 of any rc file.  If the file is readable and regular, the shell
 *	 will use it.  Thus, users should always protect themselves and
 *	 not do something like `chmod 0666 $h/.osh.login' w/o having
 *	 a legitimate reason to do so.
 */
static void
rcfile(int *rcflag)
{
	int nfd, r;
	char path[PATHMAX];
	const char *file, *home;

	/*
	 * Try each rc file in turn.
	 */
	while (*rcflag < 4) {
		file = NULL;
		*path = '\0';
		switch (*rcflag) {
		case 1:
			file = _PATH_SYSTEM_LOGIN;
			break;
		case 2:
			file = _PATH_DOT_LOGIN;
			/*FALLTHROUGH*/
		case 3:
			if (file == NULL)
				file = _PATH_DOT_OSHRC;
			home = getenv("HOME");
			if (home != NULL && *home != '\0') {
				r = snprintf(path, sizeof(path),
					"%s/%s", home, file);
				if (r < 0 || r >= (int)sizeof(path)) {
					err(-1, "%s/%s: %s", home, file,
					    strerror(ENAMETOOLONG));
					*path = '\0';
				}
			}
			file = path;
			break;
		}
		(*rcflag)++;
		if (file == NULL || *file == '\0')
			continue;
		if ((nfd = open(file, O_RDONLY | O_NONBLOCK)) == -1)
			continue;

		/* It must be a regular file (or a link to a regular file). */
		if (!fdtype(nfd, S_IFREG)) {
			err(-1, FMT2S, file, ERR_EXEC);
			(void)close(nfd);
			continue;
		}

		if (dup2(nfd, FD0) == -1 ||
		    fcntl(FD0, F_SETFL, O_RDONLY & ~O_NONBLOCK) == -1)
			err(SH_ERR, FMT2S, file, strerror(errno));

		/* success - clean up, return, and execute the rc file */
		(void)close(nfd);
		stype |= RCFILE;
		return;
	}
	stype &= ~RCFILE;

	/*
	 * Restore original standard input or die trying.
	 */
	if (dup2(dupfd0, FD0) == -1)
		err(SH_ERR, FMT1S, strerror(errno));
}

/*
 * Remove any unquoted quote characters from the argument pointed
 * to by ap.  This function never returns on error.
 */
static void
atrim(char *ap)
{
	size_t siz;
	char buf[LINEMAX], c;
	char *a, *b;

	*buf = '\0';
	for (a = ap, b = buf; b < &buf[LINEMAX]; a++, b++) {
		switch (*a) {
		case '\0':
			*b = '\0';
			siz = (b - buf) + 1;
			(void)memcpy(ap, buf, siz);
			return;
		case '"':
		case '\'':
			c = *a++;
			while (*a != c && b < &buf[LINEMAX]) {
				if (*a == '\0')
					goto aterr;
				*b++ = *a++;
			}
			b--;
			continue;
		case '\\':
			if (*++a == '\0') {
				a--, b--;
				continue;
			}
			break;
		}
		*b = *a;
	}

aterr:
	err(ESTATUS, "%s %s", ERR_TRIM, ap);
}

/*
 * Return a pointer to the first unquoted glob character (`*', `?', `[')
 * in the argument pointed to by ap.  Otherwise, return a NULL pointer on
 * error or if the argument contains no glob characters.
 */
static char *
gchar(const char *ap)
{
	char c;
	const char *a;

	for (a = ap; *a != '\0'; a++)
		switch (*a) {
		case '"':
		case '\'':
			for (c = *a++; *a != c; a++)
				if (*a == '\0')
					return NULL;
			continue;
		case '\\':
			if (*++a == '\0')
				return NULL;
			continue;
		case '*':
		case '?':
		case '[':
			return (char *)a;
		}
	return NULL;
}

/*
 * Prepare the glob() pattern pointed to by ap.  Remove any unquoted
 * quote characters, and escape (w/ a backslash `\') any previously
 * quoted glob or quote characters as needed.  Reallocate memory for
 * (if needed), make a copy of, and return a pointer to the new
 * glob() pattern, nap.  This function never returns on error.
 */
static char *
gtrim(char *ap)
{
	size_t siz;
	char buf[PATHMAX], c;
	char *a, *b, *nap;

	*buf = '\0';
	for (a = ap, b = buf; b < &buf[PATHMAX]; a++, b++) {
		switch (*a) {
		case '\0':
			*b = '\0';
			siz = (b - buf) + 1;
			nap = ap;
			if (siz > strlen(ap) + 1) {
				xfree(nap);
				nap = xmalloc(siz);
			}
			(void)memcpy(nap, buf, siz);
			return nap;
		case '"':
		case '\'':
			c = *a++;
			while (*a != c && b < &buf[PATHMAX]) {
				switch (*a) {
				case '\0':
					goto gterr;
				case '*': case '?': case '[': case ']':
				case '-': case '"': case '\'': case '\\':
					*b = '\\';
					if (++b >= &buf[PATHMAX])
						goto gterr;
					break;
				}
				*b++ = *a++;
			}
			b--;
			continue;
		case '\\':
			switch (*++a) {
			case '\0':
				a--, b--;
				continue;
			case '*': case '?': case '[': case ']':
			case '-': case '"': case '\'': case '\\':
				*b = '\\';
				if (++b >= &buf[PATHMAX])
					goto gterr;
				break;
			}
			break;
		}
		*b = *a;
	}

gterr:
	err(FC_ERR, "%s %s", ERR_TRIM, ap);
	/*NOTREACHED*/
	return NULL;
}

/*
 * Allocate memory, and check for error.
 * Return a pointer to the allocated space on success.
 * Do not return on error (ENOMEM).
 */
static void *
xmalloc(size_t s)
{
	void *mp;

	if ((mp = malloc(s)) == NULL) {
		err(ESTATUS, FMT1S, ERR_NOMEM);
		/*NOTREACHED*/
	}
	return mp;
}

/*
 * Reallocate memory, and check for error.
 * Return a pointer to the reallocated space on success.
 * Do not return on error (ENOMEM).
 */
static void *
xrealloc(void *p, size_t s)
{
	void *rp;

	if ((rp = realloc(p, s)) == NULL) {
		err(ESTATUS, FMT1S, ERR_NOMEM);
		/*NOTREACHED*/
	}
	return rp;
}

/*
 * Allocate memory for a copy of the string src, and copy it to dst.
 * Return a pointer to dst on success.
 * Do not return on error (ENOMEM).
 */
static char *
xstrdup(const char *src)
{
	size_t siz;
	char *dst;

	siz = strlen(src) + 1;
	dst = xmalloc(siz);
	(void)memcpy(dst, src, siz);
	return dst;
}

/*
 * Handle all errors detected by the shell.  This includes printing any
 * specified message to the standard error and setting the exit status.
 * This function may or may not return depending on the context of the
 * call, the value of es, and the value of the global variable stype.
 */
static void
err(int es, const char *msgfmt, ...)
{
	va_list va;

	if (msgfmt != NULL) {
		va_start(va, msgfmt);
		omsg(FD2, msgfmt, va);
		va_end(va);
	}
	switch (es) {
	case -1:
		status = SH_ERR;
		/*FALLTHROUGH*/
	case 0:
		if (STYPE(SOURCE)) {
			(void)lseek(FD0, (off_t)0, SEEK_END);
			err_source = 1;
			return;
		}
		if (STYPE(RCFILE) && (status == 130 || status == 131)) {
			(void)lseek(FD0, (off_t)0, SEEK_END);
			return;
		}
		if (!STYPE(INTERACTIVE)) {
			if (!STYPE(ONELINE))
				(void)lseek(FD0, (off_t)0, SEEK_END);
			break;
		}
		return;
	default:
		status = es;
	}
	EXIT(status);
}

/*
 * Print any specified message to the file descriptor pfd.
 */
static void
fdprint(int pfd, const char *msgfmt, ...)
{
	va_list va;

	va_start(va, msgfmt);
	omsg(pfd, msgfmt, va);
	va_end(va);
}

/*
 * Create and output the message specified by err() or fdprint() to
 * the file descriptor ofd.  The resulting message is terminated by
 * a newline on success.  A diagnostic is written to FD2 on error.
 */
static void
omsg(int ofd, const char *msgfmt, va_list va)
{
	int r;
	char fmt[FMTSIZE];
	char msg[MSGSIZE];
	const char *e;

	e = "omsg: Internal error\n";
	r = snprintf(fmt, sizeof(fmt), "%s\n", msgfmt);
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

/*
 * Check if the file descriptor fd refers to an open file
 * of the specified type; return 1 if true or 0 if false.
 */
static int
fdtype(int fd, mode_t type)
{
	struct stat sb;
	int rv = 0;

	if (fstat(fd, &sb) < 0)
		return rv;

	switch (type) {
	case FD_OPEN:
		/* Descriptor refers to any type of open file. */
		rv = 1;
		break;
	case FD_DIROK:
		/* Does descriptor refer to an existent directory? */
		if (S_ISDIR(sb.st_mode))
			rv = sb.st_nlink > 0;
		break;
	case S_IFDIR:
	case S_IFREG:
		rv = (sb.st_mode & S_IFMT) == type;
		break;
	}
	return rv;
}

typedef	unsigned char	UChar;

static	char		**gavp;	/* points to current gav position     */
static	char		**gave;	/* points to current gav end          */
static	size_t		gavtot;	/* total bytes used for all arguments */

static	char		**gavnew(/*@only@*/ char **);
static	char		*gcat(const char *, const char *, int);
static	char		**glob1(/*@only@*/ char **, char *, int *);
static	int		glob2(const UChar *, const UChar *);
static	void		gsort(char **);
/*@null@*/ static
	DIR		*gopendir(/*@out@*/ char *, const char *);

/*
 * Attempt to generate file-name arguments which match the given
 * pattern arguments in av.  Return a pointer to a newly allocated
 * argument vector, gav, on success.  Do not return on error.
 */
static char **
glob(char **av)
{
	char **gav;	/* points to generated argument vector */
	int pmc = 0;	/* pattern-match count                 */

	gav = xmalloc(GAVNEW * sizeof(char *));

	*gav = NULL, gavp = gav;
	gave = &gav[GAVNEW - 1];
	while (*av != NULL) {
		*av = gtrim(*av);
		gav = glob1(gav, *av, &pmc);
		av++;
	}
	gavp = NULL;

	if (pmc == 0)
		err(SH_ERR, FMT1S, ERR_NOMATCH);

	return gav;
}

static char **
gavnew(char **gav)
{
	size_t siz;
	ptrdiff_t gidx;

	if (gavp == gave) {
		gidx = gavp - gav;
		siz  = (gidx + 1 + GAVNEW) * sizeof(char *);
		gav  = xrealloc(gav, siz);
		gavp = gav + gidx;
		gave = &gav[gidx + GAVNEW];
	}
	return gav;
}

static char *
gcat(const char *src1, const char *src2, int slash)
{
	size_t siz;
	char *b, buf[PATHMAX], c, *dst;
	const char *s;

	*buf = '\0', b = buf, s = src1;
	while ((c = *s++) != '\0') {
		if (b >= &buf[PATHMAX - 1])
			err(SH_ERR, FMT1S, strerror(ENAMETOOLONG));
		*b++ = c;
	}
	if (slash)
		*b++ = '/';
	s = src2;
	do {
		if (b >= &buf[PATHMAX])
			err(SH_ERR, FMT1S, strerror(ENAMETOOLONG));
		*b++ = c = *s++;
	} while (c != '\0');

	siz = b - buf;
	gavtot += siz;
	if (gavtot > GAVMAX)
		err(SH_ERR, FMT1S, ERR_ALTOOLONG);
	dst = xmalloc(siz);

	(void)memcpy(dst, buf, siz);
	return dst;
}

static char **
glob1(char **gav, char *as, int *pmc)
{
	DIR *dirp;
	struct dirent *entry;
	ptrdiff_t gidx;
	int slash;
	char dirbuf[PATHMAX], *ps;
	const char *ds;

	ds = as;
	if ((ps = gchar(as)) == NULL) {
		atrim(as);
		gav = gavnew(gav);
		*gavp++ = gcat(as, "", 0);
		*gavp = NULL;
		return gav;
	}
	slash = 0;
	for (;;) {
		if (ps == ds) {
			ds = "";
			break;
		}
		if (*--ps == '/') {
			*ps = '\0';
			if (ds == ps)
				ds = "/";
			else
				slash = 1;
			ps++;
			break;
		}
	}
	if ((dirp = gopendir(dirbuf, *ds != '\0' ? ds : ".")) == NULL) {
		err(SH_ERR, FMT1S, ERR_NODIR);
		/*NOTREACHED*/
	}
	if (*ds != '\0')
		ds = dirbuf;
	gidx = gavp - gav;
	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_name[0] == '.' && *ps != '.')
			continue;
		if (glob2((UChar *)entry->d_name, (UChar *)ps)) {
			gav = gavnew(gav);
			*gavp++ = gcat(ds, entry->d_name, slash);
			(*pmc)++;
		}
	}
	(void)closedir(dirp);
	gsort(gav + gidx);
	*gavp = NULL;
	return gav;
}

#define	UCHAR(c)	((UChar)c)
#define	EOS		UCHAR('\0')

static int
glob2(const UChar *ename, const UChar *pattern)
{
	int cok, rok;		/* `[...]' - cok (class), rok (range) */
	UChar c, ec, pc;
	const UChar *e, *n, *p;

	e = ename;
	p = pattern;

	ec = *e++;
	switch (pc = *p++) {
	case '\0':
		return ec == EOS;

	case '*':
		/*
		 * Ignore all but the last `*' in a group of consecutive
		 * `*' characters to avoid unnecessary glob2() recursion.
		 */
		while (*p++ == UCHAR('*'))
			;	/* nothing */
		if (*--p == EOS)
			return 1;
		e--;
		while (*e != EOS)
			if (glob2(e++, p))
				return 1;
		break;

	case '?':
		if (ec != EOS)
			return glob2(e, p);
		break;

	case '[':
		if (*p == EOS)
			break;
		for (c = EOS, cok = rok = 0, n = p + 1; ; ) {
			pc = *p++;
			if (pc == UCHAR(']') && p > n) {
				if (cok > 0 || rok > 0)
					return glob2(e, p);
				break;
			}
			if (*p == EOS)
				break;
			if (pc == UCHAR('-') && c != EOS && *p != UCHAR(']')) {
				if ((pc = *p++) == UCHAR('\\'))
					pc = *p++;
				if (*p == EOS)
					break;
				if (c <= ec && ec <= pc)
					rok++;
				else if (c == ec)
					cok--;
				c = EOS;
			} else {
				if (pc == UCHAR('\\')) {
					pc = *p++;
					if (*p == EOS)
						break;
				}
				c = pc;
				if (ec == c)
					cok++;
			}
		}
		break;

	case '\\':
		if (*p != EOS)
			pc = *p++;
		/*FALLTHROUGH*/

	default:
		if (pc == ec)
			return glob2(e, p);
	}
	return 0;
}

static void
gsort(char **ogavp)
{
	char **p1, **p2, *sap;

	p1 = ogavp;
	while (gavp - p1 > 1) {
		p2 = p1;
		while (++p2 < gavp)
			if (strcmp(*p1, *p2) > 0) {
				sap = *p1;
				*p1 = *p2;
				*p2 = sap;
			}
		p1++;
	}
}

static DIR *
gopendir(char *buf, const char *dname)
{
	char *b;
	const char *d;

	for (*buf = '\0', b = buf, d = dname; b < &buf[PATHMAX]; b++, d++)
		if ((*b = *d) == '\0') {
			atrim(buf);
			break;
		}
	return (b >= &buf[PATHMAX]) ? NULL : opendir(buf);
}
