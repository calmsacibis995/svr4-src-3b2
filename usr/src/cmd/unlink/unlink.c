/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unlink:unlink.c	1.4"
main(argc, argv) char *argv[]; {
	if(argc!=2) {
		write(2, "Usage: /usr/sbin/unlink name\n", 29);
		exit(1);
	}
	unlink(argv[1]);
	exit(0);
}
