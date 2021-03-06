/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/swagen.c	1.5"
#include <stdio.h>
#include <string.h>
#include "systems.h"
#include "symbols.h"
#include "instab.h"
#include "program.h"
#include "gendefs.h"

#define MULOP 0
#define DIVOP 1
#define MODOP 2

#define SIGNED 0
#define UNSIGNED 1
#define l_abs(x)  ((x) < 0 ? (x*-1) : (x))

extern symbol *lookup();
extern struct mnemonic *find_mnemonic();
extern void addrgen();
extern int optype();
extern short
	workaround; /* no software workaround flag */
extern int warlevel;
extern int Genline;
extern unsigned short
	line;	/* current line number */

/*
 * chip fix routines:
 * The following three routines (mtoregcheck, intaft1check, and intaft2check)
 * are chip fix routines for the Interrupt After TSTW bug.
 *
 * If an interupt is responded to after a certain sequence of instructions,
 * (described below) the last instruction in the sequence may not set flags
 * at all.
 *
 * Instruction Sequences which can cause problems:
 * First Instruction:
 *	Type 1) Instruction which does a store to a register and whose
 *		last source operand comes from memory. This does not
 *		include any discontinuities, SWAPs, RESTORE, or instructions
 *		without destinations (ie. BIT, TST, CMP).
 *	Type 2) Instruction which does a store to a register and is a
 *		multiple cycle ALU operation (ie. multiply, divide, modulo,
 *		insert field, move negate).
 * Second Instruction:
 *	Any instruction which is executed in 3 cycles. These are:
 *	TSTW %reg	MOVW %reg,%reg	MCOMW %reg %reg
 *	INCW %reg	DECW %reg	CLRW %reg
 * The fix for thisproblem is to insert a NOP before the second instruction
 * in the sequence.
 *
 */

/* This routine will return a 1 so that an indicator may be set if
 * the first instruction in the dangerous sequence is assembled.
 */
mtoregcheck(src, dest)
register OPERAND *src, *dest;
{
#ifdef	CHIPFIX
	if (warlevel >= NO_AWAR)
		return(0);
	if ((dest->type == REGMD) && ((1<<src->type) & MEM) && workaround)
		return(1);
	else
		return(0);
#else
		return(0);
#endif
} /* mtoregcheck */


/* This routine will generate a NOP if the indicator is set, and
 * the operand is a register operand.
 */
void
intaft1check(opnd, mtoreg)
register OPERAND *opnd;
short	mtoreg;
{
#ifdef	CHIPFIX
	if ((warlevel >= NO_AWAR) || mtoreg != 1)
		return;
	if ((opnd->type == REGMD) && workaround)
		generate(8, NOACTION, 0x70 , NULLSYM);	/* NOP */
#endif
} /* intaft1check */


/* This routine will generate a NOP if the indicator is set, and
 * both of the operands are register operands.
 */
void
intaft2check(opnd1, opnd2, mtoreg)
register OPERAND *opnd1, *opnd2;
short	mtoreg;
{
#ifdef	CHIPFIX
	if ((warlevel >= NO_AWAR) || mtoreg != 1)
		return;
	if ((opnd1->type == REGMD) && (opnd2->type == REGMD && workaround))
		generate(8, NOACTION, 0x70, NULLSYM); /* NOP */
#endif
} /* intaft2check */


/*
 *	chip fix routine: Due to pipelining on the cpu chip
 *	it is possible for the chip to have the "next" instruction
 *	almost completely executed while it is waiting for the memory
 *	write of the "current" instr. to be completed, even executed
 *	to the point where the PC is incremented past the "next" struct mnemonic.
 *	If an interrupt occurs in a situation like this then the PC
 *	will be restored to one past the "next" struct mnemonic, thereby, in effect,
 *	skipping one struct mnemonic. The fix for this is to place a NOP after
 *	every struct mnemonic. that does a memory write, except for:
 *		CALL, call, SAVE, save, PUSHW, pushw, PUSHAW, pushaw
 *	These don't need the NOP padding.
 */
void
pcintcheck(dest)
register OPERAND *dest;
{
#ifdef	CHIPFIX
	if (warlevel >= NO_AWAR)
		return;
	if (((1<<dest->type) & MEM) && workaround)
			generate(8,NOACTION,0x70,NULLSYM);	/* NOP */
#endif
} /* pcintcheck */


