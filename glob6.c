/*
 * glob6.c - a port of the Sixth Edition (V6) Unix global command
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
 *	@(#)glob6.c	1.4 (jneitzel) 2007/01/14
 */
/*
 *	Derived from: Sixth Edition Unix /usr/source/s1/glob.c
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
OSH_SOURCEID("glob6.c	1.4 (jneitzel) 2007/01/14");
#endif	/* !lint */

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pexec.h"

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

#define	GLOB6_ERR	2

#define	ASCII		0177
#define	QUOTE		0200

typedef	unsigned char	UChar;

static	char		**gavp;	/* points to current gav position     */
static	char		**gave;	/* points to current gav end          */
static	size_t		gavtot;	/* total bytes used for all arguments */

static	char		**gavnew(/*@only@*/ char **);
static	char		*gcat(const char *, const char *);
/*@noreturn@*/ static
	void		gerr(const char *);
static	char		**glob1(/*@only@*/ char **, char *, int *);
static	int		glob2(const UChar *, const UChar *);
static	void		gsort(char **);
/*@null@*/ static
	DIR		*gopendir(const char *);

/*
 * NAME
 *	glob6 - global command (file name generation)
 *
 * SYNOPSIS
 *	glob6 command [arg ...]
 *
 * DESCRIPTION
 *	See glob6(1) the manual page for full details.
 */
int
main(int argc, char **argv)
{
	char **gav;	/* points to generated argument vector */
	int pmc = 0;	/* pattern-match count                 */

	/*
	 * Set-ID execution is not supported.
	 */
	if (geteuid() != getuid() || getegid() != getgid())
		gerr("Set-ID execution denied\n");

	if (argc < 2)
		gerr("Arg count\n");

	if ((gav = malloc(GAVNEW * sizeof(char *))) == NULL)
		gerr("Out of memory\n");

	*gav = NULL, gavp = gav;
	gave = &gav[GAVNEW - 1];
	while (*++argv != NULL)
		gav = glob1(gav, *argv, &pmc);
	gavp = NULL;

	if (pmc == 0)
		gerr("No match\n");

	(void)pexec(gav[0], gav);
	if (errno == ENOEXEC)
		gerr("No shell!\n");
	if (errno == E2BIG)
		gerr("Arg list too long\n");
	gerr("Command not found.\n");
	/*NOTREACHED*/
	return GLOB6_ERR;
}

static char **
gavnew(char **gav)
{
	size_t siz;
	ptrdiff_t gidx;
	char **nav;

	if (gavp == gave) {
		gidx = gavp - gav;
		siz  = (gidx + 1 + GAVNEW) * sizeof(char *);
		if ((nav = realloc(gav, siz)) == NULL)
			gerr("Out of memory\n");
		gav  = nav;
		gavp = gav + gidx;
		gave = &gav[gidx + GAVNEW];
	}
	return gav;
}

static char *
gcat(const char *src1, const char *src2)
{
	size_t siz;
	char *b, buf[PATHMAX], c, *dst;
	const char *s;

	*buf = '\0', b = buf, s = src1;
	while ((c = *s++) != '\0') {
		if (b >= &buf[PATHMAX - 1])
			gerr("File name too long\n");
		if ((c &= ASCII) == '\0') {
			*b++ = '/';
			break;
		}
		*b++ = c;
	}

	s = src2;
	do {
		if (b >= &buf[PATHMAX])
			gerr("File name too long\n");
		*b++ = c = *s++;
	} while (c != '\0');

	siz = b - buf;
	gavtot += siz;
	if (gavtot > GAVMAX)
		gerr("Arg list too long\n");
	if ((dst = malloc(siz)) == NULL)
		gerr("Out of memory\n");

	(void)memcpy(dst, buf, siz);
	return dst;
}

static void
gerr(const char *msg)
{

	(void)write(STDERR_FILENO, msg, strlen(msg));
	exit(GLOB6_ERR);
}

static char **
glob1(char **gav, char *as, int *pmc)
{
	DIR *dirp;
	struct dirent *entry;
	ptrdiff_t gidx;
	const char *ds;
	char *ps;

	ds = ps = as;
	while (*ps != '*' && *ps != '?' && *ps != '[')
		if (*ps++ == '\0') {
			gav = gavnew(gav);
			*gavp++ = gcat(as, "");
			*gavp = NULL;
			return gav;
		}
	if (strlen(as) >= PATHMAX)
		gerr("Pattern too long\n");
	for (;;) {
		if (ps == ds) {
			ds = "";
			break;
		}
		if ((*--ps & ASCII) == '/') {
			*ps = '\0';
			if (ds == ps)
				ds = "/";
			*ps++ = (char)QUOTE;
			break;
		}
	}
	if ((dirp = gopendir(*ds != '\0' ? ds : ".")) == NULL)
		gerr("No directory\n");
	gidx = gavp - gav;
	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_name[0] == '.' && (*ps & ASCII) != '.')
			continue;
		if (glob2((UChar *)entry->d_name, (UChar *)ps)) {
			gav = gavnew(gav);
			*gavp++ = gcat(ds, entry->d_name);
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
	if ((ec = *e++) != EOS)
		if ((ec &= ASCII) == EOS)
			ec = (UChar)QUOTE;

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
				pc = *p++ & ASCII;
				if (*p == EOS)
					break;
				if (c <= ec && ec <= pc)
					rok++;
				else if (c == ec)
					cok--;
				c = EOS;
			} else {
				c = pc & ASCII;
				if (ec == c)
					cok++;
			}
		}
		break;

	default:
		if ((pc & ASCII) == ec)
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
gopendir(const char *dname)
{
	char *b, buf[PATHMAX];
	const char *d;

	for (*buf = '\0', b = buf, d = dname; b < &buf[PATHMAX]; b++, d++)
		if ((*b = (*d & ASCII)) == '\0')
			break;
	return (b >= &buf[PATHMAX]) ? NULL : opendir(buf);
}
