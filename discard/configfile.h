/* configfile */


#ifndef	CONFIGFILE_INCLUDE
#define	CONFIGFILE_INCLUDE



/* object defines */

#define	CONFIGFILE		struct configfile




#include	<vecstr.h>



struct configfile {
	unsigned long	magic ;		/* magic number */
	vecstr	defines ;	/* defined variables */
	vecstr	unsets ;	/* unset ENV variables */
	vecstr	exports ;	/* environment variables */
	char	*root ;			/* program root */
	char	*tmpdir ;		/* environment variable */
	char	*logfname ;		/* log file name */
	char	*workdir ;		/* working directory */
	char	*directory ;		/* directory */
	char	*user ;			/* default username */
	char	*group ;		/* default groupname */
	char	*pidfname ;		/* traditionally hold PID */
	char	*lockfname ;		/* lock file */
	char	*interrupt ;
	char	*polltime ;
	char	*filetime ;
	char	*port ;			/* port to listen on */
	char	*userpass ;		/* user password file */
	char	*machpass ;		/* machine password file */
	char	*srvtab ;		/* SRVTAB */
	char	*sendmail ;		/* SENDMAIL program path */
	char	*envfname ;		/* environment file */
	char	*pathfname ;		/* PATH file */
	char	*devicefname ;		/* daemon device file path */
	char	*seedfname ;		/* seed file path */
	char	*logsize ;		/* default target log length */
	char	*organization ;
	char	*timeout ;
	char	*removemul ;		/* remove multiplier */
	char	*acctab ;		/* access table file */
	char	*paramfname ;		/* parameter file */
	char	*nrecips ;		/* number of recips at a time */
	char	*helpfname ;
	char	*statfname ;		/* status file name */
	char	*passfname ;		/* pass-FD file name */
	char	*options ;
	char	*interval ;		/* poll interval */
	char	*stampdir ;		/* timestamp directory */
	char	*maxjobs ;		/* maximum jobs */
	int	badline ;	/* line number of bad thing */
	int	srs ;		/* secondary return status */
	int	loglen ;	/* log file length */
} ;



#ifndef	CONFIGFILE_MASTER

extern int configfile_start(CONFIGFILE *,char *) ;
extern int configfile_finish(CONFIGFILE *) ;

#endif /* CONFIGFILE_MASTER */


#endif /* CONFIGFILE_INCLUDE */



