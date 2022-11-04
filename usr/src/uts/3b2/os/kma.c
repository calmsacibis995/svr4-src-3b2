/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/kmem.c	1.43.1.1"

/*
 *	kernel memory allocator
 *
 *	public routines:
 *		kmem_init()		initialization
 *		kmem_alloc()		allocate memory
 *		kmem_zalloc()		allocate and zero-out memory
 *		kmem_fast_alloc()	quick allocation
 *		kmem_fast_zalloc()	quick zero'd-out allocation
 *		kmem_free()		free memory
 *		kmem_fast_free()	free memory from kmem_fast_[z]alloc
 *
 *	internal routines:
 *		kmem_allocpool()	allocate memory ("buffer") pool
 *		kmem_freepool()		free memory pool
 *		kmem_coalesce()		coalesce freed memory with "buddy"
 *
 *	STREAMS-only routines:	(for bufcall() compatibility)
 *		kmem_avail()		returns estimate of avail mem
 *					
 */

#define	PARANOID	1	/* include debugging/tracing		*/
				/*	this should eventually go in	*/
				/*	kmacct driver			*/

#include	"sys/types.h"
#include	"sys/param.h"
#include	"sys/sysmacros.h"
#include	"sys/immu.h"
#include	"sys/cmn_err.h"
#include	"sys/kmem.h"
#include	"sys/tuneable.h"
#include	"sys/debug.h"
#include	"sys/sysinfo.h"
#include	"sys/inline.h"
#include	"sys/stream.h"
#include	"sys/strsubr.h"
#include	"sys/systm.h"
#ifdef PARANOID
#include	"sys/kmacct.h"
#include	"sys/errno.h"
#endif /* PARANOID */

/******************************CONSTANTS*********************************/

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif /* !TRUE */

#ifndef SUCCESS
#define	SUCCESS	0
#define	TRYAGAIN	1
#define	FAILURE	-1
#endif /* !SUCCESS */

#define	SMALLBYTES	4096	/* small pool size in bytes		*/
#define	SMALLCLICKS	btoc(SMALLBYTES)	/* ... in pages		*/
#define	BIGBYTES	16384	/* big pool size in bytes		*/
#define	BIGCLICKS	btoc(BIGBYTES)		/* ... in pages		*/

#define	NIDLEP		32	/* size of list of free pools		*/

#ifndef _KERNEL
#define	MAXPOOLS	256	/* # pools for user-level debug trace	*/
#endif /* !_KERNEL */

#ifndef KM_NOSLEEP
#define	KM_NOSLEEP	1	/* must agree with NOSLEEP in sys/immu.h*/
#endif /* !KM_NOSLEEP */

/*
 *	A buffer's freestate is DELAYED if it is in the free list, but
 *	is still marked as allocated in that buffer pool's bitmap.
 */
#define	DELAYED	1

/*
 *	Have different classes of pools to handle different size buffer
 *	requests.  Use 2 classes: "small" and "big".
 *	Within the small pool, buffers vary in size from MINSMALL
 *	to MAXSMALL; in the big pool, they vary from MINBIG to MAXBIG.
 *	MINSMALL TO MAXBIG must be a continuum (no missing size in a power
 *	of 2).  However, we also define the allocable sizes from
 *	each pool class (infix 'A' in all parameter names).  Thus MINASMALL
 *	is the smallest allocable buffer, and MAXABIG is the largest
 *	allocable buffer.  We assume MAXASMALL == MAXSMALL and
 *	MINABIG == MINBIG (no internal holes).
 *
 *	Since freebuf structure (see below) is overlaid on buffers in
 *	free list, the smallest we're able to allocate is 16 bytes.
 *	However, keep MINSMALL set to 8 because the bitmap computations
 *	go more easily when 1 word in the bitmap corresponds to 1 MAX-size
 *	buffer, and 1 bit to 1 MIN-size buffer.  Since we want MAXSMALL 
 *	to be 256, MINSMALL must be 8.
 *
 *	The *IDX paramters are used to find the free list corresponding
 *	to buffers of particular size (*IDX = log2(size)).  Freelist[0] 
 *	corresponds to MINSMALL buffers;  since there is a continuum, use
 *	the exponents as a convenient way to reference the list. 
 *
 *	DO NOT CHANGE THESE LIGHTLY!! HIDDEN DEPENDENCIES LURK!!
 */

#define	MINSMALL	8	/* smallest buffer in small pool	*/
#define	MAXSMALL	256	/* largest buffer in small pool		*/
#define	MINBIG		512	/* smallest buffer in big pool		*/
#define	MAXBIG		16384	/* largest buffer in big pool		*/

#define	MINSIDX		3	/* exp of smallest class in small pool	*/
#define	MAXSIDX		8	/* exp of largest class in small pool	*/
#define	MINBIDX		9	/* exp of smallest class in big pool	*/
#define	MAXBIDX		14	/* exp of largest class in big pool	*/

#define	MINASMALL	16	/* smallest allocable in small pool	*/
#define	MAXASMALL	MAXSMALL/* largest allocable in small pool	*/
#define	MINABIG		MINBIG	/* smallest allocable in big pool	*/
#define	MAXABIG		4096	/* largest allocable in big pool	*/

#define	MINASIDX	4	/* exp of smallest allocable in small*/
#define	MAXASIDX	MAXSIDX	/* exp of largest allocable in small	*/
#define	MINABIDX	MINBIDX	/* exp of smallest allocable in big	*/
#define	MAXABIDX	12	/* exp of largest allocable in big	*/

/*	Index of list for smallest allocable buffer in small pool	*/
#define	SMINOFFSET	(MINASIDX - MINSIDX)
/*	Index of list for largest allocable buffer in small pool	*/
#define	SMAXOFFSET	(MAXASIDX - MINSIDX)

/*	Index of list for smallest allocable buffer in big pool	*/
#define	BMINOFFSET	(MINABIDX - MINSIDX)
/*	Index of list for largest allocable buffer in big pool	*/
#define	BMAXOFFSET	(MAXABIDX - MINSIDX)

/*
 *	Bitmaps:	BITMASK<i> masks off <i> bits
 */
#define	BITMASK1	(ulong)0x80000000
#define	BITMASK2	(ulong)0xc0000000
#define	BITMASK4	(ulong)0xf0000000
#define	BITMASK8	(ulong)0xff000000
#define	BITMASK16	(ulong)0xffff0000
#define	BITMASK32	(ulong)0xffffffff


/*
 *	Constants for hashing.  We hash on a buffer address to find
 *	which pool it's in.
 */
#define	SHIFT		14	/* log2(BIGBYTES)			*/
#define	NHASH		128	/* # hash table entries. must be power	*/
				/* of 2.				*/




/******************************STRUCTURES********************************/

/*
 *	The freebuf structure is overlaid on every buffer in the free
 *	list, so we don't use extra space for buffer headers.  This
 *	limits the minimum allocable buffer size to 16 bytes.
 */

typedef struct fbuf {
	struct fbuf	*fb_nextp;	  /* next free buffer		*/
	struct fbuf	*fb_prevp;	  /* previous free buffer	*/
	ulong		fb_mask;	  /* mask to apply to bitmap	*/
	union {
		struct bpool	*fb_poolp; /* corresp. pool		*/
		ulong		fb_state;  /* DELAYED or not		*/
		ulong		*fb_mapp;  /* corresp. word of bitmap	*/
	} fb_union;
} freebuf;

/*
 *	The freelist structure is the free list header for every class
 *	(size) of buffer.  Maintains pointers and state information
 *	(counters) about the class.
 */

typedef struct flist {
	freebuf		*fl_nextp;	/* first buffer in free list	*/
	freebuf		*fl_prevp;	/* last buffer in free list	*/
	ulong		fl_slack;	/* state var. for memory mgmt	*/
	ulong		fl_mask;	/* mask to apply to bitmap	*/
	ulong		fl_nbits;	/* # of bits to mask on/off	*/
} freelist;

/*
 *	Structure for buffer pools.
 *
 *	We keep count of the number of buffers from the largest class
 *	in the pool that are in use.  Whenever a MAXA* buffer is allocated
 *	(including when it is split to satisfy smaller requests), increment
 *	the counter.  When a MAXA* is freed, decrement.  When the counter
 *	goes to zero, the pool may be freed.
 *
 */

typedef struct bpool {
	unchar		*bp_startp;	/* unaligned start address	*/
	unchar		*bp_alignp;	/* aligned start address	*/
	ulong		bp_inuse;	/* # of MAXA* buffers in use	*/
	ulong		*bp_bitmapp;	/* pointer to bitmap		*/
	ulong		bp_expmin;	/* exponent of smallest class	*/
	ulong		bp_expmax;	/* exponent of largest class	*/
} bufpool;

/*
 *	Hash structure
 *	To make the VALID check more efficient, we duplicate the pool
 *	(aligned) start and end address in the hashpool structure.
 */

typedef struct hpool {
	bufpool		*hp_poolp;	/* pool that owns this link	*/
	unchar		*hp_alignp;	/* buffer pool start address	*/
	unchar		*hp_endp;	/* buffer pool end address	*/
	struct hpool	*hp_nextp;	/* next pool in hash chain	*/
	struct hpool	*hp_prevp;	/* prev pool in hash chain	*/
} hashpool;


/*******************************MACROS***********************************/

/*	Buffer free list macros (macros rather than routines for speed)	*/

/*
 *	Free BUFHDR to the head of LIST
 */

#define	HEADFREE(list, bufhdr) \
		(bufhdr)->fb_nextp = (list)->fl_nextp; \
		(bufhdr)->fb_nextp->fb_prevp = (bufhdr); \
		(bufhdr)->fb_prevp = (freebuf *)(list); \
		(list)->fl_nextp = (bufhdr);


/*
 *	Free BUFHDR to tail of LIST.
 */

#define	TAILFREE(list, bufhdr) \
		(bufhdr)->fb_prevp = (list)->fl_prevp; \
		(bufhdr)->fb_prevp->fb_nextp = (bufhdr); \
		(bufhdr)->fb_nextp = (freebuf *)(list); \
		(list)->fl_prevp = (bufhdr);

/*
 *	Take BUFHDR from (the middle of) its list
 */

#define	UNLINK(bufhdr) \
		(bufhdr)->fb_nextp->fb_prevp = (bufhdr)->fb_prevp; \
		(bufhdr)->fb_prevp->fb_nextp = (bufhdr)->fb_nextp;

/*
 *	Take BUFHDR from the head of the list LIST
 */

#define	UNLINKHEAD(list, bufhdr) \
		(list)->fl_nextp = (bufhdr)->fb_nextp; \
		(bufhdr)->fb_nextp->fb_prevp = (freebuf *)(list);



/*	Pool hash list macros	*/

/*
 *	Calculate the hash bucket for the pool that contains
 *	buffer with address A
 */

#define	HASH(a)		((((ulong)a) >> SHIFT) & (NHASH - 1))


/*
 *	Test if address A is truly in hashpool H
 */

