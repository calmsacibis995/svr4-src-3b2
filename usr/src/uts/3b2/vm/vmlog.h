/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_VMLOG_H
#define _VM_VMLOG_H

#ident	"@(#)kernel:vm/vmlog.h	1.4"

/*
 * For temporary debugging only
 */

extern int	do_vmlog;
extern void	_vmlog(/* what, a, b, c */);

#ifdef VMDEBUG
#define VMLOG(N, A, B, C) \
	{ if (do_vmlog) _vmlog((int)N, (int)(A), (int)(B), (int)(C)); }
#else
#define VMLOG(N, A, B, C)
#endif

#define X_ANONALLOC			1
#define X_ANONINCREF_PAGEWAIT		2
#define X_ANONINCREF_UNLOAD		3
#define X_ANONDECREF			4
#define X_ANONDECREF_FREE		5
#define X_ANONDECREF_ABORT		6
#define X_ANONGETPAGE			7
#define X_ANONGETPAGE_GOTIT_WAIT	8
#define X_ANONGETPAGE_GOTIT		9
#define X_ANONGETPAGE_VOP		10
#define X_ANONPRIVATE			11
#define X_ANONPRIVATE_GETPAGE		12
#define X_ANONPRIVATE_ALLOC		13
#define X_ANONZERO			14
#define X_ANONZERO_ALLOC		15
#define X_ANONZERO_LOAD			16

#define X_PAGELOOKUP_CLEANUP		19
#define X_PAGELOOKUP_WAIT		20
#define X_PAGELOOKUP_WAITDONE		21
#define X_PAGEENTER			22
#define X_PAGEABORT			23
#define X_PAGEABORT_KEPT		24
#define X_PAGEABORT_INTRANS		25
#define X_PAGEABORT_UNLOAD		26
#define X_PAGEABORT_FREE		27
#define X_PAGEABORT_GONE		28
#define X_PAGEFREE			29
#define X_PAGEFREE_GONE			30
#define X_PAGEFREE_INTRANS		31
#define X_PAGEFREE_NOTINTRANS		32
#define X_PAGEFREE_FREEMEM_WAIT		33
#define X_PAGEGET_SLEEP			34
#define X_PAGEGET_GOTMEM		35

#define X_PVNFAIL			40
#define X_PVNDONE			41
#define X_PVNDONE_ZERO			42
#define X_PVNDONE_SYNC_READ		43
#define X_PVNDONE_OTHER			44
#define X_PVNDONE_PAGE			45
#define X_PVNDONE_INVAL			46
#define X_PVNDONE_WRITE_ERR		47
#define X_PVNDONE_DIRTY_FREE		48
#define X_PVNDONE_DIRTY_UNLOCK		49
#define X_PVNDONE_UNLOCK		50
#define X_PVNDIRTY_ABORT		51
#define X_PVNDIRTY_FREE_RET		52
#define X_PVNDIRTY_INTRANS		53
#define X_PVNDIRTY_LOCK			54
#define X_PVNDIRTY_WAIT			55
#define X_PVNDIRTY_LOST			56
#define X_PVNDIRTY_NOMOD		57
#define X_PVNDIRTY_INVAL		58
#define X_PVNDIRTY_FREE			59
#define X_PVNDIRTY_NOMOD_RET		60
#define X_PVNDIRTY_ADD			61
#define X_VPLIST_UNKEEP_NEXTL		62
#define X_VPLIST_KEEP_NEXT		63
#define X_VPLIST_NEXT_NULL		64
#define X_VPLIST_GETDIRTY		65
#define X_VPLIST_KEEPSAV		66
#define X_VPLIST_UNKEEP_SAV		67
#define X_VPLIST_UNKEEP_NEXT		69
#define X_RANGE_GETDIRTY		70

#endif	/* _VM_VMLOG_H */
