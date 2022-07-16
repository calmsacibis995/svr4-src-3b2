/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/olddefs.h	1.7"

# define IMPPROMOTE
# define IMPANTI
# define IMPEXPAND
# define IMPMERGE

				/* Options.	*/
# define PEEPHOLE


struct pvent			/* assembler pseudo-variable save list node */
	{struct pvent *next;	/* list link */
	 char *name;		/* variable name */
	 int value;		/* value assigned by .set pseudo op */
	};

		/* LD table, table of quantities receiving live/dead analysis */
#define	LIVEDEAD	96
#define	WDSIZE		(sizeof(unsigned long int) * B_P_BYTE)
#define	NVECTORS	((LIVEDEAD+WDSIZE-1) / WDSIZE)

				/* Variable classes. */
#define VC_NONE		0	/* Ld table entry is not a register. */
#define VC_CSCR1ST	0x1	/* 1st set CPU scratch register. */
#define VC_CSVD		0x2	/* Saved CPU register. */
#define VC_CSCR2ND	0x4	/* 2nd set CPU scratch register. */
#define	VC_ISTK		0x8	/* Integer stack variables. */
#define VC_STAT		0x10	/* local static SNSQ */
#define VC_STATEXT	0x20	/* non-local static SNSQ */
#define VC_EXTDEF	0x40	/* defined global SNSQ */
#define VC_EXT		0x80	/* undefined global SNSQ */
#define VC_ALL		0xffffffff /* All variable classes. */

struct ldent
	{unsigned int index;	/* Index of THIS entry in ld table. */
	 AN_Id laddr;		/* AN_Id of THIS live-dead entry.	*/
	 int word;		/* Index of word of THIS entry in vectors. */
	 unsigned long int bit_mask; /* Bit mask of THIS entry in vectors. */
				/* Conflicts with other NAQs. */
	 unsigned long int conflicts[NVECTORS]; 
	 struct ldent *var_next; /* Variable list pointer. */
	 struct ldent *sub_next; /* Subset list pointer. */
	 struct ldent *passign;	/* Ld entry of assigned register. */
	 unsigned long int var_class; /* Class of variable. */
	 unsigned int regpaired:1; /* 1 if var is one of register pair.	*/
	};

/* predicates and functions 
	t - text node pointer
	o - opcode table index
	i - operand index
	a - address node pointer */

# define ishb(t) (0)

# define isdeadcc(t) (!islivecc(t))

# define bboptim(f,l) 0

#define IsExpand(ty) ( (ty)==Tsbyte || (ty)==Tbyte || (ty)==Thalf || \
		(ty)==Tuhalf || (ty)==Tword || (ty)==Tuword )
#define IsSigned(ty) ((boolean) ((ty)==Tsbyte || (ty)==Thalf || (ty)==Tword))
#define IsUnSigned(ty) ((boolean) ((ty)==Tubyte || (ty)==Tuhalf || (ty)==Tuword))
#define IsFP(ty) ( (ty)==Tsingle || (ty)==Tdouble || (ty)==Tdblext )
#define IsInteger(ty) (IsExpand(ty) || (ty) == Tdecint)

# define MXRETREG 	2	/* default value in case no comment on
				 * return instruction */
# define LIVEREGS 0x0

				/* Line number stuff */
# define IDTYPE int
# define IDVAL 0
				/* States of optimization mode */
#define	ODEFAULT	0
#define OSPEED		1
#define OSIZE		2

#define ALLNSKIP(p)	p = skipprof(GetTxNextNode((TN_Id) NULL)); p != (TN_Id) NULL; p = GetTxNextNode(p)

		/* Target Dependent stuff too hard to do at run-time. */
	/* Maximum dimension of some LICM tables.	*/
#define	LICM_SIZE	128
	/* Maximum size of value tracing table.	*/
#define	VT_TAB_SIZE	96
