/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:fsrootboot/fsrootboot.c	1.3"

#include "sys/types.h"
#include <sys/stat.h>
#include <stdio.h>
#include "sys/param.h"
#include <signal.h>
#include <termio.h>
#include "sys/psw.h"
#include "sys/boot.h"
#include "sys/sbd.h"
#include "sys/firmware.h"
#include "sys/fsiboot.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/uadmin.h"
#include "sys/nvram.h"
#include <sys/sys3b.h>


main()
{
	struct stat sb;
	struct termio ta;
	struct nvparams nvram;
	char aconfig[] = AUBOOT;
	char bootname[20];

	ioctl(0,TCGETA,&ta); 
	ta.c_lflag &= ~ISIG;
	ioctl(0,TCSETAW,&ta); 
	nvram.addr = (char *)(UNX_NVR->bootname);
	nvram.data = (char *)bootname;
	nvram.cnt = sizeof(bootname);
	sys3b(RNVR,&nvram); 

	if ((strcmp(bootname, SYSTEM) == 0)){
		nvram.addr = (char *)(UNX_NVR->bootname);
		nvram.data = (char *)aconfig;
		nvram.cnt = sizeof(aconfig);
		sys3b(WNVR,&nvram); 
		printf("SYSTEM REBOOT, AUTO-CONFIGURATION WILL CONTINUE\n");
		fflush(stdout);
		sleep(2);
		uadmin(A_SHUTDOWN, AD_BOOT, 0);
	} else {
		uadmin(A_SHUTDOWN, AD_IBOOT, 0);
	}

}
