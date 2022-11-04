/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_SEG_DEV_H
#define _VM_SEG_DEV_H

#ident	"@(#)kernel:vm/seg_dev.h	1.6"

/*
 * Structure who's pointer is passed to the segvn_create routine
 */
struct segdev_crargs {
	int	(*mapfunc)();	/* map function to call */
	u_int	offset;		/* starting offset */
	dev_t	dev;		/* device number */
	u_char	prot;		/* protection */
	u_char	maxprot;	/* maximum protection */
};

/*
 * (Semi) private data maintained by the seg_dev driver per segment mapping
 */
struct	segdev_data {
	int	(*mapfunc)();	/* really returns struct pte, not int */
	u_int	offset;		/* device offset for start of mapping */
	struct	vnode *vp;	/* Vnode associated with device */
	u_char	pageprot;	/* true if per page protections present */
	u_char	prot;		/* current segment prot if pageprot == 0 */
	u_char	maxprot;	/* maximum segment protections */
	struct	vpage *vpage;	/* per-page information, if needed */
};

#ifdef _KERNEL

#if defined(__STDC__)
extern int segdev_create(struct seg *, void *);
#else
extern int segdev_create();
#endif

#endif /* _KERNEL */

#endif	/* _VM_SEG_DEV_H */
