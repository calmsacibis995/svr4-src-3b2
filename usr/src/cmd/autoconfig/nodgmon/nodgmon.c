/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:nodgmon/nodgmon.c	1.2"

#include "sys/types.h"
#include "sys/psw.h"
#include "sys/boot.h"
#include "sys/sbd.h"
#include "sys/firmware.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/nvram.h"
#include <sys/sys3b.h>



main()
{
	struct nvparams nvram;
	static char fastboot[]="fast boot";


	nvram.addr = (char *)(UNX_NVR->bootname);
	nvram.data = (char *)fastboot;
	nvram.cnt = sizeof(fastboot);
	sys3b(WNVR,&nvram);

}
	
