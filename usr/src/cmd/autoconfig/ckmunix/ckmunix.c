/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:ckmunix/ckmunix.c	1.6"

#include <stdio.h>
#include "sys/param.h"
#include "sys/types.h"
#include <signal.h>
#include <termio.h>
#include <utmp.h>
#include "sys/psw.h"
#include "sys/boot.h"
#include "sys/sbd.h"
#include "sys/firmware.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/uadmin.h"
#include "sys/nvram.h"
#include <sys/sys3b.h>

main()
{
	struct termio ta;
	struct nvparams nvram;
	int fd,pid;
	struct utmp utmp;
	char bootname[20];

	ioctl(0,TCGETA,&ta); 
	ta.c_lflag &= ~ISIG;
	ioctl(0,TCSETAW,&ta); 
	nvram.addr = (char *)(UNX_NVR->bootname);
	nvram.data = (char *)bootname;
	nvram.cnt = sizeof(bootname);
	sys3b(RNVR,&nvram); 
	
#ifndef DEBUG

	if (bootname[0] == '\0')
		exit(0);

	fd = creat("/etc/.sysfile",0444);

	if (bootname[0] == '/')
		write(fd,bootname,strlen(bootname));
	else {
		write(fd,"/stand/",7);
		write(fd,bootname,strlen(bootname));
	}

	close(fd);

	if ((pid = fork()) == 0){
		if (execl("/sbin/sh","/sbin/sh","/sbin/buildsys","-s",(char *)0) == -1){
                               printf("exec failed, machine will go to firmware mode\n");
                                shutdown(2);
 
		}

	} 
        if (pid == -1){
        	printf("fork failed, machine will go to firmware mode\n");
        	shutdown(2);
        }
        wait(0);
 

#endif
	printf("auto-configuration failed, consult a System Administrator\n");
	printf("executing /sbin/sh, to return to firmware mode exit this shell\n");
	signal(SIGINT,SIG_IGN);
	if ((pid=fork()) == 0){
		ta.c_lflag |= ISIG;
		ioctl(0,TCSETAW,&ta); 
		signal(SIGINT,SIG_DFL);
		if (execl("/sbin/sh","/sbin/sh",(char *)0) == -1){
			printf("exec failed, machine will go to firmware mode\n");
			shutdown(2);
		}
	}
	if (pid == -1){
		printf("fork failed, machine will go to firmware mode\n");
		shutdown(2);
	}
	wait(0);
	shutdown(2);
}

shutdown(level)
int level;
{
#ifndef DEBUG
	int fcn;


	system("umountall");
	sync();
	sync();
	sync();
	sync();
	switch (level){
		case 0:
			fcn = AD_HALT;
			break;
		case 1:
			fcn = AD_BOOT;
			break;
		case 2:
			fcn = AD_IBOOT;
			break;
	}
	uadmin(A_SHUTDOWN,fcn,0);
#endif
}
	
