/* mfswatch */

/* watch (listen on) the specified service-access-points */
/* version %I% last modified %G% */


#define	CF_DEBUGS	0		/* compile-time debugging */
#define	CF_DEBUG	0		/* switchable debug print-outs */
#define	CF_DEBUGFORK	0		/* debug fork problem */
#define	CF_SIGCHILD	1		/* catch SIGCHLD? */
#define	CF_PROGHANDLE	1		/* call 'proghandle() */
#define	CF_DUPUP	1		/* 'dupup(3dam)' low FDs */
#define	CF_LOADNAMES	0		/* load-names here */
#define	CF_MKSUBLOGID	1		/* use 'mksublogid(3dam)' */
#define	CF_LOGCHECK	1		/* call 'proglog_check()' */


/* revision history:

	= 2008-06-23, David A�D� Morano
        I updated this subroutine to just poll for machine status and write the
        Machine Status (MS) file. This was a cheap excuse for not writing a
        whole new daemon program just to poll for machine status. I hope this
        works out! :-)

*/

/* Copyright � 2008 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

        This subroutine is responsible for listening on the given socket and
        spawning off a program to handle any incoming connection. Some of the
        "internal" messages are handled here (the easy ones -- or the ones that
        fit here best). The rest (that look like client-sort-of requests) are
        handled in the 'standing' object module.


*******************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<limits.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>
#include	<bfile.h>
#include	<varsub.h>
#include	<vecstr.h>
#include	<sockaddress.h>
#include	<connection.h>
#include	<poller.h>
#include	<exitcodes.h>
#include	<localmisc.h>

#include	"sreqdb.h"
#include	"listenspec.h"
#include	"defs.h"
#include	"proglog.h"
#include	"mfsmain.h"
#include	"mfsconfig.h"
#include	"mfslisten.h"
#include	"mfsmsg.h"


/* local defines */

#define	MFSWATCH	struct mfswatch

#ifndef	IPCDIRMODE
#define	IPCDIRMODE	0777
#endif

#define	W_OPTIONS	(WNOHANG)

#define	IPCBUFLEN	MSGBUFLEN

#ifndef	CMSGBUFLEN
#define	CMSGBUFLEN	256
#endif

#define	NIOVECS		1
#define	NFDS		3

#define	O_SRVFLAGS	(O_RDWR | O_CREAT)

#ifndef	POLLINTMULT
#define	POLLINTMULT	1000
#endif

#ifndef	TIMEBUFLEN
#define	TIMEBUFLEN	80
#endif

#ifndef	TO_LOGFLUSH
#define	TO_LOGFLUSH	10
#endif


/* external subroutines */

extern int	snddd(char *,int,uint,uint) ;
extern int	snsdd(char *,int,const char *,uint) ;
extern int	sncpy1(char *,int,const char *) ;
extern int	mkpath2(char *,const char *,const char *) ;
extern int	mkpath3(char *,const char *,const char *,const char *) ;
extern int	sfshrink(const char *,int,const char **) ;
extern int	sfbasename(const char *,int,const char **) ;
extern int	sfdirname(const char *,int,const char **) ;
extern int	matstr(const char **,const char *,int) ;
extern int	ctdeci(char *,int,int) ;
extern int	bufprintf(const char *,int,...) ;
extern int	dupup(int,int) ;
extern int	opentmpfile(const char *,int,mode_t,char *) ;
extern int	nlspeername(const char *,const char *,char *) ;
extern int	mksublogid(char *,int,const char *,int) ;
extern int	acceptpass(int,struct strrecvfd *,int) ;
extern int	varsub_addvec(VARSUB *,VECSTR *) ;
extern int	isasocket(int) ;
extern int	isBadSend(int) ;

#if	CF_DEBUGS || CF_DEBUG 
extern int	debugprintf(const char *,...) ;
extern int	strlinelen(const char *,int,int) ;
extern int	progexports(PROGINFO *,const char *) ;
#endif /* CF_DEBUGS */

#if	(CF_DEBUG || CF_DEBUGS) && CF_DEBUGFORK
extern int	debugfork(const char *) ;
#endif

extern cchar	*strsigabbr(int) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*timestr_log(time_t,char *) ;
extern char	*timestr_logz(time_t,char *) ;
extern char	*timestr_elapsed(time_t,char *) ;


/* external variables */


/* local structures */

struct mfswatch {
	SREQDB		reqs ;		/* service requests */
	POLLER		pm ;		/* Poll-Manager */
	time_t		ti_lastmark ;
	time_t		ti_lastmaint ;
	time_t		ti_lastconfig ;
	int		njobs ;
	int		minpoll ;	/* poll interval in milliseconds */
	int		f_exit ;
} ;


/* forward references */

static int	mfswatch_beginer(PROGINFO *) ;
static int	mfswatch_ender(PROGINFO *) ;

statis int	mfswatch_uptimer(PROGINFO *) ;
static int	mfswatch_handle(PROGINFO *,POLLER_SPEC *) ;

static int	procwatcher(PROGINFO *,SUBINFO *,vecstr *) ;
static int	procwatchmark(PROGINFO *,SUBINFO *,int) ;
static int	procwatchjobs(PROGINFO *,SUBINFO *) ;
static int	procwatchpoll(PROGINFO *,SUBINFO *) ;
static int	procwatchnew(PROGINFO *,SUBINFO *,struct clientinfo *) ;
static int	procwatchint(PROGINFO *,SUBINFO *) ;
static int	procwatchpolling(PROGINFO *,SUBINFO *,int,int) ;

static int	procwatchmaint(PROGINFO *,SUBINFO *) ;
static int	procwatchmaintout(PROGINFO *,LISTENSPEC_INFO *,int,int) ;

static int	procwatchsubcmd(PROGINFO *,const char *) ;

static int	procwatchsubcmd_clear(PROGINFO *) ;


/* local variables */


/* exported subroutines */


int mfswatch_begin(PROGINFO *pip)
{
	int		rs ;
	if (pip->watch == NULL) {
	    const int	oszie = sizeof(MFSWATCH) ;
	    void	*p ;
	    if ((rs = uc_malloc(osize,&p)) >= 0) {
	        pip->watch = p ;
	        if ((rs = mfswatch_beginer(pip)) >= 0) {
		    pip->open.watch = TRUE ;
	        }
	    }
	} else {
	    rs = SR_BUGCHECK ;
	}
	return rs ;
}
/* end subroutine (mfswatch_begin) */


int mfswatch_end(PROGINFO *pip)
{
	int		rs = SR_OK ;
	int		rs1 ;
	if (pip->watch != NULL) {
	    if (pip->open.watch) {
	        pip->open.watch = FALSE ;
	        rs1 = mfswatch_ender(pip) ;
	        if (rs >= 0) rs = rs1 ;
	    }
	    rs1 = uc_free(pip->watch) ;
	    if (rs >= 0) rs = rs1 ;
	    pip->watch = NULL ;
	}
	return rs ;
}
/* end subroutine (mfswatch_end) */


