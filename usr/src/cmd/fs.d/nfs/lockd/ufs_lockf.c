/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/ufs_lockf.c	1.3"
/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
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
#include <sys/types.h>
#include "prot_lock.h"

int				LockID = 0; /* Monotonically increasing id */
int				max_processes = 0; /* statistics keeper */
struct process_locks            proc_lists[MAXPROCESS],
                                *proc_locks = NULL;

/*
 * Function get_proc_list(p)
 *
 * This function will use one of the process lock list slots and assign
 * it to this process.
 * XXX - enhance performance with free_procs list pointer ?
 */
struct process_locks *
get_proc_list(p)
	long	p;	/* Process ID to use */
{
	int	i;	/* loop temp */

	printf("...get_proc_list()");
	for (i=0; (i < MAXPROCESS) && (proc_lists[i].pid != 0); i++);
	if (i == MAXPROCESS) return (NULL);
	max_processes++;
	proc_lists[i].pid = p;
	proc_lists[i].next = proc_locks;
	proc_locks = &proc_lists[i];
	proc_lists[i].lox = NULL;
	return (&proc_lists[i]);
}

/*
 * Function add_proc_lock(l)
 *
 * Add's the passed lock to the list of locks owned by this lock's process.
 * should probably be a macro.
 */
void
add_proc_lock(l)
	struct data_lock	*l;

{
	printf("...add_proc_lock()");
	l->NextProc = (l->MyProc)->lox;
	(l->MyProc)->lox = l;
}

/*
 * Function rem_proc_lock(l)
 *
 * The slightly more complicated brother of add above it removes the
 * given lock from the process' lock list.
 */
void
rem_proc_lock(l)
	struct data_lock *l;
{
	struct data_lock 	*t, /* Temps, Thislock and previous lock */
				*p;
	struct process_locks 	*cur, /* More temps to follow process list */
				*prev;

	printf("...rem_proc_lock()");
	/* Find the lock */
	for (t=(l->MyProc)->lox, p=NULL; (t && (t != l)); p=t, t=t->NextProc);
	/* Update the list */
	if (t == l) {
		if (p)
			p->NextProc = t->NextProc;
		else {
			(l->MyProc)->lox = l->NextProc;
			if (!(l->NextProc)) {
				/*
				 * This is the last lock this process was
				 * holding so free list pointer
				 */
				for (cur = proc_locks, prev=NULL;
					(l->MyProc != cur);
					prev=cur, cur=cur->next);
				if (prev)
					prev->next = cur->next;
				else
					proc_locks = cur->next;
				/* Free this process lock holder. */
				cur->pid = 0;
			}
		}
	}
}

/*
 * return a string representation of the lock.
 */
void
print_lock(l)
	struct data_lock	*l;
{

	printf("[ID=%d, pid=%d, base=%d, len=%d, type=%s rsys=%x]",
		l->LockID, l->lld.l_pid, l->lld.l_start, l->lld.l_len,
		(l->lld.l_type == F_WRLCK) ? "EXCL" : "SHRD",
		l->lld.l_sysid);
	return;
}
