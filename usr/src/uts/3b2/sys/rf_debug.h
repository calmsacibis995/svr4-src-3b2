/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_RF_DEBUG_H
#define _SYS_RF_DEBUG_H

#ident	"@(#)head.sys:sys/rf_debug.h	1.4"
/* RFS debugging */

#define DB_MNT_ADV	0x002	/* advertising and remote mounts */
#define DB_RFSTART	0x004	/* rfstart, rfstop */

/* Used in NPACK driver */
#define NO_RETRANS      0x20000  /* turn off retransmission */
#define NO_RECOVER      0x40000  /* turn off rf_recovery */
#define NO_MONITOR      0x80000  /* turn off monitor */

#define DB_LOOPBCK	0x800000  /* allow machine to mount own resources */

#if defined(_KERNEL)

extern	long	dudebug;

#if defined(DEBUG)

#define	DUPRINT1(x,y1) if (dudebug & x) cmn_err(CE_CONT,y1);
#define	DUPRINT2(x,y1,y2) if (dudebug & x) cmn_err(CE_CONT,y1,y2);
#define	DUPRINT3(x,y1,y2,y3) if (dudebug & x) cmn_err(CE_CONT,y1,y2,y3);
#define	DUPRINT4(x,y1,y2,y3,y4) if (dudebug & x) cmn_err(CE_CONT,y1,y2,y3,y4);
#define	DUPRINT5(x,y1,y2,y3,y4,y5) \
	if (dudebug & x) cmn_err(CE_CONT,y1,y2,y3,y4,y5);
#define DUPRINT6(x,y1,y2,y3,y4,y5,y6) \
	if (dudebug & x) cmn_err(CE_CONT,y1,y2,y3,y4,y5,y6);

#else

#define	DUPRINT1(x,y1)
#define	DUPRINT2(x,y1,y2)
#define	DUPRINT3(x,y1,y2,y3)
#define	DUPRINT4(x,y1,y2,y3,y4)
#define	DUPRINT5(x,y1,y2,y3,y4,y5)
#define	DUPRINT6(x,y1,y2,y3,y4,y5,y6)

#endif /* DEBUG */

#endif /* _KERNEL */

#endif /* _SYS_RF_DEBUG_H */