#define	VALID(a, h) \
	((h)->hp_alignp <= (unchar *)(a) && (unchar *)(a) < (h)->hp_endp)

/*
 *	Add hashpool P to the hash chain under hash bucket I
 *	NOTE: no check on valid i -- caller must do it.
 */

#define	ADDHASH(p, i) \
			if ( Km_HashLists[i] != (hashpool *)NULL ) \
				Km_HashLists[i]->hp_prevp = (p); \
			(p)->hp_nextp = Km_HashLists[i]; \
			(p)->hp_prevp = (hashpool *)NULL; \
			Km_HashLists[i] = (p);


/*
 *	Remove pool P from hash chain
 *	NOTE: no check on valid i -- caller must do it.
 */

#define	DELHASH(p, i) \
			if ( (p)->hp_prevp ) \
				(p)->hp_prevp->hp_nextp = (p)->hp_nextp; \
			if ( (p)->hp_nextp ) \
				(p)->hp_nextp->hp_prevp = (p)->hp_prevp; \
			if ( (p) == Km_HashLists[(i)] ) \
				Km_HashLists[(i)] = (p)->hp_nextp;



/*
 *	Find the pool P containing address A
 *	Uses extern Km_Hp as hash pointer.
 */

#define	LOOKUP(a, p) \
			for ( p = (bufpool *)NULL, \
			  Km_Hp = Km_HashLists[HASH(a)]; \
			Km_Hp != (hashpool *)NULL; \
			Km_Hp = Km_Hp->hp_nextp ) { \
				if ( VALID(a, Km_Hp) ) { \
					p = Km_Hp->hp_poolp; \
					break; \
				} \
			}


/******************************EXTERNAL VARIABLES************************/

/*
 *	The Km_FreeLists array heads the list of free buffers.  It has
 *	one structure element for each size of memory we can allocate,
 *	from MINSMALL to MAXBIG, in power of 2.  The layout is:
 *	    <first free><last free><#slack><mask><nmaskbits>
 *	The free buffers are in a doubly-linked list, initially
 *	empty (hence first and last both point to the head of the list).
 */
STATIC freelist Km_FreeLists[] = {

		{ (freebuf *)&Km_FreeLists[0],	/* MINSMALL	*/
		  (freebuf *)&Km_FreeLists[0],
			0, BITMASK1, 1 },
		{ (freebuf *)&Km_FreeLists[1],	/* MINASMALL	*/
		  (freebuf *)&Km_FreeLists[1],
			0, BITMASK2, 2 },
		{ (freebuf *)&Km_FreeLists[2],
		  (freebuf *)&Km_FreeLists[2],
			0, BITMASK4, 4 },
		{ (freebuf *)&Km_FreeLists[3],
		  (freebuf *)&Km_FreeLists[3],
			0, BITMASK8, 8 },
		{ (freebuf *)&Km_FreeLists[4],
		  (freebuf *)&Km_FreeLists[4],
			0, BITMASK16, 16 },
		{ (freebuf *)&Km_FreeLists[5],	/* MAXSMALL, MAXASMALL	*/
		  (freebuf *)&Km_FreeLists[5],
			0, BITMASK32, 32 },
		{ (freebuf *)&Km_FreeLists[6],	/* MINBIG, MINABIG	*/
		  (freebuf *)&Km_FreeLists[6],
			0, BITMASK1, 1 },
		{ (freebuf *)&Km_FreeLists[7],
		  (freebuf *)&Km_FreeLists[7],
			0, BITMASK2, 2 },
		{ (freebuf *)&Km_FreeLists[8],
		  (freebuf *)&Km_FreeLists[8],
			0, BITMASK4, 4 },
		{ (freebuf *)&Km_FreeLists[9],	/* MAXABIG	*/
		  (freebuf *)&Km_FreeLists[9],
			0, BITMASK8, 8 },
		{ (freebuf *)&Km_FreeLists[10],
		  (freebuf *)&Km_FreeLists[10],
			0, BITMASK16, 16 },
		{ (freebuf *)&Km_FreeLists[11],	/* MAXBIG	*/
		  (freebuf *)&Km_FreeLists[11],
			0, BITMASK32, 32 }
};



STATIC hashpool	*Km_HashLists[NHASH];
				/* array of hash buckets; the pools that*/
				/*	hash to bucket i are doubly	*/
				/*	linked under Km_HashLists[i].	*/
STATIC hashpool	*Km_Hp;		/* hash ptr used in LOOKUP macro	*/
STATIC bufpool	*Km_NewSmall;	/* most recently allocated small pool	*/
STATIC bufpool	*Km_NewBig;	/* most recently allocated big pool	*/
STATIC bufpool	*Km_Idlep[NIDLEP];/* pools to be freed			*/
STATIC bufpool	*Km_Idle2p[NIDLEP];/* dup Km_Idle - see kmem_freepool	*/
STATIC int	Km_IdleI;	/* index into Km_Idlep			*/
STATIC int	Km_IdleJ;	/* index into Km_Idle2p			*/
STATIC int	Km_SmallWanted;	/* min small request slept for		*/
STATIC int	Km_BigWanted;	/* min big request slept for		*/
STATIC int	Km_OsizeWanted;	/* min outsize request slept for	*/

/*	
 * 	use flags to keep track of pool allocation/freeing.
 *	otherwise would have to splhi large pieces of code.
 */
STATIC int	Km_SmAllocOn;	/* TRUE -> are in the middle of		*/
				/*	allocating a small pool		*/
STATIC int	Km_BgAllocOn;	/* TRUE -> are in the middle of		*/
				/*	allocating a big pool		*/
STATIC int	Km_FreeOn;	/* TRUE -> are in the middle of		*/
				/*	freeing a pool			*/

int	Km_pools[KMEM_NCLASS];	/* # of pools in each class	*/
int	Km_allocspool;		/* # of calls to kmem_allocspool */
int	Km_allocbpool;		/* # of calls to kmem_allocbpool */
int	Km_freepool;		/* # of calls to kmem_freepool */

/*
 *	To prevent thrashing under light traffic, we allocate "golden"
 *	buffers, one from the most recently allocated pool in each class.
 *	Since these are the first buffers allocated, they are at the
 *	high end of each pool and thus cause minimal fragmentation.
 */
unchar	*Km_Golden[2];

#ifdef PARANOID
/*
 * if Km_Paranoid is set, will call kmem_check() to check freelist headers
 * on entering and leaving kmem_alloc(), kmem_free(), and kmem_coalesce().
 * Km_Paranoid may be set from demon proms ("sw Km_Paranoid 1").
 * if the freelist is trashed, will call demon.
 * if Km_Paranoid and Km_CheckAll both are set, it will check the entire
 * freelist, not just the headers.
 *
 * if kmacctflag is set, the KMACCT driver will collect counts of memory
 * usage to detect memory allocated but never freed.
 *
 * if Km_TraceAddr is set, will call demon when allocate or free that addr.
 *
 */
int	Km_Paranoid = FALSE;
int	Km_CheckAll = FALSE;
int	kmacctflag = FALSE;	/* for KMACCT driver	*/
int	Km_Recurse;		/* count recursion depth in kmem_coalsece */
unchar	*Km_TraceAddr = (unchar *)NULL;

/*
 * use Km_Alloc and Km_Free to keep track of who alloc'd & freed addrs.
 * have NREC of each -- will wrap when full.
 */

#define NUMCALL 6
#define NREC    500

struct callrec {
	caddr_t	caller[NUMCALL];
	unchar	*addr;
	ulong	size;
} Km_Alloc[NREC+1], Km_Free[NREC+1];

STATIC int      Km_i, Km_j;	/* indices for Km_Alloc & Km_Free resp. */
STATIC int      Km_iwrap, Km_jwrap;	/* wrap flags	*/
STATIC int      Km_Low = MINASMALL;
STATIC int      Km_High = MAXABIG;
STATIC int	Km_Max = 5; 	/* max number of alloc/frees to print */

#endif /* PARANOID */


/*	kmem routines	*/
void		kmem_init();
_VOID		*kmem_alloc();
_VOID		*kmem_zalloc();
_VOID		*kmem_fast_alloc();
_VOID		*kmem_fast_zalloc();
void		kmem_free();
void		kmem_fast_free();
void		kmem_freepool(); /* called from main, so not static */
ulong		kmem_avail();	 /* only for STREAMS bufcall support */
STATIC int	kmem_allocspool();
STATIC int	kmem_allocbpool();
STATIC void	kmem_coalesce();

#ifdef PARANOID
void		kmem_check();	/* check freelist 			*/
void     	kmem_trace();	/* trace usage of an address		*/
int		cntausers();	/* count allocations of an addr		*/
int		cntfusers();	/* count free's of an addr		*/
void		cntusers();	/* count alloc's and free's of an addr	*/
void		prausers();	/* print alloc's of an addr		*/
void		prfusers();	/* print free's of an addr		*/
void		prusers();	/* print alloc's and free's of an addr	*/
STATIC void	prarange();	/* internal routine for print trace	*/
STATIC void	prfrange();	/* internal routine for print trace	*/
STATIC void     kmem_getcaller();/* get stack trace			*/
void		kmem_trace();	/* trace address usage			*/

extern int	s3blookup();	/* to get symbol names (is in DEBUG)	*/
#endif /* PARANOID */


/*	external variables	*/
extern int	availrmem;
extern int	availsmem;
extern u_int 	pages_pp_kernel;
extern char	qrunflag;

/*	external routines	*/
extern int	sptalloc();
extern int	sptfree();

/*
 * kmem_init	initialize memory manager
 *		run from main at boot time
 *
 *		call kmem_alloc[sb]pool to allocate 1 pool of each size
 *		small pool *MUST* be allocated before big, because
 *		will call kmem_alloc() to get memory for structs and
 *		bitmap for big pool.
 *		allocate a "golden" buffer from each pool to prevent
 *		thrashing under light load.
 */
void
kmem_init()
{
	if ( kmem_allocspool(KM_NOSLEEP) != SUCCESS )
		cmn_err(CE_PANIC,
		"kmem_init: failed to allocate small pool\n");
	Km_Golden[0] = (unchar *)kmem_alloc(MINASMALL, KM_NOSLEEP);
	if ( kmem_allocbpool(KM_NOSLEEP) != SUCCESS )
		cmn_err(CE_PANIC,
		"kmem_init: failed to allocate big pool\n");
	Km_Golden[1] = (unchar *)kmem_alloc(MINABIG, KM_NOSLEEP);
	return;
}


/*
 * kmem_allocspool		allocate small buffer pool
 *
 * input:
 *	flags		flags (see below)
 * return codes:
 *	SUCCESS		ok
 *	TRYAGAIN	in the middle of allocating a pool -- try again later
 *	FAILURE		fall on sword
 *
 * Get a chunk of memory from system page allocator, align both ends,
 * set up bufpool & hashpools structure, link onto the hash chain.
 * Walk thru the memory, breaking it into MAXA* pieces (largest allocable size),
 * and putting these pieces on the MAXA* free list.
 *
 * flags:
 *	KM_NOSLEEP		do not sleep
 *
 * Note:
 *	Much of the code is 3B2-dependent.
 *	Aligning the start and end of the buffer pool causes the pool
 *	to be slightly smaller than SMALLBYTES.
 */