/*
 *	chip fix routine: if the psw register is used as the source of
 *	an instruction, the flag values may not be set yet from the 
 *	previous instruction due to a timing problem.  If the instruction
 *	is preceded by a NOP then the bits have time to settle and all
 *	is well.
 *	This chip fix routine will only fix IS25 instructions, BELLMAC-32B
 *	instructions are left untouched.
 *
 *	If the psw register is used as the destination of
 *	an instruction NOP will follow the instructions
 */

pswcheck(addr)
register OPERAND *addr;
{
        if( ! workaround )
		return(0);
	if ((addr->type == REGMD) && (addr->reg == PSWREG)) {
		generate(8,NOACTION,0x70,NULLSYM);	/* NOP */
		return(0);				/* NOP needed */
	}
	return(1);					/* NOP not needed */
}

#if	M32RSTFIX
/*
 *  The following routine is a workaround for the
 *  RESTORE chip bug.
 *
 *  The RESTORE instruction will be mapped into the
 *  following sequence of instructions.
 *
 *	RESTORE %rX  ===>	MOVAW  -Y(%fp), %sp
 *				POPW	%r8
 *				POPW	%rZ
 *				  .
 *				  .
 *				POPW	%rX
 *				POPW	%fp
 *
 *	where:
 *		Y is  4 * (3 - X)  (ie. 12-4X )
 *		Z is  Y - 1
 *
 *	and if %rW is %fp then the sequence is the following:
 *	RESTORE %fp ===>	MOVAW	-24(%fp),%sp
 *				POPW	%fp
 */
void
restorefix(num)
register OPERAND *num;
{
	register int numregs, rnum;
	struct mnemonic *newins;
	static OPERAND dispfp = {NOTYPE, DSPMD,FPREG,0,ABS,NULLSYM,1L};

	if (num->type == REGMD) 
			numregs = 9 - num->reg;
	if (num->type == IMMD) 
			numregs = num->expval;
	newins = find_mnemonic("MOVAW");
	generate(newins->nbits,NOACTION,newins->opcode,NULLSYM);
	dispfp.expval = -24 + (4 * numregs);
	addrgen(newins,&dispfp,NOTYPE,1);
	generate(8,NOACTION,(long)(CREGMD<<4|SPREG),NULLSYM);
	/* generate MOVAW (12-4X)(%fp),%sp */
	newins = find_mnemonic("POPW");
	for (rnum=8; numregs > 0; numregs--,rnum--) {
		/* generate POPW with the register being 9-numreg */
		generate(newins->nbits,NOACTION,newins->opcode,NULLSYM);
		generate(8,NOACTION,(long)(CREGMD<<4|rnum),NULLSYM);
	}
	generate(newins->nbits,NOACTION,newins->opcode,NULLSYM);
	generate(8,NOACTION,(long)(CREGMD<<4|FPREG),NULLSYM);
}
#endif	/* M32RSTFIX */

#if ER13WAR
/* 
 * Number:	 er 1
 * 
 * Name: Flag Setting for Multiply/Divide
 * 
 * Description: There are two situations where the CPU sets the N & V
 * 	     flags but not the Z flag, even though the result is zero.
 * 	     The source operands must be of opposite signs, with
 * 	     an unsigned destination.  For multiply one source must be 0.
 * 	     For divide the absolute value of the numerator must be 
 * 	     non-zero and greater than the absolute value of the denominator.
 * 
 * Resolution: generate a warning diagnostic
 * 
*/
void
mulchk3(insptr,addr1,addr2,addr3)
struct mnemonic *insptr;
OPERAND *addr1,*addr2,*addr3;
{
	if (!strncmp(insptr->name, "MUL",3) ||
	    !strncmp(insptr->name, "DIV",3)) {
		int type1,type2,type3;

		type1 = ((addr1->newtype == NOTYPE) ? optype(insptr->tag,1) :
			(addr1->newtype&0x04));
		type2 = ((addr2->newtype == NOTYPE) ? type1 :
			(addr2->newtype&0x04));
		type3 = ((addr3->newtype == NOTYPE) ? type2 :
			(addr3->newtype&0x04));

		if ((addr1->type == IMMD) && (addr1->expval < 0))
			type1 = 1;
		if ((addr2->type == IMMD) && (addr2->expval < 0))
			type2 = 1;

	if ((type1 || type2) && !type3)	/* src is signed, dst is unsigned */
werror("Mixed signs with unsigned destination can cause erroneous flags"); 
	}
} 

#endif

