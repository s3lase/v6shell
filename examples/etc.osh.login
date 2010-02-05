: @SYSCONFDIR@/osh.login - " Modify to taste. "
:
: "  @(#)$Id$  "
:
: "  The author of this file, J.A. Neitzel <jan (at) v6shell (dot) org>,  "
: "  hereby grants it to the public domain.                               "
:
: "  From:  http://v6shell.org/rc_files  "
:

sigign + 1 2 3 13 14 15 ; : " Ignore HUP, INT, QUIT, PIPE, ALRM, and TERM. "
sigign + 18 21 22 ;       : " Ignore job-control signals: TSTP, TTIN, TTOU "

:
: " Set a default PATH and umask for all users. "
:
setenv	PATH	/opt/local/bin:/usr/local/bin:/bin:/usr/bin:/usr/X11/bin
umask	0022

: fd2 -e echo "debug: Executing `@SYSCONFDIR@/osh.login' now..."

setenv	MAIL	/var/mail/$u
setenv	CTTY	$t
stty status '^T' <-

if X$h = X -o ! -d $h'' goto finish
if ! { mkdir $h/.osh.setenv.$$ } goto finish

	:
	: " Use the output from `uname -n' to `setenv HOST value'. "
	: " Notice that doing so requires using a temporary file.  "
	:
	uname -n | sed 's,\([^.]*\).*,setenv HOST \1,' >$h/.osh.setenv.$$/HOST
	source $h/.osh.setenv.$$/HOST
	rm -r $h/.osh.setenv.$$
	: fallthrough

: finish

sigign - 1 2 3 13 14 15 ; : " Reset the ignored, non-job-control signals. "
