/* hasStrContent */

/* determine if the given string is empty */
/* last modified %G% version %I% */


#define	CF_DEBUGS	0		/* non-switchable debug print-outs */


/* revision history:

	= 1998-07-01, David A�D� Morano
	This subroutine was originally written. This is sort of experiemental
	(so far). We are struggling a little bit over its name.

*/

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

        This subroutine determines if a given string is empty or not.

	Synopsis:

	int hasStrContent(cchar *sp,int sl)

	Arguments:

	sp		string pointer
	sl		string length

	Returns:

	1		TRUE (empty)
	0		FALSE (not empty)


*******************************************************************************/


#include	<envstandards.h>
#include	<sys/types.h>
#include	<localmisc.h>


/* local defines */


/* external subroutines */

extern int	hasallwhite(cchar *,int) ;


/* external variables */


/* local structures */


/* forward references */


/* local variables */


/* exported subroutines */


int hasStrContent(cchar *sp,int sl)
{
	int		f = FALSE ;
	if (sp != NULL) {
	    f = (! hasallwhite(sp,sl)) ;
	}
	return f ;
}
/* end subroutine (hasStrContent) */


int hasNotEmptry(cchar *sp,int sl)
{
	return hasStrContent(sp,sl) ;
}
/* end subroutine (hasNotEmpty) */


int hascontent(cchar *sp,int sl)
{
	return hasStrContent(sp,sl) ;
}
/* end subroutine (hascontent) */


