/* testcrash (TESTCRASH) */

/* this is a PCSPOLLS module for performing TESTCRASH polls */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_WORK		1		/* work */
#define	CF_ENABLE	1		/* enabled */


/* revision history:

	= 2008-10-07, David A�D� Morano
	This subroutine was originally written.

*/

/* Copyright � 2008 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

	Description:

	This object is a PCSPOLLS module for performing TESTCRASH polls.

	Synopsis:

	int testcrash_start(op,pr,sn,envv,pcp)
	PCSPOLLS	*op ;
	const char	*pr ;
	const char	*sn ;
	const char	**envv ;
	PCSCONF		*pcp ;

	Arguments:

	op		object pointer
	pr		program-root
	sn		search-name (of program calling us)
	envv		calling environment
	pcp		PCSCONF pointer

	Returns:

	>=0		OK
	<0		error code


*******************************************************************************/


#include	<envstandards.h>	/* must be before others */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<limits.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>
#include	<pcsconf.h>
#include	<storebuf.h>
#include	<upt.h>
#include	<userinfo.h>
#include	<logfile.h>
#include	<localmisc.h>

#include	"pcspolls.h"
#include	"thrbase.h"


/* local defines */

#define	TESTCRASH		struct testcrash_head
#define	TESTCRASH_MAGIC		0x88773424
#define	TESTCRASH_LCNAME	"log"
#define	TESTCRASH_LBNAME	"pcspolls"

#define	WORK		struct work_head
#define	WORKARGS	struct work_args


/* external subroutines */

extern int	sncpy1(char *,int,const char *) ;
extern int	sncpy2(char *,int,const char *,const char *) ;
extern int	sncpy3(char *,int,const char *,const char *,const char *) ;
extern int	sncpy4(char *,int,const char *,const char *,const char *,
			const char *) ;
extern int	snwcpy(char *,int,const char *,int) ;
extern int	mkpath2(char *,const char *,const char *) ;
extern int	mkpath3(char *,const char *,const char *,const char *) ;
extern int	pathadd(char *,int,const char *) ;
extern int	nleadstr(const char *,const char *,int) ;
extern int	cfdeci(const char *,int,int *) ;
extern int	cfdecui(const char *,int,uint *) ;
extern int	logfile_userinfo(LOGFILE *,USERINFO *,time_t,
			const char *,const char *) ;
extern int	hasNotDots(const char *,int) ;
extern int	isNotPresent(int) ;

#if	CF_DEBUGS
extern int	debugprintf(const char *,...) ;
extern int	strlinelen(const char *,int,int) ;
#endif

extern const char	*getourenv(const char **,const char *) ;

extern char	*strnchr(const char *,int,int) ;
extern char	*strnpbrk(const char *,int,const char *) ;
extern char	*timestr_log(time_t,char *) ;


/* external variables */


/* local structures */

struct testcrash_flags {
	uint		working:1 ;
} ;

struct testcrash_head {
	uint		magic ;
	THRBASE		t ;
	struct testcrash_flags	f ;
	WORKARGS	*wap ;
	int		dummy ;
} ;

struct work_args {
	TESTCRASH	*op ;
	const char	*pr ;
	const char	*sn ;
	const char	**envv ;
	PCSCONF		*pcp ;
} ;

struct work_flags {
	uint		dummy:1 ;
} ;

struct work_head {
	uint		magic ;
	THRBASE		*tip ;
	WORKARGS	*wap ;
	volatile int	f_term ;
	struct work_flags	f ;
} ;

enum cmds {
	cmd_noop,
	cmd_exit,
	cmd_overlast
} ;


/* forward references */

static int workargs_load(WORKARGS *,TESTCRASH *,
		const char *,const char *,const char **,PCSCONF *) ;

static int	worker(THRBASE *,WORKARGS *) ;

static int work_start(WORK *,THRBASE *,WORKARGS *) ;
static int work_term(WORK *) ;
static int work_finish(WORK *) ;

static int mklogentry(const char *,const char *,const char **,PCSCONF *) ;


/* local variables */


/* exported variables */

PCSPOLLS_NAME	testcrash = {
	"testcrash",
	sizeof(TESTCRASH),
	0
} ;


