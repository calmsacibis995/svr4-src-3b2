/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:m32com/macdefs2.h	1.8"
/*	macdefs - machine dependent macros and parameters
 *
 *		Bellmac32 w/Mao/Berenbaum stack frame
 *
 *			R. J. Mascitti
 */

#define NINS	1200

#ifdef LION
#define LOCAL_INIT local_init
#endif

#define TREESZ	2500
/* initial values for stack offsets in bits */

#define ARGINIT 0
#define AUTOINIT 0

/* sizes of types in bits */

#define SZCHAR 8
#define SZSHORT 16
#define SZINT 32
#define SZPOINT 32

#define SZFLOAT 32
#define SZDOUBLE 64
#define SZFPTR 64
#define SZFIELD 32

/* structure sizes must be a multiple of 32 bits */

#define STMULT 32

/* alignments of types in bits */

#define ALCHAR 8
#define ALSHORT 16
#define ALINT 32
#define ALPOINT 32
#define ALSTACK 32
#define ALSTRUCT 32
#define ALINIT 32
#define	ALFIELD 32			/* force bit fields to have this alignment */
#define ALFRAME 32

#define ALFLOAT 32
#define ALDOUBLE 32
#define ALFPTR 32

#define NOLONG			/* map longs to ints */

/* format for labels (if anybody cares) */

#define LABFMT ".L%d"

/* type size in number of regs/temps */

#ifndef MAU
#	define szty(t) (((t) & (TFPTR | TDOUBLE)) ? 2 : 1)
#else
#	define szty(t) (((t) == TFPTR) ? 2 : 1)
#endif

/* number of scratch registers: */

#ifdef MAU

	/* number of scratch registers:
	** 3 are integer/pointer
	** 2 is MAU
	*/
	
#	define NRGS 5
	
	/* total registers
	** 9 are integer/pointer (3 scratch, 6 user register)
	** 4 are MAU             (2 scratch, 2 user register)
	*/
	
#	define TOTREGS 13
	
	/* highest cpu register */
	
#	define CPUREGHI 11

#else /* function call m32 */

#	define NRGS 3
	
	/* total registers
	** 9 are integer/pointer (3 scratch, 6 user register)
	*/
	
#	define TOTREGS	9

#endif


/* Define symbol so we can allocate odd register pairs; in fact,
** no pairs are allocated
*/

#define ODDPAIR

/* params addressed with positive offsets from ap, args with negative from sp */

#undef BACKPARAM
#define BACKARGS

/* temps and autos addressed with positive offsets from fp */

#undef BACKTEMP
#undef BACKAUTO

/* bytes run left to right */

#define LTORBYTES
#undef RTOLBYTES

/* function calls and arguments */
/* OK to nest function calls and ALLOCs */ 

#undef NOFNEST
#undef NOANEST

/* evaluate args left to right (3b style) */

#define LTORARGS

/* chars are unsigned */

#define CHSIGN

/* structures returned in temporary location -- define magic string */

#define TMPSRET	"	movaw	T,%r2\n"

/* optimizer throws away unless %r2 */

#define AUXREG 2


/* comment indicator */

#define COMMENTSTR "#"

/* volatile operand end */

#ifdef VOL_SUPPORT
#	define VOL_OPND_END	','
	extern int cur_opnd;
#	define vol_opnd_end()	{cur_opnd <<= 1;}
#endif

/* string containing ident assembler directive for ident feature */

#define IDENTSTR	".ident"

/* asm markers */

#define	ASM_COMMENT	"#ASM"
#define	ASM_END		"#ASMEND"

/* reflect my high costing in default load/store cost */

#define CLOAD(x) 55
#define CSTORE(x) 55
#define CTEST(x)  55

#define MYLOCCTR

#define EXIT(x) myexit(x)

/* Register number for debugging */

#ifdef MAU
extern int regout[]; 	/* only used for the OUTREGNO(p) macro */
#	define	OUTREGNO(p) ( regout[p->offset])
#else
#	define	OUTREGNO(p) (p->offset)
#endif

/* Turn on debug information */

#define	SDB

