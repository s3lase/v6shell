/*
 * goto.c - a port of the Sixth Edition (V6) Unix transfer command
 */
/*-
 * Copyright (c) 2004, 2005, 2006
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
 *	@(#)goto.c	1.4 (jneitzel) 2006/06/25
 */
/*
 *	Derived from: Sixth Edition Unix /usr/source/s1/goto.c
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
OSH_SOURCEID("goto.c	1.4 (jneitzel) 2006/06/25");
#endif	/* !lint */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define	LABELSIZE	64	/* size of the label buffer */

static off_t	offset;

static int	getlabel(/*@out@*/ char *, int, size_t);
static int	xgetc(void);

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
int
main(int argc, char **argv)
{
	size_t siz;
	char label[LABELSIZE];

	if (argc < 2 || *argv[1] == '\0' || isatty(STDIN_FILENO)) {
		(void)fprintf(stderr, "goto: error\n");
		return 124;
	}
	if ((siz = strlen(argv[1]) + 1) > sizeof(label)) {
		(void)fprintf(stderr, "goto: %s: label too long\n", argv[1]);
		return 124;
	}
	if (lseek(STDIN_FILENO, (off_t)0, SEEK_SET) == -1) {
		(void)fprintf(stderr, "goto: cannot seek\n");
		return 124;
	}

	while (getlabel(label, *argv[1] & 0377, siz))
		if (strcmp(label, argv[1]) == 0) {
			(void)lseek(STDIN_FILENO, offset, SEEK_SET);
			return 0;
		}

	(void)fprintf(stderr, "goto: %s: label not found\n", argv[1]);
	return 1;
}

/*
 * Get possible label for comparison against argv[1].
 * Return 1 if possible label found.
 * Return 0 at end-of-file.
 */
static int
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
		 * Copy possible label (first word only) to buf.
		 * However, if it is too long, abort the copy,
		 * and simply ignore it.
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
		return 1;
	}
	*buf = '\0';
	return 0;
}

static int
xgetc(void)
{

	offset++;
	return getchar();
}
