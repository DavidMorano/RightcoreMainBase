/* main */


#include	<sys/types.h>

#include	<vsystem.h>
#include	<bfile.h>
#include	<randomvar.h>

#include	"cq.h"



int main()
{
	bfile	outfile ;

	RANDOMVAR	rv ;

	CQ		cq ;

	void	*ep, *endp ;

	uint	uiw ;

	int	rs, i ;
	int	j, k, n ;


	rs = bopen(&outfile,BFILE_STDOUT,"wct",0666) ;

	if (rs < 0)
	    goto ret0 ;

	rs = randomvar_start(&rv,FALSE,0) ;

	if (rs < 0)
	    goto ret1 ;

	rs = cq_start(&cq) ;

	if (rs < 0)
	    goto ret2 ;

	for (i = 0 ; i < 1000 ; i += 1) {

	    for (j = 0 ; j < 100 ; j += 1) {

	        for (k = 0 ; k < 10 ; k += 1) {

	            randomvar_getuint(&rv,&uiw) ;

	            uiw &= 0x3FF ;

	            rs = uc_malloc(uiw,(void *) &ep) ;

	            if (rs < 0)
	                bprintf(&outfile,"uc_malloc() rs=%d\n",rs) ;

	            rs = cq_ins(&cq,ep) ;

	            if (rs < 0)
	                bprintf(&outfile,"cq_ins() rs=%d\n",rs) ;

	        } /* end for */

	        for (k = 0 ; k < 10 ; k += 1) {

	            rs = cq_rem(&cq,&ep) ;

	            if (rs < 0) {

	                bprintf(&outfile,"cq_rem() rs=%d\n",rs) ;

	            }

	            if (ep == NULL) {

	                bprintf(&outfile,"NULL entry \n") ;

	            }

	            if (ep != NULL)
	                free(ep) ;

	        } /* end for */

	        rs = cq_rem(&cq,&ep) ;

	    } /* end for */

	    u_sbrk(0,&endp) ;

	    bprintf(&outfile,"endp=%08lx\n",endp) ;

	} /* end for */

ret3:
	cq_finish(&cq) ;

ret2:
	randomvar_finish(&rv) ;

ret1:
	bclose(&outfile) ;

ret0:
	return 0 ;
}
/* end subroutine (main) */




