/*
 * osh - an enhanced port of the Sixth Edition (V6) UNIX Thompson shell
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

#ifndef	DEFS_H
#define	DEFS_H

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

/*
 * Following standard conventions, file descriptors 0, 1, and 2 are used
 * for standard input, standard output, and standard error respectively.
 */
#define	FD0		STDIN_FILENO
#define	FD1		STDOUT_FILENO
#define	FD2		STDERR_FILENO

/*
 * Exit status values
 */
#define	FC_ERR		124	/* fatal child error (changed in pwait())   */
#define	SH_ERR		2	/* shell-detected error (default value)     */
#define	SH_FALSE	1
#define	SH_TRUE		0

/*
 * Diagnostics
 */
#define	COLON		": "
#define	ERR_PAREN	") expected"
#define	ERR_GARGCOUNT	"Arg count"
#define	ERR_ALTOOLONG	"Arg list too long"
#define	ERR_BADDESCR	"Bad file descriptor"
#define	ERR_DUP2	"Cannot dup2"
#define	ERR_FORK	"Cannot fork - try again"
#define	ERR_PIPE	"Cannot pipe - try again"
#define	ERR_TRIM	"Cannot trim"
#define	ERR_GNOTFOUND	"Command not found."
#define	ERR_NAMTOOLONG	"File name too long"
#define	ERR_ALINVAL	"Invalid argument list"
#define	ERR_AVIINVAL	"Invalid argv index"
#define	ERR_NODIR	"No directory"
#define	ERR_NOHOMEDIR	"No home directory"
#define	ERR_NOMATCH	"No match"
#define	ERR_NOPWD	"No previous directory"
#define	ERR_NOSHELL	"No shell!"
#define	ERR_NOTTY	"No terminal!"
#define	ERR_NOMEM	"Out of memory"
#define	ERR_PATTOOLONG	"Pattern too long"
#define	ERR_SETID	"Set-ID execution denied"
#define	ERR_TMARGS	"Too many args"
#define	ERR_TMCHARS	"Too many characters"
#define	ERR_ARGCOUNT	"arg count"
#define	ERR_ARGUMENT	"argument expected"
#define	ERR_BADDIR	"bad directory"
#define	ERR_BADMASK	"bad mask"
#define	ERR_BADNAME	"bad name"
#define	ERR_BADSIGNAL	"bad signal"
#define	ERR_CREATE	"cannot create"
#define	ERR_EXEC	"cannot execute"
#define	ERR_OPEN	"cannot open"
#define	ERR_SEEK	"cannot seek"
#define	ERR_COMMAND	"command expected"
#define	ERR_DIGIT	"digit expected"
#define	ERR_GENERIC	"error"
#define	ERR_EXPR	"expression expected"
#define	ERR_LABNOTFOUND	"label not found"
#define	ERR_LABTOOLONG	"label too long"
#define	ERR_NOARGS	"no args"
#define	ERR_NOTDIGIT	"not a digit"
#define	ERR_NOTFOUND	"not found"
#define	ERR_OPERATOR	"operator expected"
#define	ERR_SYNTAX	"syntax error"
#define	ERR_OPUNKNOWN	"unknown operator"
#define	ERR_BRACE	"} expected"
#define	FMT1S		"%s\n"
#define	FMT2S		"%s: %s\n"
#define	FMT3S		"%s: %s: %s\n"

#define	LABELSIZE	64	/* label buffer size for the goto(1) utility */

#define	F_GZ		1	/* `-s'  primary for the if(1) utility       */
#define	F_OT		2	/* `-ot' primary for the if(1) utility       */
#define	F_NT		3	/* `-nt' primary for the if(1) utility       */
#define	F_EF		4	/* `-ef' primary for the if(1) utility       */

#define	DOLDIGIT(d, c)	((d) >= 0 && (d) <= 9 && "0123456789"[(d) % 10] == (c))
#define	DOLSUB		true
#define	EQUAL(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)
#define	FORKED		true
#define	RETERR		true

typedef	unsigned char	UChar;
#define	UCHAR(c)	((UChar)c)
#define	EOS		UCHAR('\0')

#endif	/* !DEFS_H */
