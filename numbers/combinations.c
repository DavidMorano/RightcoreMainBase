/* combinations */

/* n-choose-k function WITHOUT repitition */


#define	CF_DEBUGS	0		/* compile-time debugging */


/* revision history:

	= 1998-09-10, David A�D� Morano
	This was originally written.

*/

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

	We calculate the combinations of the given number.

	Synopsis:

	LONG combinations(int n,int k)

	Arguments:

	n	number of items to choose from
	k	nuber of item to choose without repitition

	Returns:

	-	the Fibonacci number of the input

	= Notes:

	+ k == 0 -> ans = 1
	+ k == 1 -> ans = n 
	+ k == n -> ans = 1
	+ k > n  -> ans = 0		(sort of a special case)


*******************************************************************************/


#include	<envstandards.h>
#include	<limits.h>
#include	<localmisc.h>


/* external subroutines */

#if	CF_DEBUGS
extern int	debugprintf(const char *,...) ;
extern int	strlinelen(const char *,int,int) ;
#endif


/* external subroutines */

extern LONG	factorial(int) ;
extern LONG	permutations(int,int) ;


/* forward references */


/* local variables */


/* exported subroutines */


LONG combinations(int n,int k)
{
	LONG		ans = -1 ;
	if ((n >= 0) && (k >= 0)) {
	    ans = 1 ;
	    if (k == 1) {
		ans = n ;
	    } else if (k == n) {
		ans = 1 ;
	    } else if (k > 0) {
	        const LONG	num = permutations(n,k) ;
	        const LONG	den = factorial(k) ;
		if ((num >= 0) && (den > 0)) {
	            ans = num / den ;
		} else {
		    ans = -1 ;
		}
	    }
	} else if (k > n) {
	    ans = 0 ;
	}
	return ans ;
}
/* end subroutine (combinations) */


LONG multicombinations(int n,int k)
{
	return combinations(n+k-1,k) ;
}
/* end subroutine (multicombinations) */