STATIC int
kmem_allocspool(flags)
int	flags;		/* flags	*/
{

	register freelist	*listp;	/* ptr to corres. free list	*/
	register bufpool	*poolp;	/* ptr to corres. pool		*/
	register hashpool	*hashp;	/* ptr to hash chain		*/
	register unchar		*memp;	/* ptr to current buffer	*/
	register unchar		*startp;/* save unaligned start  	*/
	register unchar		*endp;	/* ptr to end of buffer space	*/
	ulong			*wordp;	/* bitmap ptr for current buf	*/
	int			 oldpri;/* save interrupt priority	*/
	int			 hashi;	/* hash index			*/
	int			 hashj;	/* hash index			*/

	++Km_allocspool;	/* count calls to kmem_allocspool */
	/*
	 * if are already allocating a pool -- don't start a new one
	 */
	if ( Km_SmAllocOn == TRUE )
		 return(TRYAGAIN);
	Km_SmAllocOn = TRUE;

	/*
	 *	Set up the buffer pool and "free" the memory
	 */


	if ( ((availrmem - SMALLCLICKS) < tune.t_minarmem) ||
	    ((availsmem - SMALLCLICKS) < tune.t_minasmem) ) {
		Km_SmAllocOn = FALSE;
		wakeup((caddr_t)&Km_SmAllocOn);
		return(FAILURE);
	}
	availrmem -= SMALLCLICKS;
	availsmem -= SMALLCLICKS;

	if ( !(memp = (unchar *)sptalloc(SMALLCLICKS, PG_V, 0,
	(flags & NOSLEEP))) ) {
		availrmem += SMALLCLICKS;
		availsmem += SMALLCLICKS;
		Km_SmAllocOn = FALSE;
		wakeup((caddr_t)&Km_SmAllocOn);
		return(FAILURE);
	}
	pages_pp_kernel += SMALLCLICKS;
	startp = memp;

	/* set list ptr to free list for MAXASMALL bufs	*/
	listp = Km_FreeLists + SMAXOFFSET;

	/*	Truncate any odd-aligned ending to the pool */
	endp = (unchar *) (((ulong)memp + SMALLBYTES) &
					(~(MAXASMALL - 1)));

	/*	Align the start of the pool */
	memp = (unchar *) (((ulong)memp + MAXASMALL - 1) &
					(~(MAXASMALL - 1)));

	/*
	 * Take 2 buffers off the top: one to use as bufpool
	 * and hashpool structs, the other as bitmap.
	 * This will work so long as 
	 *	(sizeof(bufpool) + 
	 *	2*sizeof(hashpool)) <= MAXASMALL
	 * and
	 *	SMALLBYTES/(MINSMALL*8) <= MAXASMALL
	 *
	 * (need one bit in bitmap for each min-size buffer
	 * in pool, so to get bitmap size in bytes we divide 
	 * pool size by (min buffer size * 8).)
	 *
	 * In other words, dependencies lurk.
	 *
	 */
	poolp = (bufpool *)memp;
	wordp = poolp->bp_bitmapp = (ulong *)((ulong)memp + MAXASMALL);
	poolp->bp_startp = startp;
	poolp->bp_alignp = memp;
	poolp->bp_inuse = 0;
	poolp->bp_expmin = MINSIDX;
	poolp->bp_expmax = MAXSIDX;

	/*
	 * Every pool is hashed into at least 1 list.
	 * If the hash function of the end address is different
	 * than that of the start, create another hash structure
	 * and add it under the second hash chain.
	 * This double-hashing at init avoids having to 
	 * look thru 2 hashchains in LOOKUP during
	 * alloc's & free's.
	 */
	hashi = HASH(startp);
	hashj = HASH(startp + SMALLBYTES - 1); /* hash unaligned end */
	hashp = (hashpool *)((ulong)memp + sizeof(bufpool));
	hashp->hp_poolp = poolp;
	hashp->hp_alignp = memp;
	hashp->hp_endp = endp;
	oldpri = splhi();
	ADDHASH(hashp, hashi);
	if ( hashi != hashj ) {
		hashp = (hashpool *)((ulong)hashp + sizeof(hashpool));
		hashp->hp_poolp = poolp;
		hashp->hp_alignp = memp;
		hashp->hp_endp = endp;
		ADDHASH(hashp, hashj);
	}

	/*
	 * Skip memp over the 2 buffers we've used
	 */
	memp += (2 * MAXASMALL);

	/*
	 * "Free" the new memory.
	 * Since the buffer are max size and cannot be coalesced,
	 * their state is assumed to be DELAYED and fb_union
	 * is used to cache pool pointer.
	 */
	while ( memp < endp ) {
		HEADFREE(listp, (freebuf *)memp);
		((freebuf *)memp)->fb_union.fb_poolp = poolp;
		memp += MAXASMALL;
	}
	/*
	 * All maxsize bufs are DELAYED, so mark busy in bitmap
	 */
	memp = (unchar *)(wordp + (SMALLBYTES/MAXSMALL));
	do {
		*wordp = BITMASK32;
	} while ( ++wordp < (ulong *)memp );
	splx(oldpri);

	/*
	 * init counters, remembering that we've stolen 2 buffers
	 */
	kmeminfo.km_mem[KMEM_SMALL] += 
		(ulong)endp - (ulong)poolp->bp_alignp - (2*MAXASMALL);
	++Km_pools[KMEM_SMALL];
	Km_NewSmall = poolp;

	Km_SmAllocOn = FALSE;
	wakeup((caddr_t)&Km_SmAllocOn);
	return(SUCCESS);
}

/*
 * kmem_allocbpool		allocate big buffer pool
 *
 * input:
 *	flags		flags (see below)
 * return codes:
 *	SUCCESS		ok
 *	TRYAGAIN	in the middle of allocating a pool -- try again later
 *	FAILURE		fall on sword
 *
 * Get a chunk of memory from system page allocator, align both ends,
 * set up bufpool & hashpools structure, link onto the hash chain.
 * Walk thru the memory, breaking it into MAXA* pieces (largest allocable size),
 * and putting these pieces on the MAXA* free list.
 *
 * flags:
 *	KM_NOSLEEP		do not sleep
 *
 * Note:
 *	Much of the code is 3B2-dependent.
 *	Aligning the start and end of the buffer pool causes the pool
 *	to be slightly smaller than BIGBYTES.
 */

STATIC int
kmem_allocbpool(flags)
int	flags;		/* flags	*/
{

	register freelist	*listp;	/* ptr to corres. free list	*/
	register bufpool	*poolp;	/* ptr to corres. pool		*/
	register hashpool	*hashp;	/* ptr to hash chain		*/
	register unchar		*memp;	/* ptr to current buffer	*/
	register unchar		*startp;/* save unaligned start  	*/
	register unchar		*endp;	/* ptr to end of buffer space	*/
	ulong			*wordp;	/* bitmap ptr for current buf	*/
	int			 oldpri;/* save interrupt priority	*/
	int			 hashi;	/* hash index			*/
	int			 hashj;	/* hash index			*/

	++Km_allocbpool;	/* count calls to kmem_allocbpool */
	/*
	 * if are already allocating a pool -- don't start a new one
	 */
	if ( Km_BgAllocOn == TRUE )
		 return(TRYAGAIN);
	Km_BgAllocOn = TRUE;

	/*
	 *	Set up the buffer pool and "free" the memory
	 */


	if ( ((availrmem - BIGCLICKS) < tune.t_minarmem) ||
	     ((availsmem - BIGCLICKS) < tune.t_minasmem) ) {
		Km_BgAllocOn = FALSE;
		wakeup((caddr_t)&Km_BgAllocOn);
		return(FAILURE);
	}
	availrmem -= BIGCLICKS;
	availsmem -= BIGCLICKS;

	if ( !(memp = (unchar *)sptalloc(BIGCLICKS, PG_V, 0,
	(flags & NOSLEEP))) ) {
		availrmem += BIGCLICKS;
		availsmem += BIGCLICKS;
		Km_BgAllocOn = FALSE;
		wakeup((caddr_t)&Km_BgAllocOn);
		return(FAILURE);
	}
	startp = memp;

	/* set list ptr to free list for MAXABIG bufs	*/
	listp = Km_FreeLists + BMAXOFFSET;

	/*	Truncate any odd-aligned ending to the pool */
	endp = (unchar *) (((ulong)memp + BIGBYTES) &
					(~(MAXABIG - 1)));

	/*	Align the start of the pool */
	memp = (unchar *) (((ulong)memp + MAXABIG - 1) &
					(~(MAXABIG - 1)));

	/*
	 * Get space for bufpool struct and bitmap from kmem_alloc().
	 */
	if ( (poolp = (bufpool *)kmem_alloc( (sizeof(bufpool) +
	(2 * sizeof(hashpool)) + (BIGBYTES/(MINBIG*8))), flags))
	   == (bufpool *)NULL ) {
		sptfree((caddr_t)memp, BIGCLICKS,1);
		availrmem += BIGCLICKS;
		availsmem += BIGCLICKS;
		Km_BgAllocOn = FALSE;
		wakeup((caddr_t)&Km_BgAllocOn);
		return(FAILURE);
	}
	pages_pp_kernel += BIGCLICKS;
	hashp = (hashpool *)((ulong)poolp + sizeof(bufpool));
	wordp = poolp->bp_bitmapp =
		(ulong *)((ulong)hashp + (2 * sizeof(hashpool)));
	poolp->bp_startp = startp;
	poolp->bp_alignp = memp;
	poolp->bp_inuse = 0;
	poolp->bp_expmin = MINBIDX;
	poolp->bp_expmax = MAXBIDX;

	/*
	 * Every pool is hashed into at least 1 list.
	 * If the hash function of the end address is different
	 * than that of the start, create another hash structure
	 * and add it under the second hash chain.
	 * This double-hashing at init avoids having to 
	 * look thru 2 hashchains in LOOKUP during
	 * alloc's & free's.
	 */
	hashi = HASH(startp);
	hashj = HASH(startp + BIGBYTES - 1); /* hash unaligned end */
	hashp->hp_poolp = poolp;
	hashp->hp_alignp = memp;
	hashp->hp_endp = endp;
	oldpri = splhi();
	ADDHASH(hashp, hashi);
	if ( hashi != hashj ) {
		hashp = (hashpool *)((ulong)hashp + sizeof(hashpool));
		hashp->hp_poolp = poolp;
		hashp->hp_alignp = memp;
		hashp->hp_endp = endp;
		ADDHASH(hashp, hashj);
	}

	/*
	 * "Free" the new memory.
	 * Since the buffer are max size and cannot be coalesced,
	 * their state is assumed to be DELAYED and fb_union
	 * is used to cache pool pointer.
	 */
	while ( memp < endp ) {
		HEADFREE(listp, (freebuf *)memp);
		((freebuf *)memp)->fb_union.fb_poolp = poolp;
		memp += MAXABIG;
	}
	/*
	 * All maxsize bufs are DELAYED, so mark busy in bitmap
	 */
	memp = (unchar *)(wordp + (BIGBYTES/MAXBIG));
	do {
		*wordp = BITMASK32;
	} while ( ++wordp < (ulong *)memp );
	splx(oldpri);

	/*
	 * init counters
	 */
	kmeminfo.km_mem[KMEM_LARGE] += 
		(ulong)endp - (ulong)poolp->bp_alignp;
	++Km_pools[KMEM_LARGE];
	Km_NewBig = poolp;

	Km_BgAllocOn = FALSE;
	wakeup((caddr_t)&Km_BgAllocOn);
	return(SUCCESS);
}

