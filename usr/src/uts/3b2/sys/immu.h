/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_IMMU_H
#define _SYS_IMMU_H

#ident	"@(#)head.sys:sys/immu.h	11.15"

/*
 * Page Table Entry Definitions
 */

typedef union pte {    /*  page table entry  */
/*    	                                               */
/*  +---------------------+---+--+--+--+-+-+-+-+-+-+   */
/*  |        pfn          |lck|nr|  |  |r|w| |l|m|v|   */
/*  +---------------------+---+--+--+--+-+-+-+-+-+-+   */
/*             21            1  1  1  2 1 1 1 1 1 1    */
/*                                                     */
	struct {
		uint pg_pfn	: 21,	/* Physical page frame number */
		     pg_lock	:  1,	/* Lock in core (software) */
		     pg_ndref	:  1,	/* Needs reference (software).	*/
				:  1,	/* Unused software bit.		*/
				:  2,	/* Reserved by hardware.	*/
		     pg_ref	:  1,	/* Page has been referenced */
		     pg_w	:  1,	/* Fault on write */
		     		:  1,	/* Reserved by hardware.	*/
		     pg_last	:  1,	/* Last (partial) page in segment */
		     pg_mod	:  1,	/* Page has been modified */
		     pg_v	:  1;	/* Page is valid, i.e. present. */
	} pgm;

	uint	pg_pte;		/* Full page descriptor (table) entry */
} pte_t;

/*
 *	Page Table
 */

#define NPGPT		64	/* Nbr of pages per page table (seg). */

typedef struct ptbl {
	int page[NPGPT];
} ptbl_t;

/* Page table entry dependent constants */

#define	NBPP		2048		/* Number of bytes per page */
#define	NBPPT		256		/* Number of bytes per page table */
#define	BPTSHFT		8 		/* LOG2(NBPPT) if exact */
#define	NPTPP		8		/* Nbr of page tables per page.	*/
#define	NPTPPSHFT	3		/* Shift for NPTPP. */

#define PNUMSHFT	11		/* Shift for page number from addr. */
#define PNUMMASK	0x3F		/* Mask for page number in segment. */
#define POFFMASK        0x7FF		/* Mask for offset into page. */
#define	PNDXMASK	0x7FFFF		/* Mask for page index into section.*/
#define PGFNMASK	0x1FFFFF	/* Mask page frame nbr after shift. */

/* Page descriptor (table) entry field masks */

#define PG_ADDR		0xFFFFF800	/* physical page address */
#define PG_LOCK		0x00000400	/* page lock bit (software) */
#define PG_NDREF	0x00000200	/* need reference bit (software) */
#define PG_REF		0x00000020	/* reference bit */
#define PG_W		0x00000010	/* fault on write bit */
#define PG_LAST		0x00000004	/* last page bit */
#define PG_M		0x00000002	/* modify bit */
#define PG_V		0x00000001	/* page valid bit */

/* The page number within a section. */

#define pgndx(x)	(((x) >> PNUMSHFT) & PNDXMASK)

/* Round up page table address */

#define ptround(p)	((int *) (((int)p + PTSIZE-1) & ~(PTSIZE-1)))

/* Round down page table address */

#define pttrunc(p)	((int *) ((int)p & ~(PTSIZE-1)))

/* Page tables (64 entries == 256 bytes) to pages. */

#define	pttopgs(x)	((x + NPTPP - 1) >> NPTPPSHFT)
#define	pttob(x)	((x) << BPTSHFT)
#define	btopt(x)	(((x) + NBPPT - 1) >> BPTSHFT)

/* Form page table entry from modes and page frame number */

#define	mkpte(mode,pfn)	(mode | ((pfn) << PNUMSHFT))

/* The following macros are used to check the value
 * of the bits in a page table entry 
 */

#define PG_ISVALID(pte) 	((pte)->pgm.pg_v)
#define PG_ISLOCKED(pte)	((pte)->pgm.pg_lock)

/*	The following macros are used to set the value
 *	of the bits in a page descrptor (table) entry 
 *
 *	Atomic instruction is available to clear the present bit,
 *	other bits are set or cleared in a word operation.
 */

#define PG_SETVALID(pte)	((pte)->pg_pte |= PG_V)
#define PG_CLRVALID(pte)	((pte)->pg_pte &= ~PG_V)

#define PG_SETNDREF(pte)	((pte)->pg_pte |= PG_NDREF)
#define PG_CLRNDREF(pte)	((pte)->pg_pte &= ~PG_NDREF)

#define PG_SETLOCK(pte)  	((pte)->pg_pte |= PG_LOCK)	
#define PG_CLRLOCK(pte)  	((pte)->pg_pte &= ~PG_LOCK)

