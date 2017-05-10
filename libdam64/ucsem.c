/* ucsem */

/* Posix Semaphore (UCSEM) */


#define	CF_DEBUGS	0		/* non-switchable debug print-outs */
#define	CF_CONDUNLINK	1		/* conditional unlink */


/* revision history:

	= 1999-07-23, David A�D� Morano
	This module was originally written.

*/

/* Copyright � 1999 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

	This module provides a sanitized version of the standard POSIX�
	semaphore facility provided with some new UNIX�i.  Some operating
	system problems are managed within these routines for the common stuff
	that happens when a poorly configured OS gets overloaded!

	Enjoy!


*******************************************************************************/


#define	UCSEM_MASTER	0


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<semaphore.h>
#include	<fcntl.h>
#include	<string.h>
#include	<errno.h>

#include	<vsystem.h>
#include	<getbufsize.h>
#include	<localmisc.h>

#include	"ucsem.h"


/* local defines */

#define	UCSEM_PATHPREFIX	"/tmp/ucsem"

#define	UCSEM_CHOWNVAR		_PC_CHOWN_RESTRICTED

#define	UCSEM_USERNAME1		"sys"
#define	UCSEM_USERNAME2		"adm"

#define	UCSEM_GROUPNAME1	"sys"
#define	UCSEM_GROUPNAME2	"sys"

#define	UCSEM_UID	3
#define	UCSEM_GID	3

#define	TO_NOSPC	10
#define	TO_MFILE	10
#define	TO_NFILE	10
#define	TO_INTR		10


/* external subroutines */

extern int	mkpath2(char *,const char *,const char *) ;
extern int	sncpy2(char *,int,const char *,const char *) ;
extern int	getgroup_gid(cchar *,int) ;
extern int	msleep(int) ;

#if	CF_DEBUGS
extern int	debugprintf(cchar *,...) ;
#endif

extern char	*strwcpy(char *,const char *,int) ;


/* forward references */

int		ucsemunlink(const char *) ;
int		unlinkucsem(const char *) ;

static int	ucsemdircheck(const char *) ;
static int	ucsemdiradd(const char *,int) ;
static int	ucsemdirrm(const char *) ;

static int	getucsemgid(void) ;


/* exported subroutines */


int ucsem_open(psp,name,oflag,perm,count)
UCSEM		*psp ;
const char	name[] ;
int		oflag ;
mode_t		perm ;
uint		count ;
{
	int		rs ;
	int		to_mfile = TO_MFILE ;
	int		to_nfile = TO_NFILE ;
	int		to_nospc = TO_NOSPC ;
	int		f_exit = TRUE ;
	char		altname[UCSEM_NAMELEN + 1] ;

	if (psp == NULL) return SR_FAULT ;
	if (name == NULL) return SR_FAULT ;

	if (name[0] == '\0') return SR_INVALID ;

#if	CF_DEBUGS
	debugprintf("ucsem_open: name=%s\n",name) ;
#endif

	if (name[0] != '/') {
	    sncpy2(altname,UCSEM_NAMELEN,"/",name) ;
	    name = altname ;
	}

#if	CF_DEBUGS
	debugprintf("ucsem_open: ucsem_name=%s\n",name) ;
#endif

	memset(psp,0,sizeof(UCSEM)) ;

	repeat {
	    rs = SR_OK ;
	    psp->sp = sem_open(name,oflag,(mode_t) perm, count) ;
	    if (psp->sp == SEM_FAILED) rs = (- errno) ;
	    if (rs < 0) {
	        switch (rs) {
	        case SR_MFILE:
	            if (to_mfile-- > 0) {
	                msleep(1000) ;
		    } else {
			f_exit = TRUE ;
		    }
	            break ;
	        case SR_NFILE:
	            if (to_nfile-- > 0) {
	                msleep(1000) ;
		    } else {
			f_exit = TRUE ;
		    }
	            break ;
	        case SR_NOSPC:
	            if (to_nospc-- > 0) {
	                msleep(1000) ;
		    } else {
			f_exit = TRUE ;
		    }
	            break ;
		case SR_INTR:
		    break ;
		default:
		    f_exit = TRUE ;
		    break ;
	        } /* end switch */
	    } /* end if (error) */
	} until ((rs >= 0) || f_exit) ;
    
	if (rs >= 0) {
	    strwcpy(psp->name,name,UCSEM_NAMELEN) ;
	    if (oflag & O_CREAT) ucsemdiradd(name,perm) ;
	    psp->magic = UCSEM_MAGIC ;
	} /* end if (opened) */

	return rs ;
}
/* end subroutine (ucsem_open) */


int ucsem_close(psp)
UCSEM		*psp ;
{
	int		rs ;

	if (psp == NULL) return SR_FAULT ;

	if (psp->magic != UCSEM_MAGIC) return SR_NOTOPEN ;

	if (psp->sp == NULL) return SR_INVALID ;

	repeat {
	    if ((rs = sem_close(psp->sp)) < 0) rs = (- errno) ;
	} until (rs != SR_INTR) ;

	psp->magic = 0 ;
	return rs ;
}
/* end subroutine (ucsem_close) */


