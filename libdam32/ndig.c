/* ndig */

/* digit determination and handling */


/* revision history:

	= 1998-11-01, David A�D� Morano
	This subroutine was written for Rightcore Network Services.

*/

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

	These subroutines provide some special digit (decimal) processing
	for special floating point uses.


*******************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>


/* local defines */


/* forward references */

static int	digs(double) ;


/* external subroutines */


int ndig(double *la,int n)
{
	int		i ;
	int		t ;
	int		m = 0 ;

	for (i = 0 ; i < n ; i += 1) {
	    if ((t = digs(la[i])) > m) m = t ;
	} /* end for */

	return m ;
}
/* end subroutine (ndig) */


int ndigmax(double *la,int n,int m)
{
	double		c = 1.0 ;
	int		i ;

	for (i = 0 ; i < m ; i += 1) {
	    c = c * 10.0 ;
	}

	c = c - 0.1 ;
	for (i = 0 ; i < n ; i += 1) {
	    if (la[i] > c) la[i] = c ;
	} /* end for */

	return m ;
}
/* end subroutine (ndigmax) */


/* local subroutines */


static int digs(double f)
{
	int		i ;

	for (i = 0 ; (f > 1.0) ; i += 1) {
	    f = f / 10.0 ;
	}

	return i ;
}
/* end subroutine (digs) */


