/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/kmacct.c	1.4"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/vnode.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/signal.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/user.h"
#include "sys/cmn_err.h"
#include "sys/errno.h"
#include "sys/debug.h"
#include "sys/kmacct.h"

#define	FALSE	0
#define	TRUE	1

/*
 * Variables supplied by master.d/kmacct
 */

extern	int		nkmasym;	/* Number of entries in symbol table */
extern	int		kmadepth;	/* Depth of stack to trace */
extern	int		nkmabuf;	/* Number of buffer headers */
extern	kmasym_t	kmasymtab[];	/* KMA accounting symbol table */
extern	caddr_t		kmastack[];	/* Calling sequences */
extern	kmabuf_t	kmabuf[];	/* Buffer headers */

/*
 * Variables used by KMACCT but defined elsewhere.
 */

extern	int		kmacctflag;
extern	int		Km_pools[];

/*
 * Private variables for KMACCT.
 */

STATIC	int	kmacct_free	= 0;	/* First free entry in symbol table */
STATIC	int	kmasize;		/* Total size of accounting data */

/*
 * Calling sequence symbol table hashing.
 */

#define	KMACHASH	64	/* Number of hash buckets (must be 2^n) */
#define	KMACMASK	0x3F	/* Mask (KMACHASH - 1) */

STATIC	kmasym_t	*hashlist[KMACHASH];

/*
 * Buffer header hashing.
 */

#define	KMBFNHASH	256	/* Number of hash buckets (must be 2^n) */
#define	KMBFMASK	0xFF	/* Mask (KMBFNHASH - 1) */

STATIC	kmabuf_t	bufhash[KMBFNHASH];
STATIC	kmabuf_t	*freebuflist = (kmabuf_t *) NULL;

/*
 * Buffers from 4.0 KMEM are aligned on at least 16 byte boundaries,
 * with addresses of the form 0xAAABBCC0 (the leading AAA is usually the same
 * for all  buffers).  We hash on the low order eight bits of the sum
 * of BB and CC; for most groups of addresses, this ought to give a
 * reasonable spread among the hash buckets.
 */

#define	KMBFHASH(addr)	(int) ((((uint) (addr) >> 4) + \
				((uint) (addr) >> 12)) & KMBFMASK)

/*
 * Return a buffer header to the freelist.
 */

#define	KMBFFREE(bp)	(bp)->next = freebuflist; \
			freebuflist = (bp);

/*
 * KMACCT is a pseudo-device driver (flags "sc" in master.d file).
 * Only init(), read() and ioctl() currently do anything useful.
 */

kmacinit()
{
	register int		i, j;
	register kmabuf_t	*bp;
	register kmasym_t	*kp, **hp;
	register caddr_t	*ksp;

	/*
	 * Put all the buffer headers onto the freelist.
	 */

	for (i = 0, bp = kmabuf; i < nkmabuf; i++, bp++) {
		KMBFFREE(bp);
	}

	/*
	 * Initialize the buffer header hash list.
	 */

	for (i = 0, bp = bufhash; i < KMBFNHASH; i++, bp++) {
		bp->next = bp;
		bp->prev = bp;
	}

	/*
	 * Check for invalid settings of SDEPTH tunable parameter.
	 */

	if (kmadepth > MAXDEPTH) {
		cmn_err(CE_WARN,
			"KMACCT: SDEPTH of %d too large; reset to %d\n",
			kmadepth, MAXDEPTH);
		kmadepth = MAXDEPTH;
	}

	/*
	 * Reset the symbol table hash buckets.
	 */

	for (i = 0, hp = hashlist; i < KMACHASH; i++, hp++) {
		*hp = (kmasym_t *) NULL;
	}

	/*
	 * Zero all symbol table entries.
	 */

	for (i = 0, kp = kmasymtab; i < nkmasym; i++, kp++) {
		kp->next = (kmasym_t *) NULL;
		kp->pc = (caddr_t *) NULL;
		for (j = 0; j < KMSIZES; j++) {
			kp->reqa[j] = 0;
			kp->reqf[j] = 0;
		}
	}

	/*
	 * Reset calling sequence lists.
	 */

	for (i = 0, j = nkmasym * kmadepth, ksp = kmastack; i < j; i++)
		*ksp++ = 0;

	/*
	 * Calculate amount of accounting data that will be read by
	 * user level process.
	 */

	kmasize = sizeof(kmasym_t) * nkmasym +
		sizeof(caddr_t) * kmadepth * nkmasym;

	return 0;
}

