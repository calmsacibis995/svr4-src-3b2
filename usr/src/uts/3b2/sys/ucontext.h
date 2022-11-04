/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.sys:sys/ucontext.h	1.9"

#ifndef _SYS_UCONTEXT_H
#define _SYS_UCONTEXT_H

#include "sys/types.h"
#include "sys/regset.h"
#include "sys/signal.h"

typedef struct {
	gregset_t	gregs;	/* general register set */
	fpregset_t 	fpregs;	/* floating point register set */
} mcontext_t;

typedef struct ucontext {
	u_long		uc_flags;
	struct ucontext	*uc_link;
	sigset_t   	uc_sigmask;
	stack_t 	uc_stack;
	mcontext_t 	uc_mcontext;
	long		uc_filler[23];
} ucontext_t;

#define GETCONTEXT	0
#define SETCONTEXT	1

/* 
 * values for uc_flags
 * these are implementation dependent flags, that should be hidden
 * from the user interface, defining which elements of ucontext
 * are valid, and should be restored on call to setcontext
 */

#define	UC_SIGMASK	001
#define	UC_STACK	002
#define	UC_CPU		004
#define	UC_MAU		010

#define UC_MCONTEXT	(UC_CPU|UC_MAU)

/* 
 * UC_ALL specifies the default context
 */

#define UC_ALL		(UC_SIGMASK|UC_STACK|UC_MCONTEXT)

#ifdef _KERNEL

#if defined(__STDC__)
void savecontext(ucontext_t *, k_sigset_t);
void restorecontext(ucontext_t *);
#else
void savecontext();
void restorecontext();
#endif

#endif
#endif /* _SYS_UCONTEXT_H */
