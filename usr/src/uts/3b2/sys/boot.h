/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_BOOT_H
#define _SYS_BOOT_H

#ident	"@(#)head.sys:sys/boot.h	11.8"

#define BOOTADDR 0x2004000

#define AUTOBOOT 0
#define DEMANDBOOT 1
#define UNIXBOOT 2

#define FDBOOTBLK 0

#define FLOPDISK  0
#define HARDDISK0 1
#define HARDDISK1 2
#define HARDDISK HARDDISK0
#define ICD	  3

#define ICDBLKSZ	512	/* Size of In-Core Disk block   */

#define ICDROOT		0 	/* Minor device number of root  */
				/* file system on a In-Core     */
				/* Disk at installation.        */

#define	FLOPMINOR	0x85	/* Minor device number of root	*/
				/* file system on a bootable	*/
				/* floppy disk.			*/

#define BOOTNAME 80

struct bootcmd {
	char b_type;		/* type of boot (auto or demand) */
	char b_dev;		/* source of boot (HARDDISK, FLOPDISK, or ..) */
	char b_name[BOOTNAME];	/* full pathname of file to boot */
};

struct blk_acs {
	unsigned long blkno;
	unsigned long bufptr;
};

#endif	/* _SYS_BOOT_H */
