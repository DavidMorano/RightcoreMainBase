/* strcpyuc */


#define	CF_CHAR		1		/* use 'char(3dam)' */


/* revision history:

	= 1998-11-01, David A�D� Morano

	This subroutine was originally written.


*/

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */

/******************************************************************************

	This subroutine converts a string to upper case.


******************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>

#include	<char.h>



/* local defines */

#if	defined(CF_CHAR) && (CF_CHAR == 1)
#define	CF_CHAREXTERN	0
#define	tolc(c)		CHAR_TOLC(c)
#define	touc(c)		CHAR_TOUC(c)
#else
#define	CF_CHAREXTERN	1
#endif



/* external subroutines */

#if	CF_CHAREXTERN
extern int	tolc(int) ;
extern int	touc(int) ;
#endif


/* local variables */


/* exported subroutines */


char *strcpyuc(dst,src)
char		*dst ;
const char	*src ;
{


	while (*src != '\0') {
	    *dst++ = touc(*src) ;
	    src += 1 ;
	} /* end while */

	*dst = '\0' ;
	return dst ;
}
/* end subroutine (strcpyuc) */