#define PG_SETMOD(pte)   	((pte)->pg_pte |= PG_M)	
#define PG_CLRMOD(pte)   	((pte)->pg_pte &= ~PG_M)	

#define PG_SETW(pte)     	((pte)->pg_pte |= PG_W)
#define PG_CLRW(pte)     	((pte)->pg_pte &= ~PG_W)

#define PG_SETREF(pte)    	((pte)->pg_pte |= PG_REF)
#define PG_CLRREF(pte)    	((pte)->pg_pte &= ~PG_REF)

/*
 * Segment Descriptor (Table) Entry Definitions
 */

typedef struct sde {    /*  segment descriptor (table) entry  */
/*                                                                            */
/*  +--------+--------------+--+--------+ +--------------------------------+  */
/*  |  prot  |     len      |  |  flags | |             address            |  */
/*  +--------+--------------+--+--------+ +--------------------------------+  */
/*       8          14        2     8                      32                 */
/*                                                                            */
/*					  +--------------------------+-+-+-+  */
/*			   (V0):	  |                          |N|W|S|  */
/*					  +--------------------------+-+-+-+  */
/*						       29             1 1 1   */
/*                                                                            */
	uint seg_prot	:  8,	/* Segment protection bits */
	     seg_len	: 14,	/* Segment length in doublewords */
			:  2,	/* Reserved */
	     seg_flags	:  8;	/* Segment descriptor flags */
	union {
		paddr_t address; /* Address of PT or physical segment (cont.) */
		struct {
			uint		: 29,	/* Not used */
			     wanted	:  1,	/* "N" bit  */
			     shmswap	:  1,	/* "W" bit  */
			     sanity	:  1;	/* "S" bit  */
		} V0;
	} wd2;
} sde_t;

/*  access modes  */

#define KNONE  (unsigned char)  0x00
#define KEO    (unsigned char)  0x40	/* KRO on WE32000	*/
#define KRE    (unsigned char)  0x80
#define KRWE   (unsigned char)  0xC0	/* KRW on WE32000	*/

#define UNONE  (unsigned char)  0x00
#define UEO    (unsigned char)  0x01	/* URO on WE32000	*/
#define URE    (unsigned char)  0x02
#define URWE   (unsigned char)  0x03	/* URW on WE32000	*/

#define UACCESS (unsigned char) 0x03
#define KACCESS (unsigned char) 0xC0

#define SEG_RO	(KRWE|URE)
#define SEG_RW	(KRWE|URWE)

/* descriptor bits */

#define SDE_I_bit	(unsigned char) 0x80
#define SDE_V_bit	(unsigned char) 0x40
#define SDE_T_bit	(unsigned char) 0x10
#define	SDE_C_bit	(unsigned char) 0x04
#define	SDE_P_bit	(unsigned char) 0x01
#define SDE_flags	(unsigned char) 0x41   /*  sets V_bit and P_bit  */
#define SDE_kflags	(unsigned char) 0x45   /*  sets V_bit, C_bit and */
					       /*  P_bit.		 */

#define SDE_SOFT        7

#define SD_ISVALID(sde) 	((sde)->seg_flags & SDE_V_bit)
#define SD_INDIRECT(sde)	((sde)->seg_flags & SDE_I_bit)
#define SD_ISCONTIG(sde)	((sde)->seg_flags & SDE_C_bit)
#define SD_ISTRAP(sde)  	((sde)->seg_flags & SDE_T_bit)
#define SD_SEGBYTES(sde)	(int)((((sde)->seg_len) + 1) << 3)
#define SD_LASTPG(sde)  	((sde)->seg_len) >> 8)
#define SD_U_ACCESS(sde)	(((sde)->seg_prot) & UACCESS)
#define SD_K_ACCESS(sde)	(((sde)->seg_prot) & KACCESS)
#define SD_SEGINDX(addr)	((int)addr & MSK_IDXSEG)
#define SD_CLRVALID(sde)	((sde)->seg_flags &= ~SDE_V_bit)

/*	Segment descriptor (table) dependent constants.	*/

#define NBPS		0x20000 /* Number of bytes per segment.  */
#define PPSSHFT		6	/* Shift for pages per segment.  */
#define SOFFMASK	0x1FFFF	/* Mask for offset into segment. */
#define SEGOFF(x)	((uint)(x) & SOFFMASK)
#define	BPSDESHFT	3	/* Shift for bytes per sde.      */
#define	NBPSDT		64	/* Segment table size in bytes.  */
#define	BPSDTSHFT	6	/* Shift for bytes per seg tbl.  */
#define	NSDTPP		32	/* Number of seg tbls per page.  */
#define	SDTPPSHFT	5	/* Shift for seg tbls per page.  */