/*
 * kmem_alloc		allocate memory
 *
 * input:
 *	size		request size in bytes
 *	flags		flags (see below)
 * returns:
 *	memory address
 *	NULL on failure
 *
 * If requested buffer size is bigger than MAXABIG, allocate it directly
 * and return it.
 * Else, search the freelist for a buffer of the requested size.
 * If found, return it.
 * Else search thru freelist for a bigger buffer.  Split larger buffer
 * into "buddies" until get one of the desired size.  Return it.
 * If there are no larger buffers in pool, allocate a new pool.
 * If new pool alloc fails, return NULL.  Otherwise, repeat above with
 * new pool.
 *
 * flags:
 *	KM_NOSLEEP		do not sleep
 */

_VOID *
kmem_alloc(size, flags)
size_t	size;		/* request size in bytes		*/
int	flags;		/* flags				*/
{

	register freelist	*listp;	/* free list ptr	*/
	register freebuf	*bufp;	/* freebuf to return 	*/
	register size_t		tmpsiz;	/* size of current list	*/
	register size_t		newsiz;	/* stop-splitting size	*/
	ulong			max;	/* max buf size for pool*/
	int			oldpri;	/* save priority	*/
	int			poolrtn;/* result of pool alloc	*/


	if ( size == 0 )
		return((_VOID *)NULL);
#ifdef PARANOID
	if ( Km_Paranoid )
		kmem_check("entering kmem_alloc", Km_CheckAll);
#endif /* PARANOID */
	if ( size <= MAXASMALL ) {
		listp = Km_FreeLists + SMINOFFSET;
		tmpsiz = MINASMALL;
		max = MAXASMALL;
	} else if ( size <= MAXABIG ) {
		listp = Km_FreeLists + BMINOFFSET;
		tmpsiz = MINABIG;
		max = MAXABIG;
	} else {
		/*
		 *	outside big pool limit - allocate directly
		 */

		ulong	 clicks;
		clicks = btoc(size); /*	convert size to pages */
  osizeagain:
		if ( ((availrmem - clicks) < tune.t_minarmem) ||
	     	((availsmem - clicks) < tune.t_minasmem) ) {
			if ( !(flags & KM_NOSLEEP) ) {
				/* out of memory -- sleep for it */
				if ( size < Km_OsizeWanted ||
				Km_OsizeWanted == 0 )
					Km_OsizeWanted = size;
				(void) sleep((caddr_t)&Km_OsizeWanted, PZERO);
				goto osizeagain;
			}
#ifdef PARANOID
			if ( Km_Paranoid )
				kmem_check("leaving kmem_alloc", Km_CheckAll);
#endif /* PARANOID */
			return((_VOID *)NULL);
		}

		availrmem -= clicks;
		availsmem -= clicks;

		if ( !(bufp = (freebuf *)sptalloc((int)clicks, PG_V, 0, 
		(flags & NOSLEEP))) ) {
			availrmem += clicks;
			availsmem += clicks;
			if ( !(flags & KM_NOSLEEP) ) {
				/* out of memory -- sleep for it */
				if ( size < Km_OsizeWanted ||
				Km_OsizeWanted == 0 )
					Km_OsizeWanted = size;
				(void) sleep((caddr_t)&Km_OsizeWanted, PZERO);
				goto osizeagain;
			}
			++kmeminfo.km_fail[KMEM_OSIZE];
#ifdef PARANOID
			if ( Km_Paranoid )
				kmem_check("leaving kmem_alloc", Km_CheckAll);
#endif /* PARANOID */
			return((_VOID *)NULL);
		}
		pages_pp_kernel += clicks;
		kmeminfo.km_alloc[KMEM_OSIZE] += (ulong)ctob(clicks);
#ifdef PARANOID
		if ( Km_Paranoid ) {
			int	i;

			kmem_check("leaving kmem_alloc", Km_CheckAll);
			if ( size >= Km_Low && size <= Km_High ) {
				if ( (i = Km_i++) >= NREC ) {
					Km_iwrap++;
					i = Km_i = 0;
				}
				kmem_getcaller(NUMCALL, Km_Alloc[i].caller);
				Km_Alloc[i].size = size;
				Km_Alloc[i].addr = (unchar *)bufp;
			}
		}
		if ( kmacctflag )
			kmaccount(KMACCT_ALLOC, size, (caddr_t *)bufp);
		if ( bufp == (freebuf *)Km_TraceAddr ) {
			cmn_err(CE_NOTE,
			"^kmem_alloc: returning traced address 0x%x\n",
			bufp);
			call_demon();
		}
#endif /* PARANOID */
		return((_VOID *)bufp);
	}

  allocTop:
	while ( tmpsiz < size ) {	/* find correct freelist	*/
		tmpsiz <<= 1;
		++listp;
	}

	oldpri = splhi();

  allocMaxagain:
	/*
	 *	If there is a buffer on the correct-size free list, take it.
	 *	If it is not DELAYED (max size buffers are by definition
	 *	DELAYED), mark it allocated in bitmap.
	 *	fb_poolp will have been init'd in kmem_allocpool() or
	 *	kmem_coalesce().  fb_mapp, fb_mask will have been init'd
	 *	in kmem_coalesce().
	 */
#ifdef PARANOID
	if ( Km_Paranoid )
		kmem_check("kmem_alloc @allocMaxagain", Km_CheckAll);
#endif /* PARANOID */
	if ( (bufp = listp->fl_nextp) != (freebuf *)listp ) {
		if ( tmpsiz < max && bufp->fb_union.fb_state != DELAYED ) {
			*bufp->fb_union.fb_mapp |= bufp->fb_mask;
			++listp->fl_slack;
		} else {
			listp->fl_slack += 2;
			/*
			 * If is in largest class for this pool,
			 * bump inuse counter.
			 */
			if ( tmpsiz == max )
				++(bufp->fb_union.fb_poolp->bp_inuse);
		}

		UNLINKHEAD(listp, bufp);
		splx(oldpri);

		if ( max == MAXASMALL )
			kmeminfo.km_alloc[KMEM_SMALL] += (ulong)tmpsiz;
		else
			kmeminfo.km_alloc[KMEM_LARGE] += (ulong)tmpsiz;

#ifdef PARANOID
		if ( Km_Paranoid ) {
			int	i;

			kmem_check("leaving kmem_alloc", Km_CheckAll);
			if ( size >= Km_Low && size <= Km_High ) {
				if ( (i = Km_i++) >= NREC ) {
					Km_iwrap++;
					i = Km_i = 0;
				}
				kmem_getcaller(NUMCALL, Km_Alloc[i].caller);
				Km_Alloc[i].size = size;
				Km_Alloc[i].addr = (unchar *)bufp;
			}
		}
		if ( kmacctflag )
			kmaccount(KMACCT_ALLOC, size, (caddr_t *)bufp);
		if ( bufp == (freebuf *)Km_TraceAddr ) {
			cmn_err(CE_NOTE,
			"^kmem_alloc: returning traced address 0x%x\n",
			bufp);
			call_demon();
		}
#endif /* PARANOID */
		return((_VOID *)bufp);
	}

	/*
	 *	Otherwise, we have to look in the free lists of larger
	 *	buffers for one to split into appropriately sized pieces.
	 *	Search until we find a free buffer or until we run
	 *	out of lists.
	 */
	newsiz = tmpsiz;
	while ( tmpsiz != max ) {
		
		tmpsiz <<= 1;
		++listp;

  allocagain:
#ifdef PARANOID
		if ( Km_Paranoid )
			kmem_check("kmem_alloc @allocagain", Km_CheckAll);
#endif /* PARANOID */
		if ( (bufp = listp->fl_nextp) != (freebuf *)listp ) {
			/*
			 * We found a buffer.  Take it from the list.
			 * If it's not delayed, mark it allocated in bitmap.
			 */
			UNLINKHEAD(listp, bufp);
			if ( tmpsiz < max && bufp->fb_union.fb_state != DELAYED ) {
				*bufp->fb_union.fb_mapp |= bufp->fb_mask;
				++listp->fl_slack;
			} else {
				listp->fl_slack += 2;
				if ( tmpsiz == max )
					++(bufp->fb_union.fb_poolp->bp_inuse);
			}
				

			/*
			 * Split it until the buddies are the correct size,
			 * putting one buddy back on free list and continuing
			 * to split the other.
			 */
			do {
				--listp;
				tmpsiz >>= 1;;
				HEADFREE(listp, bufp);
				bufp->fb_union.fb_state = DELAYED;
				bufp = (freebuf *) ((ulong)bufp + tmpsiz);
			} while ( tmpsiz != newsiz );

			splx(oldpri);
			if ( max == MAXASMALL )
				kmeminfo.km_alloc[KMEM_SMALL] += (ulong)newsiz;
			else
				kmeminfo.km_alloc[KMEM_LARGE] += (ulong)newsiz;
#ifdef PARANOID
			if ( Km_Paranoid ) {
				int	i;

				kmem_check("leaving kmem_alloc", Km_CheckAll);
				if ( size >= Km_Low && size <= Km_High ) {
					if ( (i = Km_i++) >= NREC ) {
						Km_iwrap++;
						i = Km_i = 0;
					}
					kmem_getcaller(NUMCALL,
						Km_Alloc[i].caller);
					Km_Alloc[i].size = size;
					Km_Alloc[i].addr = (unchar *)bufp;
				}
			}
			if ( kmacctflag )
				kmaccount(KMACCT_ALLOC, size, (caddr_t *)bufp);
			if ( bufp == (freebuf *)Km_TraceAddr ) {
				cmn_err(CE_NOTE,
				"^kmem_alloc: returning traced address 0x%x\n",
				bufp);
				call_demon();
			}
#endif /* PARANOID */
			return((_VOID *)bufp);

		}
	}
	splx(oldpri);

	/*
	 *	We've run out of buffers.
	 *	Allocate new pool and try again.
	 */
	if ( size <= MAXASMALL ) {
  newspool:
		if ( (poolrtn = kmem_allocspool(flags)) == TRYAGAIN
		&& !(flags & KM_NOSLEEP) ) {
			/*
			 * were in the middle of allocating a pool -- hang out
			 * for a bit, then try again
			 */
			(void) sleep((caddr_t)&Km_SmAllocOn, PZERO);
			goto newspool;
		}
		if ( poolrtn == SUCCESS ) {
			/*
			 * we know that all buffers (big enough to
			 * satisfy the outstanding request) are on
			 * the MAXASMALL freelist.  tmpsiz and listp
			 * will already have correct values, so just
			 * splhi and jump back in the middle of it.
			 */
			oldpri = splhi();
			if ( newsiz < MAXASMALL )
				goto allocagain;
			goto allocMaxagain;
		}
		if ( !(flags & KM_NOSLEEP) ) {
			/*
			 * we're out of memory, but we can sleep until it's
			 * freed up.   Km_SmallWanted is the minimum for which
			 * small-request proceesses should be awakened.
			 * We may be awakened by kmem_free() (i.e., new
			 * memory may be on a list we've already looked at)
			 * so we have to go back to the beginning.
			 */
			if ( size < Km_SmallWanted || Km_SmallWanted == 0 )
				Km_SmallWanted = size;
			(void) sleep((caddr_t)&Km_SmallWanted, PZERO);
			tmpsiz = MINASMALL;
			listp = Km_FreeLists + SMINOFFSET;
			goto allocTop;
		}
		++kmeminfo.km_fail[KMEM_SMALL];
	} else {
 newbpool:
		if ( (poolrtn = kmem_allocbpool(flags)) == TRYAGAIN
		&& !(flags & KM_NOSLEEP) ) {
			/*
			 * were in the middle of allocating a pool -- hang out
			 * for a bit, then try again
			 */
			(void) sleep((caddr_t)&Km_BgAllocOn, PZERO);
			goto newbpool;
		}
		if ( poolrtn == SUCCESS ) {
			/*
			 * we know that all buffers (big enough to
			 * satisfy the outstanding request) are on
			 * the MAXABIG freelist.  tmpsiz and listp
			 * will already have correct values, so just
			 * splhi and jump back in the middle of it.
			 */
			oldpri = splhi();
			if ( newsiz < MAXABIG )
				goto allocagain;
			goto allocMaxagain;
		}
		if ( !(flags & KM_NOSLEEP) ) {
			/*
			 * we're out of memory, but we can sleep until it's
			 * freed up.   Km_BigWanted is the minimum for which
			 * big-request proceesses should be awakened.
			 * We may be awakened by kmem_free() (i.e., new
			 * memory may be on a list we've already looked at)
			 * so we have to go back to the beginning.
			 */
			 if ( size < Km_BigWanted || Km_BigWanted == 0 )
				Km_BigWanted = size;
			(void) sleep((caddr_t)&Km_BigWanted, PZERO);
			tmpsiz = MINABIG;
			listp = Km_FreeLists + BMINOFFSET;
			goto allocTop;
		}
		++kmeminfo.km_fail[KMEM_LARGE];
	}

	/*
	 *	We can't get another buffer pool, and we can't sleep
	 *	until memory is freed up.
	 */
#ifdef PARANOID
	if ( Km_Paranoid )
		kmem_check("leaving kmem_alloc", Km_CheckAll);
#endif /* PARANOID */
	return((_VOID *)NULL);
}

