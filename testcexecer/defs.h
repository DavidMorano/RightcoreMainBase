/* defs */

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */


#ifndef	DEFS_INCLUDE
#define	DEFS_INCLUDE	1


#include	<envstandards.h>

#include	<sys/types.h>
#include	<netdb.h>

#include	<logfile.h>
#include	<vecstr.h>
#include	<localmisc.h>


#ifndef	FD_STDIN
#define	FD_STDIN	0
#define	FD_STDOUT	1
#define	FD_STDERR	2
#endif

#ifndef	MSGBUFLEN
#define	MSGBUFLEN	2048
#endif

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	2048
#endif

#ifndef	MAXNAMELEN
#define	MAXNAMELEN	256
#endif

#ifndef	TIMEBUFLEN
#define	TIMEBUFLEN	80
#endif

#define	BUFLEN		((5 * 1024) + MAXHOSTNAMELEN)
#define	JOBIDLEN	14


struct proginfo_flags {
	uint		outfile:1 ;
	uint		errfile:1 ;	/* do we have STDERR ? */
	uint		log:1 ;		/* do we have a log file ? */
	uint		quiet:1 ;	/* are we in quiet mode ? */
	uint		ni:1 ;		/* no input */
	uint		no:1 ;		/* no output */
	uint		sanity:1 ;	/* sanity PINGs */
} ;

struct proginfo {
	vecstr		stores ;
	const char	**envv ;	/* program start-up environment */
	const char	*pwd ;
	const char	*progdname ;	/* program directory */
	const char	*progename ;	/* program directory */
	const char	*progname ;	/* program name */
	const char	*pr ;		/* program root directory */
	const char	*searchname ;
	const char	*version ;	/* program version string */
	const char	*banner ;
	const char	*nodename ;
	const char	*domainname ;
	const char	*hostname ;	/* concatenation of N + D */
	const char	*username ;	/* ours */
	const char	*groupname ;	/* ours */
	const char	*logid ;	/* default program LOGID */
	const char	*tmpdname ;	/* temporary directory */
	const char	*userpass ;	/* user password file */
	const char	*machpass ;	/* machine password file */
	void		*efp ;
	struct proginfo_flags	f ;
	LOGFILE		lh ;		/* program activity log */
	time_t		daytime ;
	pid_t		pid, ppid ;
	uid_t		uid, euid ;	/* UIDs */
	gid_t		gid, egid ;
	int		pwdlen ;
	int		debuglevel ;	/* debugging level */
	int		verboselevel ;	/* verbosity level */
	int		keeptime ;
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

extern int proginfo_start(struct proginfo *,const char **,const char *,
		const char *) ;
extern int proginfo_setentry(struct proginfo *,const char **,const char *,int) ;
extern int proginfo_setversion(struct proginfo *,const char *) ;
extern int proginfo_setbanner(struct proginfo *,const char *) ;
extern int proginfo_setsearchname(struct proginfo *,const char *,const char *) ;
extern int proginfo_setprogname(struct proginfo *,const char *) ;
extern int proginfo_setexecname(struct proginfo *,const char *) ;
extern int proginfo_setprogroot(struct proginfo *,const char *,int) ;
extern int proginfo_pwd(struct proginfo *) ;
extern int proginfo_rootname(struct proginfo *) ;
extern int proginfo_progdname(struct proginfo *) ;
extern int proginfo_progename(struct proginfo *) ;
extern int proginfo_nodename(struct proginfo *) ;
extern int proginfo_getpwd(struct proginfo *,char *,int) ;
extern int proginfo_getename(struct proginfo *,char *,int) ;
extern int proginfo_getenv(struct proginfo *,const char *,int,const char **) ;
extern int proginfo_finish(struct proginfo *) ;

#ifdef	__cplusplus
}
#endif

#endif /* DEFS_INCLUDE */


