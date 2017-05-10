/* encode */

/* encode the message(s) */
/* last modified %G% version %I% */


#define	CF_DEBUG	0		/* run-time debug print-outs */


/* revision history:

	= 1999-05-06, David A�D� Morano

	This program was originally written.


*/


/**************************************************************************

	This subroutine encodes the input so that it is better
	suited for encryption later.

	The format of the output "covered" file is:

	0) encoded block of random data
	1) encoded first block of the real data masked with the mask block
	2) encoded subsequent blocks of real data
	3) encoded possible blocks of the "extra message"
	4) encoded final block of real data (including any "extra message")
	5) encoded mask block
	6) the key (in clear text)
	7) random data (also in clear text)

	The metadata on the real data and a possible extra message is
	at the end of the encoded block 3 above.


***************************************************************************/


#include	<envstandards.h>	/* MUST be first to configure */

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<signal.h>
#include	<stdlib.h>
#include	<string.h>

#include	<vsystem.h>
#include	<bfile.h>
#include	<randomvar.h>
#include	<cksum.h>

#include	"localmisc.h"
#include	"ecmsg.h"
#include	"config.h"
#include	"defs.h"



/* local defines */

#ifndef	BLOCKLEN
#define	BLOCKLEN	(NOPWORDS * sizeof(ULONG))
#endif



/* external subroutines */

extern void	munge(struct proginfo *,int,
			ULONG *,ULONG *,ULONG *) ;


/* external variables */


/* local structures */


/* forward references */

static ULONG	packlong(uint,uint) ;

static void	blockfinish(struct proginfo *,
			ULONG *,int,int,struct ecmsgdesc *,
			RANDOMVAR *) ;


/* local varibles */







int encode(pip,rvp,fip,emp,ifd,ofd)
struct proginfo	*pip ;
RANDOMVAR	*rvp ;
struct fileinfo	*fip ;
ECMSG		*emp ;
int		ifd, ofd ;
{
	struct ecmsgdesc	md ;

	struct ustat	stat_i ;

	ULONG	key[NOPWORDS] ;
	ULONG	vector[NOPWORDS] ;
	ULONG	data[NOPWORDS] ;
	ULONG	mask[NOPWORDS] ;
	ULONG	out[NOPWORDS] ;

	CKSUM	filesum, msgsum ;

	time_t	filetime ;
	time_t	msgtime ;

	uint	val ;

	int	rs ;
	int	i, j, len ;
	int	filelen ;
	int	ndatablocks = 0 ;
	int	n = NOPWORDS ;
	int	f_block  ;


#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode: entered\n") ;
#endif

/* initialization */

	for (i = 0 ; i < NOPWORDS ; i += 1)
	    randomvar_getulong(rvp,(key + i)) ;

	for (i = 0 ; i < NOPWORDS ; i += 1)
	    randomvar_getulong(rvp,(data + i)) ;

	for (i = 0 ; i < NOPWORDS ; i += 1)
	    randomvar_getulong(rvp,(mask + i)) ;

/* load the vector for starters */

	for (i = 0 ; i < NOPWORDS ; i += 1)
	    vector[i] = key[i] ;

/* get the input file modification time */

#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode: modification times\n") ;
#endif

	filetime = 0 ;
	rs = u_fstat(ifd,&stat_i) ;
	if (rs < 0)
	    goto ret0 ;

	    filetime = stat_i.st_mtime ;

	rs = cksum_start(&filesum) ;
	if (rs < 0)
	    goto ret0 ;

	fip->mtime = filetime ;
	fip->len = 0 ;
	fip->cksum = 0 ;

/* get some information on the optional message */

#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode: message starters\n") ;
#endif

	msgtime = time(NULL) ;

	md.mp = emp->buf ;
	md.rlen = emp->buflen ;

#if	CF_DEBUG
	if (DEBUGLEVEL(5)) {
	    debugprintf("encode: buf(%p) buflen=%d\n", emp->buf,emp->buflen) ;
		if (emp->buf != NULL)
	    debugprintf("encode: extra=>%t<\n", 
		emp->buf,strnlen(emp->buf,emp->buflen)) ;
	}
#endif

	rs = cksum_start(&msgsum) ;
	if (rs < 0)
	    goto ret1 ;

/* checksum the extra message */

	cksum_accum(&msgsum,emp->buf,emp->buflen) ;

#if	CF_DEBUG
	if (DEBUGLEVEL(5))
	    debugprintf("encode: msglen=%u \n", emp->buflen) ;
#endif

/* make the first output block */

#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode: processing leader\n") ;
#endif /* CF_DEBUG */

	munge(pip,n,vector,data,out) ;

	rs = u_writen(ofd,out,BLOCKLEN) ;

/* process the input file */

#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode: processing file\n") ;
#endif

/* handle the first block specially */

	filelen = 0 ;
	f_block = FALSE ;
	rs = u_read(ifd,data,BLOCKLEN) ;

	len = rs ;
	if (rs < 0)
	    goto badread ;

#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode: read len=%d\n",rs) ;
#endif

	filelen += len ;

/* fill in this block as necessary */

	if ((rs >= 0) && ((md.rlen + len) > 0)) {

	    ndatablocks += 1 ;
	    cksum_accum(&filesum,data,len) ;

	    if (len < BLOCKLEN)
	        blockfinish(pip,data,n,len,&md,rvp) ;

/* this is the special part! */

	    for (i = 0 ; i < (n - 4) ; i += 1)
	        data[i] = data[i] ^ mask[i] ;

	    munge(pip,n,vector,data,out) ;

	    rs = u_writen(ofd,out,BLOCKLEN) ;

	} /* end if */

/* do the rest of the input data */

	if ((rs >= 0) && (len > 0)) {

	    while ((rs = uc_readn(ifd,data,BLOCKLEN)) > 0) {

#if	CF_DEBUG
	        if (DEBUGLEVEL(2))
	            debugprintf("encode: file reading blocks, len=%d\n",rs) ;
#endif

	        len = rs ;
	        filelen += len ;
	        ndatablocks += 1 ;
	        cksum_accum(&filesum,data,len) ;

	        if (len < BLOCKLEN)
	            blockfinish(pip,data,n,len,&md,rvp) ;

	        munge(pip,n,vector,data,out) ;

	        rs = u_writen(ofd,out,BLOCKLEN) ;

		if (rs < 0)
		    break ;

	    } /* end while */

	} /* end if (there was more input data to read) */

/* now fill in blocks with the rest of the extra message */

	while ((rs >= 0) && (md.rlen > 0)) {

	    int	mlen ;


#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	        debugprintf("encode: message reading blocks, len=%d\n",rs) ;
#endif

	    mlen = MIN(md.rlen,BLOCKLEN) ;
	    memcpy(data,md.mp,mlen) ;

	    md.mp += mlen ;
	    md.rlen -= mlen ;

	    ndatablocks += 1 ;
	    if (md.rlen > 0)
	        blockfinish(pip,data,n,mlen,&md,rvp) ;

	    munge(pip,n,vector,data,out) ;

	    rs = u_writen(ofd,out,BLOCKLEN) ;

	} /* end while */

#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode: filelen=%d ndatablocks=%d\n",
	        filelen,ndatablocks) ;