/*
 * kmem_zalloc		allocate and zero-out memory
 *
 * input:
 *	size		request size in bytes
 *	flags		flags (see below)
 * returns:
 *	memory address
 * flags:
 *	KM_NOSLEEP		do not sleep
 */
_VOID *
kmem_zalloc(size, flags)
size_t	size;		/* request size in bytes		*/
int	flags;		/* flags				*/
{
	caddr_t res;

	if ( (res = (caddr_t)kmem_alloc(size, flags)) != (caddr_t)NULL )
		bzero(res, size);
	return((_VOID *)res);
}

/*
 * The kmem_fast_* routines are for quickly allocating and freeing memory in
 * some commonly used size.  The chunks argument is used to reduce the
 * number of calls to kmem_alloc, and to reduce memory fragmentation.
 * The base argument is a caller allocated caddr_t * which is the base
 * of the free list of pieces of memory.  None of this memory is ever
 * freed, so these routines should be used only for structures that
 * will be reused often.
 */
_VOID *
kmem_fast_alloc(base, size, chunks, flags)
caddr_t	*base;	/* holds linked list of freed items */
size_t	size;	/* size of each item - constant for given base */
int	chunks;	/* number of items to alloc when needed */
int	flags;	/* flags (KM_SLEEP or KM_NOSLEEP) */
{
	caddr_t	p;

	if (*base == 0) {	/* no free chunks */
		if ( (p = (caddr_t)kmem_alloc((size_t)(size * chunks), flags))
		== (caddr_t)NULL)
			return((_VOID *)NULL);
		while (--chunks >= 0) {
			*(caddr_t *)(p + chunks*size) = *base;
			*base = (p + chunks*size);
		}
	}
	p = *base;
	*base = *(caddr_t *)p;
	return ((_VOID *)p);
}

void
kmem_fast_free(base, p)
caddr_t *base, p;
{

	*(caddr_t *)p = *base;
	*base = p;
}

/*
 * Like kmem_fast_alloc, but bzero the memory upon allocation.
 */
_VOID *
kmem_fast_zalloc(base, size, chunks, flags)
caddr_t *base;	/* holds linked list of freed items */
size_t	size;	/* size of each item - constant for given base */
int	chunks;	/* number of items to alloc when needed */
int	flags;	/* flags (KM_SLEEP or KM_NOSLEEP) */
{
	caddr_t res;

	if ( (res = (caddr_t)kmem_fast_alloc(base, size, chunks, flags))
	!= (caddr_t)NULL )
		bzero(res, size);
	return((_VOID *)res);
}

/*
 * kmem_free		release memory allocated via kmem_alloc()
 *
 * input:
 *	addr		address to be freed
 *	size		size (in bytes)
 * return:
 *	none
 *
 * if addr is outsize buffer, free it directly and return.
 * else, find freelist for this buffer.
 * if is in lazy state, free "locally": return to freelist, set state
 * to DELAYED, don't update bitmap.
 * if in reclaiming or accelerated state, call kmem_coalesce to merge
 * with its buddy.
 * if in accelerated state and if there's another DELAYED buffer on the
 * freelist, call kmem_coalesce to coalesce that one, too.
 * return.
 */
void
kmem_free(addr, size)
_VOID	*addr;	/* address to be freed	*/
size_t	size;	/* size (in bytes)	*/
{

	register freebuf	*bufp;	/* overlay freebuf on addr */
	register freebuf	*buf2p;	/* freebuf for accelerated */
					/*     mem. coalescing	   */
	register freelist	*listp;	/* corresponding freelist  */
	register size_t		tmpsiz;	/* size for list	   */
	ulong			max;	/* max size for list	   */
	int			oldpri;	/* save priority	   */
	/*
	 *	for STREAMS bufcall support
	 */
	extern char		strbcflag; /* bufcall functions ready to go */
	extern char		strbcwait; /* bufcall functions waiting     */


	if ( addr == (_VOID *)NULL || size == 0)
		return;
	bufp = (freebuf *)addr;

#ifdef PARANOID
	if ( Km_Paranoid ) {
		int	j;

		kmem_check("entering kmem_free", Km_CheckAll);
		if ( size >= Km_Low && size <= Km_High ) {
			if ( (j = Km_j++) >= NREC ) {
				Km_jwrap++;
				j = Km_j = 0;
			}
			kmem_getcaller(NUMCALL, Km_Free[j].caller);
			Km_Free[j].size = size;
			Km_Free[j].addr = (unchar *)bufp;
		}
	}
	if ( kmacctflag )
		kmaccount(KMACCT_FREE, size, (caddr_t *)addr);
	if ( bufp == (freebuf *)Km_TraceAddr ) {
		cmn_err(CE_NOTE,
		"^kmem_free: freeing traced address 0x%x\n",
		bufp);
		call_demon();
	}
#endif /* PARANOID */


	if ( size > MAXABIG ) {
		/*
		 *	was allocated directly -- free the same way
		 */
		ulong	clicks;

		/*
		 * address must be aligned on page boundary
		 */
		if ( ((ulong)bufp & (ulong)(PAGESIZE-1)) != 0 )
			cmn_err(CE_PANIC,
			    "kmem_free: buffer 0x%x (size 0x%x bytes) not aligned for PAGESIZE 0x%x\n",
			    bufp, size, PAGESIZE);

		clicks = btoc(size);
		sptfree((caddr_t)addr, (int)clicks, 1);
		availrmem += clicks;
		availsmem += clicks;
		pages_pp_kernel -= clicks;
		kmeminfo.km_alloc[KMEM_OSIZE] -= (ulong)ctob(clicks);
		/*
		 *	we released memory, so wake up anyone who's
		 *	waiting on it (including STREAMS bufcall functions)
		 */
		if ( Km_OsizeWanted > 0 && Km_OsizeWanted <= size ) {
			Km_OsizeWanted = 0;
			wakeup((caddr_t)&Km_OsizeWanted);
		}
		if ( strbcwait && !strbcflag ) {
			setqsched();
			strbcflag = TRUE;
		}
#ifdef PARANOID
		if ( Km_Paranoid )
			kmem_check("leaving kmem_free", Km_CheckAll);
#endif /* PARANOID */
		return;
	}

	if ( size <= MAXASMALL ) {
		listp = Km_FreeLists + SMINOFFSET;
		tmpsiz = MINASMALL;
		max = MAXASMALL;
	} else {
		listp = Km_FreeLists + BMINOFFSET;
		tmpsiz = MINABIG;
		max = MAXABIG;
	}

	while ( tmpsiz < size ) {	/* find correct free list	*/
		tmpsiz <<= 1;
		++listp;
	}

	/*
	 * address to be freed must be properly aligned.
	 */
	ASSERT(((ulong)bufp & (ulong)(tmpsiz-1)) == 0);

	if ( size <= MAXASMALL )
		kmeminfo.km_alloc[KMEM_SMALL] -= (ulong)tmpsiz;
	else
		kmeminfo.km_alloc[KMEM_LARGE] -= (ulong)tmpsiz;

	oldpri = splhi();
	/* 
	 *	Buffer state is set to DELAYED if the class is in "lazy" mode.
	 *	Since they can't be coalesced, buffers from the largest class
	 *	are always DELAYED.
	 */
	if ( (listp->fl_slack >= 2 ) || (tmpsiz == max) ) {
		listp->fl_slack -= 2;
		HEADFREE(listp, bufp);
		if ( tmpsiz < max )
			bufp->fb_union.fb_state = DELAYED;
		else {
			/*
			 * if all MAXA* buffers have been freed, and if
			 * this isn't the most recently allocated pool in
			 * this class, free the pool
			 */
			LOOKUP(addr, bufp->fb_union.fb_poolp);
			if ( bufp->fb_union.fb_poolp == (bufpool *)NULL )
				cmn_err(CE_PANIC,
				"kmem_free %d: lookup failed addr 0x%x size %u\n",
				__LINE__, addr, size);
			if ( --(bufp->fb_union.fb_poolp->bp_inuse) == 0 &&
			bufp->fb_union.fb_poolp != Km_NewSmall &&
			bufp->fb_union.fb_poolp != Km_NewBig ) {
				if ( Km_IdleI < NIDLEP )
					Km_Idlep[Km_IdleI++] =
						bufp->fb_union.fb_poolp;
#ifdef DEBUG
				else
					cmn_err(CE_WARN,
					"kmem_free: idle list full\n");
#endif /* DEBUG */
				wakeup((caddr_t)Km_Idlep);
			}
		}
	} else {
		/*
		 * not in lazy mode, not maxsize buffer.
		 * Coalesce this buffer with its buddy.
		 * If after that we're in "accelerated" mode and there's
		 * a DELAYED buffer on this freelist, coalesce that one, too.
		 */
		kmem_coalesce(bufp, listp, (ulong)tmpsiz, max);

		if ( listp->fl_slack == 0
		&&  (buf2p = listp->fl_nextp) != (freebuf *)listp
		&& buf2p->fb_union.fb_state == DELAYED ) {
				UNLINKHEAD(listp, buf2p);
				kmem_coalesce(buf2p, listp, (ulong)tmpsiz, max);
		} else
			listp->fl_slack = 0;

	}
	splx(oldpri);

	/*
	 *	we released memory, so wake up anyone who's waiting on it
	 *	and arrange for STREAMS to wake up bufcall functions.
	 */
	if ( size <= MAXASMALL ) {
		if ( Km_SmallWanted > 0 && Km_SmallWanted <= size ) {
			Km_SmallWanted = 0;
			wakeup((caddr_t)&Km_SmallWanted);
		}
	} else if ( Km_BigWanted > 0 && Km_BigWanted <= size ) {
		Km_BigWanted = 0;
		wakeup((caddr_t)&Km_BigWanted);
	}
	if ( strbcwait && !strbcflag ) {
		setqsched();
		strbcflag = TRUE;
	}

#ifdef PARANOID
	if ( Km_Paranoid )
		kmem_check("leaving kmem_free", Km_CheckAll);
#endif /* PARANOID */
	return;
}



