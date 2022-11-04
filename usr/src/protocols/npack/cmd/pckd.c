/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)npack:cmd/pckd.c	1.2"
#include "sys/stropts.h"
#include "stdio.h"
#include "fcntl.h"

main(argc, argv)
int argc;
char *argv[];
{
	int muxfd1, muxfd2;

	if (argc != 2) {
		fprintf(stderr, "pckd: USAGE:  pckd  emd_device\n");
		exit(1);
	}
	if ((muxfd1 = open(argv[1], O_RDWR)) == -1) {
		perror("pckd: open emd failed");
		exit(2);
	}
	if ((muxfd2 = open("/dev/npack", O_RDWR)) == -1) {
		perror("pckd: open npack failed");
		exit(2);
	}
	if (ioctl(muxfd2, I_LINK, muxfd1) == -1) {
		perror("pckd: I_LINK ioctl failed");
		exit(2);
	}
	switch  (fork()) {
	case 0:
		break;
	case -1:
		perror("pckd: fork");
		exit(2);
	default:
		exit(0);
	}
	setpgrp();
	fclose(stdin);
	fclose(stderr);
	fclose(stdout);
	pause();
}
