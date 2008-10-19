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
 * Shell special built-in (sbi) command keys
 */
enum sbikey {
	SBI_NULL,     SBI_CHDIR, SBI_ECHO,   SBI_EXEC,   SBI_EXIT,
	SBI_FD2,      SBI_GOTO,  SBI_IF,     SBI_LOGIN,  SBI_NEWGRP,
	SBI_SETENV,   SBI_SHIFT, SBI_SIGIGN, SBI_SOURCE, SBI_UMASK,
	SBI_UNSETENV, SBI_WAIT,  SBI_UNKNOWN
};

extern	uid_t	euid;	/* effective shell user ID */

/* osh.c */
enum sbikey	cmd_lookup(const char *);
void		fd_print(int, const char *, /*@printflike@*/ ...);
void		omsg(int, const char *, va_list);

/* util.c */
int		uexec(enum sbikey, int, char **);

#endif	/* !OSH_H */
