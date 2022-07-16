/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)emd:cmd/emdreset.c	1.2"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <sys/emduser.h>

extern int errno, optind;
extern char *optarg;

main(argc, argv)
int argc;
char *argv[];
{
	register int i;
	int fd;
	struct strioctl strioc;

	if (argc != 2) {
		fprintf(stderr, "Usage:  emdreset dev\n");
		exit(1);
	}
	if ((fd = open(argv[1], O_RDWR)) < 0 ) {
		perror("emdreset: cannot open driver");
		exit(1);
	}
	strioc.ic_cmd = EI_RESET;
	strioc.ic_timout = INFTIM;
	strioc.ic_len = 0;
	strioc.ic_dp = NULL;
	if (ioctl(fd, I_STR, &strioc) < 0) {
		perror("emdreset: EI_RESET ioctl failed");
		exit(1);
	}
	exit(0);
}