#define	sdetob(x)	((x) << BPSDESHFT)
#define	btosde(x)	((x) >> BPSDESHFT)
#define	sdttopgs(x)	(((x) + NSDTPP - 1) >> SDTPPSHFT)
#define	sdttob(x)	((x) << BPSDTSHFT)
#define	btosdt(x)	(((x) + NBPSDT - 1) >> BPSDTSHFT)

/*
 * Memory Management Unit Definitions
 */

/*    	                                                     */
/*  Virtual Address:                                         */
/*    	                                                     */
/*  +-----------+---------------+----------+--------------+  */
/*  |  section  |    segment    |   page   |    offset    |  */
/*  +-----------+---------------+----------+--------------+  */
/*        2            13             6           11         */
/*    	                                                     */

typedef struct _VAR {
	uint v_sid	:  2,	/*  section number      */
	     v_ssl	: 13,	/*  segment number      */
	     v_psl	:  6,	/*  page number         */
	     v_byte	: 11;	/*  offset within page  */
} VAR;

/*  masks to extract portions of a VAR  */
#define SECNMASK	0x3	/* Mask for section number after shift. */
#define SECNSHFT	30	/* Shift for section number from address. */
#define SEGNMASK	0x1FFF	/* Mask for segment number after shift. */
#define SEGNSHFT	17	/* Shift for segment number from address. */

#define SECNUM(x)   (((uint)(x) >> SECNSHFT) & SECNMASK)
#define SEGNUM(x)   (((uint)(x) >> SEGNSHFT) & SEGNMASK)
#define PAGNUM(x)   (((uint)(x) >> PNUMSHFT) & PNUMMASK)
#define PAGOFF(x)   (((uint)(x)) & POFFMASK)

#define MSK_IDXSEG  0x1ffff  /*  lower 17 bits == PSL || BYTE  */

typedef struct  _FLTCR { /*  fault code register  */
	uint		: 21,
	     reqacc	:  4,
	     xlevel	:  2,
	     ftype	:  5;
} FLTCR;

/*  access types */

#define	AT_SPOPWRITE	 1
#define AT_SPOPFTCH	 3
#define	AT_ADFTCH	 8
#define	AT_OPFTCH	 9
#define	AT_WRITE	10
#define	AT_OWRITE	11
#define	AT_IFAD		12
#define	AT_IPFTCH	13
#define	AT_IFTCH	14
#define	AT_MT		 0

/*  access execution level */

#define	XLVL_K		0
#define	XLVL_U		3

/*  Fault types  */

#define F_NONE       0x0
#define F_MPROC	     0x1
#define F_RMUPDATE   0x2
#define F_SDTLEN     0x3
#define F_PWRITE     0x4
#define F_PTLEN      0x5
#define F_INVALID    0x6
#define F_SEG_N_P    0x7
#define F_OBJECT     0x8
#define F_PT_N_P     0x9
#define F_P_N_P      0xa
#define F_INDIRECT   0xb
#define F_ACCESS     0xd
#define F_OFFSET     0xe
#define F_ACC_OFF    0xf
#define F_D_P_HIT    0x1f

typedef struct _CONFIG { /*  configuration register  */
	uint		: 30,
	     ref	:  1,
	     mod	:  1;
} CR;

typedef paddr_t SRAMA;	/* Segment descriptor table physical address */

typedef struct _SRAMB {	/* SRAMB Area */

/*
 *	+---------+-------------+----------+
 *	| reserve | # segs      | reserve  |
 *	+---------+-------------+----------+
 *	    9		13	   10
 */
	unsigned int		:  9;	/* reserved */
	unsigned int	SDTlen	: 13;	/* seg id of last seg in section */
	unsigned int		: 10;	/* reserved */
} SRAMB;

/* Virtual start address of sections */

#define VSECT0		0x00000000
#define VSECT1		0x40000000
#define VSECT2		0x80000000
#define VSECT3		0xC0000000

#define SRAMBSHIFT	10
#define MAXSDTSEG	8192	/* Maxmum number of segment tbl entries */

#define OFF 0
#define ON  1

/*  MMU-specific addresses  */

extern char *mmusrama, *mmusramb, *mmufltcr, *mmufltar, *mmucr, *mmuvar;

#define srama  ((SRAMA *) (&mmusrama))
#define sramb  ((SRAMB *) (&mmusramb))
#define fltcr  ((FLTCR *) (&mmufltcr))
#define fltar  ((int *)   (&mmufltar))
#define crptr  ((CR *)    (&mmucr))
#define varptr ((int *)   (&mmuvar))

/*
 * Peripheral mode functions
 */

