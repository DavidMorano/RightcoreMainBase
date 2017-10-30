/* uc_sigwaitinfo */

/* interface component for UNIX� library-3c */


#define	CF_DEBUGS	0		/* compile-time debugging */


/* revision history:

	= 2000-05-14, David A�D� Morano
	Originally written for Rightcore Network Services.

*/

/* Copyright � 2000 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

	This modeule implements both:
		uc_sigwaitinfo
		uc_sigtimedwait


*******************************************************************************/

#include	<envstandards.h>

#include	<sys/types.h>
#include	<signal.h>
#include	<errno.h>

#include	<vsystem.h>
#include	<timespec.h>
#include	<localmisc.h>


/* local defines */

#define	TO_AGAIN	60		/* sixty (60) seconds */


/* external subroutines */

extern int	msleep(int) ;


/* local variables */


/* exported subroutines */


int uc_sigwaitinfo(const sigset_t *ssp,siginfo_t *sip)
{
	int		rs ;
	int		to_again = TO_AGAIN ;
	int		f_exit = FALSE ;

	repeat {
	    if ((rs = sigwaitinfo(ssp,sip)) < 0) rs = (- errno) ;
	    if (rs < 0) {
	        switch (rs) {
	        case SR_AGAIN:
	            if (to_again-- > 0) {
	                msleep(1000) ;
		    } else {
	                f_exit = TRUE ;
		    }
		    break ;
		default:
		    f_exit = TRUE ;
		    break ;
	        } /* end switch */
	    } /* end if (error) */
	} until ((rs >= 0) || f_exit) ;

	return rs ;
}
/* end subroutine (uc_sigwaitinfo) */


int uc_sigtimedwait(const sigset_t *ssp,siginfo_t *sip,const TIMESPEC *tsp)
{
	TIMESPEC	ts ;
	int		rs ;
	int		to_again = TO_AGAIN ;
	int		f_exit = FALSE ;

	if (tsp == NULL) {
	    tsp = &ts ;
	    timespec_init(&ts,0,0) ;
	}

	repeat {
	    if ((rs = sigtimedwait(ssp,sip,tsp)) < 0) rs = (- errno) ;
	    if (rs < 0) {
	        switch (rs) {
	        case SR_AGAIN:
	            if (to_again-- > 0) {
	                msleep(1000) ;
		    } else {
			f_exit = TRUE ;
		    }
		    break ;
		default:
		    f_exit = TRUE ;
		    break ;
	        } /* end switch */
	    } /* end if (error) */
	} until ((rs >= 0) || f_exit) ;

	return rs ;
}
/* end subroutine (uc_sigtimedwait) */


int uc_sigwaitinfoto(const sigset_t *ssp,siginfo_t *sip,const TIMESPEC *tsp)
{
	int		rs ;
	if (tsp == NULL) {
	     rs = uc_sigwaitinfo(ssp,sip) ;
	} else {
	     rs = uc_sigtimedwait(ssp,sip,tsp) ;
	}
	return rs ;
}
/* end subroutine (uc_sigwaitinfoto) */

