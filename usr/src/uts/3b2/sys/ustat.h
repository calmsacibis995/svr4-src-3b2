/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.sys:sys/ustat.h	11.7"

/* WARNING: The ustat system call will become obsolete in the
** next major release following SVR4. Application code should
** migrate to the replacement system call statvfs(2).
*/

#ifndef _SYS_USTAT_H
#define _SYS_USTAT_H

#ifndef _SYS_TYPES_H
#include "sys/types.h"
#endif


struct  ustat {
	daddr_t	f_tfree;	/* total free */
	o_ino_t	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};



#endif /* _SYS_USTAT_H */
