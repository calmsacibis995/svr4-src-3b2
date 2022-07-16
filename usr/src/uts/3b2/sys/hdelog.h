/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_HDELOG_H
#define _SYS_HDELOG_H

#ident	"@(#)head.sys:sys/hdelog.h	11.9"
/* values for filling in readtype are: */
#define HDECRC		1	/* for CRC data checking */
#define HDEECC		2	/* for Error Correction Code data processing */

/* values for filling in severity are: */
#define HDEMARG		1	/* for marginal blocks */
#define HDEUNRD		2	/* for unreadable blocks */

/* Due to compatibility problems with add-on (ctc xdc scsi) 3b2 drivers 
** the hdelogger will not support the expanded dev format. All drivers
** using the hde defect table management must be configured within the
** bounds of the OLD device format i.e., 7-bit majors and 8-bit minors. 
** Conformant DDI/DKI 3B2 disk drivers need to call the cmpdev() function
** before dev is passed to the hde driver.
*/

struct hdedata {
	o_dev_t	diskdev;	/* the major/minor disk device number */
				/* (major number for character dev) */
	char	dskserno[12];	/* disk pack serial number */
				/* protection of removable media */
	short	pad;		/* to aid 3b2/80186 alignment */
	daddr_t	blkaddr;	/* physical block address
				 * in machine-independent form */
	char	readtype;	/* CRC or EC */
	char	severity;	/* marginal or unreadable */
	char	badrtcnt;	/* number of unreadable tries */
	char	bitwidth;	/* bitwidth of corrected error: 0 if CRC */
	time_t	timestmp;	/* time stamp helps pathological cases*/
	int	pad2;		/* round size to 2^n */
};

/* structure for hdeeqdt[] table declared in space.h */
struct hdeedd {
	o_dev_t	hdeedev;
	short	hdetype;
	short	hdeflgs;
	o_pid_t	hdepid;
	daddr_t	hdepdsno;
};

#define EQD_ID		0
#define EQD_IF		1
#define EQD_TAPE	2
#define EQD_EHDC	3
#define EQD_EFC		4

/* size of internal log report queue of the hdelog driver: */
#define HDEQSIZE	19

#if defined(__STDC__)
extern int hdeeqd(o_dev_t, daddr_t, short);
extern int hdelog(struct hdedata *);
#else
extern int hdeeqd();
extern int hdelog();
#endif	/* __STDC__ */

#endif	/* _SYS_HDELOG_H */
