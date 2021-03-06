/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_DU_DEP_H
#define _SYS_DU_DEP_H

#ident	"@(#)head.sys:sys/du_dep.h	11.2"
/*
 * PORTS CIO "dumb" Definitions
 *
 ***** WARNING ***** WARNING ***** WARNING ***** WARNING
 *	This header file is shared by both sw and fw
 ***** WARNING ***** WARNING ***** WARNING ***** WARNING
 */

typedef long	RAPP;	/* FW request codes */
typedef long	CAPP;	/* FW completion codes */

#define CQSIZE		10		/* size one completion queue  */
#define RQSIZE		5		/* size all requeust queues   */
#define NUM_QUEUES	1		/* single request queue */

#define RAM_START	0
#define RAM_END		32768

#endif	/* _SYS_DU_DEP_H */