/* exported subroutines */


int testcrash_start(op,pr,sn,envv,pcp)
TESTCRASH	*op ;
const char	*pr ;
const char	*sn ;
const char	**envv ;
PCSCONF		*pcp ;
{
	WORKARGS	*wap ;

	const int	wsize = sizeof(WORKARGS) ;

	int	rs ;


	if (op == NULL)
	    return SR_FAULT ;

#if	CF_DEBUGS
	debugprintf("testcrash_start: entered\n") ;
	debugprintf("testcrash_start: pr=%s\n",pr) ;
	debugprintf("testcrash_start: sn=%s\n",sn) ;
#endif

	memset(op,0,sizeof(TESTCRASH)) ;

	if ((rs = uc_malloc(wsize,&wap)) >= 0) {
	    int	(*thr)(THRBASE *,void *) = (int (*)(THRBASE *,void *)) worker ;
	    workargs_load(wap,op,pr,sn,envv,pcp) ;
	    if ((rs = thrbase_start(&op->t,thr,wap)) >= 0) {
		op->f.working = TRUE ;
		op->wap = wap ;
		op->magic = TESTCRASH_MAGIC ;
	    }
	    if (rs < 0) {
	        uc_free(wap) ;
		op->wap = NULL ;
	    }
	} /* end if (memory-allocation) */

#if	CF_DEBUGS
	debugprintf("testcrash_start: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (testcrash_start) */


int testcrash_finish(op)
TESTCRASH	*op ;
{
	int	rs = SR_OK ;
	int	rs1 ;


	if (op == NULL) return SR_FAULT ;
	if (op->magic != TESTCRASH_MAGIC) return SR_NOTOPEN ;

#if	CF_DEBUGS
	debugprintf("testcrash_finish: f_working=%d\n",op->f.working) ;
#endif

	if (op->f.working) {
	    op->f.working = FALSE ;
	    rs1 = thrbase_finish(&op->t) ;
	    if (rs >= 0) rs = rs1 ;
	}

	if (op->wap != NULL) {
	    rs1 = uc_free(op->wap) ;
	    if (rs >= 0) rs = rs1 ;
	    op->wap = NULL ;
	}

#if	CF_DEBUGS
	debugprintf("testcrash_finish: ret rs=%d\n",rs) ;
#endif

	op->magic = 0 ;
	return rs ;
}
/* end subroutine (testcrash_finish) */


#ifdef	COMMENT

int testcrash_info(op,ip)
TESTCRASH	*op ;
TESTCRASH_INFO	*ip ;
{
	int	rs = SR_OK ;


	if (op == NULL)
	    return SR_FAULT ;

	if (op->magic != TESTCRASH_MAGIC)
	    return SR_NOTOPEN ;

	if (ip != NULL) {
	    memset(ip,0,sizeof(TESTCRASH_INFO)) ;
	    ip->dummy = 1 ;
	}

	return rs ;
}
/* end subroutine (testcrash_info) */


int testcrash_cmd(op,cmd)
TESTCRASH	*op ;
int		cmd ;
{
	int	rs = SR_OK ;


	if (op == NULL) return SR_FAULT ;
	if (op->magic != TESTCRASH_MAGIC) return SR_NOTOPEN ;

	return (rs >= 0) ? cmd : rs ;
}
/* end subroutine (testcrash_cmd) */

#endif /* COMMENT */


/* private subroutines */


static int workargs_load(wap,op,pr,sn,envv,pcp)
WORKARGS	*wap ;
TESTCRASH	*op ;
const char	*pr ;
const char	*sn ;
const char	**envv ;
PCSCONF		*pcp ;
{
	memset(wap,0,sizeof(WORKARGS)) ;
	wap->op = op ;
	wap->pr = pr ;
	wap->sn = sn ;
	wap->envv = envv ;
	wap->pcp = pcp ;
	return SR_OK ;
}
/* end subroutine (workargs_load) */


static int worker(THRBASE *tip,WORKARGS *wap)
{
	WORK		w ;

	const int	to = 1 ;

	int	rs ;
	int	ctime = 0 ;
	int	f_exit = FALSE ;

#if	CF_DEBUGS
	debugprintf("testcrash/worker: started\n") ;
#endif

#if	CF_WORK
	if ((rs = work_start(&w,tip,wap)) >= 0) {

	    while ((rs = thrbase_cmdrecv(tip,to)) >= 0) {
		const int	cmd = rs ;

	        switch (cmd) {
		case cmd_noop:
		    ctime += 1 ;
#if	CF_DEBUGS
	debugprintf("testcrash/worker: timed-poll\n") ;
#endif
		    break ;
	        case cmd_exit:
		    f_exit = TRUE ;
#if	CF_DEBUGS
	debugprintf("testcrash/worker: exit\n") ;
#endif
		    rs = work_term(&w) ;
		    break ;
	        } /* end switch */

		if (f_exit) break ;
	    } /* end while */

	    work_finish(&w) ;
	} /* end if (work) */
#endif /* CF_WORK */

#if	CF_DEBUGS
	debugprintf("testcrash/worker: ret rs=%d ctime=%u\n",rs,ctime) ;
#endif

	return (rs >= 0) ? ctime : rs ;
}
/* end subroutine (worker) */


static int work_start(WORK *wp,THRBASE *tip,WORKARGS *wap)
{
	int		rs = SR_OK ;
	int		c = 0 ;
	const char	*pr, *sn ;

	if (wp == NULL) return SR_FAULT ;

	memset(wp,0,sizeof(WORK)) ;
	wp->tip = tip ;
	wp->wap = wap ;

	pr = wap->pr ;
	sn = wap->sn ;

#if	CF_DEBUGS
	debugprintf("testcrash/work_start: pr=%s\n",pr) ;
	debugprintf("testcrash/work_start: sn=%s\n",sn) ;
#endif

#if	CF_ENABLED
	if (pr != NULL) {
	    PCSCONF	*pcp = wap->pcp ;
	    const char	**envv = wap->envv ;
	    rs = mklogentry(pr,sn,envv,pcp) ;
	    c = rs ;
	}
#endif /* CF_ENABLED */

#if	CF_DEBUGS
	debugprintf("testcrash/work_start: ret rs=%d c=%u\n",rs,c) ;
#endif

	return (rs >= 0) ? c : rs ;
}
/* end subroutine (work_start) */


static int work_finish(WORK *wp)
{
	int	rs = SR_OK ;

	if (wp == NULL) return SR_FAULT ;

#if	CF_DEBUGS
	debugprintf("testcrash/work_finish: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (work_finish) */


static int work_term(WORK *wp)
{
	if (wp == NULL) return SR_FAULT ;
#if	CF_DEBUGS
	debugprintf("testcrash/work_term: entered\n") ;
#endif
	return SR_OK ;
}
/* end subroutine (work_term) */


static int mklogentry(pr,sn,envv,pcp)
const char	*pr ;
const char	*sn ;
const char	**envv ;
PCSCONF		*pcp ;
{
	int	rs = SR_OK ;
	int	rs1 ;

	char	lfname[MAXPATHLEN+1] ;

	const char	*lcname = TESTCRASH_LCNAME ;
	const char	*lbname = TESTCRASH_LBNAME ;

	if ((rs = mkpath3(lfname,pr,lcname,lbname)) >= 0) {
	    struct ustat	sb ;
	    if (u_stat(lfname,&sb) >= 0) {
		USERINFO	u ;
		if ((rs = userinfo_start(&u,NULL)) >= 0) {
		    LOGFILE	lh, *lhp = &lh ;
		    const char	*logid = u.logid ;
		    if ((rs1 = logfile_open(lhp,lfname,0,0666,logid)) >= 0) {
		        time_t	daytime = time(NULL) ;
			const char	*pv = "�" ;
	                logfile_userinfo(lhp,&u,daytime,sn,pv) ;
		        logfile_close(lhp) ;
		    } else if (! isNotPresent(rs1))
		        rs = rs1 ;
		    userinfo_finish(&u) ;
		} /* end if (userinfo) */
	    } /* end if (stat) */
	} /* end if (mkpath) */

	return rs ;
}
/* end subroutine (mklogentry) */



