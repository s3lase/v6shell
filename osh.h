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

#ifndef	OSH_H
#define	OSH_H

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
#define	FC_ERR		124	/* fatal child error (changed in pwait()) */
#define	SH_ERR		2	/* shell-detected error (default value)   */
#define	SH_FALSE	1
#define	SH_TRUE		0

#define	FMT1S		"%s\n"
#define	FMT2S		"%s: %s\n"
#define	FMT3S		"%s: %s: %s\n"

#define	EQUAL(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)

/*
 * Shell special built-in (sbi) command keys
 */
enum sbikey {
	SBI_NULL, SBI_CHDIR,  SBI_EXIT,   SBI_LOGIN,    SBI_NEWGRP, SBI_SHIFT,
	SBI_WAIT, SBI_SIGIGN, SBI_SETENV, SBI_UNSETENV, SBI_UMASK,  SBI_SOURCE,
	SBI_EXEC, SBI_IF, SBI_GOTO, SBI_FD2, SBI_UNKNOWN
};

/*
 * Shell sbi command structure
 */
struct sbicmd {
	const char *sbi_command;
	const enum sbikey sbi_key;
};
extern	const struct sbicmd	sbi[];	/* shell sbi command structure array */
extern	uid_t			euid;	/* effective shell user ID           */

enum sbikey	cmd_key_lookup(const char *);
void		omsg(int, const char *, va_list);
int		uexec(enum sbikey, int, char **);

#endif	/* !OSH_H */
