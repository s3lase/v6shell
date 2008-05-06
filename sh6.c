/*
 * sh6.c - a port of the Sixth Edition (V6) Unix Thompson shell
 */
/*-
 * Copyright (c) 2004-2008
 *	Jeffrey Allen Neitzel <jneitzel (at) v6shell (dot) org>.
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
 *	Derived from: Sixth Edition Unix /usr/source/s2/sh.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pexec.h"

/*
 * Constants
 */
#define	LINEMAX		2048	/* 1000 in the original Sixth Edition shell */
#define	WORDMAX		1024	/*   50 ...                                 */

#ifdef	_POSIX_OPEN_MAX
#define	FDFREEMIN	_POSIX_OPEN_MAX
#else
#define	FDFREEMIN	20	/* Value is the same as _POSIX_OPEN_MAX.  */
#endif
#define	FDFREEMAX	65536	/* Arbitrary maximum value for fd_free(). */
#define	PIDMAX		99999	/* Maximum value for both prn() and apid. */

#define	FD0		STDIN_FILENO
#define	FD1		STDOUT_FILENO
#define	FD2		STDERR_FILENO

#define	CH_SIGINT	01
#define	CH_SIGQUIT	02

#define	FC_ERR		124	/* fatal child error (changed in pwait()) */
#define	SH_ERR		2	/* shell-detected error (default value)   */

#define	ASCII		0177
#define	QUOTE		0200

/*
 * Macros
 */
#define	DOLDIGIT(d, c)	((d) >= 0 && (d) <= 9 && "0123456789"[(d) % 10] == (c))
#define	DOLSUB		true
#define	EQUAL(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)
#define	EXIT(s)		((getpid() == spid) ? exit((s)) : _exit((s)))

/*
 * Shell command-tree structure
 */
