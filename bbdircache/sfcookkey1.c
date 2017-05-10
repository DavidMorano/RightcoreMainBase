/* sfcookkey */

/* string-find a cookie key */


#define	CF_DEBUGS	0		/* compile-time debug print-outs */


/* revision history:

	= 1998-03-01, David A�D� Morano

	The subroutine was adapted from others programs that did similar
	types of functions.


*/

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine finds a cookie key in a string.  

	Synopsis:

	int sfcookkey(sp,sl,rpp)
	const char	*sp ;
	int		sl ;
	const char	**rpp ;

	Arguments:

	sp	supplied string to test
	sl	length of supplied string to test
	ss	"thing" characters
	rpp	pointer to store result "thing" pointer

	Returns:

	>=0	length of result "thing" 
	<0	error


*******************************************************************************/


#include	<envstandards.h>

#include	<sys/types.h>
#include	<sys/param.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>

#include	<vsystem.h>
#include	<ascii.h>
#include	<localmisc.h>


/* local defines */

#ifndef	SUFBUFLEN
#define	SUFBUFLEN		MAXNAMELEN
#endif

#define	DEBLEN(slen,max)	(((slen) >= 0) ? MIN((slen),(max)) : (max))

#undef	CH_COOK
#define	CH_COOK		'%'


/* external subroutines */

extern int	isalnumlatin(int) ;

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strnpbrk(const char *,int,const char *) ;
extern char	*strnchr(const char *,int,int) ;
extern char	*strnrchr(const char *,int,int) ;
extern char	*strnsub(const char *,int,const char *) ;


/* external variables */


/* local structures */


/* forward references */

static int	getthing(const char *,int,const char *,const char **) ;
static int	hasgood(const char *,int) ;
static int	isour(int) ;


/* local variables */


/* exported subroutines */


int sfcookkey(sp,sl,rpp)
const char	*sp ;
int		sl ;
const char	**rpp ;
{
	const int	sch = CH_COOK ;

	int	cl = -1 ;

	const char	*cp = NULL ;

	if (sl >= 1) {
	    const char	*ss = "{}" ;
	    const char	*tp ;
	    while ((tp = strnchr(sp,sl,sch)) != NULL) {

	        sl -= ((tp+1)-sp) ;
	        sp = (tp+1) ;
		if (sl > 0) {
		    if (sp[0] == sch) {
			sp += 1 ;
			sl -= 1 ;
		    } else {
	                cl = getthing(sp,sl,ss,&cp) ;
	                if (cl >= 0) break ;
		    }
		}

	    } /* end while */
	} /* end if */

	if (rpp != NULL)
	    *rpp = (cl >= 0) ? cp : NULL ;

#if	CF_DEBUGS
	debugprintf("sfcookkey: ret rs=%d\n",cl) ;
#endif

	return cl ;
}
/* end subroutine (sfcookkey) */


/* local subroutines */


static int getthing(sp,sl,ss,rpp)
const char	*sp ;
int		sl ;
const char	ss[] ;
const char	**rpp ;
{
	int	cl = -1 ;
	int	f = FALSE ;

	const char	*cp = NULL ;

	char	buf[3] ;

	if (sl > 0) {
	    const char	*tp ;
	    buf[0] = '$' ;
	    buf[1] = ss[0] ;
	    buf[2] = '\0' ;
	    if ((tp = strnsub(sp,sl,buf)) != NULL) {
	        cl = sl - ((tp + 2) - sp) ;
	        cp = (tp + 2) ;
	        if ((tp = strnchr(cp,cl,ss[1])) != NULL) {
	            cl = (tp - cp) ;
	            f = hasgood(cp,cl) ;
	        }
	    }
	} /* end if */

	if (rpp != NULL)
	    *rpp = (f) ? cp : NULL ;

#if	CF_DEBUGS
	debugprintf("sfcookkey: ret f=%u cl=%d\n",f,cl) ;
#endif

	return (f) ? cl : -1 ;
}
/* end subroutine (getthing) */


static int hasgood(sp,sl)
const char	sp[] ;
int		sl ;
{
	int	i ;
	int	ch ;
	int	f = TRUE ;

#if	CF_DEBUGS
	debugprintf("sfcookkey/hasgood: sl=%d s=>%t<\n",
		sl,sp,strnlen(sp,DEBLEN(sl,40))) ;
#endif

	for (i = 0 ; i < sl ; i += 1) {

	    ch = (sp[i] & 0xff) ;
	    if (ch == '=') break ;

	    f = isour(ch) ;
	    if (! f) break ;

	} /* end for */

	return f ;
}
/* end subroutine (hasgood) */


static int isour(ch)
int	ch ;
{
	int	f = isalnumlatin(ch) ;
	f = f || (ch == '_') || (ch == '-') || (ch == '=') || (ch == '/') ;
	return f ;
}
/* end subroutine (isour) */


