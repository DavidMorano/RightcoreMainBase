RNEWSPOST


This file is README, part of the NNTP CLIENT distribution. 
The contents of this file are copyright, 1992, Stan Barber.

Permission is hereby granted to copy, reproduce, redistribute or otherwise
use the contents of this file as long as: there is no monetary profit gained
specifically from the use or reproduction of the contents of this file, it is
not sold, rented, traded or otherwise marketed, and this copyright notice is
included prominently in any copy made. 
The author make no claims as to the fitness or correctness of the content of 
this file for any use whatsoever, and it is provided as is. Any use of the
contents of this file is at the user's own risk. 

This distribution contains all the parts necessary to build a replacement
for the inews that inject locally created news articles into the news system
for processing and distribution. It is important to note that this software
is only to be installed on CLIENT systems. There is a different software
distribution that is install on SERVER systems. Hopefully, this will make
it much easier for news administrators and systems managers in getting NNTP
up and running without having to read and deciper the cryptic details of
how NNTP and netnews work.

Once you have unpacked the distribution, you run the Configure program to
set up the Makefile and config.h files. Then you run a make to create
the inews program. To install it, type "make install" while running as
the super user.

If you want to use this kit with older NNTP distibutions or older news
reader software that expects the distribution layout of NNTP version 1.5,
make a directory called common and copy all the softeware from this 
distibution to that directory. If you already have NNTP 1.5 somewhere, you
can just copy all this software into the common directory that is there.
Change directories to where you just copied all the software and type
"make compat" and this will set up the needed evironment for backwards
compatibility.

This software has been tested on the following platforms:

Sun 4 running SunOS 4.1.1 and 4.1.2.
Solbourne running OS/MP 4.1A.2.
Vax 3500 running Ultrix 3.1
386 running Interactive System V release 3.2 (with TCP 1.2).
ATT System V release 3 with Wollongong TCP.
HP 9000/823 with HP-UX A.08.00 E.
Silicon Graphics IRIS 4D with IRIX 4.0.1.
Stardent 3000

Bugs, enhancements and other comments should be directed to "nntp@tmc.edu"
or the newsgroup "news.software.nntp"

STAN BARBER