#if	ER21WAR
void
er21(insptr,addr1,addr2,addr3)
struct mnemonic *insptr;
OPERAND *addr1,*addr2,*addr3;
{
	if (!strncmp(insptr->name, "MOD",3) && (insptr->name[4] == '3')) {
		int type1,type2,type3;

		type1 = ((addr1->newtype == NOTYPE) ? optype(insptr->tag,1) :
			(addr1->newtype&0x04));
		type2 = ((addr2->newtype == NOTYPE) ? type1 :
			(addr2->newtype&0x04));
		type3 = ((addr3->newtype == NOTYPE) ? type2 :
			(addr3->newtype&0x04));

		if ((addr1->type == IMMD) && (addr1->expval < 0))
			type1 = 1;
		if ((addr2->type == IMMD) && (addr2->expval < 0))
			type2 = 1;

		if (type2 && !type3)  /* type 2 is signed, type 3 is unsigned */
werror("Mixed signs with unsigned destination can cause erroneous flags");
	}
} /*  er21 */

#endif


void
decrstk(i)
int i;
{
	struct mnemonic *tmpins;

	Genline = line;
	tmpins = find_mnemonic("SUBW2");
	generate(8, NOACTION, tmpins->opcode, NULLSYM);
	generate(8, NOACTION, i, NULLSYM);
	generate(8, NOACTION, (long)((CREGMD<<4)|SPREG), NULLSYM);
}

void
incrstk(i)
int i;
{
	struct mnemonic *tmpins;

	Genline = line;
	tmpins = find_mnemonic("ADDW2");
	generate(8, NOACTION, tmpins->opcode, NULLSYM);
	generate(8, NOACTION, i, NULLSYM);
	generate(8, NOACTION, (long)((CREGMD<<4)|SPREG), NULLSYM);
}

void
gen2(op,addr1,addr2)
char *op;
OPERAND *addr1;
OPERAND *addr2;
{
	struct mnemonic *tmpins;

	Genline = line;
	tmpins = find_mnemonic(op);
	generate (tmpins->nbits, NOACTION, tmpins->opcode, NULLSYM);
	addrgen(tmpins,addr1,addr1->newtype,1);
	addrgen(tmpins,addr2,addr2->newtype,2);
}

#if ER16FIX || ER76FIX

/*
 *	ER76:
 *
 *	If an interrupt is applied during a particular one cycle window during
 *	the execution of a divide or modulo instruction that has a numerator
 *	of zero, the destination is written and then the interrupt is taken
 *	with the PC still pointing to the divide or modulo instruction.  After
 *	the interrupt is processed, the divide or modulo instruction will be
 *	re-executed.  This is only a problem if the destination is used
 *	as part of the address calculation for one of the sources. For example:
 *
 *		MODW3	%r5,%r6,%r5
 *
 *			or
 *
 *		MODW3	4(%r5),%r6,%r5
 *
 *	Identical denominator and destination (op1, op3) always have the problem.
 *	Identical numerator and destination never have the problem. (We clobber
 *	zero with zero).  If the numerator or denominator use the destination
 *	in their address calculation, they will exhibit the fault.
 */

