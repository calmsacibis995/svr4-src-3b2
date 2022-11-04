/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_BOOTCONF_H
#define _VM_BOOTCONF_H

#ident	"@(#)kernel:vm/bootconf.h	1.3"

/*
 * Boot time configuration information objects
 */

#define	MAXFSNAME	16
#define	MAXOBJNAME	128
/*
 * Boot configuration information
 */
struct bootobj {
	char	bo_fstype[MAXFSNAME];	/* filesystem type name (e.g. nfs) */
	char	bo_name[MAXOBJNAME];	/* name of object */
	int	bo_flags;		/* flags, see below */
	int	bo_offset;		/* number of blocks */
	int	bo_size;		/* number of blocks */
	struct vnode *bo_vp;		/* vnode of object */
};

/*
 * flags
 */
#define	BO_VALID	0x01		/* all information in object is valid */
#define	BO_BUSY		0x02		/* object is busy */

extern struct bootobj rootfs;
extern struct bootobj dumpfile;
extern struct bootobj swapfile;

#endif	/* _VM_BOOTCONF_H */
