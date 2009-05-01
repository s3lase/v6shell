/*
 * glob6.c - a port of the Sixth Edition (V6) UNIX global command
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
 *	Derived from: Sixth Edition UNIX /usr/source/s1/glob.c
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

#include "defs.h"
#include "err.h"
#include "pexec.h"

static	const char	**gavp;	/* points to current gav position     */
static	const char	**gave;	/* points to current gav end          */
static	size_t		gavtot;	/* total bytes used for all arguments */

static	const char	**gavnew(/*@only@*/ const char **);
static	char		*gcat(const char *, const char *);
static	const char	**glob1(/*@only@*/ const char **, char *, int *);
static	bool		glob2(const UChar *, const UChar *);
static	void		gsort(const char **);
/*@null@*/
static	DIR		*gopendir(const char *);

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
	const char **gav;	/* points to generated argument vector */
	int pmc = 0;		/* pattern-match count                 */

	setmyerrexit(util_errexit);
	setmypid(getpid());

	/*
	 * Set-ID execution is not supported.
	 */
	if (geteuid() != getuid() || getegid() != getgid())
		err(SH_ERR, FMT1S, ERR_SETID);

	if (argc < 2)
		err(SH_ERR, FMT1S, ERR_GARGCOUNT);

	if ((gav = malloc(GAVNEW * sizeof(char *))) == NULL)
		err(SH_ERR, FMT1S, ERR_NOMEM);

	*gav = NULL, gavp = gav;
	gave = &gav[GAVNEW - 1];
	while (*++argv != NULL)
		gav = glob1(gav, *argv, &pmc);
	gavp = NULL;

	if (pmc == 0)
		err(SH_ERR, FMT1S, ERR_NOMATCH);

	(void)pexec(gav[0], (char *const *)gav);
	if (errno == ENOEXEC)
		err(SH_ERR, FMT1S, ERR_NOSHELL);
	if (errno == E2BIG)
		err(SH_ERR, FMT1S, ERR_ALTOOLONG);
	err(SH_ERR, FMT1S, ERR_GNOTFOUND);
	/*NOTREACHED*/
	return SH_ERR;
}

static const char **
gavnew(const char **gav)
{
	size_t siz;
	ptrdiff_t gidx;
	const char **nav;
	static unsigned mult = 1;

	if (gavp == gave) {
		mult *= GAVMULT;
		gidx  = (ptrdiff_t)(gavp - gav);
		siz   = (size_t)((gidx + (GAVNEW * mult)) * sizeof(char *));
		if ((nav = realloc(gav, siz)) == NULL)
			err(SH_ERR, FMT1S, ERR_NOMEM);
		gav   = nav;
		gavp  = gav + gidx;
		gave  = &gav[gidx + (GAVNEW * mult) - 1];
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
			err(SH_ERR, FMT1S, strerror(ENAMETOOLONG));
		if ((c &= ASCII) == '\0') {
			*b++ = '/';
			break;
		}
		*b++ = c;
	}

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
	if ((dst = malloc(siz)) == NULL)
		err(SH_ERR, FMT1S, ERR_NOMEM);

	(void)memcpy(dst, buf, siz);
	return dst;
}

static const char **
glob1(const char **gav, char *as, int *pmc)
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
		err(SH_ERR, FMT1S, ERR_PATTOOLONG);
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
		err(SH_ERR, FMT1S, ERR_NODIR);
	gidx = (ptrdiff_t)(gavp - gav);
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

static bool
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
			return true;
		e--;
		while (*e != EOS)
			if (glob2(e++, p))
				return true;
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
	return false;
}

static void
gsort(const char **ogavp)
{
	const char **p1, **p2, *sap;

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