er16fix(insptr, arg1, arg2, arg3)
struct mnemonic *insptr;
OPERAND *arg1;
OPERAND *arg2;
OPERAND *arg3;
{
	static int er16chk();
	static int er76chk();
	extern int overlap();
	OPERAND c_tmp1;
	OPERAND *c_arg1 = &c_tmp1;
	OPERAND c_tmp2;
	OPERAND *c_arg2 = &c_tmp2;
	OPERAND c_tmp3;
	OPERAND *c_arg3 = &c_tmp3;
	struct mnemonic *tmpins;
	struct mnemonic *mov = find_mnemonic("MOVW");
	long opcode;
	unsigned char ntype;

	if (!workaround)
		return(0);
	if (warlevel == ALLWAR) {
		if (!er16chk(arg1,arg3) && !er16chk(arg2,arg3))
			if (insptr->name[1] == 'u' || insptr->name[1] == 'U' ||
				insptr->name[2] == 'u' || !er76chk(arg1,arg2,arg3))
					return(0);
	}
	else {
		if (insptr->name[1] == 'u' || insptr->name[1] == 'U' ||
			insptr->name[2] == 'u' || !er76chk(arg1,arg2,arg3))
				return(0);
	}

	if (insptr->name[0] == 'u')	/* if it is 'u' are definitely staying */
		if (insptr->name[4] == 'b')
			arg1->newtype = UBYTE;
		else if (insptr->name[4] == 'h')
			arg1->newtype = UHALF;
		else arg1->newtype = UWORD;

	switch (insptr->opcode) {
	case 0xea:		/* MULH3|mulh3|umulh3  */
		if (insptr->name[0] != 'M')
			insptr = find_mnemonic("MULH3");
		break;
	case 0xeb:		/* MULB3|mulb3|umulb3  */
		if (insptr->name[0] != 'M')
			insptr = find_mnemonic("MULB3");
		break;
	case 0xe8:		/* MULW3|mulw3|umulw3 */
		if (insptr->name[0] != 'M')
			insptr = find_mnemonic("MULW3");
		break;
	case 0xee:		/* DIVH3|divh3|udivh3  */
		if (insptr->name[0] != 'D')
			insptr = find_mnemonic("DIVH3");
		break;
	case 0xef:		/* DIVB3|divb3|udivb3  */
		if (insptr->name[0] != 'D')
			insptr = find_mnemonic("DIVB3");
		break;
	case 0xec:		/* DIVW3|divw3|udivw3 */
		if (insptr->name[0] != 'D')
			insptr = find_mnemonic("DIVW3");
		break;
	case 0xe6:		/* MODH3|modh3|umodh3  */
		if (insptr->name[0] != 'M')
			insptr = find_mnemonic("MODH3");
		break;
	case 0xe7:		/* MODB3|modb3|umodb3  */
		if (insptr->name[0] != 'M')
			insptr = find_mnemonic("MODB3");
		break;
	case 0xe4:		/* MODW3|modw3|umodw3 */
		if (insptr->name[0] != 'M')
			insptr = find_mnemonic("MODW3");
		break;
	default:
		return(0);
	}

/*
 * If the denominator(addr1) and the result(addr3) do not
 * "overlap" we can generate a better workaround:
 *
 *	MOVx	arg2,arg3
 *	yyyx2	arg1,arg3
 *
 *	where yyy = {MOD|MUL|DIV}
 *	and   x   = {W|H|B}
 *
 * "Overlap" means that they could be the same or overlapping memory
 * locations, or the same register.
 *
 */
		/* assume largest size, can't change types if dyadic */
	if (!overlap(arg1,4,arg3,4) && (arg2->newtype == arg3->newtype)) {
		opcode = mov->opcode;		/* MOV of proper size */
		if (insptr->name[3] == 'H')
			opcode += 2;
		else if (insptr->name[3] == 'B')
			opcode += 3;
		generate(mov->nbits,NOACTION,opcode, NULLSYM);
/*
 * Strange expand modes as follows:
 *	arg1's expand mode is passed to addrgen,
 *	if arg2 has a type it will be used, else arg1's
 */
		addrgen(mov,arg2,arg1->newtype,1);
/*
 * And similarly, we pass the last controlling type
 * for arg3
 */
		addrgen(mov,arg3,
			((arg2->newtype == NOTYPE) ? 
				arg1->newtype :
				arg2->newtype),2);

	/* anding with 0xBF produces the dyadic instruction */
		generate(insptr->nbits,NOACTION,(insptr->opcode&0xBF),NULLSYM);
		addrgen(insptr,arg1,NOTYPE,1);
		addrgen(insptr,arg3,arg2->newtype,2);
		return(1);
	}

	ntype = NOTYPE;
	if (arg1->newtype == NOTYPE)
		switch(optype(insptr->tag,1)) {
		case 0:	/* byte */
			ntype = UBYTE;
			break;
		case 1:	/* half */
			ntype = SHALF;
			break;
		case 2:	/* word */
			ntype = NOTYPE;
			break;
		}
	else
		ntype = arg1->newtype;
	if (arg2->newtype != NOTYPE)
		ntype = arg2->newtype;
	if (arg3->newtype != NOTYPE)
		ntype = arg3->newtype;
	if (arg3->type == REGMD)
		ntype = SWORD;

	incrstk(12);

	*c_arg1 = *arg1;
	*c_arg2 = *arg2;
	*c_arg3 = *arg3;

	if (((1<<c_arg1->type) & ((1<<DSPMD)|(1<<DSPDFMD)|(1<<REGDFMD))) &&
	    (c_arg1->reg == SPREG))
		c_arg1->expval -= 12;
/* SUBW3 &12,%sp,c_arg1 */
	if ((c_arg1->type == REGMD) && (c_arg1->reg == SPREG)) {
		c_arg1->type = DSPMD;
		c_arg1->exptype = ABS;
		c_arg1->symptr = NULLSYM;
		c_arg1->expval = -8;

		Genline = line;
		tmpins = find_mnemonic("SUBW3");
		generate(tmpins->nbits, NOACTION, tmpins->opcode, NULLSYM);
		generate(8, NOACTION, 0x12L, NULLSYM);
		generate(8, NOACTION, (long)((CREGMD<<4)|SPREG), NULLSYM);
		addrgen(tmpins, c_arg1, NOTYPE, 3);
		generate (8, NOACTION, 0x70, NULLSYM); /* NOP */
	}

	if (((1<<c_arg2->type) & ((1<<DSPMD)|(1<<DSPDFMD)|(1<<REGDFMD))) &&
	    (c_arg2->reg == SPREG))
		c_arg2->expval -= 12;
/* SUBW3  &12,%sp,c_arg2	*/
	if ((c_arg2->type == REGMD) && (c_arg2->reg == SPREG)) {
		c_arg2->type = DSPMD;
		c_arg2->exptype = ABS;
		c_arg2->symptr = NULLSYM;
		c_arg2->expval = -4;

		Genline = line;
		tmpins = find_mnemonic("SUBW3");
		generate(tmpins->nbits, NOACTION, tmpins->opcode, NULLSYM);
		generate(8, NOACTION, 0x12L, NULLSYM);
		generate(8, NOACTION, (long)((CREGMD<<4)|SPREG), NULLSYM);
		addrgen(tmpins, c_arg2, NOTYPE, 3);
		generate (8, NOACTION, 0x70, NULLSYM); /* NOP */
	}

	c_arg3->type = DSPMD;
	c_arg3->reg = SPREG;
	c_arg3->exptype = ABS;
	c_arg3->symptr = NULLSYM;
	c_arg3->expval = -12;
	if (arg3->type == REGMD)
		c_arg3->newtype = ntype;

	Genline = line;
	generate(8,NOACTION,insptr->opcode,NULLSYM);
	addrgen(insptr,c_arg1,NOTYPE,1);
	addrgen(insptr,c_arg2,NOTYPE,2);
	addrgen(insptr,c_arg3,NOTYPE,3);
	generate (8, NOACTION, 0x70, NULLSYM); /* NOP */

	if (((1<<arg3->type) & ((1<<DSPMD)|(1<<DSPDFMD)|(1<<REGDFMD))) &&
	    (arg3->reg == SPREG))
		arg3->expval -= 12;

	c_arg3->newtype = ntype;
	gen2("MOVW",c_arg3,arg3);
	if (arg3->type != REGMD)
		generate (8, NOACTION, 0x70, NULLSYM); /* NOP */

	decrstk(12);

	if (((1<<arg3->type) & ((1<<DSPMD)|(1<<DSPDFMD)|(1<<REGDFMD))) &&
	    (arg3->reg == SPREG))
		arg3->expval += 12;

	*c_arg3 = *arg3;
	arg3->newtype = ntype;
	c_arg3->newtype = NOTYPE;
	gen2("MOVW",arg3,c_arg3);	/* Restore condition codes */

	if (arg3->type != REGMD)
		generate (8, NOACTION, 0x70, NULLSYM); /* NOP */
	return(1);
}
#endif



