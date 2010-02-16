/*
 * osh - an enhanced port of the Sixth Edition (V6) UNIX Thompson shell
 */
/*-
 * Copyright (c) 2004-2010
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

#ifndef	DEFS_H
#define	DEFS_H

/*
 * required header files
 */
#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef	PATH_MAX
#define	PATHMAX		PATH_MAX
#else
#define	PATHMAX		1024
#endif

#ifdef	ARG_MAX
#define	GAVMAX		ARG_MAX
#else
#define	GAVMAX		1048576
#endif
#define	GAVMULT		2U	/* base GAVNEW reallocation multiplier        */
#define	GAVNEW		128U	/* base # of new arguments per gav allocation */

#ifdef	_POSIX_OPEN_MAX
#define	FDFREEMIN	_POSIX_OPEN_MAX
#else
#define	FDFREEMIN	20	/* Value is the same as _POSIX_OPEN_MAX.    */
#endif
#define	FDFREEMAX	4096	/* Arbitrary maximum value for fd_free().   */

#define	LINEMAX		2048	/* 1000 in the original Sixth Edition shell */
#define	WORDMAX		1024	/*   50 ...                                 */

#define	ASCII		0177
#define	QUOTE		0200

#define	SBUFMM(m)	((32 * (m)) + 1)/* small buffer max multiplier      */
#define	DOLMAX		SBUFMM(1)	/* used by osh(1) and sh6(1) shells */
#define	LABELMAX	SBUFMM(2)	/* used by goto(1) utility          */

/*
 * Following standard conventions, file descriptors 0, 1, and 2 are used
 * for standard input, standard output, and standard error respectively.
 */
#define	FD0		STDIN_FILENO
#define	FD1		STDOUT_FILENO
#define	FD2		STDERR_FILENO

#define	F_GZ		1		/* `-s'  primary for if(1) utility  */
#define	F_OT		2		/* `-ot' ...                        */
#define	F_NT		3		/* `-nt' ...                        */
#define	F_EF		4		/* `-ef' ...                        */

#define	DOLSUB		true
#define	FORKED		true
#define	RETERR		true

#define	EQUAL(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)
#define	IS_DIGIT(d, c)	((d) >= 0 && (d) <= 9 && "0123456789"[(d) % 10] == (c))

/*
 * special character literals
 */
#define	BANG		'!'
#define	COLON		':'
#define	DOLLAR		'$'
#define	DOT		'.'
#define	EOL		'\n'
#define	EOS		'\0'
#define	HASH		'#'
#define	SLASH		'/'
#define	SPACE		' '
#define	TAB		'\t'
#define	BQUOT		'\\'
#define	DQUOT		'"'
#define	SQUOT		'\''

#define	LPARENTHESIS	'('
#define	RPARENTHESIS	')'
#define	SEMICOLON	';'
#define	AMPERSAND	'&'
#define	VERTICALBAR	'|'
#define	CARET		'^'
#define	LESSTHAN	'<'
#define	GREATERTHAN	'>'

#define	ASTERISK	'*'
#define	QUESTION	'?'
#define	LBRACKET	'['
#define	RBRACKET	']'
#define	HYPHEN		'-'

/*
 * special string literals
 */
#define	EOC		";&\n"
#define	GLOBCHARS	"*?["
#define	QUOTPACK	"\"'"
#define	REDIRERR	"(<>"
#define	WORDPACK	" \t\"'();&|^<>\n"

/*
 * typedefs and related macros
 */
typedef	unsigned char	UChar;
#define	UCHAR(c)	((UChar)(c))
#define	UCPTR(p)	((UChar *)(p))

#endif	/* !DEFS_H */