/*
 * kmem_coalesce	coalesce buddies into larger buffers
 *
 * input:
 *	bufp		ptr to buffer to coalesce
 *	listp		ptr to corresponding free list
 *	tmpsiz		size of list
 *	max		max allocable size in this class
 * return: none
 *
 * Given a free buffer, try to merge it with its buddy, then merge
 * that larger buffer with its buddy, and so on.  End when merge
 * into a class that's in lazy mode, or when can't merge because
 * the buddy's busy, or when merge into MAXA* class.
 *
 * If are in accelerated mode, each time we merge a buddy, we check
 * the new size freelist for another idle buffer and try to coalesce
 * that one, too.
 *
 */

STATIC void
kmem_coalesce(bufp, listp, tmpsiz, max)
register freebuf	*bufp;	/* ptr to buffer to coalesce	  */
register freelist	*listp;	/* ptr to corresponding free list */
ulong			tmpsiz;	/* size of list			  */
ulong			max;	/* max size for class		  */
{
	register ulong		offset;	/* how far addr is into pool	*/
	register ulong		shift;	/* how much to shift list mask	*/
	register ulong		mask;	/* correctly-shifted mask	*/
	register bufpool	*poolp; /* ptr to corresp. pool		  */
	ulong			*wordp;	/* which word of pool bitmap	*/
	ulong			bshift;	/* how far to shift buddy's mask*/
	ulong			bmask;	/* correctly-shifted buddy mask	*/
	freebuf			*buf2p;	/* for accelerated coalescing	*/

#ifdef PARANOID
	++Km_Recurse;
	if ( Km_Paranoid )
		kmem_check("entering kmem_coalesce", Km_CheckAll);
#endif /* PARANOID */
	/*
	 * find the associated pool
	 * compute the mask for this buffer
	 */
	LOOKUP(bufp, poolp);
	if ( poolp == (bufpool *)NULL )
		cmn_err(CE_PANIC,
		  "kmem_coalesce %d: lookup failed addr 0x%x size %u\n",
		  __LINE__, bufp, tmpsiz);
	offset = (unchar *)bufp - poolp->bp_alignp;
	shift = (offset >> poolp->bp_expmin) & 0x1f;
	wordp = (ulong *)(poolp->bp_bitmapp + (offset >> poolp->bp_expmax));
	mask = listp->fl_mask >> shift;

  freeagain:

#ifdef PARANOID
	if ( Km_Paranoid )
		kmem_check("kmem_coalesce @freeagain", Km_CheckAll);
#endif /* PARANOID */

	/*
	 * Compute the mask for the buddy
	 */
	bshift = shift ^ listp->fl_nbits;
	bmask = listp->fl_mask >> bshift;

	if ( !(*wordp & bmask ) ) {
		/*
		 *	The buddy isn't allocated -- coalesce.
		 *	Take buddy off free list, get ptr to coalesced buffer,
		 *	create its mask, free it to next larger list.
		 */
		bufp = (freebuf *)((ulong)bufp ^ tmpsiz);
		UNLINK(bufp);

		mask |= bmask;
		shift &= bshift;
		bufp = (freebuf *)((ulong)bufp & (~tmpsiz));
		tmpsiz <<= 1;
		++listp;

		/*
		 *	If in "lazy" mode or if is maxsize buffer, set
		 *	state to DELAYED and mark it allocated in bitmap.
		 */
		if ( (listp->fl_slack >= 2) || (tmpsiz == max) ) {
			*wordp |= mask;
			HEADFREE(listp, bufp);
			if ( tmpsiz < max ) {
				bufp->fb_union.fb_state = DELAYED;
				listp->fl_slack -= 2;
			} else {
				/*
				 * if pool is idle, free it
				 */
				bufp->fb_union.fb_poolp = poolp;
				if ( --(poolp->bp_inuse) == 0 &&
				poolp != Km_NewSmall && poolp != Km_NewBig ) {
					if ( Km_IdleI < NIDLEP )
						Km_Idlep[Km_IdleI++] = poolp;
#ifdef DEBUG
					else
						cmn_err(CE_WARN,
						"kmem_coalesce: idle list full\n");
#endif /* DEBUG */
					wakeup((caddr_t)Km_Idlep);
				}
			}
			/*
			 * if anyone's waiting on memory, wake 'em up
			 */
			if ( max == MAXASMALL ) {
				if ( Km_SmallWanted > 0 &&
				Km_SmallWanted <= tmpsiz ) {
					Km_SmallWanted = 0;
					wakeup((caddr_t)&Km_SmallWanted);
				}
			} else if ( Km_BigWanted > 0 && Km_BigWanted <= tmpsiz ) {
				Km_BigWanted = 0;
				wakeup((caddr_t)&Km_BigWanted);
			}
		} else {
			/*
			 * not in lazy mode, not maxsize buffer
			 * If in "accelerated" mode and there's a delayed
			 * buffer on the free list, coalesce it.
			 * At any rate, continue to coalesce the present
			 * (bufp) buffer.
			 */
			if ( listp->fl_slack == 0 
			&& (buf2p = listp->fl_nextp) != (freebuf *)listp
			&& buf2p->fb_union.fb_state == DELAYED ) {
					UNLINKHEAD(listp, buf2p);
					kmem_coalesce(buf2p, listp, tmpsiz, max);
			} else
				listp->fl_slack = 0;
			goto freeagain;
		}
#ifdef PARANOID
		if ( Km_Paranoid )
			kmem_check("leaving kmem_coalesce", Km_CheckAll);
		--Km_Recurse;
#endif /* PARANOID */
		return;
	}
	/*
	 *	The buddy is allocated -- can't coalesce.
	 *	Mark this one unallocated in bitmap and add to tail
	 *	of freelist (so we won't re-allocate it quickly).
	 *	Save the laboriously computed bitmap pointer 
	 *	and mask so we don't have to do all that again.
	 */
	*wordp &= ~mask;
	bufp->fb_union.fb_mapp = wordp;
	bufp->fb_mask = mask;
	TAILFREE(listp, bufp);
#ifdef PARANOID
	if ( Km_Paranoid )
		kmem_check("leaving kmem_coalesce", Km_CheckAll);
	--Km_Recurse;
#endif /* PARANOID */
	return;
}

/*
 * kmem_freepool		free buffer pool
 *
 * kmem_freepool sleeps, waiting to free buffer pools.
 *
 * to free pool:
 * find corrresponding freelist (MAXASMALL or MAXBIG).
 * walk thru it, checking each freebuf on the list to see whether it's
 * in the about-to-be-freed pool.  if yes, remove it from the list.
 * find the pool's hash structure(s), remove from hash chain.
 * free the pool.
 * if it's a big pool, free the structures and bitmap. (if it's a small
 * one, freeing the pool itself also frees the structs and bitmap.)
 */