/* Enable assembly language comments */

#define	ASSYCOMMENT

/* user-level fix-up at symbol definition time */

#define FIXDEF(p)	fixdef(p)

/* support for structure debug info */

#define FIXSTRUCT(p,q)	strend(p)

/* We want bitfields all to act like integers because we want to
** use the extract/insert instructions, which must deal with
** word-aligned data.
*/

#define	FLDINTADR

#ifdef M32B
#	define IMPSWREG
#	undef IMPREGAL
#endif

/* Commutable operators are symetric anyway */
#define NOCOMMUTE 1
 
/* To turn on proper IEEE floating point standard comparison  */
/* of non trapping NaN's.  Two floating point comparisons:    */
/* CMPE for exception raising on all NaN's; CMP for no        */
/* exception raising for non-trapping NaN's, used for fp ==   */
/* and !=      */
 
#define IEEE

/* expand string space in sty */
#ifdef MAU
#	define	NSTRING	14000
#else
#	define 	NSTRING	18000
#endif /* ifdef MAU */

#ifdef	M32B
#define	BCSZ	125		/* expand break/continue label table */
#endif

/* bypass initialization of INTs through templates */

#define	MYINIT	sincode

/* turn on register set code */

#define	REGSET
typedef	unsigned short RST;		/* bit vectors fit herein */

/* for NAIL:*/

/* Bits per allocation unit*/
#define AUSIZE 8

/*Allocation units must be in units of 2**2 units*/

# define AUALIGN 2

/*Number of bits in the save area (saved registers*/

#define SAVESIZE (9*SZINT)

/*assembler conventions for symbols that can appear in names:
                        rst char in symbol:*/

#define ASMFIRSTOK "_.abcdefghijklmnopqrstuvwxyz"


#define ASMCHARS "_.abcdefghijklmnopqrstuvwxyz1234567890"

/*macros for exceptions*/

/*3B20 overflow bit is not sticky; it is cleared on subsequent ops.
        Therefore, this must be treated as a trap machine.*/

#define EXTRAP

/* exceptions: we can honor integer and float exceptions */
/* integer overflow sets the v bit; float sends a signal */

#define EXCANH (EXINT | EXFLOAT)

/* we can ignore - in fact must ignore - the others */

#define EXCANI (EXLSHIFT | EXCAST | EXDBY0 )
 
#define NULLHAND "_null_hand"

#define EX_BEFORE(p,g,q) ex_before(p,g,q)
/* protection from optimization (for exact semantics)*/

#define PROT_START "#PROTECT"
#define PROT_END "#ENDPROT"

/* Special ret_type patch to tell the optimizer how much is returned*/

#define MYRET_TYPE(t)  myret_type(t)

#define INTARGS

/*This instance doesn't support file static commons*/
#define ALLCOMMGLOBAL

/* number of templates allowed in stin	*/
#define NOPTB 	450

/* This instance doesn't support overlapping block moves. This
** tells NIFTY to generate a safe overlapping block move through
** a series of assignments
*/
#define NOBMOVEO

/*Ada can make a lot of rewrites to trees, so tell the watchdog
not to be so nervous. The default WATCHDOG can call it quits
on OK (if big) trees. A value of 200 allows 20,000 node
trees. (down boy!) */

#define WATCHDOG 200

extern void
	acon(),
	conput(),
	adrput(),
	upput(),
	zzzcode(),
	insput(),
	bycode(),
	fincode(),
	sincode(),
	lineid(),
	ex_before(),
	genswitch(),
	genladder(),
	bfcode(),
	efcode(),
	bfdata(),
	stasg(),
	starg(),
	protect(),
	unprot(),
	defalign(),
	defnam(),
	definfo(),
	deflab(),
	myflags(),
#ifdef MAU
	rsavemau(),
	rrestmau(),
#endif
	begf();

extern int
	locctr(),
	getlab(),
	special();

extern int picflag;
#define emit_str(s)	fputs(s,outfile)
#define MYREADER	myreader
extern void myreader();
#define ELF_OBJ
#define FORCE_LC(lc)	(-(lc))		/*forcing location counter*/
