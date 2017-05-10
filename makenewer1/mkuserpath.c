/* mkuserpath */

/* make a user-path */


#define	CF_DEBUGS	0		/* compile-time debug print-outs */
#define	CF_UGETPW	1		/* use |ugetpw(3uc)| */


/* revision history:

	= 1998-07-10, David A�D� Morano
	This subroutine was originally written.

*/

/* Copyright � 1998 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

	This subroutine creates a resolved filename path from the coded form.

	Synopsis:

	int mkuserpath(rbuf,pp,pl) ;
	char		rbuf[] ;
	const char	*pp ;
	int		pl ;

	Arguments:

	rbuf		result buffer (should be MAXPATHLEN+1 long)
	pp		source path pointer
	pl		source path length

	Returns:

	<0		error
	==0		no expansion
	>0		expansion


*******************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>
#include	<getbufsize.h>
#include	<getax.h>
#include	<ugetpw.h>
#include	<getxusername.h>
#include	<localmisc.h>


/* local defines */

#if	CF_UGETPW
#define	GETPW_NAME	ugetpw_name
#else
#define	GETPW_NAME	getpw_name
#endif /* CF_UGETPW */


/* external subroutines */

extern int	snwcpy(char *,int,const char *,int) ;
extern int	mkpath1(char *,const char *) ;
extern int	mkpath2(char *,const char *,const char *) ;
extern int	mkpath2w(char *,const char *,const char *,int) ;
extern int	strwcmp(const char *,const char *,int) ;

#if	CF_DEBUGS
extern int	debugprintf(const char *,...) ;
extern int	strlinelen(const char *,int,int) ;
#endif

extern char	*strwcpy(char *,const char *,int) ;
extern char	*strnchr(const char *,int,int) ;


/* local structures */


/* forward references */

static int	mkpathsquiggle(char *,cchar *,cchar *,int) ;
static int	mkpathuserfs(char *,cchar *,int) ;
static int	mkpathusername(char *,cchar *,int,cchar *,int) ;


/* local variables */


/* exported subroutines */


int mkuserpath(char *rbuf,cchar *un,cchar *pp,int pl)
{
	int		rs = SR_OK ;

#if	CF_DEBUGS
	debugprintf("mkuserpath: ent\n") ;
#endif

	if (rbuf == NULL) return SR_FAULT ;

	rbuf[0] = '\0' ;
	if (pp == NULL) return SR_FAULT ;

	if (pl < 0) pl = strlen(pp) ;

	if (pl > 0) {
	    if (pp[0] == '~') {
	        pp += 1 ;
	        pl -= 1 ;
	        rs = mkpathsquiggle(rbuf,un,pp,pl) ;
	    } else {
	        rs = mkpathuserfs(rbuf,pp,pl) ;
	    }
	} /* end if */

	return rs ;
}
/* end subroutine (mkuserpath) */


/* local subroutines */


static int mkpathuserfs(char *rbuf,const char *pp,int pl)
{
	int		rs = SR_OK ;

#if	CF_DEBUGS
	debugprintf("mkpathuserfs: p=>%t<\n",pp,strlinelen(pp,pl,60)) ;
#endif

	if (pl < 0) pl = strlen(pp) ;
	while ((pl > 0) && (pp[0] == '/')) {
	    pp += 1 ;
	    pl -= 1 ;
	}

#if	CF_DEBUGS
	debugprintf("mkpathuserfs: p=>%t<\n",pp,strlinelen(pp,pl,60)) ;
#endif

	if ((pl >= 2) && (strncmp("u/",pp,2) == 0)) {

	    pp += 2 ;
	    pl -= 2 ;
	    if (pl > 0) {

/* get the username */

	        while (pl && (pp[0] == '/')) {
	            pp += 1 ;
	            pl -= 1 ;
	        }

#if	CF_DEBUGS
	        debugprintf("mkpathuserfs: p=>%t<\n",
	            pp,strlinelen(pp,pl,60)) ;
#endif

	        if (pl > 0) {
	            const char	*tp ;
	            const char	*up = pp ;
	            int		ul = pl ;
	            if ((tp = strnchr(pp,pl,'/')) != NULL) {
	                ul = (tp - pp) ;
	                pl -= ((tp+1)-pp) ;
	                pp = (tp+1) ;
	            } else {
	                pp += pl ;
	                pl = 0 ;
	            }

#if	CF_DEBUGS
	            debugprintf("mkpathuserfs: pop rs=%d u=%t s=%s\n",
			rs,up,ul,pp) ;
#endif
	            rs = mkpathusername(rbuf,up,ul,pp,pl) ;
	        } /* end if (positive) */

	    } /* end if (positive) */

	} /* end if (user-fs called for) */

#if	CF_DEBUGS
	debugprintf("mkpathuserfs: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (mkpathuserfs) */


static int mkpathsquiggle(char *rbuf,const char *un,const char *pp,int pl)
{
	int		rs ;
	int		ul = pl ;
	const char	*tp ;
	const char	*up = pp ;

	if (pl < 0) pl = strlen(pp) ;

	if ((tp = strnchr(pp,pl,'/')) != NULL) {
	    ul = (tp-pp) ;
	    pl -= ((tp+1)-pp) ;
	    pp = (tp+1) ;
	} else {
	    pp += pl ;
	    pl = 0 ;
	}

	if ((ul == 0) && (un != NULL)) {
	    up = un ;
	    ul = -1 ;
	}

	rs = mkpathusername(rbuf,up,ul,pp,pl) ;

	return rs ;
}
/* end subroutine (mkpathsqiggle) */


static int mkpathusername(char *rbuf,cchar *up,int ul,cchar *sp,int sl)
{
	int		rs = SR_OK ;
	const char	*un = up ;
	char		ubuf[USERNAMELEN+1] ;

#if	CF_DEBUGS
	debugprintf("mkpathusername: ul=%d u=%t s=%t\n",ul,up,ul,sp,sl) ;
#endif

	if (ul >= 0) {
	    rs = strwcpy(ubuf,up,MIN(ul,USERNAMELEN)) - ubuf ;
	    un = ubuf ;
	}

#if	CF_DEBUGS
	debugprintf("mkpathusername: mid rs=%d u=%s s=%t\n",rs,un,sp,sl) ;
#endif

	if (rs >= 0) {
	    struct passwd	pw ;
	    const int		pwlen = getbufsize(getbufsize_pw) ;
	    char		*pwbuf ;
	    if ((rs = uc_malloc((pwlen+1),&pwbuf)) >= 0) {
	        if ((un[0] == '\0') || (un[0] == '-')) {
	            rs = getpwusername(&pw,pwbuf,pwlen,-1) ;
	        } else {
	            rs = GETPW_NAME(&pw,pwbuf,pwlen,un) ;
	        }
	        if (rs >= 0) {
	            if (sl > 0) {
	                rs = mkpath2w(rbuf,pw.pw_dir,sp,sl) ;
	            } else {
	                rs = mkpath1(rbuf,pw.pw_dir) ;
	            }
	        }
	        uc_free(pwbuf) ;
	    } /* end if (memory-allocation) */
	} /* end if (ok) */

#if	CF_DEBUGS
	debugprintf("mkpathusername: ret rs=%d\n",rs) ;
#endif

	return rs ;
}
/* end subroutine (mkpathusername) */