struct tnode {
/*@null@*/struct tnode	 *nleft;	/* Pointer to left node.           */
/*@null@*/struct tnode	 *nright;	/* Pointer to right node.          */
/*@null@*/struct tnode	 *nsub;		/* Pointer to TSUBSHELL node.      */
/*@null@*/char		**nav;		/* Argument vector for TCOMMAND.   */
/*@null@*/char		 *nfin;		/* Pointer to input file (<).      */
/*@null@*/char		 *nfout;	/* Pointer to output file (>, >>). */
	  uint8_t	  ntype;	/* Node type (see below).          */
	  uint8_t	  nflags;	/* Node/command flags (see below). */
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

static	char		apid[6];	/* $$ - ASCII shell process ID      */
/*@null@*/
static	const char	*argv2p;	/* string for `-c' option           */
static	int		chintr;		/* SIGINT / SIGQUIT flag for child  */
static	int		dolc;		/* $N dollar-argument count         */
/*@null@*/
static	const char	*dolp;		/* $N and $$ dollar-value pointer   */
static	char	*const	*dolv;		/* $N dollar-argument value array   */
static	bool		error;		/* error flag for read/parse errors */
static	const char	*error_message;	/* error message for read errors    */
static	bool		glob_flag;	/* glob flag for `*', `?', `['      */
static	char		line[LINEMAX];	/* command-line buffer              */
static	char		*linep;
static	const char	*name;		/* $0 - shell command name          */
static	int		nul_count;	/* `\0'-character count (per line)  */
static	int		one_line_flag;	/* one-line flag for `-t' option    */
static	char		peekc;		/* just-read, pushed-back character */
/*@null@*/ /*@observer@*/
static	const char	*prompt;	/* interactive-shell prompt pointer */
static	pid_t		spid;		/* shell process ID                 */
static	int		status;		/* shell exit status                */
static	char		*word[WORDMAX];	/* argument/word pointer array      */
static	char		**wordp;

/*
 * Function prototypes
 */
static	void		rpx_line(void);
static	void		get_word(void);
static	char		xgetc(bool);
static	char		readc(void);
static	struct tnode	*talloc(void);
static	void		tfree(/*@null@*/ /*@only@*/ struct tnode *);
/*@null@*/
static	struct tnode	*syntax(char **, char **);
/*@null@*/
static	struct tnode	*syn1(char **, char **);
/*@null@*/
static	struct tnode	*syn2(char **, char **);
/*@null@*/
static	struct tnode	*syn3(char **, char **);
static	void		vscan(/*@null@*/ char **, int (*)(int));
static	void		ascan(/*@null@*/ char *, int (*)(int));
static	int		tglob(int);
static	int		trim(int);
static	bool		any(int, const char *);
static	void		execute(/*@null@*/ struct tnode *,
				/*@null@*/ int *, /*@null@*/ int *);
/*@maynotreturn@*/
static	void		err(/*@null@*/ const char *, int);
static	void		prn(int);
static	void		prs(/*@null@*/ const char *);
static	void		xputc(int);
static	void		pwait(pid_t);
static	void		sh_init(void);
static	void		sh_magic(void);
static	void		fd_free(void);
static	bool		fd_isdir(int);
/*@out@*/
static	void		*xmalloc(size_t);

/*
 * NAME
 *	sh6 - shell (command interpreter)
 *
 * SYNOPSIS
 *	sh6 [- | -c string | -t | file [arg1 ...]]
 *
 * DESCRIPTION
 *	See the sh6(1) manual page for full details.
 */
int
main(int argc, char **argv)
{

	sh_init();
	if (argv[0] == NULL || *argv[0] == '\0')
		err("Invalid argument list", SH_ERR);
	if (fd_isdir(FD0))
		goto done;

	if (argc > 1) {
		name = argv[1];
		dolv = &argv[1];
		dolc = argc - 1;
		if (*argv[1] == '-') {
			chintr = 1;
			if (argv[1][1] == 'c' && argc > 2) {
				dolv  += 1;
				dolc  -= 1;
				argv2p = argv[2];
			} else if (argv[1][1] == 't')
				one_line_flag = 2;
		} else {
			(void)close(FD0);
			if (open(argv[1], O_RDONLY) != FD0) {
				prs(argv[1]);
				err(": cannot open", SH_ERR);
			}
			if (fd_isdir(FD0))
				goto done;
		}
	} else {
		chintr = 1;
		if (isatty(FD0) != 0 && isatty(FD2) != 0) {
			prompt = (geteuid() != 0) ? "% " : "# ";
			(void)signal(SIGTERM, SIG_IGN);
		}
	}
	if (chintr != 0) {
		chintr = 0;
		if (signal(SIGINT, SIG_IGN) == SIG_DFL)
			chintr |= CH_SIGINT;
		if (signal(SIGQUIT, SIG_IGN) == SIG_DFL)
			chintr |= CH_SIGQUIT;
	}
	fd_free();
	sh_magic();

loop:
	if (prompt != NULL)
		prs(prompt);
	rpx_line();
	goto loop;

done:
	return status;
}

/*
 * Read, parse, and execute a command line.
 */
static void
rpx_line(void)
{
	struct tnode *t;
	sigset_t nmask, omask;
	char *wp;

	linep = line;
	wordp = word;
	error = false;
	nul_count = 0;
	do {
		wp = linep;
		get_word();
	} while (*wp != '\n');

	if (error) {
		err(error_message, SH_ERR);
		return;
	}

	if (wordp - word > 1) {
		(void)sigfillset(&nmask);
		(void)sigprocmask(SIG_SETMASK, &nmask, &omask);
		t = NULL;
		t = syntax(word, wordp);
		(void)sigprocmask(SIG_SETMASK, &omask, NULL);
		if (error)
			err("syntax error", SH_ERR);
		else
			execute(t, NULL, NULL);
		tfree(t);
		t = NULL;
	}
}

/*
 * Copy a word from the standard input into the line buffer,
 * and point to it w/ *wordp.  Each copied word is represented
 * in line as an individual `\0'-terminated string.
 */
static void
get_word(void)
{
	char c, c1;

	*wordp++ = linep;

loop:
	switch (c = xgetc(DOLSUB)) {
	case ' ':
	case '\t':
		goto loop;

	case '"':
	case '\'':
		c1 = c;
		while ((c = xgetc(!DOLSUB)) != c1) {
			if (c == '\n') {
				if (!error)
					error_message = "syntax error";
				peekc = c;
				*linep++ = '\0';
				error = true;
				return;
			}
			if (c == '\\') {
				if ((c = xgetc(!DOLSUB)) == '\n')
					c = ' ';
				else {
					peekc = c;
					c = '\\';
				}
			}
			*linep++ = c | QUOTE;
		}
		break;

	case '\\':
		if ((c = xgetc(!DOLSUB)) == '\n')
			goto loop;
		c |= QUOTE;
		peekc = c;
		break;

	case '(': case ')':
	case ';': case '&':
	case '|': case '^':
	case '<': case '>':
	case '\n':
		*linep++ = c;
		*linep++ = '\0';
		return;

	default:
		peekc = c;
	}

	for (;;) {
		if ((c = xgetc(DOLSUB)) == '\\') {
			if ((c = xgetc(!DOLSUB)) == '\n')
				c = ' ';
			else
				c |= QUOTE;
		}
		if (any(c, " \t\"'();&|^<>\n")) {
			peekc = c;
			if (any(c, "\"'"))
				goto loop;
			*linep++ = '\0';
			return;
		}
		*linep++ = c;
	}
	/*NOTREACHED*/
}

/*
 * If dolsub == true, get either the next literal character from the
 * standard input or substitute the current $ dollar w/ the next
 * character of its value, which is pointed to by dolp.  Otherwise,
 * get only the next literal character from the standard input.
 */
static char
xgetc(bool dolsub)
{
	int n;
	char c;

	if (peekc != '\0') {
		c = peekc;
		peekc = '\0';
		return c;
	}

	if (wordp >= &word[WORDMAX - 2]) {
		wordp -= 4;
		while (xgetc(!DOLSUB) != '\n')
			;	/* nothing */
		wordp += 4;
		error_message = "Too many args";
		goto geterr;
	}
	if (linep >= &line[LINEMAX - 5]) {
		linep -= 10;
		while (xgetc(!DOLSUB) != '\n')
			;	/* nothing */
		linep += 10;
		error_message = "Too many characters";
		goto geterr;
	}

getd:
	if (dolp != NULL) {
		c = *dolp++ & ASCII;
		if (c != '\0')
			return c;
		dolp = NULL;
	}
	c = readc();
	if (c == '$' && dolsub) {
		c = readc();
		if (c >= '0' && c <= '9') {
			n = c - '0';
			if (DOLDIGIT(n, c) && n < dolc)
				dolp = (n > 0) ? dolv[n] : name;
			goto getd;
		}
		if (c == '$') {
			dolp = apid;
			goto getd;
		}
	}
	/* Ignore all NUL characters. */
	if (c == '\0') do {
		if (++nul_count >= LINEMAX) {
			error_message = "Too many characters";
			goto geterr;
		}
		c = readc();
	} while (c == '\0');
	return c;

geterr:
	error = true;
	return '\n';
}

/*
 * Read and return an ASCII-equivalent character from the string
 * pointed to by argv2p or from the standard input.  When reading
 * from argv2p, return the character or `\n' at end-of-string.
 * When reading from the standard input, return the character.
 * Otherwise, exit the shell w/ the current value of the global
 * variable status when appropriate.
 */
static char
readc(void)
{
	char c;

	if (argv2p != NULL) {
		if ((c = (*argv2p++ & ASCII)) == '\0') {
			argv2p = NULL;
			one_line_flag = 1;
			c = '\n';
		}
		return c;
	}
	if (one_line_flag == 1)
		exit(status);
	if (read(FD0, &c, (size_t)1) != 1)
		exit(status);
	if ((c &= ASCII) == '\n' && one_line_flag == 2)
		one_line_flag = 1;
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
	t->nav    = NULL;
	t->nfin   = NULL;
	t->nfout  = NULL;
	t->ntype  = 0;
	t->nflags = 0;
	return t;
}

/*
 * Deallocate memory for the shell command tree pointed to by t.
 */
static void
tfree(struct tnode *t)
{

	if (t == NULL)
		return;
	switch (t->ntype) {
	case TLIST:
	case TPIPE:
		tfree(t->nleft);
		tfree(t->nright);
		break;
	case TCOMMAND:
		free(t->nav);
		break;
	case TSUBSHELL:
		tfree(t->nsub);
		break;
	}
	free(t);
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
	error = true;
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
	uint8_t flags;
	int ac, c, n, subcnt;
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
					fin = *p;
				} else {
					if (fout != NULL)
						goto synerr;
					fout = *p;
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
			t->nav[ac] = p1[ac];
		t->nav[ac] = NULL;
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
	error = true;
	return NULL;
}

/*
 * Scan the entire argument vector pointed to by vp.
 * Each character of each argument is scanned w/ the function
 * pointed to by func.  Do nothing if vp is a NULL pointer.
 */
static void
vscan(char **vp, int (*func)(int))
{
	char *ap, c;

	if (vp == NULL)
		return;
	while ((ap = *vp++) != NULL)
		while ((c = *ap) != '\0')
			*ap++ = (*func)(c);
}

/*
 * Scan the argument pointed to by ap, a `\0'-terminated string.
 * Each character of the string is scanned w/ the function pointed
 * to by func.  Do nothing if ap is a NULL pointer.
 */
static void
ascan(char *ap, int (*func)(int))
{
	char c;

	if (ap == NULL)
		return;
	while ((c = *ap) != '\0')
		*ap++ = (*func)(c);
}

/*
 * Set the global variable glob_flag to true (1) if the character
 * c is a glob character.  Return the character in all cases.
 */
static int
tglob(int c)
{

	if (any(c, "*?["))
		glob_flag = true;
	return c;
}

/*
 * Return the character c converted back to its ASCII form.
 */
static int
trim(int c)
{

	return c & ASCII;
}

/*
 * Return true (1) if the character c matches any character in
 * the string pointed to by as.  Otherwise, return false (0).
 */
static bool
any(int c, const char *as)
{
	const char *s;

	s = as;
	while (*s != '\0')
		if (*s++ == c)
			return true;
	return false;
}

/*
 * Execute the shell command tree pointed to by t.
 */
static void
execute(struct tnode *t, int *pin, int *pout)
{
	struct tnode *t1;
	pid_t cpid;
	uint8_t f;
	int i, pfd[2];
	const char **gav;
	const char *cmd, *p;

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
			err("Cannot pipe - try again", SH_ERR);
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
			err("execute: Invalid command", SH_ERR);
			return;
		}
		cmd = t->nav[0];
		if (EQUAL(cmd, ":")) {
			status = 0;
			return;
		}
		if (EQUAL(cmd, "chdir")) {
			ascan(t->nav[1], trim);
			if (t->nav[1] == NULL) {
				prs(cmd);
				err(": arg count", SH_ERR);
			} else if (chdir(t->nav[1]) == -1) {
				prs(cmd);
				err(": bad directory", SH_ERR);
			} else
				status = 0;
			return;
		}
		if (EQUAL(cmd, "exit")) {
			if (prompt == NULL) {
				(void)lseek(FD0, (off_t)0, SEEK_END);
				EXIT(status);
			}
			return;
		}
		if (EQUAL(cmd, "login") || EQUAL(cmd, "newgrp")) {
			if (prompt != NULL) {
				p = (*cmd == 'l') ? PATH_LOGIN : PATH_NEWGRP;
				vscan(t->nav, trim);
				(void)signal(SIGINT, SIG_DFL);
				(void)signal(SIGQUIT, SIG_DFL);
				(void)pexec(p, (char *const *)t->nav);
				(void)signal(SIGINT, SIG_IGN);
				(void)signal(SIGQUIT, SIG_IGN);
			}
			prs(cmd);
			err(": cannot execute", SH_ERR);
			return;
		}
		if (EQUAL(cmd, "shift")) {
			if (dolc > 1) {
				dolv = &dolv[1];
				dolc--;
				status = 0;
				return;
			}
			prs(cmd);
			err(": no args", SH_ERR);
			return;
		}
		if (EQUAL(cmd, "wait")) {
			pwait(-1);
			return;
		}
		/*FALLTHROUGH*/

