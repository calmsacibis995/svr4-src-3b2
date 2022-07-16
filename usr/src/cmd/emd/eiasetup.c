/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)emd:cmd/eiasetup.c	1.3"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/firmware.h>

char *DEFADDR = "800010";
char *MEM = "/dev/kmem";

main()
{
	int fd;
	struct serno s;
	struct vectors v;
	int ret;

	if ((fd = open(MEM, O_RDONLY)) < 0) {
		perror("eiasetup: open failed");
		exit(1);
	}
	if (lseek(fd, VBASE, 0) < 0) {
		perror("eiasetup: first lseek failed");
		exit(1);
	}
	ret = read(fd, &v, sizeof(struct vectors));
	if (ret != sizeof(struct vectors)) {
		if (ret < 0)
			perror("eiasetup: first read failed");
		exit(1);
	}
	if (lseek(fd, (int)v.p_serno+(int)VROM, 0) < 0) {
		perror("eiasetup: second lseek failed");
		exit(1);
	}
	ret = read(fd, &s, sizeof(struct serno));
	if (ret != sizeof(struct serno)) {
		if (ret < 0)
			perror("eiasetup: second read failed");
		exit(1);
	}
	printf("%s%2.2x%2.2x%2.2x\n", DEFADDR, s.serial3, s.serial2, s.serial1);
	exit(0);
}
