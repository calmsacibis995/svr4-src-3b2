/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_STRPUMP_H
#define _SYS_STRPUMP_H

#ident	"@(#)head.sys:sys/strpump.h	1.4"

#define	PUMP	(('p'<<8)|8)

#define P_LOAD	(PUMP|1)
#define P_RST	(PUMP|2)
#define P_FCF	(PUMP|3)
#define P_GAD	(PUMP|4)
#define P_SYSGEN	(PUMP|5)
#define P_EQUIP	(PUMP|6)

#define	A_MINOR	127	/* the administrative minor used to pump the firmware */

#define PUMPBSIZE 256*3

#define PU_OTHER (-1)
#define PU_TIMEOUT (-2)
#define PU_DEVCH (-3)
#define PU_NULL	8

struct pumpst
{
	unsigned short size;
	char *address;
	unsigned long flags;
	unsigned char data[PUMPBSIZE];
};


#endif	/* _SYS_STRPUMP_H */
