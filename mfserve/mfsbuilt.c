/* mfsbuilt */

/* perform various functions on a job */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0		/* compile-time debugging */


/* revision history:

	= 1998-09-01, David A�D� Morano
	This subroutine was originally written.

*/

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

        This module is responsible for providing means to store a job and the
        retrieve it later by its PID.


*******************************************************************************/


#define	MFSBUILT_MASTER	0


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<limits.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>
#include	<dlfcn.h>

#include	<vsystem.h>
#include	<fsdir.h>
#include	<localmisc.h>

#include	"mfsbuilt.h"


/* local defines */

#define	ENT		struct mfsbuilt_ent


/* external subroutines */

extern int	mkpath1(char *,cchar *) ;
extern int	mkpath2(char *,cchar *,cchar *) ;
extern int	pathadd(char *,int,cchar *) ;
extern int	mktmpfile(char *,mode_t,cchar *) ;
extern int	mkdirs(const char *,mode_t) ;
extern int	chmods(const char *,mode_t) ;
extern int	matstr(cchar **,cchar *,int) ;
extern int	hasNotDots(cchar *,int) ;
extern int	isNotPresent(int) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strnchr(cchar *,int,int) ;


/* external variables */


/* global variables */


/* local structures */

struct mfsbuilt_ent {
	cchar		*a ;		/* allocation */
	cchar		*svc ;		/* service name */
	cchar		*fname ;	/* file (component) name */
	void		*sop ;		/* shared object pointer (handle) */
	int		rcount ;	/* reference count */
} ;


/* forward references */

static int	mfsbuilt_dump(MFSBUILT *) ;
static int	mfsbuilt_load(MFSBUILT *) ;
static int	mfsbuilt_fins(MFSBUILT *) ;

static int	ent_start(ENT *,cchar *,int,cchar *,int) ;
static int	ent_finish(ENT *) ;

static int	mkfile(cchar *,cchar **) ;

static int	hasService(cchar *,int) ;


/* local variables */

static const char	*exts[] = {
	"so",
	"o",
	NULL
} ;


/* exported subroutines */


int mfsbuilt_start(MFSBUILT *op,cchar *dname)
{
	int		rs ;
	cchar		*cp ;

	if (op == NULL) return SR_FAULT ;
	if (dname == NULL) return SR_FAULT ;

	memset(op,0,sizeof(MFSBUILT)) ;

	if ((rs = uc_mallocstrw(dname,-1,&cp)) >= 0) {
	    HDB		*dbp = &op->db ;
	    const int	n = MFSBUILT_NENTS ;
	    const int	at = FALSE ;
	    op->dname = cp ;
	    if ((rs = hdb_start(dbp,n,at,NULL,NULL)) >= 0) {
		if ((rs = mfsbuilt_dump(op)) >= 0) {
		    if ((rs = mfsbuilt_load(op)) >= 0) {
		        op->magic = MFSBUILT_MAGIC ;
		    }
		}
		if (rs < 0) {
		    mfsbuilt_fins(op) ;
		    hdb_finish(dbp) ;
		}
	    }
	    if (rs < 0) {
		uc_free(op->dname) ;
		op->dname = NULL ;
	    }
	}

	return rs ;
}
/* end subroutine (mfsbuilt_start) */


