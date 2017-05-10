/* u_setreuid */

/* translation layer interface for UNIX� equivalents */


#define	CF_DEBUGS	0		/* compile-time debugging */


/* revision history:

	= 1998-11-01, David A�D� Morano
	This subroutine was written for Rightcore Network Services (RNS).

*/

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<poll.h>
#include	<errno.h>

#include	<vsystem.h>
#include	<localmisc.h>


/* exported subroutines */


int u_setreuid(uid_t ruid,uid_t euid)
{
	int		rs ;

	if ((rs = setreuid(ruid,euid)) < 0)
	    rs = (- errno) ;

#if	CF_DEBUGS
	debugprintf("u_setreuid: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (u_setreuid) */