int mfswatch_service(PROGINFO *pip)
{
	MFSWATCH	*wip = pip->watch ;
	int		rs ;
	int		rs1 ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatchpoll: ent\n") ;
#endif

	if ((rs = mfswatch_uptimer(pip)) >= 0) {
	POLLER		*pmp = &wip->pm ;
	POLLER_SPEC	ps ;

	if ((rs = poller_wait(pmp,&ps,wip->minpoll)) > 0) {
	    pip->daytime = time(NULL) ;
	    if ((rs = mfswatch_handle(pip,&ps)) >= 0) {
		while ((rs = poller_have(pmp,&ps)) > 0) {
	    	    rs = mfswatch_handle(pip,&ps) ;
		    if (rs < 0) break ;
		} /* end while */
	    } /* end if */
	} else if (rs == 0) {
	    pip->daytime = time(NULL) ;
	} else if (rs == SR_INTR) {
	    pip->daytime = time(NULL) ;
	    rs = SR_OK ;
	} /* end if */

	if (rs >= 0) {
	   const int	to = TO_MAINT ;
	   if ((pip->daytime-op->ti_lastmaint) >= to) {
		op->ti_lastmaint = op->daytime ;
		rs = mfslisten_maint(pip,pmp) ;
	   }
	}

	if ((rs >= 0) && (pip->config != NULL)) {
	   const int	to = TO_CONFIG ;
	   if ((pip->daytime-op->ti_lastconfig) >= to) {
		CONFIG	*cfp = pip->config ;
		op->ti_lastconfig = op->daytime ;
		rs = config_check(cfp) ;
	   }
	}

	} /* end if (mfswatch_uptimer) */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("mfswatch_service: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (mfswatch_service) */


/* local subroutines */


statis int mfswatch_uptimer(PROGINFO *pip)
{
	MFSWATCH	*wip = pip->watch ;

	if (wip->njobs <= 0) {
	    if (wip->minpoll < 100) {
		wip->minpoll = 100 ;
	    }
	    wip->minpoll += 100 ;
	} else {
	    wip->minpoll += 10 ;
	}

	if (wip->minpoll < 10) {
	    wip->minpoll = 10 ;
	} else if (wip->minpoll > POLLINTMULT) {
	    wip->minpoll = POLLINTMULT ;
	}

	return SR_OK ;
}
/* end subroutine (mfswatch_uptimer) */


static int mfswatch_handle(PROGINFO *pip,POLLER_SPEC *psp)
{
	MFSWATCH	*wip = pip->watch ;
	POLLER		*pmp ;
	const int	fd = psp->fd ;
	const int	re = psp->re ;
	int		rs = SR_OK ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchpoll: fd=%u re=\\b%08�\n",fd,re) ;
#endif

/* handle activity on the intra-program service message portal */

	pmp = &wip->pm ;
	if ((rs = mfsadj_req(pip,fd,re)) == 0) {
	    rs = mfslisten_handle(pip,pmp,fd,re) ;
	    if (rs > 0) {
		logflush(pip) ;
	    }
	} else (rs > 0) {
	    rs = mfsadj_register(pip,pmp) ;
	} /* end if (our intra-program message portal) */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("mfswatch_handle: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (mfswatch_handle) */


int mfswatch_newjob(PROGINFO *pip,int ifd,int ofd)
{
	int		rs = SR_OK ;


	return rs ;
}
/* end subroutine (mfswatch_handle) */


/* private subroutines */


static int mfswatch_beginer(PROGINFO *pip)
{
	MFSWATCH	*wip = pip->watch ;
	int		rs ;
	if ((rs = sreqdb_start(&wip->reqs,td,0)) >= 0) {
	    POLLER	*pmp = &wip->pm ;
	    if ((rs = poller_start(pmp)) >= 0) {
		if ((rs = mfsadj_begin(pip)) >= 0) {
		    if ((rs = mfsadj_register(pip,pmp)) >= 0) {
			rs = mfslisten_maint(pip,pmp) ;
		    }
		    if (rs < 0)
			mfsadj_end(pip) ;
		}
	        if (rs < 0)
		    poller_finish(pmp) ;
	    }
	    if (rs < 0)
		sreqdb_finish(&wip->reqs) ;
	} /* end if (poller_begin) */
	return rs ;
}
/* end subroutine (mfswatch_beginer) */


static int mfswatch_ender(PROGINFO *pip)
{
	MFSWATCH	*wip = pip->watch ;
	int		rs = SR_OK ;
	int		rs1 ;

	rs1 = mfsadj_end(pip) ;
	if (rs >= 0) rs = rs1 ;

	rs1 = poller_start(&wip->pm) ;
	if (rs >= 0) rs = rs1 ;

	rs1 = sreqdb_start(&wip->reqs) ;
	if (rs >= 0) rs = rs1 ;

	return rs ;
}
/* end subroutine (mfswatch_ender) */


static int procwatcher(PROGINFO *pip,SUBINFO *wip,vecstr *nlp)
{
	PROGINFO_IPC	*ipp = &pip->ipc ;
	POLLER_SPEC	ps ;
	time_t		ti_start ;
	int		rs = SR_OK ;
	int		rs1 = 0 ;
	int		to_standcheck = TO_STANDCHECK ;
	int		to_maint = TO_MAINT ;
	int		to_log = TO_LOGFLUSH ;
	int		to_broken = TO_BROKEN ;
	int		loopcount = 0 ;
	int		salen ;
	int		standint ;
	int		f_logged = FALSE ;
	char		timebuf[TIMEBUFLEN + 1] ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatcher: ent f_daemon=%u\n",pip->f.daemon) ;
#endif

	wip->ti_lastmark = pip->daytime ;

	if_exit = FALSE ;
	if_int = FALSE ;
	if_child = FALSE ;

	pip->subserial = 0 ;

	ti_start = pip->daytime ;
	loopcount = 0 ;

/* let's go! */

	if (pip->f.daemon) {

	    if (pip->fd_listentcp >= 0) {
	        ps.fd = pip->fd_listentcp ;
	        ps.events = (POLLIN | POLLPRI) ;
	        poller_reg(&wip->pm,&ps) ;
	    }

	    if (pip->fd_listenpass >= 0) {

#if	SYSHAS_STREAMS
		u_ioctl(pip->fd_listenpass,I_SRDOPT,RMSGD) ;
#endif

	        ps.fd = pip->fd_listenpass ;
	        ps.events = (POLLIN | POLLPRI) ;
	        poller_reg(&wip->pm,&ps) ;
	    }

	} /* end if (daemon mode) */

/* register our "request" channel */

	ps.fd = ipp->fd_req ;
	ps.events = (POLLIN | POLLPRI) ;
	poller_reg(&wip->pm,&ps) ;

/* if we are not in daemon mode, then we have a job waiting on FD_STDIN */

	if (! pip->f.daemon) {
	    struct clientinfo	ci, *cip = &ci ;

	    if ((rs = clientinfo_start(cip)) >= 0) {

	        cip->fd_input = FD_STDIN ;
	        cip->fd_output = FD_STDOUT ;

	        salen = sizeof(SOCKADDRESS) ;
	        rs1 = u_getpeername(cip->fd_input,&cip->sa,&salen) ;
	        if (rs1 >= 0)
	            cip->salen = salen ;

	        rs = procwatchnew(pip,wip,&ci) ;

	        clientinfo_finish(cip) ;
	    } /* end if (clientinfo) */

	} /* end if (not running daemon mode) */

/* top of loop */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatcher: loop-top rs=%d\n",rs) ;
#endif

	if ((rs >= 0) && pip->changed.pc && pip->f.daemon)
	    rs = procwatchmaint(pip,wip) ;

	if (pip->open.logprog)
	    proglog_flush(pip) ;

	while ((rs >= 0) && (! wip->f_exit) && (! if_exit)) {

/* do the poll */

	    if (rs >= 0) {
	        rs = procwatchpoll(pip,wip) ;
		f_logged = f_logged || (rs > 0) ;
	    }

/* are there any completed jobs yet? */

	    if (rs >= 0) {
	        rs = procwatchjobs(pip,wip) ;
		f_logged = f_logged || (rs > 0) ;
	    }

/* external interrupt */

	    if ((rs >= 0) && pip->f.daemon && if_int) {
		if_int = FALSE ;
		to_maint = 0 ;
		rs = procwatchint(pip,wip) ;
		f_logged = f_logged || (rs > 0) ;
	    } /* end if (external interrupt) */

/* broken */

	    if ((rs >= 0) && pip->f.daemon && (to_broken-- == 0)) {
		int	c ;

	        to_broken = TO_BROKEN ;
		to_maint = 0 ;
		rs = procwatchsubcmd_clear(pip) ;
		c = rs ;
		if ((rs >= 0) && (c > 0) && pip->open.logprog) {
			    f_logged = TRUE ;
	    		    proglog_printf(pip,
				"%s broken re-activation (%u)",
				timestr_logz(pip->daytime,timebuf),c) ;
		}

	    } /* end if (broken listner re-activation poll) */

/* maintenance */

	    if ((rs >= 0) && pip->f.daemon && (to_maint-- == 0)) {
	        to_maint = TO_MAINT ;

		if ((rs >= 0) && pip->f.pc) {
		    rs = progconfigcheck(pip) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatcher: progconfigcheck() rs=%d\n",rs) ;
#endif

		    if (rs > 0) {
			if (pip->open.logprog) {
			    f_logged = TRUE ;
	    		    proglog_printf(pip,
				"%s re-configuration",
				timestr_logz(pip->daytime,timebuf)) ;
			}
			rs = procwatchsubcmd_clear(pip) ;
		    } /* end if */

		} /* end if (re-configuration check) */

	        if ((rs >= 0) && pip->changed.pc) {
	            rs = procwatchmaint(pip,wip) ;
		    f_logged = f_logged || (rs > 0) ;
		}

/* maintenance the PID mutex file */

	        if (rs >= 0) {
	            rs = progpidcheck(pip) ;
		}

	        if (rs >= 0) {
	            rs = progsvccheck(pip) ;
	            f_logged = f_logged || (rs > 0) ;
	        }

	        if (rs >= 0) {
	            rs = progacccheck(pip) ;
	            f_logged = f_logged || (rs > 0) ;
	        }

	        if (rs >= 0) {
	            rs = procwatchmark(pip,wip,FALSE) ;
	            f_logged = f_logged || (rs > 0) ;
	        }

/* maintenance the log file */

#if	CF_LOGCHECK
	        if ((rs >= 0) && pip->open.logprog && ((loopcount % 10) == 1))
	            rs = proglog_check(pip) ;
#endif

	    } /* end if (general maintenance) */

/* check up on the standing server object */

	    if ((rs >= 0) && (to_standcheck-- == 0)) {

	        to_standcheck = TO_STANDCHECK ;
	        rs = standing_check(&wip->ourstand,pip->daytime) ;
	        standint = rs ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatcher: standing_check() rs=%d\n",rs) ;
#endif

	        if ((rs >= 0) && (standint > 0)) {
	            to_standcheck = standint / 5 ;
	            if (to_standcheck <= 0)
	                to_standcheck = 1 ;
	        }

	    } /* end if (standing part minimum interval check) */

/* exit on no jobs remaining? */

	    if (rs >= 0) {

	        wip->njobs = jobdb_count(&wip->ourjobs) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatcher: njobs=%u\n",wip->njobs) ;
#endif

	        if (wip->njobs == 0) {
	            if (pip->f.daemon) {
	                if ((pip->intrun > 0) &&
	                    ((pip->daytime - ti_start) > pip->intrun))
	                    wip->f_exit = TRUE ;
	            } else {
	                wip->f_exit = TRUE ;
		    }
	        } /* end if (running on a run-interval) */

	    } /* end if */

	    if ((rs >= 0) && (f_logged || (to_log-- == 0))) {
		to_log = TO_LOGFLUSH ;
		f_logged = FALSE ;
	        proglog_flush(pip) ;
	    }

	    if ((rs >= 0) && if_exit) {
	        rs = SR_INTR ;
	        break ;
	    }

	    pip->changed.pc = FALSE ;
	    loopcount += 1 ;

	} /* end while */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatcher: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (procwatcher) */


static int procwatchmark(PROGINFO *pip,SUBINFO *wip,int f_force)
{
	LISTENSPEC	*lsp ;
	LISTENSPEC_INFO	li ;
	vecobj		*llp = &pip->listens ;
	int		rs = SR_OK ;
	int		i ;
	int		f_to ;
	int		f_logged = FALSE ;
	char		timebuf[TIMEBUFLEN + 1] ;

	if (! pip->open.logprog)
	    goto ret0 ;

	if ((pip->intmark <= 0) && (! f_force))
	    goto ret0 ;

	f_to = ((pip->daytime - wip->ti_lastmark) >= pip->intmark) ;
	if ((! f_to) && (! f_force))
	    goto ret0 ;

	wip->ti_lastmark = pip->daytime ;
	f_logged = TRUE ;
	proglog_printf(pip,"%s mark> %s\n",
	        timestr_logz(pip->daytime,timebuf),
	        pip->nodename) ;

	for (i = 0 ; vecobj_get(llp,i,&lsp) >= 0 ; i += 1) {
	    if (lsp == NULL) continue ;

	    if ((rs = listenspec_info(lsp,&li)) > 0) {
		int	ls = li.state ;
		const char *sn ;

			    if (ls & LISTENSPEC_MDELPEND) {
				sn = "D" ;
			    } else if (ls & LISTENSPEC_MBROKEN) {
				sn = "B" ;
			    } else if (ls & LISTENSPEC_MACTIVE) {
				sn = "A" ;
			    } else {
				sn = "C" ;
			    }

	        proglog_printf(pip,"i=%u %s type=%s addr=%s",
			i,sn,li.type,li.addr) ;

	    } /* end if */

	} /* end for */

ret0:
	return (rs >= 0) ? f_logged : rs ;
}
/* end subroutine (procwatchmark) */


static int procwatchpoll(PROGINFO *pip,SUBINFO *wip)
{
	PROGINFO_IPC	*ipp = &pip->ipc ;
	POLLER_SPEC	ps ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		ns ;
	int		salen ;
	int		fd, re ;
	int		f_logged = FALSE ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatchpoll: ent\n") ;
#endif

	if (wip->njobs <= 0) {
	    if (wip->minpoll < 100)
	        wip->minpoll = 100 ;
	    wip->minpoll += 100 ;
	} else
	    wip->minpoll += 10 ;

	if (wip->minpoll < 10) {
	    wip->minpoll = 10 ;
	} else if (wip->minpoll > POLLINTMULT)
	    wip->minpoll = POLLINTMULT ;

	if (if_child) {
	    if_child = FALSE ;
	    wip->minpoll = 0 ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	    debugprintf("procwatchpoll: SIGCHILD\n") ;
#endif

	} /* end if */

#if	CF_DEBUG && 0
	if (DEBUGLEVEL(4)) {
	    POLLER_CUR	cur ;
	    debugprintf("procwatchpoll: minpoll=%u\n",wip->minpoll) ;
		poller_curbegin(&wip->pm,&cur) ;
		while (poller_enum(&wip->pm,&cur,&ps) >= 0) {
	        debugprintf("procwatchpoll: fd=%d e=%08b\n",
			ps.fd,ps.events) ;
		}
		poller_curend(&wip->pm,&cur) ;
	        debugprintf("procwatchpoll: poller_wait() minpoll=%u\n",
		    wip->minpoll) ;
	}
#endif /* CF_DEBUG */

	rs = poller_wait(&wip->pm,&ps,wip->minpoll) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatchpoll: poller_wait() rs=%d\n",rs) ;
#endif

	pip->daytime = time(NULL) ;
	if (rs == SR_INTR) rs = SR_OK ;

	if (rs <= 0)
	    goto ret0 ;

	fd = ps.fd ;
	re = ps.revents ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchpoll: fd=%u re=\\b%08�\n",fd,re) ;
#endif

/* handle activity on the intra-program service message portal */

	if ((rs >= 0) && (fd == ipp->fd_req) && (re != 0)) {

	    if ((re & POLLIN) || (re & POLLPRI)) {
	        rs = procwatchpollipc(pip,wip) ;
	        f_logged = f_logged || (rs > 0) ;
	    } /* end if (poll came back on input) */

	    if (rs >= 0) {
	        ps.fd = fd ;
	        ps.events = (POLLIN | POLLPRI) ;
	        rs = poller_reg(&wip->pm,&ps) ;
	    }

	} /* end if (our intra-program message portal) */

/* handle any activity on our listen socket or FIFO */

	if ((rs >= 0) &&
	    ((fd == pip->fd_listentcp) || (fd == pip->fd_listenpass)) &&
	    (re != 0)) {

	    if ((re & POLLIN) || (re & POLLPRI)) {
	        struct clientinfo	ci, *cip = &ci ;

	        if ((rs = clientinfo_start(&ci)) >= 0) {

		    ns = -1 ;
	            if (fd == pip->fd_listentcp) {

	                salen = sizeof(SOCKADDRESS) ;
	                rs1 = u_accept(pip->fd_listentcp,&cip->sa,&salen) ;
	                ns = rs1 ;
	                if (rs1 >= 0) {
	                    cip->salen = salen ;
	                    cip->fd_input = u_dup(ns) ;
	                    cip->fd_output = u_dup(ns) ;
	                }

	            } else if (fd == pip->fd_listenpass) {
	                struct strrecvfd	passer ;
			int	size = sizeof(struct strrecvfd) ;
			int	to = pip->to_recvfd ;

	                cip->salen = -1 ;
	                rs1 = acceptpass(pip->fd_listenpass,&passer,to) ;
	                ns = rs1 ;
	                if (rs1 >= 0) {

	                    cip->fd_input = u_dup(ns) ;
	                    cip->fd_output = u_dup(ns) ;
	                    salen = sizeof(SOCKADDRESS) ;
	                    rs1 = u_getpeername(ns,&cip->sa,&salen) ;
	                    if (rs1 >= 0)
	                        cip->salen = salen ;

	                } else if (rs1 == SR_BADMSG) {
			    u_read(pip->fd_listenpass,&passer,size) ;
			}

	            } /* end if (getting FD of new connection) */

	            if (ns >= 0) {
	                u_close(ns) ;
			if (rs >= 0) {
	                    rs = procwatchnew(pip,wip,cip) ;
			}
	            } /* end if */

	            clientinfo_finish(cip) ;
	        } /* end if (clientinfo) */

	    } /* end if (our listen socket-portals) */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatchpoll: mid3 rs=%d\n",rs) ;
#endif

	    if (rs >= 0) {
	        ps.fd = fd ;
	        ps.events = (POLLIN | POLLPRI) ;
	        rs = poller_reg(&wip->pm,&ps) ;
	    }

	} /* end if (something from 'poll') */

/* handle events on listeners */

	if (rs >= 0) {
	    rs = procwatchpolling(pip,wip,fd,re) ;
	}

ret0:

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatchpoll: ret rs=%d f_logged=%u\n",rs,f_logged) ;
#endif

	return (rs >= 0) ? f_logged : rs ;
}
/* end subroutine (procwatchpoll) */


static int procwatchpollipc(PROGINFO *pip,SUBINFO *wip)
{
	struct msghdr	*mp ;
	IPCMSGINFO	mi, *mip = &mi ;
	PROGINFO_IPC	*ipp = &pip->ipc ;
	int		rs ;
	int		*ip = NULL ;
	int		f_logged = FALSE ;

	ipcmsginfo_init(mip) ;

	mp = &mip->ipcmsg ;
	if ((rs = u_recvmsg(ipp->fd_req,mp,0)) > 0) {
	    const int	rcode = MKCHAR(mip->ipcbuf[0]) ;
	    mip->ipcmsglen = rs ;

/* check if we were passed a FD */

	if (mp->msg_controllen > 0) {
	    struct cmsghdr	*cmp = CMSG_FIRSTHDR(mp) ;
	    const int		fdlen = sizeof(int) ;
	    while (cmp != NULL) {

	        ip = (int *) CMSG_DATA(cmp) ;
	        if ((cmp->cmsg_level == SOL_SOCKET) && 
	            (cmp->cmsg_len == CMSG_LEN(fdlen)) &&
	            (cmp->cmsg_type == SCM_RIGHTS) && (ip != NULL)) {

	            if ((mip->ns < 0) && (rcode == muximsgtype_passfd)) {
	                mip->ns = *ip ;
	            } else {
	                u_close(*ip) ;
		    }

	        } /* end if */
	        cmp = CMSG_NXTHDR(mp,cmp) ;

	    } /* end while */
	} /* end if (had a control-part) */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatch/pollipc: passfd? ns=%d\n",mip->ns) ;
#endif

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatch/pollipc: rcode=%u\n",rcode) ;
#endif

/* handle the messages (according to our type-code) */

	switch (rcode) {
	case muximsgtype_noop:
	    rs = procwatchpollipc_noop(pip,wip,mip) ;
	    break ;
	case muximsgtype_passfd:
	    rs = procwatchpollipc_passfd(pip,wip,mip) ;
	    break ;
	case muximsgtype_exit:
	    rs = procwatchpollipc_exit(pip,wip,mip) ;
	    break ;
	case muximsgtype_getlistener:
	    rs = procwatchpollipc_getlistener(pip,wip,mip) ;
	    break ;
	case muximsgtype_mark:
	    rs = procwatchpollipc_mark(pip,wip,mip) ;
	    f_logged = TRUE ;
	    break ;
	case muximsgtype_gethelp:
	    rs = procwatchpollipc_gethelp(pip,wip,mip) ;
	    break ;
	case muximsgtype_cmd:
	    rs = procwatchpollipc_cmd(pip,wip,mip) ;
	    break ;
	default:
	    rs = procwatchpollipc_default(pip,wip,mip) ;
	    break ;
	} /* end switch */

	if (mip->ns >= 0) {
	    u_close(mip->ns) ;
	    mip->ns = -1 ;
	}

	} /* end if (u_recvmsg) */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatch/pollipc: ret rs=%d\n",rs) ;
#endif

	return (rs >= 0) ? f_logged : rs ;
}
/* end subroutine (procwatchpollipc) */


static int procwatchpollipc_noop(PROGINFO *pip,SUBINFO *wip,IPCMSGINFO *mip)
{
	struct muximsg_response	m0 ;
	struct muximsg_noop	m1 ;
	PROGINFO_IPC	*ipp = &pip->ipc ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		blen ;

	rs1 = muximsg_noop(&m1,1,mip->ipcbuf,IPCBUFLEN) ;
	if (rs1 < 0)
	    goto ret0 ;

	m0.pid = pip->spid ;
	m0.tag = m1.tag ;
	m0.rc = 0 ;
	rs = muximsg_response(&m0,0,mip->ipcbuf,IPCBUFLEN) ;
	blen = rs ;
	if (rs >= 0) {

	    mip->ipcmsg.msg_control = NULL ;
	    mip->ipcmsg.msg_controllen = 0 ;
	    mip->vecs[0].iov_len = blen ;
	    rs = u_sendmsg(ipp->fd_req,&mip->ipcmsg,0) ;
	if (rs == SR_ACCESS) {
	    if (pip->open.logprog)
	        proglog_printf(pip,"could not reply (%d)",rs) ;
	    rs = SR_OK ;
	}

	} /* end if */

ret0:
	return rs ;
}
/* end subroutine (procwatchpollipc_noop) */


static int procwatchpollipc_passfd(PROGINFO *pip,SUBINFO *wip,IPCMSGINFO *mip)
{
	struct muximsg_response	m0 ;
	struct muximsg_passfd	m2 ;
	struct clientinfo	ci, *cip = &ci ;
	PROGINFO_IPC	*ipp = &pip->ipc ;
	uint		rc = 0 ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		salen ;
	int		blen ;

/* check for valid message */

	rs1 = muximsg_passfd(&m2,1,mip->ipcbuf,IPCBUFLEN) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatch/pollipc_passfd: muximsg_passfd() rs=%d\n",
	        rs1) ;
#endif

	if ((rs1 < 0) && (mip->ns >= 0)) {
	    u_close(mip->ns) ;
	    mip->ns = -1 ;
	}

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatch/pollipc: ns=%d\n",mip->ns) ;
#endif

	if (rs1 < 0)
	    goto ret0 ;

	if (mip->ns >= 0) {

	    rs = clientinfo_start(cip) ;
	    if (rs >= 0) {

	        cip->fd_input = u_dup(mip->ns) ;
	        cip->fd_output = u_dup(mip->ns) ;
	        salen = sizeof(SOCKADDRESS) ;
	        rs1 = u_getpeername(mip->ns,&cip->sa,&salen) ;
	        if (rs1 >= 0)
	            cip->salen = salen ;

	        {
	            u_close(mip->ns) ;
	            mip->ns = -1 ;
	        }

	        rs = procwatchnew(pip,wip,cip) ;
	        rc = (rs > 0) ? muximsgrc_ok : muximsgrc_error ;

#if	CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("progwatch/pollipc_passfd: "
			"procwatchnew() rs=%d\n",rs) ;
#endif

	        clientinfo_finish(cip) ;
	    } /* end if (clientinfo) */

	} else {

	    rc = muximsgrc_nofd ;
	    if (pip->open.logprog) {
	        proglog_printf(pip,
	            "no FD was passed\n") ;
	    }

	} /* end if (nothing was there) */

	if (rs >= 0) {

#if	CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("progwatch/pollipc: response rc=%u\n",rc) ;
#endif

	    m0.pid = pip->spid ;
	    m0.tag = m2.tag ;
	    m0.rc = rc ;
	    rs = muximsg_response(&m0,0,mip->ipcbuf,IPCBUFLEN) ;
	    blen = rs ;
	    if (rs >= 0) {

	        mip->ipcmsg.msg_control = NULL ;
	        mip->ipcmsg.msg_controllen = 0 ;
	        mip->vecs[0].iov_len = blen ;
	        rs = u_sendmsg(ipp->fd_req,&mip->ipcmsg,0) ;
	if (rs == SR_ACCESS) {
	    if (pip->open.logprog)
	        proglog_printf(pip,"could not reply (%d)",rs) ;
	    rs = SR_OK ;
	}

#if	CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("progwatch/pollipc_passfd: u_sendmsg() rs=%d\n",
	                rs) ;
#endif

	    } /* end if */

	} /* end if */

ret0:
	return rs ;
}
/* end subroutine (procwatchpollipc_passfd) */


static int procwatchpollipc_exit(PROGINFO *pip,SUBINFO *wip,IPCMSGINFO *mip)
{
	struct muximsg_response	m0 ;
	struct muximsg_exit	m2 ;
	PROGINFO_IPC	*ipp = &pip->ipc ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		blen ;

	rs1 = muximsg_exit(&m2,1,mip->ipcbuf,IPCBUFLEN) ;
	if (rs1 < 0)
	    goto ret0 ;

	wip->f_exit = TRUE ;

	m0.pid = pip->spid ;
	m0.tag = m2.tag ;
	m0.rc = 0 ;
	rs = muximsg_response(&m0,0,mip->ipcbuf,IPCBUFLEN) ;
	blen = rs ;
	if (rs >= 0) {

	    mip->ipcmsg.msg_control = NULL ;
	    mip->ipcmsg.msg_controllen = 0 ;
	    mip->vecs[0].iov_len = blen ;
	    rs = u_sendmsg(ipp->fd_req,&mip->ipcmsg,0) ;
	if (rs == SR_ACCESS) {
	    if (pip->open.logprog)
	        proglog_printf(pip,"could not reply (%d)",rs) ;
	    rs = SR_OK ;
	}

	} /* end if */

ret0:
	return rs ;
}
/* end subroutine (procwatchpollipc_exit) */


static int procwatchpollipc_getlistener(PROGINFO *pip,SUBINFO *wip,
		IPCMSGINFO *mip)
{
	struct muximsg_getlistener	i9 ;
	struct muximsg_listener		i10 ;
	PROGINFO_IPC	*ipp = &pip->ipc ;
	LISTENSPEC	*lsp ;
	LISTENSPEC_INFO	li ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		idx ;
	int		blen ;

	rs1 = muximsg_getlistener(&i9,1,mip->ipcbuf,IPCBUFLEN) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatch/pollipc_getlistener: "
		"muximsg_getlistener() "
		"rs=%d ml=%u\n",
		rs1,i9.msglen) ;
#endif

	if (rs1 < 0)
	    goto ret0 ;

	memset(&i10,0,sizeof(struct muximsg_listener)) ;
	i10.pid = pip->spid ;
	i10.tag = i9.tag ;
	i10.idx = i9.idx ;
	i10.name[0] = '\0' ;
	i10.addr[0] = '\0' ;

	idx = i9.idx ;
	rs1 = vecobj_get(&pip->listens,idx,&lsp) ;

	if ((rs1 >= 0) && (lsp != NULL)) {
	    if ((rs1 = listenspec_info(lsp,&li)) >= 0) {

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	debugprintf("procwatch/pollipc_getlistener: idx=%u listenspec_info() "
		"ls=%u\n",idx,li.state) ;
	debugprintf("procwatch/pollipc_getlistener: t=%s a=%s\n",
		li.type,li.addr) ;
#endif

		strwcpy(i10.name,li.type,MUXIMSG_LNAMELEN) ;
		strwcpy(i10.addr,li.addr,MUXIMSG_LADDRLEN) ;
		i10.ls = (li.state & 0xff) ;
	    }
	}

	i10.rc = (rs1 >= 0) ? muximsgrc_ok : muximsgrc_notavail ;

	rs1 = muximsg_listener(&i10,0,mip->ipcbuf,IPCBUFLEN) ;
	blen = rs1 ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	debugprintf("procwatch/pollipc_getlistener: muximsg_listener() "
		"rs=%d ml=%u\n",
		rs1,i10.msglen) ;
#endif

	if ((rs1 == SR_OVERFLOW) || (rs1 == SR_TOOBIG)) {
	    i10.addr[0] = '\0' ;
	    rs1 = muximsg_listener(&i10,0,mip->ipcbuf,IPCBUFLEN) ;
	    blen = rs1 ;
	}

	if (rs >= 0) {

	    mip->ipcmsg.msg_control = NULL ;
	    mip->ipcmsg.msg_controllen = 0 ;
	    mip->vecs[0].iov_len = blen ;
	    rs = u_sendmsg(ipp->fd_req,&mip->ipcmsg,0) ;
	if (rs == SR_ACCESS) {
	    if (pip->open.logprog) {
	        proglog_printf(pip,"could not reply (%d)",rs) ;
	    }
	    rs = SR_OK ;
	}

#if	CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("procwatch/pollipc_getlistener: "
			"u_sendmsg() rs=%d\n", rs) ;
#endif

	} /* end if */

ret0:

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	debugprintf("procwatch/pollipc_getlistener: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (procwatchpollipc_getlistener) */


static int procwatchpollipc_gethelp(PROGINFO *pip,SUBINFO *wip,IPCMSGINFO *mip)
{
	struct muximsg_gethelp	i13 ;
	struct muximsg_help	i14 ;
	PROGINFO_IPC	*ipp = &pip->ipc ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		idx ;
	int		blen ;
	const char	*cnp = NULL ;	/* command name-pointer */

	rs1 = muximsg_gethelp(&i13,1,mip->ipcbuf,IPCBUFLEN) ;
	if (rs1 < 0)
	    goto ret0 ;

	memset(&i14,0,sizeof(struct muximsg_help)) ;
	i14.pid = pip->spid ;
	i14.tag = i13.tag ;
	i14.idx = i13.idx ;
	i14.rc = muximsgrc_ok ;
	i14.name[0] = '\0' ;

	idx = i13.idx ;
	rs1 = progcmdname(pip,idx,&cnp) ;

	if ((rs1 >= 0) && (cnp != NULL)) {
	    rs1 = sncpy1(i14.name,MUXIMSG_LNAMELEN,cnp) ;
	    if (rs1 < 0)
		i14.rc = muximsgrc_overflow ;
	} else
	    i14.rc = muximsgrc_notavail ;

	rs1 = muximsg_help(&i14,0,mip->ipcbuf,IPCBUFLEN) ;
	blen = rs1 ;

	if ((rs1 == SR_OVERFLOW) || (rs1 == SR_TOOBIG)) {
	    i14.name[0] = '\0' ;
	    i14.rc = muximsgrc_overflow ;
	    rs1 = muximsg_help(&i14,0,mip->ipcbuf,IPCBUFLEN) ;
	    blen = rs1 ;
	}

	if (rs >= 0) {

	    mip->ipcmsg.msg_control = NULL ;
	    mip->ipcmsg.msg_controllen = 0 ;
	    mip->vecs[0].iov_len = blen ;
	    rs = u_sendmsg(ipp->fd_req,&mip->ipcmsg,0) ;
	    if (isBadSend(rs)) {
	        proglog_printf(pip,"could not reply (%d)",rs) ;
	        rs = SR_OK ;
	    }

	} /* end if */

ret0:
	return rs ;
}
/* end subroutine (procwatchpollipc_gethelp) */


static int procwatchpollipc_mark(PROGINFO *pip,SUBINFO *wip,IPCMSGINFO *mip)
{
	struct muximsg_mark	i11 ;
	struct muximsg_response	i0 ;
	PROGINFO_IPC	*ipp = &pip->ipc ;
	const int	ipclen = IPCBUFLEN ;
	int		rs = SR_OK ;
	int		rs1 ;

	if ((rs1 = muximsg_mark(&i11,1,mip->ipcbuf,ipclen)) >= 0) {

	    i0.pid = pip->spid ;
	    i0.tag = i11.tag ;
	    i0.rc = muximsgrc_ok ;
	    if ((rs1 = muximsg_response(&i0,0,mip->ipcbuf,ipclen)) >= 0) {
	        int	blen = rs1 ;

	        mip->ipcmsg.msg_control = NULL ;
	        mip->ipcmsg.msg_controllen = 0 ;
	        mip->vecs[0].iov_len = blen ;
	        rs = u_sendmsg(ipp->fd_req,&mip->ipcmsg,0) ;
	        if (isBadSend(rs)) {
	            proglog_printf(pip,"could not reply (%d)",rs) ;
	            rs = SR_OK ;
	        }

	        procwatchmark(pip,wip,TRUE) ;

	    } /* end if (muximsg_response) */

	} /* end if (muximsg_mark) */

	return rs ;
}
/* end subroutine (procwatchpollipc_mark) */


static int procwatchpollipc_cmd(PROGINFO *pip,SUBINFO *wip,IPCMSGINFO *mip)
{
	struct muximsg_response	m0 ;
	struct muximsg_cmd	m15 ;
	PROGINFO_IPC	*ipp = &pip->ipc ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		blen ;

	rs1 = muximsg_cmd(&m15,1,mip->ipcbuf,IPCBUFLEN) ;
	if (rs1 < 0)
	    goto ret0 ;

	m0.pid = pip->spid ;
	m0.tag = m15.tag ;
	m0.rc = 0 ;
	rs = muximsg_response(&m0,0,mip->ipcbuf,IPCBUFLEN) ;
	blen = rs ;
	if (rs >= 0) {

	    rs1 = procwatchsubcmd(pip,m15.cmd) ;

	    m0.rc = muximsgrc_ok ;
	    switch (rs1) {
	    case SR_INVALID:
		m0.rc = muximsgrc_invalid ;
		break ;
	    default:
		m0.rc = muximsgrc_error ;
		break ;
	    } /* end switch */

	    mip->ipcmsg.msg_control = NULL ;
	    mip->ipcmsg.msg_controllen = 0 ;
	    mip->vecs[0].iov_len = blen ;
	    rs = u_sendmsg(ipp->fd_req,&mip->ipcmsg,0) ;
	    if (isBadSend(rs)) {
	        if (pip->open.logprog)
	            proglog_printf(pip,"could not reply (%d)",rs) ;
	        rs = SR_OK ;
	    }

	} /* end if */

ret0:
	return rs ;
}
/* end subroutine (procwatchpollipc_cmd) */


static int procwatchpollipc_default(PROGINFO *pip,SUBINFO *wip,IPCMSGINFO *mip)
{
	PROGINFO_IPC	*ipp = &pip->ipc ;
	STANDING	*osp = &wip->ourstand ;
	time_t		dt = pip->daytime ;
	const int	rcode = MKCHAR(mip->ipcbuf[0]) ;
	int		rs ;
	int		len ;
	char		resbuf[MSGBUFLEN + 1] ;

	if ((rs = standing_request(osp,dt,rcode,mip->ipcbuf,resbuf)) >= 0) {
	    len = rs ;
	    mip->ipcmsg.msg_control = NULL ;
	    mip->ipcmsg.msg_controllen = 0 ;
	    mip->vecs[0].iov_base = resbuf ;
	    mip->vecs[0].iov_len = len ;
	    rs = u_sendmsg(ipp->fd_req,&mip->ipcmsg,0) ;
	    if (isBadSend(rs)) {
	        proglog_printf(pip,"could not reply (%d)",rs) ;
	        rs = SR_OK ;
	    }
	} /* end if (standing_request) */

	return rs ;
}
/* end subroutine (procwatchpollipc_default) */


static int procwatchjobs(PROGINFO *pip,SUBINFO *wip)
{
	int		rs = SR_OK ;
	int		rs1 ;
	int		f = FALSE ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("progwatchjobs: njobs=%u\n",wip->njobs) ;
#endif

	if (wip->njobs > 0) {
	    int		cs ;
	    if ((rs1 = u_waitpid(-1,&cs,W_OPTIONS)) > 0) {
	        JOBDB_ENT	*jep ;
	        pid_t		pid = rs1 ;
		int		ji ;
		char		timebuf[TIMEBUFLEN + 1] ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("progwatchjobs: child exit pid=%u stat=%d\n",
	            rs1,(cs & 0xFF)) ;
#endif

	    if ((ji = jobdb_findpid(&wip->ourjobs,pid,&jep)) >= 0) {

#if	CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("progwatchjobs: found child ji=%d\n",ji) ;
#endif

	        timestr_logz(pip->daytime,timebuf) ;

	        if (pip->open.logprog) {
		    f = TRUE ;
	            proglog_setid(pip,jep->jobid) ;
		}

	    if (WIFEXITED(cs)) {

		if (pip->debuglevel > 0) {
	            bprintf(pip->efp,
		        "%s: server(%u) exited normally ex=%u\n",
		        pip->progname,pid,WEXITSTATUS(cs)) ;
		}

		if (pip->open.logprog) {
	            proglog_printf(pip,
		        "%s server(%u) exited normally ex=%u",
		        timebuf,pid,WEXITSTATUS(cs)) ;
		}

	    } else if (WIFSIGNALED(cs)) {
		const int	sig = WTERMSIG(cs) ;
		const char	*ss ;
		char		sigbuf[20+1] ;

		if ((ss = strsigabbr(sig)) == NULL) {
		     ctdeci(sigbuf,20,sig) ;
		     ss = sigbuf ;
		}

#if	CF_DEBUG
	        if (DEBUGLEVEL(4))
	            debugprintf("progwatchjobs: signalled sig=%s\n",ss) ;
#endif

		if (! pip->f.quiet) {
	            bprintf(pip->efp,
		        "%s: server(%u) was signalled sig=%s\n",
		        pip->progname,pid,ss) ;
		}

		if (pip->open.logprog) {
	            proglog_printf(pip,
		        "%s server(%u) was signalled sig=%s",
		        timebuf,pid,ss) ;
		}

	    } else {

		if (! pip->f.quiet) {
	            bprintf(pip->efp,
		        "%s: server(%u) exited abnormally cs=%u\n",
		        pip->progname,pid,cs) ;
		}

		if (pip->open.logprog) {
	            proglog_printf(pip,
		        "%s server(%u) exited abnormally cs=%u",
		    	timebuf,pid,cs) ;
		}

	    } /* end if */

		if (pip->open.logprog) {
	            rs = proglogout(pip, "stdout", jep->ofname) ;
	            if (rs >= 0)
	                rs = proglogout(pip, "stderr", jep->efname) ;
	            proglog_printf(pip,"elapsed time %s\n",
	                timestr_elapsed((pip->daytime - jep->stime),
	                timebuf)) ;
	            proglog_setid(pip,pip->logid) ;
	        } /* end if (have logging) */

		wip->njobs -= 1 ;
	        rs1 = jobdb_del(&wip->ourjobs,ji) ;

#if	CF_DEBUG
		if (DEBUGLEVEL(4))
		debugprintf("procwatchjobs: jobdb_del() rs=%d\n",rs1) ;
#endif

	    } else if (pip->open.logprog) {
	        proglog_printf(pip,"unknown PID=%u\n",pid) ;
	    }

	    } /* end if (a child process exited) */
	} /* end if (jobs outstanding) */

#if	CF_DEBUG
	if (DEBUGLEVEL(4)) {
	debugprintf("procwatchjobs: ret rs=%d f=%u\n",rs,f) ;
	debugprintf("procwatchjobs: ret njobs=%u\n",wip->njobs) ;
	}
#endif

	return (rs >= 0) ? f : rs ;
}
/* end subroutine (procwatchjobs) */


/* spawn a job */
static int procwatchnew(PROGINFO *pip,SUBINFO *wip,struct clientinfo *cip)
{
	JOBDB_ENT	*jep ;
	pid_t		pid ;
	int		rs ;
	int		i ;
	char		peername[PEERNAMELEN + 1] ;
	char		logid[JOBDB_JOBIDLEN + 1] ;
	char		timebuf[TIMEBUFLEN + 1] ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: ent isascoket=%u\n",
	        isasocket(cip->fd_input)) ;
#endif

/* enter this job into the database */

	cip->stime = pip->daytime ;

#if	CF_MKSUBLOGID
	rs = mksublogid(logid,JOBDB_JOBIDLEN,pip->logid,pip->subserial) ;
#else
	rs = snsdd(logid,JOBDB_JOBIDLEN,pip->logid,pip->subserial) ;
#endif	/* CF_MKSUBLOGID */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: mksublogid() rs=%d jobid=%s\n",
		rs,logid) ;
#endif

	if (rs < 0)
	    goto ret0 ;

	if (pip->open.logprog)
	    proglog_setid(pip,logid) ;

/* can we get the peername of the other end of this socket, if a socket? */

	peername[0] = '\0' ;
	if (rs >= 0)
	    rs = progpeername(pip,cip,peername) ;

	if (rs < 0)
	    goto ret0 ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: peername=%s\n",peername) ;
#endif

	if (pip->debuglevel > 0) {
	    bprintf(pip->efp,"%s: %s request\n",pip->progname,
	            timestr_logz(cip->stime,timebuf)) ;
	    if (peername[0] != '\0')
	        bprintf(pip->efp,"%s: from=%s\n",pip->progname,peername) ;
	} /* end if */

	if (pip->open.logprog) {
	    proglog_printf(pip,"%s request",
	            timestr_logz(cip->stime,timebuf)) ;
	    if (peername[0] != '\0')
	        proglog_printf(pip,"from=%s", peername) ;
	} /* end if */

	if ((rs = jobdb_newjob(&wip->ourjobs,logid,FALSE)) >= 0) {
	    int	ji = rs ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: jobdb_newjob() rs=%d\n",rs) ;
#endif

	jobdb_get(&wip->ourjobs,ji,&jep) ;
	if (pip->open.logprog) proglog_flush(pip) ;
	bflush(pip->efp) ;

/* do it */

#if	CF_DEBUG && CF_DEBUGFORK
	if (DEBUGLEVEL(4)) {
	    debugprintf("procwatchnew: debugfork\n") ;
	    rs = debugfork("progwatch2") ;
	    debugprintf("procwatchnew: debugfork() rs=%d\n",rs) ;
	}
#endif

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: uc_fork()\n") ;
#endif

	if ((rs = uc_fork()) == 0) { /* child */
	    int		oflags ;
	    int		nsi, nso ;
	    int		ex = EX_OK ;
	    int		f_nsiok, f_nsook ;

/* we are now the CHILD!! */

	    pip->logid = jep->jobid ;
	    cip->pid = getpid() ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4)) {
	        debugprintf("progwatch/procwatchnew: child pid=%d\n",
		cip->pid) ;
	    }
#endif

#if	CF_LOADNAMES
	    clientinfo_loadnames(cip,pip->domainname) ;
#endif

/* close stuff we do not need */

	    if (pip->f.daemon) {

	        if (pip->fd_listenpass >= 0) {
	            u_close(pip->fd_listenpass) ;
	            pip->fd_listenpass = -1 ;
	        }

	        if (pip->fd_listentcp >= 0) {
	            u_close(pip->fd_listentcp) ;
	            pip->fd_listentcp = -1 ;
	        }

	    } /* end if (daemon-mode) */

	    nsi = cip->fd_input ;
	    nso = cip->fd_output ;

/* move some FDs that we do need, if necessary */

	    f_nsiok = (nsi == FD_STDIN) ;

	    f_nsook = (nso == FD_STDOUT) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("procwatchnew: 0 nsi=%d nso=%d\n",
		nsi,nso) ;
#endif

/* we must use 'dupup()' and NOT 'uc_moveup()'! */

	    if ((nsi < 3) && (! f_nsiok))
	        nsi = dupup(nsi,3) ;

	    if ((nso < 3) && (! f_nsook))
	        nso = dupup(nso,3) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4))
	        debugprintf("procwatchnew: 1 nsi=%d nso=%d\n",
		nsi,nso) ;
