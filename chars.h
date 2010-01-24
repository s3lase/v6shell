/*
 * osh - an enhanced port of the Sixth Edition (V6) UNIX Thompson shell
 */
/*-
 * Copyright (c) 2010
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

#ifndef	CHARS_H
#define	CHARS_H

/*
 * Characters
 */
typedef	unsigned char	UChar;
#define	UCHAR(c)	((UChar)(c))
#define	UCPTR(p)	((UChar *)(p))

#define	COLON		':'
#define	DOLLAR		'$'
#define	DOT		'.'
#define	EOL		'\n'
#define	EOS		'\0'
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
 * Strings
 */
#define	WORDPACK	" \t\"'();&|^<>\n"
#define	QUOTPACK	"\"'"
#define	EOC		";&\n"
#define	REDERR		"(<>"

#endif	/* !CHARS_H */
