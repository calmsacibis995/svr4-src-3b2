/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_SYSMACROS_H
#define _SYS_SYSMACROS_H

#ident	"@(#)head.sys:sys/sysmacros.h	11.11"

#include "sys/param.h"

/*
 * Some macros for units conversion
 */

/* Core clicks to segments and vice versa */

#define ctos(x)		(((x) + (NCPS-1)) >> CPSSHIFT)
#define	ctost(x)	((x) >> CPSSHIFT)
#define	stoc(x)		((x) * NCPS)

/* byte address to segment and vice versa  */
#define sgnum(x)	(((unsigned)(x) >> 17) & 0x1fff)
#define stob(x)		((((unsigned)(x)) & 0x1fff) << 17)
#define	btos(x)		((unsigned)(x) >> 17)
#define secnum(x)	((unsigned)(x) >> 30)

/* Core clicks to immu max offset and vice versa */
#define ctomo(x)	((x) * 256 -1)
#define motoc(x)	((((x)+1)*8) >> BPCSHIFT)
#define motob(x)	(8*((x)+1)-1)

/*
 * Disk blocks (sectors) and bytes.
 */
#define	dtob(DD)	((DD) << SCTRSHFT)
#define	btod(BB)	(((BB) + NBPSCTR - 1) >> SCTRSHFT)
#define	btodt(BB)	((BB) >> SCTRSHFT)

/*
 * Disk blocks (sectors) and pages.
 */
#define NDPP		4	/* Number of disk blocks per page */
#define DPPSHFT		2	/* Shift for disk blocks per page. */
#define	ptod(PP)	((PP) << DPPSHFT)
#define	dtop(DD)	(((DD) + NDPP - 1) >> DPPSHFT)
#define dtopt(DD)	((DD) >> DPPSHFT)

/* clicks to bytes */
#ifdef BPCSHIFT
#define	ctob(x)	((x)<<BPCSHIFT)
#else
#define	ctob(x)	((x)*NBPC)
#endif

/* bytes to clicks */
#ifdef BPCSHIFT
#define	btoc(x)	(((unsigned)(x)+(NBPC-1))>>BPCSHIFT)
#define	btoct(x)	((unsigned)(x)>>BPCSHIFT)
#else
#define	btoc(x)	(((unsigned)(x)+(NBPC-1))/NBPC)
#define	btoct(x)	((unsigned)(x)/NBPC)
#endif

/* common macros */

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) < (b) ? (b) : (a))

/* WARNING: The device number macros defined here should not be used by device 
** drivers or user software. Device drivers should use the device functions
** defined in the DDI/DKI interface (see also ddi.h). Application software should 
** make use of the library routines available in makedev(3). A set of new device 
** macros are provided to operate on the expanded device number format supported
** in SVR4. Macro versions of the DDI device functions are provided for use by
** kernel proper routines only. Macro routines bmajor(), major(), minor(),
** emajor(), eminor(), and makedev() will be removed or their definitions 
** changed at the next major release following SVR4.
*/

#define O_BITSMAJOR	7	/* # of SVR3 major device bits */
#define O_BITSMINOR	8	/* # of SVR3 minor device bits */
#define O_MAXMAJ	0x7f	/* SVR3 max major value */
#define O_MAXMIN	0xff	/* SVR3 max major value */


#define L_BITSMAJOR	14	/* # of SVR4 major device bits */
#define L_BITSMINOR	18	/* # of SVR4 minor device bits */
#define L_MAXMAJ	0xff	/* Although 14 bits are reserved, 
				** the 3b2 major number is restricted
				** to 8 bits. 
				*/

#define L_MAXMIN	0x3ffff	/* MAX minor for 3b2 software drivers.
				** For 3b2 hardware devices the minor is
				** restricted to 256 (0-255)
				*/

#ifdef _KERNEL

/* major part of a device internal to the kernel */
#define NMAJORENTRY	256	/* Number of entries in major/minor array */