#endif

/* setup the input and output for this spawned process */

	    for (i = 0 ; i < 3 ; i += 1) {
	        int	f_keep ;

	        f_keep = ((i == nsi) && f_nsiok) ;
	        if (! f_keep)
	            f_keep = ((i == nso) && f_nsook) ;

	        if (! f_keep)
	            u_close(i) ;

	    } /* end for */

	    if (! f_nsiok)
	        u_dup(nsi) ;

	    if (! f_nsook)
	        u_dup(nso) ;

	    oflags = (O_WRONLY | O_CREAT) ;
	    i = u_open(jep->efname,oflags,0666) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(4)) {
	        struct ustat	sb ;
	        int	rs1 ;
	        rs1 = u_fstat(cip->fd_input,&sb) ;
	        debugprintf("procwatchnew: fd=%d u_fstat() rs=%d "
	            "mode=%08X f_issock=%u\n",
	            cip->fd_input,rs1,sb.st_mode,
			((S_ISSOCK(sb.st_mode)) ? 1 : 0)) ;
	        debugprintf("progwatch/procwatchnew: STDERR=%d\n",i) ;
#ifdef	COMMENT
		{
	        int 	cl ;
	        cchar *cp = "hello from underworld\n" ;
	        cl = strlen(cp) ;
	        uc_writen(i,cp,cl) ;
		}
#endif
	    }