#endif

/* prepare the check information (in network byte order) */

/* file information */

	mask[n - 4] = packlong(htonl(filelen),0) ;

	cksum_getsum(&filesum,&val) ;

	mask[n - 3] = packlong(htonl(val),htonl(filetime)) ;

	fip->len = filelen ;
	fip->cksum = val ;

/* message information */

	mask[n - 2] = packlong(htonl(emp->buflen),0) ;

	cksum_getsum(&msgsum,&val) ;

	mask[n - 1] = packlong(htonl(val),htonl(msgtime)) ;

/* mask these last four words with the first four words of the block */

	j = 0 ;
	for (i = (n - 4) ; i < n ; i += 1)
	    mask[i] = mask[i] ^ mask[j++] ;

	munge(pip,n,vector,mask,out) ;

	rs = u_write(ofd,out,BLOCKLEN) ;

/* write out the key */

#if	CF_DEBUG
	if (pip->debuglevel > 1) {
	    offset_t	offset ;
	    u_tell(ofd,&offset) ;
	    debugprintf("encode: writing key offset=%lld\n",offset) ;
	}
#endif /* CF_DEBUG */

	if (rs >= 0)
	    rs = u_write(ofd,key,BLOCKLEN) ;

/* finally write out some random bytes making over-all output length random */

	randomvar_getint(rvp,&len) ;

	len = len & (BLOCKLEN - 1) ;

	j = (len / sizeof(ULONG)) + 1 ;
	for (i = 0 ; i < j ; i += 1)
	    randomvar_getulong(rvp,(data + i)) ;

	if (rs >= 0)
	    rs = u_write(ofd,data,len) ;

/* done */
done:
badread:
ret2:
	cksum_finish(&msgsum) ;

ret1:
	cksum_finish(&filesum) ;

ret0:

#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode: ret rs=%d\n",rs) ;
#endif

	return (rs >= 0) ? filelen : rs ;
}
/* end subroutine (encode) */



/* LOCAL SUBROUTINES */



static void blockfinish(pip,block,n,len,mdp,rvp)
struct proginfo		*pip ;
ULONG			block[] ;
int			n ;
int			len ;		/* length filled already */
struct ecmsgdesc	*mdp ;
RANDOMVAR		*rvp ;
{
	int	blocklen = n * sizeof(ULONG) ;
	int	rlen, mlen ;
	int	nwords, nchars ;
	int	i, j ;

	uchar	*dp ;


#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode/blockfinish: entered\n") ;
#endif

/* fill in this block as necessary */

	if (((mdp->rlen + len) <= 0) || (len >= blocklen))
	    return ;

	rlen = BLOCKLEN - len ;
	dp = (uchar *) block ;
	dp += len ;

/* fill with the optional message first */

	if (mdp->rlen > 0) {

	    mlen = MIN(mdp->rlen,rlen) ;
	    memcpy(dp,mdp->mp,mlen) ;

	    mdp->mp += mlen ;
	    mdp->rlen -= mlen ;
	    dp += mlen ;
	    rlen -= mlen ;

	} /* end if */

/* fill the rest with random data */

	if (rlen > 0) {

	    ULONG	rv ;

	    int		k ;


	    nwords = rlen / sizeof(ULONG) ;
	    nchars = rlen % sizeof(ULONG) ;
	    i = len / sizeof(ULONG) ;
	    j = sizeof(ULONG) - nchars ;

	    randomvar_getulong(rvp,&rv) ;

	    memcpy(dp,&rv,nchars) ;

	    i += 1 ;
	    for (k = 0 ; k < (NOPWORDS - i) ; k += 1)
	        randomvar_getulong(rvp,(block + i)) ;

	} /* end if */

#if	CF_DEBUG
	if (DEBUGLEVEL(2))
	    debugprintf("encode/blockfinish: ret\n") ;
#endif

}
/* end subroutine (blockfinish) */


static ULONG packlong(h1,h0)
uint	h0, h1 ;
{
	ULONG	r, r0, r1 ;


	r0 = h0 ;
	r1 = h1 ;
	r = r0 | (r1 << 32) ;
	return r ;
}
/* end subroutine (packlong) */



