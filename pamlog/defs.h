/* defs */

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */


#ifndef	DEFS_INCLUDE
#define	DEFS_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<time.h>

#include	<bfile.h>
#include	<localmisc.h>


#ifndef	FD_STDIN
#define	FD_STDIN	0
#define	FD_STDOUT	1
#define	FD_STDERR	2
#endif

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	2048
#endif

#ifndef	MAXNAMELEN
#define	MAXNAMELEN	256
#endif

#ifndef	MSGBUFLEN
#define	MSGBUFLEN	2048
#endif

#ifndef	SVCNAMELEN
#define	SVCNAMELEN	32
#endif

#ifndef	PASSWDLEN
#define	PASSWDLEN	32
#endif

#ifndef	LOGNAMELEN
#define	LOGNAMELEN	32
#endif

#ifndef	LOGIDLEN
#define	LOGIDLEN	15
#endif

#ifndef	MAILADDRLEN
#define	MAILADDRLEN	(3 * MAXHOSTNAMELEN)
#endif

#ifndef	TIMEBUFLEN
#define	TIMEBUFLEN	80
#endif

#ifndef	LINEBUFLEN
#ifdef	LINE_MAX
#define	LINEBUFLEN	MAX(LINE_MAX,2048)
#else
#define	LINEBUFLEN	2048
#endif
#endif

#ifndef	DEVPREFIX
#define	DEVPREFIX	"/dev/"
#endif

#ifndef	DEVPREFIXLEN
#define	DEVPREFIXLEN	5
#endif

#define	BUFLEN		100

#ifndef	DEBUGLEVEL
#define	DEBUGLEVEL(n)	(mip->debuglevel >= (n))
#endif


struct proginfo_flags {
	uint		outfile:1 ;
	uint		errfile:1 ;
	uint		quiet:1 ;
} ;

struct proginfo {
	const char	**envv ;
	const char	*pwd ;
	const char	*progdname ;
	const char	*progname ;
	const char	*pr ;
	const char	*version ;
	const char	*banner ;
	const char	*searchname ;
	const char	*helpfname ;
	const char	*tmpdname ;
	bfile		*efp ;
	struct proginfo_flags	f ;
	time_t		daytime ;
	int		debuglevel ;
	int		verboselevel ;
} ;


#endif /* DEFS_INCLUDE */