int mfsbuilt_finish(MFSBUILT *op)
{
	HDB		*dbp ;
	int		rs = SR_OK ;
	int		rs1 ;

	if (op == NULL) return SR_FAULT ;

	rs1 = mfsbuilt_fins(op) ;
	if (rs >= 0) rs = rs1 ;

	dbp = &op->db ;
	rs1 = hdb_finish(dbp) ;
	if (rs >= 0) rs = rs1 ;

	if (op->dname != NULL) {
	    rs1 = uc_free(op->dname) ;
	    if (rs >= 0) rs = rs1 ;
	    op->dname = NULL ;
	}

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (mfsbuilt_finish) */


int mfsbuilt_have(MFSBUILT *op,cchar *sp,int sl)
{
	HDB		*dbp ;
	HDB_DATUM	k, v ;
	int		rs ;
	int		rl = 0 ;
	if (op == NULL) return SR_FAULT ;
	if (sp == NULL) return SR_FAULT ;
	dbp = &op->db ;
	if (sl < 0) sl = strlen(sp) ;
	k.buf = sp ;
	k.len = sl ;
	if ((rs = hdb_fetch(dbp,k,NULL,&v)) >= 0) {
	    rl = v.len ;
	} else if (isNotPresent(rs)) {
	    rs = SR_OK ;
	}
	return (rs >= 0) ? rl : rs ;
}
/* end subroutine (mfsbuilt_have) */


/* private subroutines */


static int mfsbuilt_dump(MFSBUILT *op)
{
	int		rs ;

	rs = mfsbuilt_fins(op) ;

	return rs ;
}
/* end subroutine (mfsbuilt_dump) */


static int mfsbuilt_load(MFSBUILT *op)
{
	FSDIR		d ;
	FSDIR_ENT	de ;
	int		rs ;
	int		rs1 ;
	if ((rs = fsdir_open(&d,op->dname)) >= 0) {
	    char	pbuf[MAXPATHLEN+1] ;
	    if ((rs = mkpath1(pbuf,op->dname)) >= 0) {
	        const int	plen = rs ;
	        while ((rs = fsdir_read(&d,&de)) > 0) {
		    cchar	*ep = de.name ;
		    int		el = rs ;
		    if (hasNotDots(ep,el)) {
		        int	sl ;
		        if ((sl = hasService(ep,el)) > 0) {
			    if ((rs = pathadd(pbuf,plen,ep)) >= 0) {
				rs = mfsbuilt_ent(op,ep,sl,pbuf,rs) ;
			    }
		        }
		    }
		    if (rs < 0) break ;
	        } /* end while */
	    } /* end if (mkpath) */
	    rs1 = fsdir_close(&d) ;
	    if (rs >= 0) rs = rs1 ;
	} /* end if (fsdir) */
	return rs ;
}
/* end subroutine (mfsbuilt_load) */


static int mfsbuilt_fins(MFSBUILT *op)
{
	HDB		*dbp = &op->db ;
	HDB_DATUM	k, v ;
	HDB_CUR		c ;
	int		rs ;
	int		rs1 ;

	if ((rs = hdb_curbegin(dbp,&c)) >= 0) {
	    ENT		*ep ;
	    int		i ;
	    for (i = 0 ; hdb_enum(dbp,&c,&k,&v) >= 0 ; i += 1) {
		ep = (ENT *) v.buf ;
		if (ep != NULL) {
		    rs1 = ent_finish(ep) ;
		    if (rs >= 0) rs = rs1 ;
		    rs1 = uc_free(ep) ;
		    if (rs >= 0) rs = rs1 ;
		}
	    } /* end for */
	    rs1 = hdb_curend(dbp,&c) ;
	    if (rs >= 0) rs = rs1 ;
	} /* end if (hdb-cur) */

	return rs ;
}
/* end subroutine (mfsbuilt_fins) */


static int ent_start(ENT *ep,cchar *sp,int sl,cchar *fp,int fl)
{
	int		rs ;
	int		size = 0 ;
	char		*bp ;
	if (sl < 0) sl = strlen(sp) ;
	if (fl < 0) fl = strlen(fp) ;
	size += (sl+1) ;
	size += (fl+1) ;
	memset(ep,0,sizeof(ENT)) ;
	if ((rs = uc_malloc(size,&bp)) >= 0) {
	    ep->a = bp ;
	    ep->svc = bp ;
	    bp = (strwcpy(bp,sp,sl)+1) ;
	    ep->fname = bp ;
	    bp = (strwcpy(bp,sp,sl)+1) ;
	}
	return rs ;
}
/* end subroutine (ent_start) */


static int ent_finish(ENT *ep)
{
	int		rs = SR_OK ;
	int		rs1 ;

	if (ep->sop != NULL) {
	    dlclose(ep->sop) ;
	    ep->sop = NULL ;
	    ep->rcount = 0 ;
	}

	if (ep->a != NULL) {
	    rs1 = uc_free(ep->a) ;
	    if (rs >= 0) rs = rs1 ;
	    ep->a = NULL ;
	}

	return rs ;
}
/* end subroutine (ent_finish) */


static int mkfile(cchar *template,cchar **rpp)
{
	int		rs ;
	int		tl = 0 ;
	char		tbuf[MAXPATHLEN + 1] ;

	tbuf[0] = '\0' ;
	if ((rs = mktmpfile(tbuf,0666,template)) >= 0) {
	    int	tl = rs ;
	    rs = uc_mallocstrw(tbuf,tl,rpp) ;
	    if (rs < 0) {
	        u_unlink(tbuf) ;
		*rpp = NULL ;
	    } /* end if (error-recovery) */
	} /* end if (mktmpfile) */

	return (rs >= 0) ? tl : rs ;
}
/* end subroutines (mkfile) */


static int hasService(cchar *sp,int sl)
{
	cchar		*tp ;
	if (sl < 0) sl = strlen(sp) ;
	if ((tp = strnchr(sp,sl,'.')) != NULL) {
	    cchar	*ep = (tp+1) ;
	    const int	el = ((sp+sl)-(tp+1)) ;
	    if (matstr(exts,ep,el) < 0) {
		sl = 0 ;
	    }
	}
	return sl ;
}
/* end subroutine (hasService) */