	case TSUBSHELL:
		f = t->nflags;
		if ((cpid = ((f & FNOFORK) != 0) ? 0 : fork()) == -1) {
			err("Cannot fork - try again", SH_ERR);
			return;
		}
		/**** Parent! ****/
		if (cpid != 0) {
			if (pin != NULL && (f & FPIN) != 0) {
				(void)close(pin[0]);
				(void)close(pin[1]);
			}
			if ((f & FPRS) != 0) {
				prn((int)cpid);
				prs("\n");
			}
			if ((f & FAND) != 0)
				return;
			if ((f & FPOUT) == 0)
				pwait(cpid);
			return;
		}
		/**** Child! ****/
		/*
		 * Redirect (read) input from pipe.
		 */
		if (pin != NULL && (f & FPIN) != 0) {
			if (dup2(pin[0], FD0) == -1)
				err("Cannot dup2", FC_ERR);
			(void)close(pin[0]);
			(void)close(pin[1]);
		}
		/*
		 * Redirect (write) output to pipe.
		 */
		if (pout != NULL && (f & FPOUT) != 0) {
			if (dup2(pout[1], FD1) == -1)
				err("Cannot dup2", FC_ERR);
			(void)close(pout[0]);
			(void)close(pout[1]);
		}
		/*
		 * Redirect (read) input from file.
		 */
		if (t->nfin != NULL && (f & FPIN) == 0) {
			f |= FFIN;
			ascan(t->nfin, trim);
			if ((i = open(t->nfin, O_RDONLY)) == -1) {
				prs(t->nfin);
				err(": cannot open", FC_ERR);
			}
			if (dup2(i, FD0) == -1)
				err("Cannot dup2", FC_ERR);
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
			ascan(t->nfout, trim);
			if ((i = open(t->nfout, i, 0666)) == -1) {
				prs(t->nfout);
				err(": cannot create", FC_ERR);
			}
			if (dup2(i, FD1) == -1)
				err("Cannot dup2", FC_ERR);
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
				if (open("/dev/null", O_RDONLY) != FD0) {
					prs("/dev/null");
					err(": cannot open", FC_ERR);
				}
			}
		} else {
			if ((chintr & CH_SIGINT) != 0)
				(void)signal(SIGINT, SIG_DFL);
			if ((chintr & CH_SIGQUIT) != 0)
				(void)signal(SIGQUIT, SIG_DFL);
		}
		(void)signal(SIGTERM, SIG_DFL);
		if (t->ntype == TSUBSHELL) {
			if ((t1 = t->nsub) != NULL)
				t1->nflags |= (f & (FFIN | FPIN | FINTR));
			execute(t1, NULL, NULL);
			_exit(status);
		}
		glob_flag = false;
		vscan(t->nav, tglob);
		if (glob_flag) {
			for (i = 0; t->nav[i] != NULL; i++)
				;	/* nothing */
			gav = xmalloc((i + 2) * sizeof(char *));
			gav[0] = "glob6";
			cmd = gav[0];
			(void)memcpy(&gav[1],&t->nav[0],(i+1)*sizeof(char *));
			(void)pexec(cmd, (char *const *)gav);
		} else {
			vscan(t->nav, trim);
			cmd = t->nav[0];
			(void)pexec(cmd, (char *const *)t->nav);
		}
		if (errno == ENOEXEC)
			err("No shell!", 125);
		if (errno != ENOENT && errno != ENOTDIR) {
			prs(cmd);
			err(": cannot execute", 126);
		}
		prs(cmd);
		err(": not found", 127);
		/*NOTREACHED*/
	}
}