void
kmem_freepool()
{
	register bufpool	*poolp;	/* free pool pointer		*/
	register freebuf	*bufp;	/* free buffer pointer		*/
	register freelist	*listp;	/* free list pointer		*/
	register hashpool	*hashp;	/* hash pool pointer		*/
	ulong			max;	/* max size for this pool	*/
	ulong			psize;	/* aligned pool size		*/
	int			i, j;	/* hash indices			*/
	int			oldpri;	/* save priority		*/

	for ( ;; ) {
	    ++Km_freepool;	/* count "calls" to kmem_freepool() */
	    /*
	     * if already in process of freeing a pool, don't start again
	     */
	    if ( Km_FreeOn == TRUE )
	    	goto fp_sleep;
	    Km_FreeOn = TRUE;

	    /*
	     *	copy the pools to be freed from Km_Idlep to Km_Idle2p.
	     *	if we didn't do this, we'd have to splhi the entire
	     *	freepool while-loop to keep other processes from changing
	     *	Km_Idlep or Km_IdleI while we're using 'em.
	     */
	    oldpri = splhi();
	    for ( Km_IdleJ = 0; Km_IdleJ < Km_IdleI; ++Km_IdleJ )
		Km_Idle2p[Km_IdleJ] = Km_Idlep[Km_IdleJ];
	    Km_IdleI = 0;
	    splx(oldpri);

	    while ( --Km_IdleJ >= 0 ) {

		if ( (poolp = Km_Idle2p[Km_IdleJ]) == (bufpool *)NULL )
			continue;	/* while */

		/*
		 *	find max-size freelist for this class pool
		 */
		switch ( poolp->bp_expmin ) {

		case MINSIDX:
			listp = Km_FreeLists + SMAXOFFSET;
			max = MAXASMALL;
			break;
		case MINBIDX:
			listp = Km_FreeLists + BMAXOFFSET;
			max = MAXABIG;
			break;
		default:
			cmn_err(CE_PANIC,
				"kmem_freepool: bad pool ptr 0x%x\n",
				poolp);
		}

		oldpri = splhi();

		if ( poolp->bp_inuse > 0 ) {
			splx(oldpri);
			continue;	/* while */
		}

		/*
		 * walk thru max-size freelist, checking each free
		 * buffer to see whether it's in the to-be-freed pool.
		 * if it is, remove it from the list.
		 */
		for ( bufp = listp->fl_nextp; bufp != (freebuf *)listp;
		bufp = bufp->fb_nextp ) {
			if ( bufp->fb_union.fb_poolp == poolp ) {
				UNLINK(bufp);
			}
		}

		splx(oldpri);

		/*	find hash structure for this pool */
		hashp = (hashpool *)NULL;
		i = HASH(poolp->bp_startp);
		if ( max == MAXABIG )
			j = HASH(poolp->bp_startp + BIGBYTES - 1);
		else
			j = HASH(poolp->bp_startp + SMALLBYTES - 1);
		for ( hashp = Km_HashLists[i];
		hashp != (hashpool *)NULL && hashp->hp_poolp != poolp;
		hashp = hashp->hp_nextp )
			continue;
		if ( hashp == (hashpool *)NULL ) {
			cmn_err(CE_PANIC,
			"kmem_freepool %d: no hashpool in HashList[%d] for pool 0x%x\n",
			__LINE__, i, poolp);
		}
		psize = (ulong)(hashp->hp_endp) - (ulong)(hashp->hp_alignp);
		oldpri = splhi();
		DELHASH(hashp, i);
		splx(oldpri);
		if ( j != i ) {
			for ( hashp = Km_HashLists[j];
			hashp != (hashpool *)NULL && hashp->hp_poolp != poolp;
			hashp = hashp->hp_nextp )
				continue;
			if ( hashp == (hashpool *)NULL ) {
				cmn_err(CE_PANIC,
				"kmem_freepool %d: no hashpool in HashList[%d] for pool 0x%x\n",
				__LINE__, j, poolp);
			}
			oldpri = splhi();
			DELHASH(hashp, j);
			splx(oldpri);
		}

		if ( max == MAXABIG ) {
			sptfree((caddr_t)poolp->bp_startp, BIGCLICKS, 1);
			availrmem += BIGCLICKS;
			availsmem += BIGCLICKS;
			pages_pp_kernel -= BIGCLICKS;

			/*
			 * must free bufpool struct, hashpool structs,
			 * and bitmap that were gotten from kmem_alloc()
			 */
			kmem_free((_VOID *)poolp, 
				(sizeof(bufpool) + (2*sizeof(hashpool)) +
						(BIGBYTES/(MINBIG*8))));
			kmeminfo.km_mem[KMEM_LARGE] -= psize;
			--Km_pools[KMEM_LARGE];
		} else {
			/*
			 * freeing the pool will also free associated
			 * structures and bitmap
			 */
			sptfree((caddr_t)poolp->bp_startp, SMALLCLICKS, 1);
			availrmem += SMALLCLICKS;
			availsmem += SMALLCLICKS;
			pages_pp_kernel -= SMALLCLICKS;

			kmeminfo.km_mem[KMEM_SMALL] -= psize;
			--Km_pools[KMEM_SMALL];
		}
		if ( Km_OsizeWanted > 0 && Km_OsizeWanted <= psize ) {
			/*
			 * won't have any small or big requests sleeping if we
			 * have idle pools, but there could be outsize requests
			 * waiting for memory
			 */
			Km_OsizeWanted = 0;
			wakeup((caddr_t)&Km_OsizeWanted);
		}
		poolp = (bufpool *)NULL;
	    }	/* end "while ( --Km_IdleJ >= 0 )" */
	    Km_FreeOn = FALSE;
fp_sleep:
	    wakeup((caddr_t)&Km_SmAllocOn);	/* insurance against race */
	    wakeup((caddr_t)&Km_BgAllocOn);	/* insurance against race */
	    sleep((caddr_t)Km_Idlep, PZERO);
	}
}


/*
 * kmem_avail		return estimate of available memory
 *
 * This routine is provided only to support STREAMS bufcall() functionality.
 * Returns:	memory available in small & big pools, plus availrmem
 *		(since we'll attempt to allocate more pools if necessary).
 */
ulong
kmem_avail()
{
	return((ulong)( ctob(availrmem - tune.t_minarmem)
		+ (kmeminfo.km_mem[KMEM_SMALL] - kmeminfo.km_alloc[KMEM_SMALL])
		+ (kmeminfo.km_mem[KMEM_LARGE] - kmeminfo.km_alloc[KMEM_LARGE]) ));
}


#ifdef PARANOID
/*
 * kmem_check(s, all)		freelist check
 * char	s;		text to print on error
 * int	all;		whether to walk entire freelist or just headers
 *
 * for debugging, to be called from demon debugger ("c kmem_check u.u_comm 0"
 * or "c kmem_check u.u_comm 1")
 *
 * also called on entry to or exit from kmem_alloc, kmem_free, kmem_coealesce
 * if Km_Paranoid is set.
 *
 * walks freelist, looking for trashed pointers.
 * if all is set, walks entire freelist.  if not, just looks at head & tail
 * pointers (fl_nextp and fl_prevp) off Km_FreeLists.
 *
 * if any pointer is trashed, prints warning and calls demon.
 * only check pointers that are not one of the elements of the Km_FreeLists
 * array.  "trashed" if it's not properly aligned or if it's < 0x40000000
 * or > 0x40500000
 *
 */

#define	ADDR1	(freebuf *)0x40000000
#define	ADDR2	(freebuf *)0x40500000
/* BADADDR1 -- address must be 'size'-aligned */
#define BADADDR1(x)  ( ( (x) < (freebuf *)&Km_FreeLists[SMINOFFSET] \
		        || (x) > (freebuf *)&Km_FreeLists[BMAXOFFSET] ) \
		      && ( ((ulong)(x) & (ulong)(size-1)) != 0 \
		           || ((x) < ADDR1) || ((x) > ADDR2) ) )
/* BADADDR2 -- only word alignment matters */
#define BADADDR2(x)  ( ((ulong)(x) & (ulong)3) != 0 \
		           || ((x) < ADDR1) || ((x) > ADDR2) )

/* is this the size request we are interested in */
#define SIZECHECK(s,b) ((s==0) || ((b <= s) && (b > (s/2))))

int	Km_DebugInst = NULL;	/* set if DEBUG module is in kernel */
				/* after check, will be ENOPKG if   */
				/* DEBUG is not in kernel           */

void
kmem_check(s, all)
char	*s;
int	all;
{
	register freebuf	*head, *tail, *p;
	register int		i, size;
	int			bad, stop = 0, oldpri;
	void			why1(), why2();

	for ( size = MINASMALL, i = SMINOFFSET;
	      size <= MAXABIG && i <= BMAXOFFSET; ++i, size *= 2 ) {


		/*
		 * have to spl this or interrupts cause bogus failures
		 */
		oldpri = splhi();

		if ( (head = Km_FreeLists[i].fl_nextp) == (freebuf *)&Km_FreeLists[i] ) {
			splx(oldpri);
			continue;	/* empty list -- continue in 'for' */
		}
		tail = Km_FreeLists[i].fl_prevp;
		bad = 0; 	/* innocent until proven guilty */

		if ( BADADDR1(head) ) {
			cmn_err(CE_WARN,
			"^%s: %u-byte freelist header trashed:\n\t\tfl_nextp = 0x%x\n\t\tfl_prevp = 0x%x\n",
			s, size, head, tail);
			why1(head, size);
			bad++;
		} else if ( BADADDR1(tail) ) {
			cmn_err(CE_WARN,
			"^%s: %u-byte freelist header trashed:\n\t\tfl_nextp = 0x%x\n\t\tfl_prevp = 0x%x\n",
			s, size, head, tail);
			why1(tail, size);
			bad++;
		} else {
			if ( BADADDR1(head->fb_nextp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist head pointer (fl_nextp) trashed:\n\t\tfl_nextp = 0x%x\n\t\tfl_nextp->fb_nextp = 0x%x\n\t\tfl_nextp->fb_prevp = 0x%x\n",
				s, size, head, head->fb_nextp, head->fb_prevp);
				why1(head->fb_nextp, size);
				bad++;
			}
			if ( BADADDR1(head->fb_prevp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist head pointer (fl_nextp) trashed:\n\t\tfl_nextp = 0x%x\n\t\tfl_nextp->fb_nextp = 0x%x\n\t\tfl_nextp->fb_prevp = 0x%x\n",
				s, size, head, head->fb_nextp, head->fb_prevp);
				why1(head->fb_prevp, size);
				bad++;
			}
			if ( BADADDR1(tail->fb_nextp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist tail pointer (fl_prevp) trashed:\n\t\tfl_prevp = 0x%x\n\t\tfl_prevp->fb_nextp = 0x%x\n\t\tfl_prevp->fb_prevp = 0x%x\n",
				s, size, tail, tail->fb_nextp, tail->fb_prevp);
				why1(tail->fb_nextp, size);
				bad++;
			}
			if ( BADADDR1(tail->fb_prevp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist tail pointer (fl_prevp) trashed:\n\t\tfl_prevp = 0x%x\n\t\tfl_prevp->fb_nextp = 0x%x\n\t\tfl_prevp->fb_prevp = 0x%x\n",
				s, size, tail, tail->fb_nextp, tail->fb_prevp);
				why1(tail->fb_prevp, size);
				bad++;
			}


			/* if union holds a pool or bitmap pointer, check it */
			if ( head->fb_union.fb_state != DELAYED
			&& BADADDR2((freebuf *)head->fb_union.fb_mapp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist head pointer (fl_nextp) trashed:\n\t\tfl_nextp = 0x%x\n\t\tfl_nextp->fb_fb_union.fb_%s = 0x%x\n",
				s, size, head,
				( ( size == MAXASMALL || size == MAXABIG) ?
					"poolp" : "mapp" ),
				head->fb_union.fb_mapp);
				why2(head->fb_union.fb_mapp);
				bad++;
			}
			if ( tail->fb_union.fb_state != DELAYED
			&& BADADDR2((freebuf *)tail->fb_union.fb_mapp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist tail pointer (fl_nextp) trashed:\n\t\tfl_nextp = 0x%x\n\t\tfl_nextp->fb_fb_union.fb_%s = 0x%x\n",
				s, size, tail,
				( ( size == MAXASMALL || size == MAXABIG) ?
					"poolp" : "mapp" ),
				tail->fb_union.fb_mapp);
				why2(tail->fb_union.fb_mapp);
				bad++;
			}

		}
		stop += bad;
		if ( bad || !all ) {	/* don't go further with bad pointers */
			splx(oldpri);
			continue;	/* for */
		}

		/* walk entire freelist */
		for ( p = head->fb_nextp; p != (freebuf *)&Km_FreeLists[i];
		      p = p->fb_nextp ) {
			if ( BADADDR1(p->fb_nextp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist trashed:\n\t\tbufp = 0x%x\n\t\tbufp->fb_nextp = 0x%x\n\t\tbufp->fb_prevp = 0x%x\n",
				s, size, p, p->fb_nextp, p->fb_prevp);
				why1(p->fb_nextp, size);
				bad++;
				break;
			}
			if ( BADADDR1(p->fb_prevp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist trashed:\n\t\tbufp = 0x%x\n\t\tbufp->fb_nextp = 0x%x\n\t\tbufp->fb_prevp = 0x%x\n",
				s, size, p, p->fb_nextp, p->fb_prevp);
				why1(p->fb_prevp, size);
				bad++;
				break;
			}
			if ( p->fb_union.fb_state != DELAYED
			&& BADADDR2((freebuf *)p->fb_union.fb_mapp) ) {
				cmn_err(CE_WARN,
				"^%s: %u-byte freelist trashed:\n\t\tfl_nextp = 0x%x\n\t\tfl_nextp->fb_fb_union.fb_%s = 0x%x\n",
				s, size, p,
				( ( size == MAXASMALL || size == MAXABIG) ?
					"poolp" : "mapp" ),
				p->fb_union.fb_mapp);
				why2(p->fb_union.fb_mapp);
				bad++;
			}
		}
		splx(oldpri);
		stop += bad;
	}
	if (stop)
		call_demon();
	return;
}

