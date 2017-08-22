/* ecmsg */

/* email-cover subchannel message */


/* revision history:

	= 1999-06-13, David A�D� Morano
	This subroutine was written for Rightcore Network Services (RNS).

*/

/* Copyright � 1999 David A�D� Morano.  All rights reserved. */

/*******************************************************************************

        This little dittie provides a little way to help manage the subchannel
        message for the email-cover algorithm.


*******************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<string.h>

#include	<vsystem.h>
#include	<localmisc.h>

#include	"ecmsg.h"


/* local defines */


/* exported subroutines */


int ecmsg_start(ECMSG *mp)
{

	if (mp == NULL) return SR_FAULT ;

	memset(mp,0,sizeof(ECMSG)) ;

	return SR_OK ;
}
/* end subroutine (ecmsg_start) */


int ecmsg_finish(ECMSG *mp)
{
	int		rs = SR_OK ;
	int		rs1 ;

	if (mp == NULL) return SR_FAULT ;

	if (mp->buf != NULL) {
	    rs1 = uc_free(mp->buf) ;
	    if (rs >= 0) rs = rs1 ;
	}

	memset(mp,0,sizeof(ECMSG)) ;

	return rs ;
}
/* end subroutine (ecmsg_finish) */


int ecmsg_loadbuf(ECMSG *mp,cchar *mbuf,int mlen)
{
	int		rs = SR_OK ;
	int		bl ;

	if (mp == NULL) return SR_FAULT ;
	if (mbuf == NULL) return SR_FAULT ;

	if (mlen < 0) mlen = strlen(mbuf) ;
	if (mlen > ECMSG_MAXBUFLEN) mlen = ECMSG_MAXBUFLEN ;

	if (mp->buf != NULL) {
	    uc_free(mp->buf) ;
	    mp->buf = NULL ;
	    mp->buflen = 0 ;
	}

	if (mlen >= 0) {
	    char	*bp ;
	    if ((rs = uc_malloc((mlen+1),&bp)) >= 0) {
		mp->buf = bp ;
	        memcpy(bp,mbuf,mlen) ;
	        mp->buf[mlen] = '\0' ;
	        mp->buflen = mlen ;
	    }
	}

	return rs ;
}
/* end subroutine (ecmsg_loadbuf) */


int ecmsg_already(ECMSG *mp)
{
	if (mp == NULL) return SR_FAULT ;
	return (mp->buf != NULL) ;
}
/* end subroutine (ecmsg_already) */