/*
 * Handle all errors detected by the shell.
 */
static void
err(const char *msg, int es)
{

	if (msg != NULL) {
		prs(msg);
		prs("\n");
		status = es;
	}
	if (prompt == NULL) {
		(void)lseek(FD0, (off_t)0, SEEK_END);
		EXIT(status);
	}
	if (getpid() != spid)
		_exit(status);
}

/*
 * Print the integer n if it falls between 0 and PIDMAX, inclusive.
 */
static void
prn(int n)
{

	if (n < 0 || n > PIDMAX)
		return;
	if (n >= 10)
		prn(n / 10);
	xputc("0123456789"[n % 10]);
}

/*
 * Print the string pointed to by as.
 */
static void
prs(const char *as)
{
	const char *s;

	if (as == NULL)
		return;
	s = as;
	while (*s != '\0')
		xputc(*s++);
}

/*
 * Write the character c to the standard error.
 */
static void
xputc(int c)
{
	char cc;

	cc = c;
	(void)write(FD2, &cc, (size_t)1);
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
	int e, s;

	if (cp == 0)
		return;
	for (;;) {
		tp = wait(&s);
		if (tp == -1)
			break;
		if (s != 0) {
			if (WIFSIGNALED(s) != 0) {
				e = WTERMSIG(s);
				if (e >= XNSIG || (e >= 0 && sig[e] != NULL)) {
					if (tp != cp) {
						prn((int)tp);
						prs(": ");
					}
					if (e < XNSIG)
						prs(sig[e]);
					else {
						prs("Sig ");
						prn(e);
					}
					if (WCOREDUMP(s))
						prs(" -- Core dumped");
				}
				status = 128 + e;
			} else if (WIFEXITED(s) != 0)
				status = WEXITSTATUS(s);
			if (status >= FC_ERR) {
				if (status == FC_ERR)
					status = SH_ERR;
				err((status > 128) ? "" : NULL, status);
			}
		} else
			status = 0;
		if (tp == cp)
			break;
	}
}

