/* sha */


#ifndef SHA1_INCLUDE
#define SHA1_INCLUDE	1


/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach <uh@alumni.caltech edu> */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */

/* This code is in the public domain */


#ifndef ENDIAN_H
#define ENDIAN_H

/* Warning! this file is automatically generated; changes WILL disappear */

#define SHA1_BYTE_ORDER 4321

#define SHA1_VERSION 1

#endif /* ENDIAN_H */


/* Useful defines & typedefs */

#define	SHA1			SHA1_INFO


typedef unsigned char SHA1_BYTE;	/* 8-bit quantity */
typedef unsigned long SHA1_LONG;	/* 32-or-more-bit quantity */

#define SHA1_BLOCKSIZE		64
#define SHA1_DIGESTSIZE		20

typedef struct {
    SHA1_LONG digest[5];		/* message digest */
    SHA1_LONG count_lo, count_hi;	/* 64-bit bit count */
    SHA1_BYTE data[SHA1_BLOCKSIZE];	/* SHA1 data buffer */
    int local;				/* unprocessed amount in data */
} SHA1_INFO ;



#ifdef	__cplusplus
extern "C" {
#endif

extern int sha1_start(SHA1 *) ;
extern int sha1_update(SHA1 *,char *, int) ;
extern int sha1_digest(SHA1 *,unsigned char [20]) ;
extern int sha1_finish(SHA1 *) ;

#ifdef	__cplusplus
}
#endif



#ifdef SHA1_FOR_C

#include <stdlib.h>
#include <stdio.h>

void sha_stream(unsigned char [20], SHA1_INFO *, FILE *);
void sha_print(unsigned char [20]);
char *sha_version(void);
#endif /* SHA1_FOR_C */


#endif /* SHA1_INCLUDE */



