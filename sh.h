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

#ifndef	SH_H
#define	SH_H

/*
 * osh.c, util.c, and sh6.c must include this header file.
 */

/*
 * Signal child flags
 */
enum scflags {
	SC_SIGINT  = 01,
	SC_SIGQUIT = 02,
	SC_SIGTERM = 04
};

#ifdef	OSH_SHELL
/*
 * Shell special built-in (sbi) command keys
 */
enum sbikey {
	SBI_NULL,     SBI_CHDIR, SBI_ECHO,   SBI_EXEC,   SBI_EXIT,
	SBI_FD2,      SBI_GOTO,  SBI_IF,     SBI_LOGIN,  SBI_NEWGRP,
	SBI_SETENV,   SBI_SHIFT, SBI_SIGIGN, SBI_SOURCE, SBI_UMASK,
	SBI_UNSETENV, SBI_WAIT,  SBI_UNKNOWN
};
#endif

/*
 * Shell command tree node flags
 */
enum tnflags {
	FAND    = 0001,		/* A `&'  designates asynchronous execution.  */
	FCAT    = 0002,		/* A `>>' appends output to file.             */
	FFIN    = 0004,		/* A `<'  redirects input from file.          */
	FPIN    = 0010,		/* A `|' or `^' redirects input from pipe.    */
	FPOUT   = 0020,		/* A `|' or `^' redirects output to pipe.     */
	FNOFORK = 0040,		/* No fork(2) for last command in `( list )'. */
	FINTR   = 0100,		/* Child process ignores SIGINT and SIGQUIT.  */
	FPRS    = 0200		/* Print process ID of child as a string.     */
};

/*
 * Shell command tree node structure
 */
struct tnode {
/*@null@*/struct tnode	 *nleft;	/* Pointer to left node.            */
/*@null@*/struct tnode	 *nright;	/* Pointer to right node.           */
/*@null@*/struct tnode	 *nsub;		/* Pointer to TSUBSHELL node.       */
/*@null@*/char		 *nfin;		/* Pointer to input file (<).       */
/*@null@*/char		 *nfout;	/* Pointer to output file (>, >>).  */
/*@null@*/char		**nav;		/* Argument vector for TCOMMAND.    */
#ifdef	OSH_SHELL
	  enum	 sbikey	  nkey;		/* Shell sbi command key.           */
#endif
	  enum	 tnflags  nflags;	/* Shell command tree node flags.   */
	  enum {			/* Shell command tree node type.    */
		TLIST     = 1,	/* pipelines separated by `;', `&', or `\n' */
		TPIPE     = 2,	/* commands separated by `|' or `^'         */
		TCOMMAND  = 3,	/* command  [arg ...]  [< in]  [> [>] out]  */
		TSUBSHELL = 4	/* ( list )            [< in]  [> [>] out]  */
	  } ntype;
};

#ifdef	OSH_SHELL
/* osh.c */
extern	uid_t	euid;	/* effective shell user ID */

enum sbikey	cmd_lookup(const char *);
void		fd_print(int, const char *, /*@printflike@*/ ...);
void		omsg(int, const char *, va_list);

/* util.c */
int		uexec(enum sbikey, int, char **);
#endif

#endif	/* !SH_H */