extern char MAJOR[NMAJORENTRY];
#define	major(x)	(int)(MAJOR[(unsigned)((x)>>O_BITSMINOR) & O_MAXMAJ])
#define	bmajor(x)	(int)(MAJOR[(unsigned)((x)>>O_BITSMINOR) & O_MAXMAJ])

/* get internal major part of expanded device number */

#define	getmajor(x)	(int)(MAJOR[(unsigned)((x)>>L_BITSMINOR) & L_MAXMAJ])

/* minor part of a device internal to the kernel */
extern char MINOR[256];
#define	minor(x)	(int)(MINOR[(unsigned)((x)>>O_BITSMINOR)&O_MAXMAJ]+((x)&O_MAXMIN))

/* get internal minor part of expanded device number */

#define	getminor(x)	(int)(MINOR[(unsigned)((x)>>L_BITSMINOR)&L_MAXMAJ]+((x)&L_MAXMIN))

#else

/* major part of a device external from the kernel (same as emajor below) */
#define	major(x)	(int)(((unsigned)x>>O_BITSMINOR)&O_MAXMAJ)


/* minor part of a device external from the kernel  (same as eminor below)*/
#define	minor(x)	(int)(x&O_MAXMIN)

#endif	/* _KERNEL */

/* create old device number */

#define	makedev(x,y)	(unsigned short)(((x)<<O_BITSMINOR) | (y&O_MAXMIN))

/* make an new device number */
			
#define makedevice(x,y)	(unsigned long)(((x)<<L_BITSMINOR) | ((y)&L_MAXMIN))


/*
 *   emajor() allows kernel/driver code to print external major numbers
 *   eminor() allows kernel/driver code to print external minor numbers
 */

#define emajor(x)	(int)(((unsigned)(x)>>O_BITSMINOR)&O_MAXMAJ)
#define eminor(x)	(int)((x)&O_MAXMIN)

/* get external major and minor device 
** components from expanded device number
*/
#define getemajor(x)	(int)(((unsigned)(x)>>L_BITSMINOR)&L_MAXMAJ)
#define geteminor(x)	(int)((x)&L_MAXMIN)


/* convert to old dev format */

#define cmpdev(x) 	(unsigned long)((((x)>>L_BITSMINOR) > O_MAXMAJ || \
				((x)&L_MAXMIN) > O_MAXMIN) ? NODEV : \
				((((x)>>L_BITSMINOR)<<O_BITSMINOR)|((x)&O_MAXMIN)))

/* convert to new dev format */

#define expdev(x) 	(unsigned long)(((((x)>>O_BITSMINOR)&O_MAXMAJ)<<L_BITSMINOR) \
				| ((x)&O_MAXMIN))

/*
 *  Evaluate to true if the process is an RFS server.
 */
#define	RF_SERVER()	(u.u_procp->p_sysid != 0)

/* machine dependent operations - defined for RFS and STREAMS */

#ifdef	pdp11
#define	SALIGN(p)		(char *)(((int)p+1) & ~1)
#define	IALIGN(p)		(char *)(((int)p+1) & ~1)
#define LALIGN(p)		(char *)(((int)p+1) & ~3)
#endif
#ifdef	vax
#define	SALIGN(p)		(char *)(((int)p+1) & ~1)
#define	IALIGN(p)		(char *)(((int)p+3) & ~3)
#define	LALIGN(p)		(char *)(((int)p+3) & ~3)
#endif
#ifdef	u3b2
#define	SALIGN(p)		(char *)(((int)p+1) & ~1)
#define	IALIGN(p)		(char *)(((int)p+3) & ~3)
#define	LALIGN(p)		(char *)(((int)p+3) & ~3)
#endif

#define SNEXT(p)		(char *)((int)p + sizeof (short))
#define INEXT(p)		(char *)((int)p + sizeof (int))
#define LNEXT(p)		(char *)((int)p + sizeof (long))

/*
 * Macros for counting and rounding.
 */
#define howmany(x, y)	(((x)+((y)-1))/(y))
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))

#endif	/* _SYS_SYSMACROS_H */