#endif /* CF_DEBUG */

/* do it */

	    cip->stime = pip->daytime ;
	    if (pip->open.logprog)
	        proglog_printf(pip,"server pid=%u\n",
	            (int) cip->pid) ;

/* set keep-alive (if it is not a socket we ignore the failure) */

	    uc_keepalive(cip->fd_input,TRUE) ;

#if	CF_PROGHANDLE
	    if (pip->open.logprog)
	        proglog_flush(pip) ;

	    rs = proghandle(pip,&wip->ourstand,&wip->ourbuilt,cip) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: proghandle() rs=%d\n",rs) ;
#endif

#endif /* CF_PROGHANDLE */

#ifdef	COMMENT
	    if (rs < 0) {
	        u_unlink(jep->efname) ;
	        jep->efname[0] = '\0' ;
	    }
#endif /* COMMENT */

	if (pip->open.logprog) {
	    proglog_end(pip) ;
	}

	if ((rs < 0) && (ex == EX_OK)) {
	    switch (rs) {
	    case SR_INVALID:
	        ex = EX_USAGE ;
	        break ;
	    case SR_NOENT:
	        ex = EX_NOPROG ;
	        break ;
	    case SR_NXIO:
	    case SR_LIBACC:
	    case SR_NOEXEC:
	        ex = EX_NOEXEC ;
	        break ;
	    case SR_AGAIN:
	        ex = EX_TEMPFAIL ;
	        break ;
	    default:
	        ex = mapex(mapexs,rs) ;
	        break ;
	    } /* end switch */
	} /* end if */

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: exiting ex=%u (%d)\n",ex,rs) ;
#endif

	    uc_exit(ex) ;
	} else if (rs > 0) {
	    pid = rs ;

	jep->pid = pid ;
	wip->njobs += 1 ;
	pip->subserial += 1 ;

	} else if (rs < 0) {
	    jobdb_del(&wip->ourjobs,ji) ;
	    if (pip->open.logprog)
	        proglog_printf(pip,
	            "could not fork (%d)\n",rs) ;
	}

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: fork parent\n") ;
#endif

	} /* end if (jobdb_newjob) */

