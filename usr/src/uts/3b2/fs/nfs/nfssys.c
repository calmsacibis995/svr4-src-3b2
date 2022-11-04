/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/nfs/nfssys.c	1.2"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

#include "sys/types.h"
#include "rpc/types.h"
#include "sys/systm.h"
#include "sys/vfs.h"
#include "sys/errno.h"
#include "nfs/nfs.h"
#include "nfs/export.h"
#include "nfs/nfssys.h"

/*ARGSUSED*/
int
nfssys(uap, rvp)
register struct nfssysa *uap;
rval_t *rvp;
{
	union nfssysargs	a;

	switch ((int) uap->opcode) {
		case NFS_SVC:
			/* NFS server daemon */
			if (copyin((caddr_t) uap->nfssysarg_svc, (caddr_t) &a.nfs_svc_args_u, sizeof(a.nfs_svc_args_u)))
				return(EFAULT);
			else
				return(nfs_svc(&a.nfs_svc_args_u));

		case ASYNC_DAEMON:
			/* NFS client async daemon */
			return(async_daemon());		/* no args */

		case EXPORTFS:
			/* export a file system */
			return(exportfs(uap->nfssysarg_exportfs));

		case NFS_GETFH:
			/* get a file handle */
			return(nfs_getfh(uap->nfssysarg_getfh));

		default:
			return(EINVAL);
	}
}
