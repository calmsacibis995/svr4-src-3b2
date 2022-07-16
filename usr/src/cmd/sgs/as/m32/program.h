/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/program.h	1.3"
#include "expr.h"
/*
 */

/* pic relocation types */
#define PC	01
#define GOT	02
#define PLT	04

#define TXT	002
#define DAT 	003	
#define BSS	004
/* format of instruction table, mnemonics[] */
struct mnemonic {
	char		*name;
	unsigned short	tag;
	unsigned short	nbits;
	long		opcode;
	unsigned long	*opndtypes;
	unsigned short	index;
	unsigned short  roundmode;
	short		next;
};

/* MN_...
 *	these macros pick out the different fields of mnemonic.opcode
 *
 *	flow control instructions (branch, jump, jump subroutine)
 *	all have one-byte opcodes.
 *
 *	MN_REG	get the "regular" opcode; this is byte version
 *		for is25 flow instructions.  Is dyadic version for
 *		some is25 special instructions (e.g. mov[bhw])
 *	MN_ALT	get the "alternate" opcode; this is word version
 *		for is25 jsb, and is the "inverse" byte jump for
 *		is25 conditional branches.  Is monadic version for
 *		some is25 instructions (CLR[BHW] xx for mov[bhw] &0,xx)
 *
 * NOTE
 *	unconditional is25 'jmp' must not have alternate form, even
 *	though it really should.  code uses presence of alternate to
 *	indicate conditional jump.
 */

#define MN_ALT(op)	((unsigned char)((op)>>8))
#define MN_REG(op)	((unsigned char)(op))
#define MN_MKOP(r,a)	((r)|((a)<<8))


/* operand semantic information */
typedef struct {
	/*	variables that may be needed in future fp releases
	 * short fptype;	1=single fp ; 2=double fp constant
	 * double fpexpval;	holds value of floating point expression
	 */

	unsigned char  newtype;
	unsigned char  type;
	unsigned char  reg;
	EXPR_MEMBERS
	/* "expval" holds value of integer expression, coded
		 float or first word of a double constant */
	char           *fasciip;	/* holds second word of a double constant */
	/*
	 * The expression specifier describes the size of the expression
	 * and is kept here.
	 */
	long	expspec;
	BYTE    pic_rtype;
} OPERAND;

typedef	struct
	{
		char	name[sizeof(name_union)];
		BYTE	tag;
		BYTE	val;
		BYTE	nbits;
		long	opcode;
		symbol	*snext;
	} instr;

/* maps addressing modes for position independent pc relative relocation */
#define	PCREL_MODE(operand) switch(operand->type){  \
		case EXADMD: 	\
			operand->type = DSPMD; \
			operand->reg = PCREG; \
			operand->pic_rtype = PC; \
			break;	\
		case EXADDFMD:	\
			operand->type = DSPDFMD; \
			operand->reg = PCREG; \
			operand->pic_rtype = PC; \
			break; 	\
	}

#define USRNAME	1
#define MNEMON	0

#define	PSEUDO	0x25
#define	IS25	0x40
#define REGMD		0x1	/* register mode */
#define REGDFMD		0x2	/* register defered mode */
#define IMMD		0x3	/* immediate mode */
#define ABSMD		0x4	/* absolute address mode */
#define ABSDFMD		0x5	/* absolute address deferred mode */
#define EXADMD		0x6	/* external address mode */
#define EXADDFMD	0x7	/* external address deferred mode */
#define DSPMD		0x8	/* displacement mode */
#define DSPDFMD		0x9	/* displacement deferred mode */
#define FREGMD		0xa	/* mau register mode */
#define SREGMD		0xb	/* single register mode */
#define DREGMD		0xc	/* double register mode */
#define XREGMD		0xd	/* double extended register mode */

#if	FLOAT
#define	FPIMMD		0x13	/* floating point immediate mode */
#define	DECIMMD		0x14	/* floating point immediate mode */
#define	CDIMMD		0xCF	/* floating point double immediate mode */
#endif

#define TEXADMD (1<<EXADMD)
#define TREGMD	(1<<REGMD)
#define TIMMD	(1<<IMMD)
#define MEM	((1<<ABSDFMD)|(1<<ABSMD)|(1<<EXADDFMD)|(1<<EXADMD)|(1<<DSPDFMD)|(1<<DSPMD)|(1<<REGDFMD))
#define DEST	(MEM|TREGMD)
#define GEN	(DEST|TIMMD)
#define FPDEC	(MEM|(1<<FPIMMD)|(1<<DECIMMD))
#define FPINT	(MEM|TIMMD)
#define FPDEST	((1<<FREGMD)|(1<<SREGMD)|(1<<DREGMD)|(1<<XREGMD)|MEM)
#define FPGEN	(FPDEST|(1<<FPIMMD)|(1<<DECIMMD))
#define FPGEN2	(DEST|(1<<FPIMMD))


