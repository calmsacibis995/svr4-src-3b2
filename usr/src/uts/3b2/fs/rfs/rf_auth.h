/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_RFS_AUTH_H
#define _FS_RFS_AUTH_H

#ident	"@(#)fs:fs/rfs/rf_auth.h	1.2"

/*
 * Exported interfaces of RFS id mapping.
 */

extern uid_t	glid();
extern void	vattr_rmap();
extern int	rf_setidmap();
extern void	rf_freeidmap();
extern int	rf_addalist();
extern int	rf_checkalist();
extern void	rf_heapfree();
extern void	auth_init();

#define rf_remalist(clist) ((void)(!(clist) || (rf_heapfree(clist), 0)))

#endif /* _FS_RFS_AUTH_H */