int ucsem_start(psp,pshared,count)
UCSEM		*psp ;
int		pshared ;
uint		count ;
{
	int		rs ;

	if (psp == NULL) return SR_FAULT ;

	memset(psp,0,sizeof(UCSEM)) ;

	repeat {
	    if ((rs = sem_init(&psp->s,pshared,count)) < 0) rs = (- errno) ;
	} until (rs != SR_INTR) ;

	if (rs >= 0) {
	    psp->magic = UCSEM_MAGIC ;
	}

	return rs ;
}
/* end subroutine (ucsem_start) */


int ucsem_destroy(psp)
UCSEM		*psp ;
{
	int		rs ;

	if (psp == NULL) return SR_FAULT ;

	if (psp->magic != UCSEM_MAGIC) return SR_NOTOPEN ;

	if (psp->sp != NULL) return SR_INVALID ;

	repeat {
	    if ((rs = sem_destroy(&psp->s)) < 0) rs = (- errno) ;
	} until (rs != SR_INTR) ;

	psp->magic = 0 ;
	return rs ;
}
/* end subroutine (ucsem_destroy) */


int ucsem_trywait(psp)
UCSEM		*psp ;
{
	sem_t		*sp ;
	int		rs ;

	if (psp == NULL) return SR_FAULT ;

	if (psp->magic != UCSEM_MAGIC) return SR_NOTOPEN ;

	sp = (psp->sp != NULL) ? psp->sp : &psp->s ;

	repeat {
	    if ((rs = sem_trywait(sp)) < 0) rs = (- errno) ;
	} until (rs != SR_INTR) ;

	return rs ;
}
/* end subroutine (ucsem_trywait) */


int ucsem_getvalue(psp,rp)
UCSEM		*psp ;
int		*rp ;
{
	sem_t		*sp ;
	int		rs ;

	if (psp == NULL) return SR_FAULT ;

	if (psp->magic != UCSEM_MAGIC) return SR_NOTOPEN ;

	sp = (psp->sp != NULL) ? psp->sp : &psp->s ;

	repeat {
	    if ((rs = sem_getvalue(sp,rp)) < 0) rs = (- errno) ;
	} until (rs != SR_INTR) ;

	return rs ;
}
/* end subroutine (ucsem_getvalue) */


int ucsem_wait(psp)
UCSEM		*psp ;
{
	sem_t		*sp ;
	int		rs ;

	if (psp == NULL) return SR_FAULT ;

	if (psp->magic != UCSEM_MAGIC) return SR_NOTOPEN ;

	sp = (psp->sp != NULL) ? psp->sp : &psp->s ;

	repeat {
	    if ((rs = sem_wait(sp)) < 0) rs = (- errno) ;
	} until (rs != SR_INTR) ;

	return rs ;
}
/* end subroutine (ucsem_wait) */


int ucsem_waiti(psp)
UCSEM		*psp ;
{
	sem_t		*sp ;
	int		rs ;

	if (psp == NULL) return SR_FAULT ;

	if (psp->magic != UCSEM_MAGIC) return SR_NOTOPEN ;

	sp = (psp->sp != NULL) ? psp->sp : &psp->s ;

	if ((rs = sem_wait(sp)) < 0)
	    rs = (- errno) ;

	return rs ;
}
/* end subroutine (ucsem_waiti) */


int ucsem_post(psp)
UCSEM		*psp ;
{
	sem_t		*sp ;
	int		rs ;

	if (psp == NULL) return SR_FAULT ;

	if (psp->magic != UCSEM_MAGIC) return SR_NOTOPEN ;

	sp = (psp->sp != NULL) ? psp->sp : &psp->s ;

	repeat {
	    if ((rs = sem_post(sp)) < 0) rs = (- errno) ;
	} until (rs != SR_INTR) ;

	return rs ;
}
/* end subroutine (ucsem_post) */


int ucsem_unlink(psp)
UCSEM		*psp ;
{
	int		rs = SR_NOENT ;

	if (psp == NULL) return SR_FAULT ;

	if (psp->magic != UCSEM_MAGIC) return SR_NOTOPEN ;

	if (psp->name[0] != '\0') {

	    rs = unlinkucsem(psp->name) ;

#if	CF_CONDUNLINK
	    if (rs >= 0)
	        ucsemdirrm(psp->name) ;
#else
	    ucsemdirrm(psp->name) ;
#endif /* CF_CONDUNLINK */

	    psp->name[0] = '0' ;

	} /* end if (had an entry) */

	return rs ;
}
/* end subroutine (ucsem_unlink) */


/* OTHER API (but related) */