/* back at the parent process */

	if (cip->fd_input >= 0) {
	    u_close(cip->fd_input) ;
	    cip->fd_input = -1 ;
	}

	if (cip->fd_output >= 0) {
	    u_close(cip->fd_output) ;
	    cip->fd_output = -1 ;
	}

	if (pip->open.logprog)
	    proglog_setid(pip,pip->logid) ;

ret0:

#if	CF_DEBUG
	if (DEBUGLEVEL(4))
	    debugprintf("procwatchnew: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (procwatchnew) */


static int procwatchint(PROGINFO *pip,SUBINFO *wip)
{
	int		rs ;
	int		f_logged = FALSE ;

	rs = procwatchsubcmd_clear(pip) ;

	if ((rs >= 0) && pip->open.logprog) {
	    char	tbuf[TIMEBUFLEN + 1] ;
	    f_logged = TRUE ;
	    timestr_logz(pip->daytime,tbuf) ;
	    proglog_printf(pip,"%s interruption",tbuf) ;
	}

	return (rs >= 0) ? f_logged : rs ;
}
/* end subroutine (procwatchint) */


/****
+) cancel (unregister) polls for listeners that are being deleted
+) delete listeners that are being deleted
+) activate listeners that are not active
+) register polls for active listeners that are not registered
****/

static int procwatchmaint(PROGINFO *pip,SUBINFO	 *wip)
{
	LISTENSPEC	*lsp ;
	LISTENSPEC_INFO	li ;
	POLLER_SPEC	ps ;
	vecobj		*llp = &pip->listens ;
	const int	events = (POLLIN | POLLPRI) ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		ac ;
	int		fd ;
	int		opts ;
	int		i ;
	int		f_active ;
	int		f_broken ;
	int		f_logged = FALSE ;

#if	CF_DEBUG
	if (DEBUGLEVEL(5))
	    debugprintf("progwatch/procwatchpoll: checking deletes\n") ;
#endif

	for (i = 0 ; vecobj_get(llp,i,&lsp) >= 0 ; i += 1) {
	    if (lsp == NULL) continue ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(5)) {
		LISTENSPEC_INFO	li ;
	        listenspec_info(lsp,&li) ;
	        fd = listenspec_getfd(lsp) ;
	        debugprintf("progwatch/procwatchpoll: i=%u t=%s lfd=%d\n",
			i,li.type,fd) ;
	    }
#endif /* CF_DEBUG */

	    if ((rs1 = listenspec_delmarked(lsp)) > 0) {

#if	CF_DEBUG
	        if (DEBUGLEVEL(5))
	            debugprintf("progwatch/procwatchpoll: delmarked\n") ;
#endif

		if ((rs = listenspec_info(lsp,&li)) >= 0) {
		    rs1 = procwatchmaintout(pip,&li,0,0) ;
		    f_logged = f_logged || (rs1 > 0) ;
		}

		if (rs >= 0) {

	            fd = listenspec_getfd(lsp) ;
	            if (fd >= 0)
	                poller_cancelfd(&wip->pm,fd) ;

		    listenspec_finish(lsp) ;
		    vecobj_del(llp,i--) ;

		} /* end if */

	    } /* end if (marked for deletion) */
	} /* end for */

#if	CF_DEBUG
	if (DEBUGLEVEL(5))
	    debugprintf("progwatch/procwatchpoll: checking registers\n") ;
#endif

	for (i = 0 ; (rs >= 0) && (vecobj_get(llp,i,&lsp) >= 0) ; i += 1) {
	    if (lsp == NULL) continue ;

	    rs1 = listenspec_info(lsp,&li) ;

#if	CF_DEBUG
	    if (DEBUGLEVEL(5)) {
	        fd = listenspec_getfd(lsp) ;
	        debugprintf("progwatch/procwatchpoll: i=%u t=%s lfd=%d a=%d\n",
	            i,li.type,fd,rs1) ;
	    }
#endif /* CF_DEBUG */

	    f_active = (li.state & LISTENSPEC_MACTIVE) ;
	    f_broken = (li.state & LISTENSPEC_MBROKEN) ;
	    if ((rs1 >= 0) && (! f_active) && (! f_broken)) {

#if	CF_DEBUG
	    if (DEBUGLEVEL(5))
	        debugprintf("progwatch/procwatchpoll: not_active\n") ;
#endif

		opts = 0 ;
		if (pip->f.reuseaddr) opts |= LISTENSPEC_MREUSE ;

	        rs1 = listenspec_active(lsp,opts,TRUE) ;
		ac = rs1 ;

#if	CF_DEBUG
	        if (DEBUGLEVEL(5)) {
	            debugprintf("progwatch/procwatchpoll: "
		    "listenspec_active() rs=%d\n",rs1) ;
		}
#endif

	        if (rs1 >= 0) {

	            fd = listenspec_getfd(lsp) ;
	            ps.fd = fd ;
	            ps.events = events ;
	            rs = poller_reg(&wip->pm,&ps) ;

#if	CF_DEBUG
	            if (DEBUGLEVEL(5)) {
	                debugprintf("progwatch/procwatchpoll: lfd=%d\n",fd) ;
	                debugprintf("progwatch/procwatchpoll: "
				"poller_reg() rs=%d \n",rs) ;
	            }
#endif

	        } /* end if (successful activation) */

		if (rs >= 0) {
		    if ((rs = listenspec_info(lsp,&li)) >= 0) {
		        rs = procwatchmaintout(pip,&li,1,ac) ;
			f_logged = f_logged || (rs > 0) ;
		    }
		}

	    } /* end if (need activation) */

	} /* end for */

	return (rs >= 0) ? f_logged : rs ;
}
/* end subroutine (procwatchmaint) */