void
why1(x, size)
freebuf *x;
int	size;
{
	cmn_err(CE_CONT, "^kmem_check failed this test:\n");
	cmn_err(CE_CONT,
"^( ( (x) < (freebuf *)&Km_FreeLists[SMINOFFSET]\\\n");
	cmn_err(CE_CONT,
"^    || (x) > (freebuf *)&Km_FreeLists[BMAXOFFSET] ) \\\n");
	cmn_err(CE_CONT,
"^ && ( ((ulong)(x) & (ulong)(size-1)) != 0 \\\n");
	cmn_err(CE_CONT,
"^      || ((x) < ADDR1) || ((x) > ADDR2) ) )\n");
	cmn_err(CE_CONT,
"^( ( 0x%x < 0x%x\\\n", x, &Km_FreeLists[SMINOFFSET]);
	cmn_err(CE_CONT,
"^    || 0x%x > 0x%x ) \\\n", x, &Km_FreeLists[BMAXOFFSET]);
	cmn_err(CE_CONT,
"^ && ( ((ulong)0x%x & (ulong)(0x%x)) != 0 \\\n", x, (size-1));
	cmn_err(CE_CONT,
"^      || (0x%x < 0x%x) || (0x%x > 0x%x) ) )\n", x, ADDR1, x, ADDR2);

	return;
}


void
why2(x)
ulong *x;
{
	cmn_err(CE_CONT, "^kmem_check failed this test:\n");
	cmn_err(CE_CONT,
"^( ((ulong)(x) & (ulong)3) != 0 \\\n");
	cmn_err(CE_CONT,
"^  || ((x) < ADDR1) || ((x) > ADDR2) )\n");
	cmn_err(CE_CONT,
"^( (0x%x & (ulong)3) != 0 \\\n", x);
	cmn_err(CE_CONT,
"^  || (0x%x < 0x%x) || (0x%x > 0x%x) )\n", x, ADDR1, x, ADDR2);

	return;
}

/*
 * kmem_trace(addr, flag)	trace/count uses of addr
 *
 * if flag is  0 then print out all uses of addr (and before) addr;
 * if flag is !0 then print out number of uses of addr (and before) addr;
 */
void
kmem_trace(addr, flag)
register uint	addr;
int		flag;
{
	register uint	size, mask;
	register uint	baddr;

	/* since we are only saving records for this range
	 * we can only find matches in this range
	 */
	for ( size = Km_Low;  size <= Km_High; size *= 2 ) {

		/*
		 * if addr is on boundary x
		 * 	check(addr, x)
		 *	check(addr-x, x)
		 *	NOTE not checking addr + x
		 * else addr is inside an x-byte buffer
		 *	check(addr&(x-1), x);
		 */

		mask = ~(size - 1);
		baddr = (uint)addr & (uint)mask;
		if ( baddr == addr)  {

			if (flag != 0) {  /* just do count */
				cntusers(addr, size);
				cntusers(addr-size, size);
			} else {
				prausers(addr, size);
				prfusers(addr, size);
				prausers(addr-size, size);
				prfusers(addr-size, size);
			}
		} else {
			if (flag != 0) {  /* just do count */
				cntusers(baddr, size);
			} else {
				prusers(baddr, size);
			}
		}

	}
}

/* cntusers -- count alloc/free's of addr */
void
cntusers(addr, size)
uint addr, size;
{
	register int cnt;
	if (cnt = cntausers(addr, size))
		cmn_err(CE_CONT, "^%d kmem_alloc of size %d at addr 0x%x\n",
			cnt, size, addr);

	if (cnt = cntfusers(addr, size))
		cmn_err(CE_CONT, "^%d kmem_free  of size %d at addr 0x%x\n",
			cnt, size, addr);
}


/* count allocs */
int
cntausers(addr, size)
uint	addr, size;
{
	register int i, cnt = 0;

	for ( i = 0; i < NREC; i++ ) {
		if ((Km_Alloc[i].addr == (unchar *)addr) &&
		    SIZECHECK(size, Km_Alloc[i].size)) {
			cnt++;
		}
	}
	return cnt;
}

/* count frees */
int
cntfusers(addr, size)
uint	addr, size;
{
	register int i, cnt = 0;
	for ( i = 0; i < NREC; i++ ) {
		if ((Km_Free[i].addr == (unchar *)addr) &&
		    SIZECHECK(size, Km_Free[i].size)) {
			cnt++;
		}
	}
	return cnt;
}

/* prusers -- print trace of alloc/free's of addr */
void
prusers(addr, size)
uint addr, size;
{
	prausers(addr, size);
	prfusers(addr, size);
}


/* print allocs */
void
prausers(addr, size)
uint addr, size;
{
	int more = 0;

	/* We do this in reverse order to get last usage,
	 * Km_[ij] always points to next entry - entry is still valid
	 */
	if (Km_i != 0) {
		prarange(Km_i - 1, 0, addr, size, &more);
	}
	prarange(NREC - 1, Km_i, addr, size, &more);

}

/* print frees */
void
prfusers(addr, size)
uint addr, size;
{
	int more = 0;

	/* We do this in reverse order to get last usage,
	 * Km_[ij] always points to next entry - entry is still valid
	 */
	if (Km_j != 0) {
		prfrange(Km_j - 1, 0, addr, size, &more);
	}
	prfrange(NREC - 1, Km_j, addr, size, &more);

}

STATIC
void
prarange(start, end, addr, size, more)
int	start, end;
uint	addr, size;
int	*more;
{
	register int i, j;

	if ( Km_DebugInst == NULL )
		Km_DebugInst = s3blookup((uint)Km_Alloc[0].caller[0]);

	for ( i = start; i >= end; i-- ) {
		if ((Km_Alloc[i].addr == (unchar *)addr) &&
		    SIZECHECK(size, Km_Alloc[i].size)) {
			if ( (*more)++ > Km_Max ) {
				cmn_err(CE_CONT, "^Stopping at Km_Max\n");
				break;
			}
			cmn_err(CE_CONT, "^allocated %u bytes at 0x%x to:\n",
				Km_Alloc[i].size, addr );
			for ( j = 0; j < NUMCALL; ++j )
				if (Km_Alloc[i].caller[j])
					if ( Km_DebugInst != ENOPKG )
					    cmn_err(CE_CONT, "^\t\t0x%x %s\n",
					    Km_Alloc[i].caller[j],
					    (char *)s3blookup((uint)Km_Alloc[i].caller[j]));
					else
					    cmn_err(CE_CONT, "^\t\t0x%x\n",
					    Km_Alloc[i].caller[j]);
		}
	}
}

STATIC
void
prfrange(start, end, addr, size, more)
int	start, end;
uint	addr, size;
int	*more;
{
	register int i, j;

	if ( Km_DebugInst == NULL )
		Km_DebugInst = s3blookup((uint)Km_Free[0].caller[0]);

	for ( i = start; i >= end; i-- ) {
		if ((Km_Free[i].addr == (unchar *)addr) &&
		    SIZECHECK(size, Km_Free[i].size)) {
			if ( (*more)++ > Km_Max ) {
				cmn_err(CE_CONT, "^Stopping at Km_Max\n");
				break;
			}
			cmn_err(CE_CONT, "^freed %u bytes at 0x%x from:\n",
				Km_Free[i].size, addr );
			for ( j = 0; j < NUMCALL; ++j )
				if (Km_Free[i].caller[j])
					if ( Km_DebugInst != ENOPKG )
					    cmn_err(CE_CONT, "^\t\t0x%x %s\n",
					    Km_Free[i].caller[j],
					    (char *)s3blookup((uint)Km_Free[i].caller[j]));
					else
					    cmn_err(CE_CONT, "^\t\t0x%x\n",
					    Km_Free[i].caller[j]);
		}
	}
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
 * The kernel text start address and kernel text length parameters should
 * agree with the kernel text origin and length values given in the
 * kernel ifile.  These are used to detect when we traced back out of
 * the kernel stack.
 */

#define	KTXTSTRT	0x40000000
#define	KTXTLEN		0x00160000

/*
 * kmem_getcaller(num, pcstack) backtracks through the stack to retrieve
 * the previous NUM calling routines; PCSTACK is a pointer to an
 * array of at least NUM entries where the addresses are to be
 * placed.  The first address is always the routine that called
 * kmem_getcaller().  This code is taken from debug/trace.c.
 * There is no guarantee that all NUM entries belong in the same
 * trace, and there is no protection against stack underflow.
 */

STATIC void
kmem_getcaller(num, pcstack)
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

/*
 *	kmem_find:	look for 'addr' in freelists
 */
void
kmem_find(addr)
register void	*addr;
{
	register freebuf	*p;
	register int		i, size;

	for ( size = MINASMALL, i = SMINOFFSET;
	      size <= MAXABIG && i <= BMAXOFFSET;
	      ++i, size *= 2 ) {
		for ( p = Km_FreeLists[i].fl_nextp;
		      p != (freebuf *)&Km_FreeLists[i];
		      p = p->fb_nextp ) {
			if ( p == (freebuf *)addr ) {
				cmn_err(CE_NOTE,
				"^kmem_find: found address 0x%x in %u-byte freelist\nprev ptr = 0x%x, next = 0x%x\n",
				p, size, p->fb_prevp, p->fb_nextp);
				return;
			}
			if ( BADADDR1(p) ) {
				cmn_err(CE_WARN,
				"^kmem_find: %u-byte freelist trashed:\n\t\tbufp = 0x%x\n",
				size, p);
				break;
			}
		}
	}
	return;
}
#endif /* PARANOID */
