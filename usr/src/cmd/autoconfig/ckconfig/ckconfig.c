/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:ckconfig/ckconfig.c	1.2"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utmp.h>

/*
 * ckconfig - check to see whether the system should be reconfigured
 *
 */

main(argc,argv)
char **argv;

{

	struct stat un,es,ed;
	time_t base_time=0;
	struct utmp *utmp;
	int fd;

	

	if (argc != 4){
		fprintf(stderr,"WARNING: argument count wrong in %s\n",argv[0]);
		fprintf(stderr,"Usage: %s unix system edt_data\n",argv[0]);
		exit(1);
	}

	if (stat(argv[1],&un) == 0)
		base_time = un.st_mtime;
	else {
		while((utmp = getutent()) != NULL)
			if (utmp->ut_type == BOOT_TIME)
				base_time = utmp->ut_time;
		endutent();
	}

	if (base_time == 0){
		fprintf(stderr,"WARNING: unable to establish whether a new unix should be built\n");
		exit(1);
	}

	if (stat(argv[2],&es) == -1){
		fprintf(stderr,"WARNING: No file %s, can't re-configure\n",argv[2]);
		exit(1);
	}

	if (stat(argv[3],&ed) == -1){
		fprintf(stderr,"WARNING: No file %s, can't re-configure\n",argv[3]);
		exit(1);
	}

	
	if ((es.st_mtime > base_time) && (base_time >= ed.st_mtime)){
			fd = creat("/etc/.sysfile",0444);
			write(fd,argv[2],strlen(argv[2]));
			close(fd);
			exit(0);
	}
	exit(1);
}
