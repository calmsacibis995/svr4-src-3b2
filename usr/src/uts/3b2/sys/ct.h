/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_CT_H
#define _SYS_CT_H

#ident	"@(#)head.sys:sys/ct.h	11.2"

/*
 * CT User Error Codes
 * These are the error codes which are returned via the UNIX "errno"
 * external variable, and are in addition to those found in <errno.h>.
 */

#define	EUSRSPL		200	/* other user has special ioctl() open */
#define	ENOSGEN		201	/* board failed to sysgen correctly */
#define	EBRDDWN		202	/* board has been marked as down */
#define	ENOCONF		203	/* sub-device is not configured */
#define	EFWCBAD		204	/* driver lla fw routine failed */
#define	ENOTOPN		205	/* sub-device not open */
#define	EROPART		206	/* write on read only partition */
#define	EPRTOVR		207	/* partition overrun */
#define	EBDVTOC		208	/* vtoc is not marked as sane */
#define	EBDPSEC		209	/* pdsector is not marked as sane */
#define	ENSLOPN		210	/* not correctly opened using O_CTSPECIAL */
#define	ENTSUSR		211	/* format requires single user on board */
#define	ETIMOUT		212	/* brd failed to complete in time allotted */
#define	EHDWERR		213	/* hardware error occurred on board */
#define	ENOTRDY		214	/* device not ready */
#define	ERWERR		215	/* device read/write error */
#define	EWRTPRT		216	/* write on a write protected device */
#define	EBDJSIZ		217	/* bad job request size on stream request */
#define	EBDOFLG		218	/* bad open flag, unclear RD/WRT direction */
#define	ENOMED		219	/* no media present in sub-device */
#define	ESEGWRAP	220	/* user buffer crossed over 128K boundary */

/* ioctl flag define for ctopen() */

#define	O_CTSPECIAL	0200

/*
 * CT Driver User Command ioctl() codes
 *  NOTE: for format and verify commands see sys/diskette.h
 */

#define	DUMP		'd'	/* Dump vtoc, pdinfo, and pass count */
#define	GETPASS		'g'	/* Return drive usage time count */
#define	RSTPASS		'r'	/* Reset the controller tape pass count */
#define	STREAMON	's'	/* Turn on stream mode */

/*
 * DUMP Device types
 */

#define CT_TAPE6	0x1
#define CT_TAPE12	0x2
#define FD5_25SS	0x3
#define FD5_25SD	0x4
#define FD5_25DS	0x5
#define FD5_25DD	0x6
#define FD3_5SS		0x7
#define FD3_5SD		0x8
#define FD3_5DS		0x9
#define FD3_5DD		0xa
#define FD8_0SS		0xb
#define FD8_0SD		0xc
#define FD8_0DS		0xd
#define FD8_0DD		0xe
#define NULLSD		0xf

#define	CTPASS	0			/* Current Tape Pass Count in devsp */
#define	MTPASS	1			/* Maximum Tape Pass Count in devsp */

#define	MAXTPAS	4000			/* Default maximum tape pass count */
#define	MAXCPAS	2880000	/* Max. controller head pass time in 25 ms's */

#define	PASS_COUNT	dev_pdsector.devsp[CTPASS]
#define	MAX_PASS_COUNT	dev_pdsector.devsp[MTPASS]

struct dev_dump	{
	unsigned char dev_type;		/* device type */
	unsigned long hd_cnt;		/* time (25ms) since last clean */
	struct vtoc dev_vtoc;		/* device vtoc - see vtoc.h */
	struct pdsector dev_pdsector;	/* device pdsector - see vtoc.h */
};

#endif	/* _SYS_CT_H */