#define flushaddr(vaddr)	(*((int *)(varptr)) = (int)(vaddr))
#define flushsect(sectno)	(*((SRAMA *)(srama + (sectno))) = \
					(*(SRAMA *)(srama + (sectno))))
/*	The following variables describe the memory managed by
**	the kernel.  This includes all memory above the kernel
**	itself.
*/
extern int	kpbase;		/* The address of the start of	*/
				/* the first physical page of	*/
				/* memory above the kernel.	*/
				/* Physical memory from here to	*/
				/* the end of physical memory	*/
				/* is represented in the pfdat.	*/
extern int	syssegs[];	/* Start of the system segment	*/
				/* from which kernel space is	*/
				/* allocated.  The actual value	*/
				/* is defined in the vuifile.	*/
extern int	win_ublk[];	/* A window into which a	*/
				/* u-block can be mapped.	*/
extern pte_t	*kptbl;		/* Kernel page table.  Used to	*/
				/* map sysseg.			*/
extern int	maxmem;		/* Maximum available free	*/
				/* memory.			*/
extern int	freemem;	/* Current free memory.		*/
extern int	availrmem;	/* Available resident (not	*/
				/* swapable) memory in pages.	*/
extern int	availsmem;	/* Available swapable memory in	*/
				/* pages.			*/

/*	Conversion macros
*/

/*	Get page number from system virtual address.  */

#define	svtop(x)	((uint)(x) >> PNUMSHFT) 

/*	Get system virtual address from page number.  */

#define	ptosv(x)	((uint)(x) << PNUMSHFT)


/*	These macros are used to map between kernel virtual
**	and physical address.  Note that all of physical
**	memory is mapped into kernel virtual address space
**	in segment zero at the actual physical address of
**	the start of memory which is MAINSTORE.
*/

extern paddr_t	svirtophys(/* va */);
extern paddr_t	kvtophys(/* va */);

#define phystokv(paddr) (paddr)

/*	Between kernel virtual address and physical page frame number.
*/

#define phystopfn(paddr)	((u_int)(paddr) >> PNUMSHFT)
#define pfntophys(pfn)  	((pfn) << PNUMSHFT)
#define kvtopfn(vaddr)  	(kvtophys(vaddr) >> PNUMSHFT)
#define pfntokv(pfn)    	((pfn) << PNUMSHFT)

/*	Between kernel virtual addresses and the kernel
**	segment table entry.
*/

#define	kvtokstbl(vaddr) (&((sde_t *)(*(srama + 1)))[SEGNUM(vaddr)])

/*	Between kernel virtual addresses and the kernel page
**	table.
*/

#define	kvtokptbl(x)	(&kptbl[pgndx((uint)(x) - (uint)syssegs)])

/*	Test whether a virtual address is in the kernel dynamically
**	allocated area.
*/

#define	iskvir(va)	((SECNUM(va) == SCN1)  &&  \
			 (uint)va >= (uint)syssegs)

/*
 * pte_t *
 * svtopte(v)
 * returns the pte entry location of v.
 *
 * This macro works only with paged virtual address.
 */

#define svtopte(v) ((pte_t *)(phystokv(vatosde(v)->wd2.address)) + PAGNUM(SEGOFF(v)))

/*
 * vatopte(va)
 * returns the page descriptor entry location of the virtual address va.
 */

#define	vatopte(va, sde)	((pte_t *)((sde)->wd2.address) + PAGNUM(va))

/*
 * svtopfn(v)
 */

#define svtopfn(v) (PAGNUM(svirtophys(v)))

/*	Page frame number to kernel pte.
*/

#define	pfntokptbl(p)	(kvtokptbl(pfntokv(p)))

/* flags used in ptmemall() call
*/

#define PHYSCONTIG 02
#define NOSLEEP    01

/* Section Id used in flushsect() and loadmmu() */

#define SCN0	0
#define SCN1	1
#define SCN2	2
#define SCN3	3

/*	Load the mmu registers for a section with the address
**	and length found in the proc table.  This is a macro
**	rather than a function since it speeds up context
**	switches by eliminating the subroutine linkage.
*/

#define loadmmu(hatp, section)	\
{				\
	register int	ipl;	\
	register int	index;	\
				\
	ipl = spl7();		\
	index = section - SCN2;	\
				\
	srama[section] = hatp->hat_srama[index];	\
	sramb[section] = hatp->hat_sramb[index];	\
				\
	splx(ipl);		\
}

#ifdef _KERNEL

#if defined(__STDC__)
extern sde_t *vatosde(caddr_t);
extern void flushmmu(caddr_t, int);
#else
extern sde_t *vatosde();
extern void flushmmu();
#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _SYS_IMMU_H */