static er16chk(addr1,addr2)
OPERAND *addr1;
OPERAND *addr2;
{
#if ER16FIX
	if (addr2->type == REGMD)
		return(YES);

	if (addr1->type == REGMD)
		return(NO);
	switch (addr2->type) {
	case REGDFMD:
		if (addr1->type == DSPMD)
			if ((addr1->reg == addr2->reg) && (l_abs(addr1->expval) >= 4))
				return(NO);
		break;
	case DSPMD:
		if (addr1->type == DSPMD)
			if ((addr1->reg == addr2->reg) &&
			   (l_abs(addr1->expval - addr2->expval) >=4))
				return(NO);
		break;
	case ABSMD:
		if (addr1->type == ABSMD)
			if (l_abs(addr1->expval - addr2->expval) >= 4)
				return(NO);
		break;
	case IMMD:
		return(NO);
	}
	return(YES);
#else
	return(NO);
#endif
}




static er76chk(addr1,addr2,addr3)
OPERAND *addr1;
OPERAND *addr2;
OPERAND *addr3;
{
#if	ER76FIX
	switch (addr1->type) {
	case REGMD:
		if (addr3->type == REGMD && addr1->reg == addr3->reg)
			return(YES);
		break;
	case REGDFMD:
		if (addr3->type == REGMD && addr1->reg != addr3->reg)
			break;
		if (addr3->type == DSPMD && addr1->reg == addr3->reg &&
			l_abs(addr3->expval) >= 4)
			break;
		return(YES);
	case ABSMD:
		if (addr3->type == REGMD)
			break;
		if ((addr3->type == ABSMD) &&
			(l_abs(addr1->expval - addr3->expval) >= 4))
			break;
		return(YES);
	case ABSDFMD:
	case EXADMD:
	case EXADDFMD:
		if (addr3->type == REGMD)
			break;
		return(YES);
	case DSPMD:
		if (addr3->type == REGMD && addr1->reg != addr3->reg)
			break;
		if (addr3->type == DSPMD && addr1->reg == addr3->reg &&
			(l_abs(addr1->expval - addr3->expval) >= 4))
			break;
		return(YES);
	case DSPDFMD:
		if (addr3->type == REGMD && addr1->reg != addr3->reg)
			break;
		return(YES);
	}
/*
 *	Now check to see if operand two overlaps with the destination,
 *	and that the destination is a register.
 */
	if (addr2->type & ((1<<DSPMD)|(1<<DSPDFMD)|(1<<REGDFMD)))
		if (addr3->type == REGMD && addr2->reg == addr3->reg)
			return (YES);
#endif
	return(NO);
}


