/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/xnamfs/xnamvfsops.c	1.5"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/buf.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/errno.h"
#include "sys/vfs.h"
#include "sys/swap.h"
#include "sys/vnode.h"
#include "sys/fs/xnamnode.h"

#include "fs/fs_subr.h"

struct vfsops xnam_vfsops = {
	fs_nosys,	/* mount */
	fs_nosys,	/* unmount */
	fs_nosys,	/* root */
	fs_nosys,	/* statvfs */
	fs_nosys,	/* sync */
	fs_nosys,	/* vget */
	fs_nosys,	/* mountroot */
	fs_nosys,	/* swapvp */
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
};
