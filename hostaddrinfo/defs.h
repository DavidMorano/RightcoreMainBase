/* defs */


/* Copyright � 2005 David A�D� Morano.  All rights reserved. */


#ifndef	DEFS_INCLUDE
#define	DEFS_INCLUDE	1


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<time.h>

#include	<vecstr.h>
#include	<logfile.h>
#include	<dater.h>
#include	<ids.h>
#include	<localmisc.h>


#ifndef	nelem
#ifdef	nelements
#define	nelem		nelements
#else
#define	nelem(n)	(sizeof(n) / sizeof((n)[0]))
#endif
#endif

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

/* service name */
#ifndef	SVCNAMELEN
#define	SVCNAMELEN	32
#endif

#ifndef	LOGNAMELEN
#ifndef	LOGNAME_MAX
#define	LOGNAMELEN	LOGNAME_MAX
#else
#define	LOGNAMELEN	32
#endif
#endif

#ifndef	PASSWDLEN
#define	PASSWDLEN	32
#endif

#ifndef	USERNAMELEN
#ifndef	LOGNAME_MAX
#define	USERNAMELEN	LOGNAME_MAX
#else
#define	USERNAMELEN	32
#endif
#endif

#ifndef	GROUPNAMELEN
#ifndef	LOGNAME_MAX
#define	GROUPNAMELEN	LOGNAME_MAX
#else
#define	GROUPNAMELEN	32
#endif
#endif

/* Solaris project name */
#ifndef	PROJNAMELEN
#ifndef	LOGNAME_MAX
#define	PROJNAMELEN	LOGNAME_MAX
#else
#define	PROJNAMELEN	32
#endif
#endif

#ifndef	GNAMELEN
#define	GNAMELEN	80		/* GECOS name length */
#endif

#ifndef	REALNAMELEN
#define	REALNAMELEN	100		/* real name length */
#endif

#ifndef	LINEBUFLEN
#ifdef	LINE_MAX
#define	LINEBUFLEN	MAX(LINE_MAX,2048)
#else
#define	LINEBUFLEN	2048
#endif
#endif

/* timezone (zoneinfo) name */
#ifndef	TZLEN
#define	TZLEN		60
#endif

#ifndef	LOGIDLEN
#define	LOGIDLEN	15
#endif

/* mail address */
#ifndef	MAILADDRLEN
#define	MAILADDRLEN	(3 * MAXHOSTNAMELEN)
#endif

#ifndef	NULLDEV
#define	NULLDEV		"/dev/null"
#endif

#ifndef	TIMEBUFLEN
#define	TIMEBUFLEN	80
#endif

#ifndef	DEBUGLEVEL
#define	DEBUGLEVEL(n)	(pip->debuglevel >= (n))
#endif

#ifndef	STDINFNAME
#define	STDINFNAME	"*STDIN*"
#endif

#ifndef	STDOUTFNAME
#define	STDOUTFNAME	"*STDOUT*"
#endif

#ifndef	STDERRFNAME
#define	STDERRFNAME	"*STDERR*"
#endif

#ifndef	KBUFLEN
#define	KBUFLEN		100
#endif

#ifndef	VBUFLEN
#define	VBUFLEN		(2 * MAXPATHLEN)
#endif

#ifndef	EBUFLEN
#define	EBUFLEN		(3 * MAXPATHLEN)
#endif

#define	BUFLEN		MAX((MAXPATHLEN + MAXNAMELEN),MAXHOSTNAMELEN)

#ifndef	EX_OLDER
#define	EX_OLDER	1
#endif

#define	PROGINFO	struct proginfo
#define	PROGINFO_FL	struct proginfo_flags

#define	PIVARS		struct pivars

#define	ARGINFO		struct arginfo