/*
 * Initialize the shell.
 */
static void
sh_init(void)
{
	struct stat sb;
	pid_t p;
	int i;

	/*
	 * Set-ID execution is not supported.
	 */
	if (geteuid() != getuid() || getegid() != getgid())
		err("Set-ID execution denied", SH_ERR);

	/*
	 * Save the process ID of the shell both as an integer (spid)
	 * and as a 5-digit ASCII string (apid).  Each occurrence of
	 * `$$' in a command line is substituted w/ the value of apid.
	 */
	spid = p = getpid();
	for (i = 4; i >= 0 && p >= 0 && p <= PIDMAX; i--) {
		apid[i] = "0123456789"[p % 10];
		p /= 10;
	}

	/*
	 * Fail if any of the descriptors 0, 1, or 2 is not open.
	 */
	for (i = 0; i < 3; i++)
		if (fstat(i, &sb) == -1) {
			prn(i);
			err(": Bad file descriptor", SH_ERR);
		}

	/*
	 * Set the SIGCHLD signal to its default action.
	 * Correct operation of the shell requires that zombies
	 * be created for its children when they terminate.
	 */
	(void)signal(SIGCHLD, SIG_DFL);
}

/*
 * Ignore any `#!shell' sequence as the first line of a regular file.
 * The length of this line is limited to (LINEMAX - 1) characters.
 */
