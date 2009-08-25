: $h/.osh.login - " Modify to taste. "
:
: "  @(#)$Id: dot.osh.login 548 2008-11-06 23:09:27Z jneitzel $  "
:
: "  The author of this file, J.A. Neitzel <jan (at) v6shell (dot) org>,  "
: "  hereby grants it to the public domain.                               "
:
: "  From:  http://v6shell.org/rc_files/  "
:

: fd2 -e echo "debug: Executing `"$h/.osh.login"' now..."

if X$u != Xroot goto continue
	:
	: " Give the superuser a different default PATH. "
	:
	setenv	PATH	/opt/local/bin:/usr/local/bin
	setenv	PATH	$p:/bin:/sbin:/usr/bin:/usr/sbin:/usr/X11/bin
	: fallthrough

: continue - " Okay, continue as normal. "

	setenv	MANPATH	\
		/opt/local/share/man:/usr/local/man:/usr/share/man:/usr/X11/share/man
	setenv	PATH		/usr/local/v6bin:$h/v6bin:$p:/usr/games
	setenv	EXECSHELL	@EXECSHELL@
	setenv	EDITOR		'/bin/ed -p:'
	setenv	VISUAL		/opt/local/bin/vim
	setenv	PAGER		'less -is'
	setenv	LESS		-M
	setenv	TZ		UTC

	@SOURCE_OSHDIR@

	: " Print a message or two at login time. "
	echo ; date '+%A, %Y-%m-%d, %T %Z' ; echo
	if -x /usr/games/fortune /usr/games/fortune -s
	: zero status
