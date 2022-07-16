/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:restore.d/hdrs/rs.h	1.3"

/* Are we a restore command or a urestore command */
#define	IS_RESTORE(c)	(!strcmp(c, "restore"))
#define	IS_URESTORE(c)	(!strcmp(c, "urestore"))

/* Option flag #defines */
#define	AFLAG	0x1
#define	cFLAG	0x2
#define	dFLAG	0x4
#define	DFLAG	0x8
#define	FFLAG	0x10
#define	mFLAG	0x20
#define	nFLAG	0x40
#define	oFLAG	0x80
#define	PFLAG	0x100
#define	vFLAG	0x200
#define	sFLAG	0x400
#define	SFLAG	0x800

#define	NFLAGS	12

#define	NRESTORES	50	/* Only restore NRESTORES objects per command */