static int procwatchpolling(PROGINFO *pip,SUBINFO *wip,int fd,int re)
{
	LISTENSPEC	*lsp ;
	POLLER_SPEC	ps ;
	vecobj		*llp = &pip->listens ;
	int		rs = SR_OK ;
	int		rs1 ;
	int		i ;
	int		lfd ;
	int		salen ;
	int		to = 5 ;
	int		ns = -1 ;
	char		timebuf[TIMEBUFLEN + 1] ;

#if	CF_DEBUG
	if (DEBUGLEVEL(5))
	debugprintf("progwatch/procwatchpolling: ent fd=%u re=%08b\n",
		fd,re) ;
#endif

	for (i = 0 ; vecobj_get(llp,i,&lsp) >= 0 ; i += 1) {
	    if (lsp == NULL) continue ;

	    lfd = listenspec_getfd(lsp) ;
	    if (lfd == fd) {

#if	CF_DEBUG
	if (DEBUGLEVEL(5))
		debugprintf("progwatch/procwatchpolling: got fd=%u\n",fd) ;
#endif

	        if ((re & POLLIN) || (re & POLLPRI)) {
	            struct clientinfo	ci, *cip = &ci ;

	            if ((rs = clientinfo_start(&ci)) >= 0) {

	                salen = sizeof(SOCKADDRESS) ;
	                rs1 = listenspec_accept(lsp,&cip->sa,&salen,to) ;
	                ns = rs1 ;

#if	CF_DEBUG
	if (DEBUGLEVEL(5))
	debugprintf("progwatch/procwatchpolling: "
		"listenspec_accept() rs=%d\n", rs1) ;
#endif

	                if (rs1 >= 0) {
	                    cip->salen = salen ;
	                    cip->fd_input = u_dup(ns) ;
	                    cip->fd_output = u_dup(ns) ;

			    if (salen == 0) {
	                        salen = sizeof(SOCKADDRESS) ;
	                        rs1 = u_getpeername(ns,&cip->sa,&salen) ;
	                        if (rs1 >= 0)
	                            cip->salen = salen ;
			    }

	                    u_close(ns) ;

			    if (rs >= 0) {

	                        rs = procwatchnew(pip,wip,cip) ;

			        if ((rs < 0) && pip->open.logprog)
				    proglog_printf(pip,
				    "%s new-job failure (%d)",
				    timestr_logz(pip->daytime,timebuf),rs) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(5))
			debugprintf("progwatch/procwatchpolling: "
			"procwatchnew() rs=%d\n",rs) ;
#endif

			     } /* end if (new-job) */

	                } /* end if (accepted) */

	                clientinfo_finish(cip) ;
	            } /* end if (clientinfo) */

	        } /* end if (poll-input event) */

	        if (rs >= 0) {
	            ps.fd = fd ;
	            ps.events = (POLLIN | POLLPRI) ;
	            rs = poller_reg(&wip->pm,&ps) ;
	        }

	    } /* end if (matched an FD) */

	    if (rs < 0) break ;
	} /* end for */