struct proginfo_flags {
	uint		progdash:1 ;
	uint		akopts:1 ;
	uint		aparams:1 ;
	uint		quiet:1 ;
	uint		errfile:1 ;
	uint		outfile:1 ;
	uint		configfile:1 ;
	uint		pidfile:1 ;
	uint		lockfile:1 ;
	uint		logfile:1 ;
	uint		msfile:1 ;
	uint		config:1 ;
	uint		pidlock:1 ;
	uint		daemon:1 ;
	uint		log:1 ;
	uint		speed:1 ;
	uint		geekout:1 ;
	uint		nodeonly:1 ;
	uint		tmpdate:1 ;
	uint		disable:1 ;
	uint		all:1 ;
	uint		def:1 ;
	uint		list:1 ;
	uint		linebuf:1 ;
	uint		runint:1 ;
	uint		disint:1 ;
	uint		pollint:1 ;
	uint		lockint:1 ;
	uint		markint:1 ;
	uint		auth:1 ;
	uint		cname:1 ;
	uint		rs:1 ;
} ;

struct proginfo {
	vecstr		stores ;
	const char	**envv ;
	const char	*pwd ;
	const char	*progename ;		/* execution filename */
	const char	*progdname ;		/* dirname of arg[0] */
	const char	*progname ;		/* basename of arg[0] */
	const char	*pr ;			/* program root */
	const char	*searchname ;
	const char	*version ;
	const char	*banner ;
	const char	*nodename ;
	const char	*domainname ;
	const char	*rootname ;		/* distribution name */
	const char	*username ;
	const char	*groupname ;
	const char	*tmpdname ;
	const char	*maildname ;
	const char	*helpfname ;
	const char	*logid ;
	void		*efp ;
	void		*buffer ;		/* general buffer */
	void		*lip ;			/* local information */
	void		*config ;		/* for MSU */
	void		*contextp ;		/* SHELL context */
	PROGINFO_FL	have, f, final, changed ;
	PROGINFO_FL	open ;
	struct timeb	now ;
	LOGFILE		lh ;
	DATER		tmpdate, mdate ;
	IDS		id ;
	time_t		daytime ;
	time_t		lastcheck ;
	pid_t		pid ;
	int		pwdlen ;
	int		progmode ;		/* program mode */
	int		debuglevel ;
	int		verboselevel ;
	int		to ;			/* general time-out */
	int		to_open ;		/* open time-out */
	int		to_read ;		/* read time-out */
	int		to_lookup ;
	int		loglen ;		/* log-file length */
	int		af ;			/* address space */
	int		n ;
} ;

struct pivars {
	const char	*vpr1 ;
	const char	*vpr2 ;
	const char	*vpr3 ;
	const char	*pr ;
	const char	*vprname ;
} ;

struct arginfo {
	const char	**argv ;
	int		argc ;
	int		ai, ai_max, ai_pos ;
	int		ai_continue ;
} ;


#ifdef	__cplusplus
extern "C" {
#endif

extern int proginfo_start(PROGINFO *,cchar **,cchar *,cchar *) ;
extern int proginfo_setentry(PROGINFO *,const char **,const char *,int) ;
extern int proginfo_setversion(PROGINFO *,const char *) ;
extern int proginfo_setbanner(PROGINFO *,const char *) ;
extern int proginfo_setsearchname(PROGINFO *,const char *,const char *) ;
extern int proginfo_setprogname(PROGINFO *,const char *) ;
extern int proginfo_setexecname(PROGINFO *,const char *) ;
extern int proginfo_setprogroot(PROGINFO *,const char *,int) ;
extern int proginfo_pwd(PROGINFO *) ;
extern int proginfo_rootname(PROGINFO *) ;
extern int proginfo_progdname(PROGINFO *) ;
extern int proginfo_progename(PROGINFO *) ;
extern int proginfo_nodename(PROGINFO *) ;
extern int proginfo_getpwd(PROGINFO *,char *,int) ;
extern int proginfo_getename(PROGINFO *,char *,int) ;
extern int proginfo_getenv(PROGINFO *,const char *,int,const char **) ;
extern int proginfo_finish(PROGINFO *) ;

#ifdef	__cplusplus
}
#endif

#endif /* DEFS_INCLUDE */


