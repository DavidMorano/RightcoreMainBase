/* redomsg */


#define		DEBUG	0


/************************************************************************
 *									*
 * The information contained herein is for the use of AT&T Information	*
 * Systems Laboratories and is not for publications.  (See GEI 13.9-3)	*
 *									*
 *	(c) 1984 AT&T Information Systems				*
 *									*
 * Authors of the contents of this file:				*
 *									*
 *		J.Mukerji						*
		David A.D. Morano
 *									*


 * 'redo_message' creates a new temproary file, puts the current state of
 * headers into it, copies the rest of the old temp file into the new one
 * and then unlinks the old tempfile and returns the name of the new tempfile
 * in tempfile


 ************************************************************************/



#include	<stdio.h>
#include	<string.h>

#include	"config.h"
#include	"smail.h"
#include	"header.h"



/* external variables */

extern struct global	g ;



redo_message()
{
	FILE	*ftp, *ftp1 ;

	char	ttempfile[30] ;


	strcpy(ttempfile,"/tmp/smailXXXXXX") ;

	mktemp(ttempfile ) ;

	if ((ftp = fopen( ttempfile, "a" )) == NULL) {

	    printf("%s: can't open ttemp file\n",
	        g.progname) ;

	    rmtemp(0) ;
	}

	if ((ftp1 = fopen( tempfile, "r" )) == NULL) {

	    printf("%s: can't open temp file\n",
	        g.progname) ;

	    rmtemp(0) ;
	}

	chown(ttempfile,g.uid,g.gid_mail) ;

	chmod(ttempfile,0640) ;	/* don't let anyone snoop!! */

	putheader(ftp, 1) ;

	copymessage(ftp,ftp1) ;

	unlink(tempfile) ;

	strcpy(tempfile,ttempfile) ;

	fclose(ftp) ;

	fclose(ftp1) ;

	ambiguous = 0 ;
}
/* end subroutine (redomsg) */


/* copymessage copies the message body from fp2 to fp1 */

copymessage(fp1,fp2)
FILE	*fp1, *fp2 ;
{
	int	l, f_header = TRUE ;

	char	linebuf[LINELEN + 1] ;


#ifdef	COMMENT
	int lheader ;
	int inheader ;


/* NOTE: header_line returns a positive value identifying the	*/
/* type of header line, based on the keyword. If the line is	*/
/* not a header line, then it returns (-1) */

	inheader = TRUE ;
	while (fgetline(fp2,s,BUFSIZE) > 0) {

/* skip existing fields */

	    if (inheader) {

	        if (*s == '\n') {

	            inheader = FALSE ;
	            continue ;
	        }

	        if ((lheader = header_line(s)) >= 0) continue ;

	    } else lheader = header_line(s) ;

	    if ((lheader == CC) || (lheader == BCC)) continue ;

	    fputs(s,fp1) ;

/* 
	I think that the following code is garbage-ola and should be
	removed.  There does not appear to be ANY circumstance
	in this routine where a header should be copied to the
	new message file.  So I comments the junk out !
*/

#ifdef	COMMENT

/* If the line is a header line then add a '\n' 	*/
/* to the output file. This is necessary because	*/
/* header_line chops off the '\n' from the line when	*/
/* it decides that the line is a header line!!!		*/

	    if (lheader >= 0) {

		fputs("\n",fp1) ;

fprintf(stderr,
"added a newline because of header line bug\n") ;

	    }
#endif

	} /* end while */

#else
	while ((l = fgetline(fp2,linebuf,LINELEN)) > 0) {

		if (! f_header) fputs(linebuf,fp1) ;

		if (linebuf[0] == '\n') f_header = FALSE ;

	}
#endif

}
/* end subroutine (copymessage) */