kmacread()
/*
 * If the request is for the correct amount of data (see 
 * kmacioctl(KMACCT_SIZE)), return data in the following format:
 * 	kmasymtab entries
 * 	kmastack entries
 */
{
	char	*dest;

	/*
	 * Make sure proper amount of data was requested
	 */

	if (u.u_count != kmasize) {
		u.u_error = EINVAL;
		return -1;
	}

	dest = u.u_base;
	
	if (copyout((char *) kmasymtab, dest,
		(int)(sizeof(kmasym_t) * nkmasym)) == -1) {
		u.u_error = EFAULT;
		return -1;
	}

	dest += sizeof(kmasym_t) * nkmasym;

	if (copyout((char *) kmastack, dest,
		(int)(sizeof(caddr_t) * kmadepth * nkmasym))  == -1) {
		u.u_error = EFAULT;
		return -1;
	}

	u.u_rval1 = kmasize;
	return 0;
}

/* ARGSUSED */
kmacioctl(dev, cmd, arg1, flags)
	unsigned long dev;
	int cmd;
	int arg1;
	int flags;
/*
 * All of the user control of KMACCT is through ioctl calls.
 */
{
	register int		i, j;
	register kmasym_t	*kp, **hp;
	register caddr_t	*ksp;
	register kmabuf_t	*bp;

	switch (cmd) {

	case KMACCT_ON:		/* Enable KMA accounting. */

		kmacctflag = 1;
		break;

	case KMACCT_OFF:	/* Disable KMA accounting. */

		kmacctflag = 0;
		break;

	case KMACCT_ZERO:	/* Reset by zeroing the symbol table. */

		if (kmacctflag) {
			/*
			 * Cannot zero if accounting still active
			 */
			u.u_error = EBUSY;
			break;
		}

		/*
		 * Zero all buffer headers and put them on the freelist.
		 */

		for (i = 0, bp = kmabuf; i < nkmabuf; i++, bp++) {
			bp->kp = (kmasym_t *) NULL;
			bp->addr = (caddr_t *) NULL;
			KMBFFREE(bp);
		}

		/*
		 * Reset the buffer header hash buckets.
		 */

		for (i = 0, bp = bufhash; i < KMBFNHASH; i++, bp++) {
			bp->next = bp;
			bp->prev = bp;
		}

		/*
		 * Reset the symbol table hash buckets.
		 */

		for (i = 0, hp = hashlist; i < KMACHASH; i++, hp++)
			*hp = (kmasym_t *) NULL;

		/*
		 * Zero all symbol table entries.
		 */

		for (i = 0, kp = kmasymtab; i < kmacct_free; i++, kp++) {
			kp->next = (kmasym_t *) NULL;
			kp->pc = (caddr_t *) NULL;
			for (j = 0; j < KMSIZES; j++) {
				kp->reqa[j] = 0;
				kp->reqf[j] = 0;
			}
		}

		kmacct_free = 0;

		/*
		 * Reset calling sequence lists.
		 */

		for (i = 0, j = nkmasym * kmadepth, ksp = kmastack; i < j; i++)
			*ksp++ = 0;

		break;

	case KMACCT_SIZE:	/* Return the size of kmacct data. */

		u.u_rval1 = kmasize;
		break;

	case KMACCT_NDICT:	/* Return the number of symbol table entries. */

		u.u_rval1 = nkmasym;
		break;

	case KMACCT_DEPTH:	/* Return the depth of the trace. */

		u.u_rval1 = kmadepth;
		break;

	case KMACCT_STATE:	/* Return current state (on or off) */

		u.u_rval1 = kmacctflag;
		break;

	default:	/* Any other ioctl() cmd is an error. */

		u.u_error = EINVAL;
		break;
	}

	return 0;
}

 STATIC kmasym_t *
getsymentry(pcstack)
	caddr_t	pcstack[];
{
	/*
	 * Search the symbol table, looking for a structure
	 * with the same calling sequence as pc.  If one is
	 * not found (and the symbol table is not full) create
	 * a new entry with that sequence.
	 */

	register int		match, i, hl = 0;
	register caddr_t	*pc1, *pc2;
	register kmasym_t	*kp, *trail;

	/*
	 * The hashing index is the low order 6 bits (0-63) of
	 * the sum of the pc stack.
	 */

	pc1 = pcstack;

	for (i = 0; i < kmadepth; i++) 
		hl += (int) *pc1++;

	hl &= KMACMASK;
	kp = hashlist[hl];
	trail = (kmasym_t *) &hashlist[hl];

	while (kp != (kmasym_t *) NULL) {
		pc1 = pcstack;
		pc2 = kp->pc;
		match = TRUE;
		for (i = 0; i < kmadepth; i++) {
			if (*pc1++ != *pc2++) {
				match = FALSE;
				break;
			}
		}
		if (match)
			return(kp);

		trail = kp;
		kp = kp->next;
	}

	if (kmacct_free >= nkmasym)
		return ((kmasym_t *) NULL);

	kp = &kmasymtab[kmacct_free];
	kp->pc = pc1 = &kmastack[kmacct_free*kmadepth];
	pc2 = pcstack;
	for (i = 0; i < kmadepth; i++)
		*pc1++ = *pc2++;

	/*
	 * Put this on the end of the list, hoping it is used less
	 * than the items already on the list.
	 */

	trail->next = kp;
	kp->next = (kmasym_t *) NULL;
	++kmacct_free;
	return(kp);
}