static void
sh_magic(void)
{
	struct stat sb;
	size_t len;

	if (fstat(FD0, &sb) == -1 || !S_ISREG(sb.st_mode))
		return;
	if (lseek(FD0, (off_t)0, SEEK_CUR) == 0) {
		if (readc() == '#' && readc() == '!') {
			for (len = 2; len < LINEMAX; len++)
				if (readc() == '\n')
					return;
			err("Too many characters", SH_ERR);
		}
		(void)lseek(FD0, (off_t)0, SEEK_SET);
	}
}

/*
 * Attempt to free or release all of the file descriptors in the
 * range from (fd_max - 1) through (FD2 + 1); the value of fd_max
 * may fall between FDFREEMIN and FDFREEMAX, inclusive.
 */
static void
fd_free(void)
{
	long fd_max;
	int fd;

	fd_max = sysconf(_SC_OPEN_MAX);
	if (fd_max < FDFREEMIN || fd_max > FDFREEMAX)
		fd_max = FDFREEMIN;
	for (fd = (int)fd_max - 1; fd > FD2; fd--)
		(void)close(fd);
}

/*
 * Return true (1) if the file descriptor fd refers to an open file
 * which is a directory.  Otherwise, return false (0).
 */
static bool
fd_isdir(int fd)
{
	struct stat sb;

	return fstat(fd, &sb) == 0 && S_ISDIR(sb.st_mode);
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
		err("Out of memory", SH_ERR);
		exit(SH_ERR);
	}
	return mp;
}
