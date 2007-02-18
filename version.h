/*-
 * Copyright (c) 2005, 2006, 2007
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
 *	@(#)version.h	1.5 (jneitzel) 2007/01/31
 */

#ifndef	VERSION_H
#define	VERSION_H

#ifndef	OSH_RELEASE
#define	OSH_RELEASE		"osh-20070131"
#endif	/* !OSH_RELEASE */

#ifndef	OSH_SOURCEID
# if __GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 4
#  define	ATTR	__attribute__((__used__))
# elif defined(__GNUC__)
#  define	ATTR	__attribute__((__unused__))
# else
#  define	ATTR	/* nothing */
# endif
#define	OSH_SOURCEID(string)	/*@unused@*/	\
		static const char sid[] ATTR = "\100(#)" OSH_RELEASE ": " string
#endif	/* !OSH_SOURCEID */

#endif	/* VERSION_H */