/*
 * The following routines and macros are machine dependent.  They 
 * backtrack through the stack, collecting return PC's.
 */

 asm int 
getfp()
{
	MOVW	%fp, %r0
}

 asm int 
getsp()
{
	MOVW	%sp, %r0
}

 asm int 
getap()
{
	MOVW	%ap, %r0
}

/*
 * Getcaller(num, pcstack) backtracks through the stack to retrieve
 * the previous NUM calling routines; PCSTACK is a pointer to an
 * array of at least NUM entries where the addresses are to be
 * placed.  The first address is always the routine that called
 * getcaller().  This code is taken from debug/trace.c.
 * There is no guarantee that all NUM entries belong in the same
 * trace, and there is no protection against stack underflow.
 */

 STATIC void
getcaller(num, pcstack)
	int	num;
	caddr_t	pcstack[];
{
	register int	fp	= getfp();
	register int	sp	= getsp();
	register int	ap	= getap();
	register caddr_t *pc	= pcstack;	
	register int	i;
	register int	*oldpcptr; 

	for (i = 0; i < num; i++) {

		if (fp > ap) 
			oldpcptr = (int *)(fp - 9 * sizeof(char *)); 
		else 
			oldpcptr = (int *)(sp - 2 * sizeof(char *)); 

		*pc = (caddr_t) *oldpcptr++; 
	
		if ((*pc < (caddr_t) KTXTSTRT) ||
			(*pc > (caddr_t) (KTXTSTRT + KTXTLEN))) {
			*pc++ = (caddr_t) NULL;
			break;
		}

		++pc;
		sp = ap; 
		ap = *oldpcptr++; 
		if (fp > sp) 
			fp = *oldpcptr; 
	} 

	while (i < num) {
		*pc++ = (caddr_t) NULL;
		++i;
	}
}

STATIC	int	kmacctsizes[] =
	{16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 0x7FFFFFFF};

 int
kmaccount(type, size, addr)
	int	type;
	size_t	size;
	caddr_t	*addr;
{
	register kmasym_t	*kp;
	register kmabuf_t	*bp, *hl;
	register int		class = 0;
	register int		oldpri;
	caddr_t			pc[MAXDEPTH+2];

	/*
	 * Find the class of buffer, which is used as an index into
	 * the accounting arrays.
	 */

	while (size > kmacctsizes[class])
		++class;

	switch (type) {

	case KMACCT_ALLOC:

		/*
		 * The first two entries in the pc stack will be 
		 * kmaccount() and kmem_alloc() or kmem_free();
		 * ignore those.
		 */

		getcaller(kmadepth+2, pc);

		/*
		 * Get the symbol table entry (possible new) that corresponds
		 * to this calling sequence.
		 */

		if ((kp = getsymentry(&pc[2])) == (kmasym_t *) NULL) {
			cmn_err(CE_WARN, "Not enough KMARRAY entries\n");
			return -1;
		}

		/*
		 * Get a buffer header from the freelist, initialize it,
		 * and add it to the proper hash chain.
		 */
		oldpri = splhi();
		if ((bp = freebuflist) == (kmabuf_t *) NULL) {
			cmn_err(CE_WARN, "Not enough KMBUFFER entries\n");
			return -1;
		}
		freebuflist = bp->next;
		splx(oldpri);

		bp->addr = addr;
		bp->kp = kp;

		/*
		 * Add buffer header to hash chain.
		 */
		oldpri = splhi();
		hl = &bufhash[KMBFHASH(addr)];
		bp->next = hl->next;
		if (hl->next->prev)
			hl->next->prev = bp;
		bp->prev = hl;
		hl->next = bp;
		splx(oldpri);

		/*
		 * Increment the counter for allocations of this class.
		 */

		++kp->reqa[class];
		break;

	case KMACCT_FREE:

		/*
		 * Look in the hash chain for the buffer header for this
		 * buffer.  It is not necessarily an error if a header 
		 * cannnot be found.
		 */
		oldpri = splhi();
		hl = &bufhash[KMBFHASH(addr)];
		bp = hl->next;
		while (bp != hl) {
			if (addr == (bp)->addr)
				break;
			bp = bp->next;
		}
		splx(oldpri);

		if (bp == hl)
			return 0;

		/*
		 * Increment the counter for frees of this class.
		 */

		++bp->kp->reqf[class];

		/*
		 * Remove buffer header bp from the hash chain.
		 */

		bp->prev->next = bp->next;
		bp->next->prev = bp->prev;

		KMBFFREE(bp);

		break;

	default:

		break;

	}

	return 0;
}
