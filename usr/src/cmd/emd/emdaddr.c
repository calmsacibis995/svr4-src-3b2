/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)emd:cmd/emdaddr.c	1.2"

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
	struct eiseta eiseta;
	struct strioctl strioc;
	char *fromb, *tob;

	if (argc != 2 && argc != 3) {
		fprintf(stderr, "Usage:  emdaddr  device  [address]\n");
		exit(1);
	}

	if ((fd = open(argv[1], O_RDWR)) < 0 ) {
		perror("emdaddr: cannot open driver");
		exit(1);
	}

	if (argc == 2) {
		strioc.ic_cmd = EI_GETA;
		strioc.ic_timout = INFTIM;
		strioc.ic_len = sizeof(struct eiseta);
		strioc.ic_dp = (caddr_t)&eiseta;
		if (ioctl(fd, I_STR, &strioc) < 0) {
			perror("emdaddr: EI_GETA ioctl failed");
			exit(1);
		}
		fromb = eiseta.eis_addr;
		for (i = 0; i < PHYAD_SIZE; i++, fromb++) {
			register int c;

			c = *fromb >> 4;
			if (c > 9)
				c += 'a' - 10;
			else
				c += '0';
			putchar(c);
			c = *fromb & 0x0f;
			if (c > 9)
				c += 'a' - 10;
			else
				c += '0';
			putchar(c);
		}
		putchar('\n');
		exit(0);
	}
	fromb = argv[2];
	tob = eiseta.eis_addr;
	if (strlen(argv[2]) != 2*PHYAD_SIZE) {
		fprintf(stderr, "emdaddr: incorrect address length\n");
		exit(1);
	}
	i= 0;
	while (*fromb) {
		if (i == 0)
			*tob = 0;
		switch (*fromb++) {
		case '0':
			*tob |= 0;
			break;

		case '1':
			*tob |= 1;
			break;

		case '2':
			*tob |= 2;
			break;

		case '3':
			*tob |= 3;
			break;

		case '4':
			*tob |= 4;
			break;

		case '5':
			*tob |= 5;
			break;

		case '6':
			*tob |= 6;
			break;

		case '7':
			*tob |= 7;
			break;

		case '8':
			*tob |= 8;
			break;

		case '9':
			*tob |= 9;
			break;

		case 'a':
		case 'A':
			*tob |= 10;
			break;

		case 'b':
		case 'B':
			*tob |= 11;
			break;

		case 'c':
		case 'C':
			*tob |= 12;
			break;

		case 'd':
		case 'D':
			*tob |= 13;
			break;

		case 'e':
		case 'E':
			*tob |= 14;
			break;

		case 'f':
		case 'F':
			*tob |= 15;
			break;

		default:
			fprintf(stderr, "emdaddr: invalid character in address\n");
			exit(1);
		}
		if (i == 0) {
			*tob <<= 4;
			i = 1;
		} else {
			tob++;
			i = 0;
		}
	}
	strioc.ic_cmd = EI_SETA;
	strioc.ic_timout = INFTIM;
	strioc.ic_len = sizeof(struct eiseta);
	strioc.ic_dp = (caddr_t)&eiseta;
	if (ioctl(fd, I_STR, &strioc) < 0) {
		perror("emdaddr: EI_SETA ioctl failed");
		exit(1);
	}
	exit(0);
}