int ucsemunlink(name)
const char	name[] ;
{
	int		rs ;
	char		altname[MAXNAMELEN + 1] ;

	if (name == NULL) return SR_FAULT ;

	if (name[0] == '\0') return SR_INVALID ;

	if (name[0] != '/') {
	    sncpy2(altname,MAXNAMELEN,"/",name) ;
	    name = altname ;
	}

	repeat {
	    rs = SR_OK ;
	    if ((rs = sem_unlink(name)) < -1) rs = (- errno) ;
	} until (rs != SR_INTR) ;

#if	CF_CONDUNLINK
	if (rs >= 0)
	    ucsemdirrm(name) ;
#else
	ucsemdirrm(name) ;
#endif /* CF_CONDUNLINK */

	return rs ;
}
/* end subroutine (ucsemunlink) */


int unlinkucsem(name)
const char	name[] ;
{
	int		rs ;
	char		altname[MAXNAMELEN + 1] ;

	if (name == NULL) return SR_FAULT ;

	if (name[0] == '\0') return SR_INVALID ;

	if (name[0] != '/') {
	    sncpy2(altname,MAXNAMELEN,"/",name) ;
	    name = altname ;
	}

	repeat {
	    rs = SR_OK ;
	    if ((rs = sem_unlink(name)) < -1) rs = (- errno) ;
	} until (rs != SR_INTR) ;

#if	CF_CONDUNLINK
	if (rs >= 0)
	    ucsemdirrm(name) ;
#else
	ucsemdirrm(name) ;
#endif /* CF_CONDUNLINK */

	return rs ;
}
/* end subroutine (unlinkucsem) */


/* local subroutines */


static int ucsemdiradd(name,perm)
const char	name[] ;
int		perm ;
{
	int		rs ;
	char		tmpfname[MAXPATHLEN + 1] ;
	char		*pp = UCSEM_PATHPREFIX ;

	mkpath2(tmpfname,pp,name) ;

	rs = u_creat(tmpfname,perm) ;

	if (rs == SR_NOENT) {
	    rs = ucsemdircheck(pp) ;
	    if (rs >= 0)
	        rs = u_creat(tmpfname,perm) ;
	}
	if (rs >= 0) u_close(rs) ;

	return rs ;
}
/* end subroutine (ucsemdiradd) */


static int ucsemdirrm(cchar *name)
{
	int		rs ;
	char		tmpfname[MAXPATHLEN + 1] ;
	char		*pp = UCSEM_PATHPREFIX ;

	mkpath2(tmpfname,pp,name) ;

	rs = u_unlink(tmpfname) ;

	return rs ;
}
/* end subroutine (ucsemdirrm) */


static int ucsemdircheck(cchar *pp)
{
	struct ustat	sb ;
	struct passwd	pe ;
	uid_t		euid, uid ;
	long		cv ;
	const int	pwlen = getbufsize(getbufsize_pw) ;
	int		rs ;
	char		*pwbuf ;

	if ((rs = uc_malloc((pwlen+1),&pwbuf)) >= 0) {

	rs = u_stat(pp,&sb) ;

	if ((rs < 0) || (! S_ISDIR(sb.st_mode))) {

	    rs = u_mkdir(pp,0777) ;

	    if (rs >= 0)
	        rs = u_chmod(pp,(0777 | S_ISVTX)) ;

	    if (rs >= 0) {

	        rs = u_pathconf(pp,UCSEM_CHOWNVAR,&cv) ;

	        if ((rs < 0) || cv) {

	            euid = geteuid() ;

/* get a UID */

	            rs = uc_getpwnam(UCSEM_USERNAME1,&pe,pwbuf,pwlen) ;

	            if (rs < 0)
	                rs = uc_getpwnam(UCSEM_USERNAME2,&pe,pwbuf,pwlen) ;

	            uid = (rs >= 0) ? pe.pw_uid : UCSEM_UID ;

	            if (euid != uid) {
			if ((rs = getucsemgid()) >= 0) {
			    const gid_t		gid = rs ;
	              	    rs = u_chown(pp,uid,gid) ;
			}
	            } /* end if (UIDs different) */

	        } /* end if (CHOWN possibly allowed) */

	    } /* end if (was able to CHMOD) */

	} /* end if (directory did not exist) */

	    uc_free(pwbuf) ;
	} /* end if (memory-allocation) */

	return rs ;
}
/* end subroutine (ucsemdircheck) */


static int getucsemgid(void)
{
	const int	nrs = SR_NOTFOUND ;
	int		rs ;
	cchar		*gn = UCSEM_GROUPNAME1 ;
	if ((rs = getgroup_gid(gn,-1)) == nrs) {
	    gn = UCSEM_GROUPNAME2 ;
	    if ((rs = getgroup_gid(gn,-1)) == nrs) {
		rs = UCSEM_GID ;
	    }
	}
	return rs ;
}
/* end subroutine (getucsemgid) */


