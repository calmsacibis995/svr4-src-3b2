/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/space.c	1.16"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/immu.h"
#include "sys/acct.h"
#include "sys/proc.h"

struct	acct	acctbuf;
struct	vnode	*acctp;

#include "sys/buf.h"

struct	buf	bfreelist;	/* Head of the free list of buffers */
struct	pfree	pfreelist;	/* Head of physio buffer headers    */
int		pfreecnt;	/* Count of free physio buffers.    */

#include "sys/cred.h"
#include "sys/vfs.h"
#include "sys/vnode.h"

struct vnode	*rootdir;	/* vnode for root directory. */


#include "sys/sysinfo.h"

struct sysinfo sysinfo;
struct	syswait	syswait;
struct	syserr	syserr;
struct  minfo minfo;
struct  vminfo vminfo;		/* VM stats */
struct	rtminfo	rtminfo;	/* Real Time stats */
struct	kmeminfo kmeminfo;	/* KMA stats */

#include	"sys/swap.h"

int rf_state;

/*
 * The following describe the physical memory configuration.
 *
 * 	maxclick - The largest physical click number.
 * 		   ctob(maxclick) is the largest physical
 * 		   address configured plus 1.
 *
 * 	physmem	 - The amount of physical memory configured
 * 		   in clicks.  ctob(maxclick) is the amount
 * 		   of physical memory in bytes.
 * 	kpbase	 - The physical address of the start of
 * 		   the allocatable memory area.  That is,
 * 		   after all kernel tables are allocated.
 * 		   Pfdat table entries are allocated for
 * 		   physical memory starting at this address.
 * 		   It is always on a page boundary.
 */

int	maxclick;
int	physmem;
int	kpbase;


/*
 * The following are concerned with the kernel virtual
 * address space.
 *
 *	kptbl	- The address of the kernel page table.
 *		  It is dynamically allocated in startup.c
 */

pte_t	*kptbl;

/* new structure for proc table restructure */

proc_t *pactivelist;	/* the active processes header */
pincr_t *pfreelisthead;	/* the first free pincr table entry pointer */
pincr_t *pfreelisttail;	/* the last free pincr table entry pointer */

