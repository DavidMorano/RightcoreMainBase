/* config */


/* revision history:

	= 2000-05-14, David A�D� Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright � 2000 David A�D� Morano.  All rights reserved. */


#define	VERSION		"0"
#define	WHATINFO	"@(#)strerror "
#define	BANNER		"String Error"
#define	SEARCHNAME	"strerror"
#define	VARPRNAME	"LOCAL"

#ifndef	PROGRAMROOT
#define	PROGRAMROOT	"/usr/add-on/local"
#endif

#define	VARPROGRAMROOT1	"STRERROR_PROGRAMROOT"
#define	VARPROGRAMROOT2	VARPRNAME
#define	VARPROGRAMROOT3	"PROGRAMROOT"

#define	VARBANNER	"STRERROR_BANNER"
#define	VARSEARCHNAME	"STRERROR_NAME"
#define	VAROPTS		"STRERROR_OPTS"
#define	VARFILEROOT	"STRERROR_FILEROOT"
#define	VARLOGTAB	"STRERROR_LOGTAB"
#define	VARDBNAME	"STRERROR_DBNAME"
#define	VARAFNAME	"STRERROR_AF"
#define	VAREFNAME	"STRERROR_EF"

#define	VARDEBUGFNAME	"STRERROR_DEBUGFILE"
#define	VARDEBUGFD1	"STRERROR_DEBUGFD"
#define	VARDEBUGFD2	"DEBUGFD"

#define	VARSYSNAME	"SYSNAME"
#define	VARRELEASE	"RELEASE"
#define	VARMACHINE	"MACHINE"
#define	VARARCHITECTURE	"ARCHITECTURE"
#define	VARHZ		"HZ"
#define	VARNODE		"NODE"
#define	VARCLUSTER	"CLUSTER"
#define	VARSYSTEM	"SYSTEM"
#define	VARDOMAIN	"DOMAIN"
#define	VARNISDOMAIN	"NISDOMAIN"
#define	VARTERM		"TERM"
#define	VARPRINTER	"PRINTER"
#define	VARLPDEST	"LPDEST"
#define	VARPAGER	"PAGER"
#define	VARMAIL		"MAIL"
#define	VARORGANIZATION	"ORGANIZATION"
#define	VARLINES	"LINES"
#define	VARCOLUMNS	"COLUMNS"
#define	VARNAME		"NAME"
#define	VARFULLNAME	"FULLNAME"
#define	VARTZ		"TZ"
#define	VARUSERNAME	"USERNAME"
#define	VARLOGNAME	"LOGNAME"
#define	VARPATH		"PATH"
#define	VARMANPATH	"MANPATH"
#define	VARCDPATH	"CDPATH"
#define	VARLIBPATH	"LD_LIBRARY_PATH"

#define	VARHOMEDNAME	"HOME"
#define	VARTMPDNAME	"TMPDIR"
#define	VARMAILDNAME	"MAILDIR"
#define	VARMAILDNAMES	"MAILDIRS"

#define	VARPRLOCAL	"LOCAL"
#define	VARPRPCS	"PCS"

#define	TMPDNAME	"/tmp"
#define	WORKDNAME	"/tmp"
#define	LOGCNAME	"log"

#define	DEFINITFNAME	"/etc/default/init"
#define	DEFLOGFNAME	"/etc/default/login"
#define	NISDOMAINNAME	"/etc/defaultdomain"

#define	CONFIGFNAME	"conf"
#define	ENVFNAME	"environ"
#define	PATHSFNAME	"paths"
#define	HELPFNAME	"help"
#define	IPASSWDFNAME	"ipasswd"
#define	FULLFNAME	".fullname"

#define	PIDFNAME	"run/strerror"		/* mutex PID file */
#define	LOGFNAME	"var/log/strerror"	/* activity log */
#define	LOCKFNAME	"spool/locks/strerror"	/* lock mutex file */

#define	LOGSIZE		(80*1024)