#if	ER40FIX

void
er40gen()
{
	struct mnemonic *tmpins;

	if (!workaround)
		return;
	Genline = line;
	tmpins = find_mnemonic("NOP");
	generate(8,NOACTION,tmpins->opcode,NULLSYM);
}

#endif

#if	ER43ERR

#define	TMEM		0
#define	WHO_CARES	1

void
er43chk(insptr,word)
struct mnemonic *insptr;
long word;
{
	register int size = 2;
	int size1, size2, size3;
	int type1, type2, type3;
	static void maumap();

	if ((word & 0xff000000) != 0)
		return;

	maumap(word,1,&size1,&type1);
	maumap(word,2,&size2,&type2);
	maumap(word,3,&size3,&type3);
	
	switch (insptr->opcode) {
	case 0x32:	/* SPOP */
		if (type1 != TMEM && type2 != TMEM && type3 != TMEM)
			return;
		break;
	case 0x22:	/* SPOPRS */
		--size;
	/* FALLTHROUGH */
	case 0x02:	/* SPOPRD */
		--size;
	/* FALLTHROUGH */
	case 0x06:	/* SPOPRT */
		if (type3 == TMEM)
			break;
		if ((type1 == TMEM && size1 == size) && type2 != TMEM)
			return;
		if ((type2 == TMEM && size2 == size) && type1 != TMEM)
			return;
		break;
	case 0x33:	/* SPOPWS */
		--size;
	/* FALLTHROUGH */
	case 0x13:	/* SPOPWD */
		--size;
	/* FALLTHROUGH */
	case 0x17:	/* SPOPWT */
		if (type3 != TMEM || size3 != size)
			break;
		if (type1 != TMEM && type2 != TMEM)
			return;
		break;
	case 0x23:	/* SPOPS2 */
		--size;		
	/* FALLTHROUGH */
	case 0x03:	/* SPOPD2 */
		--size;
	/* FALLTHROUGH */
	case 0x07:	/* SPOPT2 */
		if (type3 != TMEM || size3 != size)
			break;
		if ((type1 == TMEM && size1 == size) && type2 != TMEM)
			return;
		if ((type2 == TMEM && size2 == size) && type1 != TMEM)
			return;
		break;
	}
	yyerror("Opcode/command word conflict in MAU's instruction.");
}

static void
maumap(word,opnum,size,type)
long word;
int opnum;
int *size;
int *type;
{
	int oprnd;	

	if (opnum == 1)
		oprnd = ((word >> 7) & 0x07);
	else if (opnum == 2)
		oprnd = ((word >> 4) & 0x07);
	else
		oprnd = (word & 0x0f);

	*size = (oprnd & 0x03);		/* size correct only if type == TMEM */
	*type = WHO_CARES;
	if (opnum == 3) {
		if ((oprnd >= 0x0c) && (oprnd != 0x0f))
			*type = TMEM;
	}
	else
		if ((oprnd & 0x04) && (oprnd != 0x07))
			*type = TMEM;
}
#endif
