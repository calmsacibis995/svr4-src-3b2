/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_TODC_H
#define _SYS_TODC_H

#ident	"@(#)head.sys:sys/todc.h	11.3"
/* Time of Day Clock */

struct clkseg {
	int :28; unsigned units:4;
	int :28; unsigned tens:4;
	};

struct clock {
	int :28; unsigned test:4;
	int :28; unsigned tenths:4;
	struct clkseg secs;
	struct clkseg mins;
	struct clkseg hours;
	struct clkseg days;
	int :28; unsigned dayweek:4;
	struct clkseg months;
	int :28; unsigned years:4;
	int :28; unsigned stop_star:4;
	int :28; unsigned int_stat:4;
};

struct todc {
	short htenths;
	short hsecs;
	short hmins;
	short hhours;
	short hdays;
	short hweekday;
	short hmonth;
	short hyear;
};

#define	OTOD		0x00041000L
#define	SBDTOD		((struct clock *)OTOD)

#if defined(__STDC__)

extern void rtodc(struct todc *);
extern void wtodc(void);

#else

extern void rtodc();
extern void wtodc();

#endif

#endif	/* _SYS_TODC_H */