#if	CF_DEBUG
	if (DEBUGLEVEL(5))
	debugprintf("progwatch/procwatchpolling: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (procwatchpolling) */


static int procwatchsubcmd(PROGINFO *pip,cchar subcmd[])
{
	int		rs = SR_OK ;

	if (subcmd[0] != '\0') {
	    int	ci ;
	    if ((ci = matstr(subcmds,subcmd,-1)) >= 0) {
	        switch (ci) {
	        case subcmd_clear:
	            rs = procwatchsubcmd_clear(pip) ;
	            break ;
	        } /* end switch */
	    } else
	        rs = SR_INVALID ;
	} else
	    rs = SR_INVALID ;

	return rs ;
}
/* end subroutine (procwatchsubcmd) */


static int procwatchsubcmd_clear(PROGINFO *pip)
{
	LISTENSPEC	*lsp ;
	LISTENSPEC_INFO	li ;
	vecobj		*llp = &pip->listens ;
	int		rs = SR_OK ;
	int		i ;
	int		c = 0 ;

	for (i = 0 ; vecobj_get(llp,i,&lsp) >= 0 ; i += 1) {
	    if (lsp != NULL) {
	        if ((rs = listenspec_info(lsp,&li)) >= 0) {
	            if (li.state & LISTENSPEC_MBROKEN) {
		        c += 1 ;
		        rs = listenspec_clear(lsp) ;
		    }
	        }
	    }
	    if (rs < 0) break ;
	} /* end for */

	return (rs >= 0) ? c : rs ;
}
/* end subroutine (procwatchsubcmd_clear) */


static int procwatchmaintout(pip,lip,f_add,ls)
PROGINFO		*pip ;
LISTENSPEC_INFO		*lip ;
int			f_add ;
int			ls ;
{
	int		rs = SR_OK ;
	int		f_logged = FALSE ;
	const char	*statstr ;
	const char	*broken = "broken" ;
	char		timebuf[TIMEBUFLEN + 1] ;
	char		tmpbuf[LOGIDLEN + 1] ;

#if	CF_DEBUG
	if (DEBUGLEVEL(4)) {
	debugprintf("progwatch/procwatchmaintout: %c l=%s\n",
		((f_add) ? 'A' : 'R'),
		lip->type) ;
	debugprintf("progwatch/procwatchmaintout: %c a=%s\n",
		((f_add) ? 'A' : 'R'),
		lip->addr) ;
	}
#endif /* CF_DEBUG */

	if (f_add) {
	    if (ls >= 0) {
	        statstr = "adding" ;
	    } else {
		int	rs1 ;
		statstr = tmpbuf ;
		rs1 = bufprintf(tmpbuf,LOGIDLEN,"%s(%d)",broken,ls) ;
		if (rs1 < 0) statstr = broken ;
	    }
	} else {
	    statstr = "removing" ;
	}

	if (pip->open.logprog) {
	    f_logged = TRUE ;
	    proglog_printf(pip,"%s %s listener=%s",
		timestr_logz(pip->daytime,timebuf),
		statstr,
		lip->type) ;
	    proglog_printf(pip,"addr=%s",
		lip->addr) ;
	} /* end if (logging) */

	if (pip->debuglevel > 0) {
	    bprintf(pip->efp,"%s: %s %s listener=%s\n",pip->progname,
		timestr_logz(pip->daytime,timebuf),
		statstr,
		lip->type) ;
	    bprintf(pip->efp,"%s: addr=%s\n",
		pip->progname,lip->addr) ;
	}

	return (rs >= 0) ? f_logged : rs ;
}
/* end subroutine (procwatchmaintout) */


