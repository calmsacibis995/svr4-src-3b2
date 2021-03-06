/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/gencode.c	1.7"
#include <stdio.h>
#include "systems.h"
#include "symbols.h"
#include "program.h"
#include "instab.h"
#include "gendefs.h"
#include "expand.h"

#define GET_RELTYPE(reloc_type) \
	switch(reloc_type) {	 \
		case PC:  action = RELPC32;	\
			  break;		\
		case GOT: action = RELGOT32;	\
			  break;		\
		case PLT: action = RELPLT32;	\
		       	  break;		\
		default: action = RELDIR32;	\
	}


static long swap_b2();
long swap_b4();
extern struct mnemonic *find_mnemonic();
extern short spanopt;		/* span-dependent optimization flag */
extern unsigned short line;	/* current line number */
extern int Genline;
extern int optype();
extern void pcintcheck();

void
addrgen(insptr,addr,expand,opnum)
register struct mnemonic *insptr;
register OPERAND *addr;
unsigned char expand, opnum;
{
	register long desc;
	long vallen = 0;
	int action = NOACTION;
	register long val = addr->expval;
	register long reg = addr->reg;

		/* check if USER specified an expand byte */
		/* check if USER specified an operand specifier */
	if (insptr->tag & IS25) {
		if (addr->newtype != NOTYPE)
			yyerror("Expand byte invalid in IS25 instruction");
		if (addr->expspec != NULLSPEC)
			yyerror("Operand specifier invalid in IS25 instruction");
	} else
		if (addr->newtype != NOTYPE)
		expand = addr->newtype;
	if (expand != NOTYPE)
		generate(8,NOACTION,(long)((CEXPAND<<4)|expand),NULLSYM);

	switch(addr->type) {

	case REGDFMD:
		if (reg == PSWREG || reg == PCREG) {
			yyerror("Register deferred mode illegal with %psw or %pc");
			return;
		}
		desc = ((CREGDFMD<<4)|reg);
		break;

	case DSPMD:
		if (reg == PSWREG) {
			yyerror("Displacement mode is illegal with %psw");
			return;
		}
		if (addr->symptr == NULLSYM
		&&  addr->expspec == NULLSPEC
		&&  spanopt) {
/* No operand specifier, and optimization allowed, and symbol ptr is null */
			if ((val == 0) && (reg != PCREG)) {
				desc = ((CREGDFMD<<4)|reg);
				break;
			}
			if ((reg == APREG) && (0 <= val && val <= 14)) {
				desc = ((CAPDSPMD<<4)|val);
				break;
			}
			if ((reg == FPREG) && (0 <= val && val <= 14)) {
				 /* FP displacement mode */
				desc = ((CFPDSPMD<<4)|val);
				break;
			}
			if (-(1<<7) <= val && val < 1<<7) {
				desc = ((CBDSPMD<<4)|reg);
				vallen = 8;
				break;
			}
			if (-(1L<<15) <= val && val < 1L<<15) {
				desc = ((CHDSPMD<<4)|reg);
				vallen = 16;
				break;
			}
			desc = ((CDSPMD<<4)|reg);
			vallen = 32;
			break;
		} /* addr->symptr == NULLSYM */

		if ((addr->symptr != NULLSYM) || 
		   ((!spanopt) && (addr->expspec == NULLSPEC))) {
			desc = ((CDSPMD<<4)|reg);
			GET_RELTYPE(addr->pic_rtype);
			vallen = 32;
			break;
		} 

/*	At this point the user must be specifying the operand 
 *	expression size through the use of an operand specifier.
 *	Generate the correct operand.
 */
		switch (addr->expspec) {
		case BYTESPEC:
			if ((unsigned) val >= (1<<8)) {
				yyerror("Expression larger than byte.");
				return;
			}
			desc = ((CBDSPMD<<4)|reg);
			vallen = 8;
			break;
		case HALFSPEC:		/* halfword displacement mode */
			if ((unsigned) val >= (1<<16)) {
				yyerror("Expression larger than halfword.");
				return;
			}
			desc = ((CHDSPMD<<4)|reg);
			vallen = 16;
			break;
		case WORDSPEC:
			desc = ((CDSPMD<<4)|reg);
			vallen = 32;
			break;
		case SHORTSPEC:
			if (0 > val || val > 14) {
				yyerror("Expression outside short offset range.");
				return;
			}
			if (reg == APREG) {	/* AP displacement mode */
				desc = ((CAPDSPMD<<4)|val);
				break;
			}
			if (reg == FPREG) {	/* FP displacement mode */
				desc = ((CFPDSPMD<<4)|val);
				break;
			}
			yyerror("Illegal register for short offset address mode.");
			return;
		default:
			yyerror("Illegal operand specifier for displacement address mode.");
			return;
		}
		break;
	case DSPDFMD: /* displacement deferred mode */
		if (reg == PSWREG) {
			yyerror("Displacement deferred mode illegal with %psw");
			return;
		}
		if (addr->symptr == NULLSYM
		&&  addr->expspec == NULLSPEC
		&&  spanopt) {
			if (-(1<<7) <= val && val < 1<<7) {
				desc = ((CBDSPDFMD<<4)|reg);
				vallen = 8;
				break;
			}
			if (-(1L<<15) <= val && val < 1L<<15) {
				desc = ((CHDSPDFMD<<4)|reg);
				vallen = 16;
				break;
			}
			desc = ((CDSPDFMD<<4)|reg);
			vallen = 32;
			break;
		}
		if ((addr->symptr != NULLSYM) ||
		     ((!spanopt) && (addr->expspec == NULLSPEC))) {
			GET_RELTYPE(addr->pic_rtype);
			desc = ((CDSPDFMD<<4)|reg);
			vallen = 32;
			break;
		}

/*
 *	At this point the user must be specifying the operand
 *	expression size through the use of an operand specifier.
 *	Generate the correct operand.
 */
		switch (addr->expspec) {
		case BYTESPEC:		/* byte displacement mode */
			if ((unsigned) val >= (1<<8)) {
				yyerror("Expression larger than byte.");
				return;
			}
			desc = ((CBDSPDFMD<<4)|reg);
			vallen = 8;
			break;
		case HALFSPEC:		/* halfword displacement mode */
			if ((unsigned) val >= (1<<16)) {
				yyerror("Expression larger than halfword.");
				return;
			}
			desc = ((CHDSPDFMD<<4)|reg);
			vallen = 16;
			break;
		case WORDSPEC:
			desc = ((CDSPDFMD<<4)|reg);
			vallen = 32;
			break;
		default:
			yyerror("Illegal operand specifier for displacement deferred address mode.");
			return;
		}

		break;
	case EXADMD: /* external address mode */
	case ABSMD: /* absolute address mode */
		desc = CABSMD;
		action = RELDIR32;
		vallen = 32;
		break;
	case EXADDFMD: /* external address deferred mode (PC relative deferred) */
	case ABSDFMD: /* absolute address deferred mode */
		desc = CABSDFMD;
		action = RELDIR32;
		vallen = 32;
		break;
	case IMMD: /* immediate mode */
		if (addr->symptr == NULLSYM
		&&  addr->expspec == NULLSPEC
		&&  spanopt) {
			if ((val < 0) &&
			    (insptr->tag & IS25) && insptr->name[0] == 'u') {
					desc = CIMMD;
					vallen = 32;
					break;
			}
			if (-(1<<4) <= val && val < 1<<6) {
				desc = val;
				break;
			}
			if (-(1<<7) <= val && val < 1<<7) {
				desc = CBIMMD;
				vallen = 8;
				break;
			}
			if (-(1L<<15) <= val && val < 1L<<15) {
				desc = CHIMMD;
				vallen = 16;
				break;
			}
			desc = CIMMD;
			vallen = 32;
			break;
		}
		if ((addr->symptr != NULLSYM) ||
		   ((!spanopt) && (addr->expspec == NULLSPEC))) {
			action = RELDIR32;
			desc = CIMMD;
			vallen = 32;
			break;
		}

/*
 *	At this point the user must be specifying the operand
 *	expression size through the use of an operand specifier.
 *	Generate the correct operand.
 */
		switch (addr->expspec) {
		case BYTESPEC:		/* byte displacement mode */
			if ((unsigned) val >= (1<<8)) {
				yyerror("Expression larger than byte.");
				return;
			}
			desc = CBIMMD;
			vallen = 8;
			break;
		case HALFSPEC:		/* halfword displacement mode */
			if ((unsigned) val >= (1<<16)) {
				yyerror("Expression larger than halfword.");
				return;
			}
			desc = CHIMMD;
			vallen = 16;
			break;
		case WORDSPEC:
			desc = CIMMD;
			vallen = 32;
			break;
		case LITERALSPEC:
			if (-(1<<4) > val || val >= 1<<6) {
				yyerror("Expression larger than literal.");
				return;
			}
			desc = val;
			break;
		default:
			yyerror("Illegal operand specifier for immediate address mode.");
			return;
		}

		break;
#if	FLOAT
	case FPIMMD:
		if (optype(insptr->tag,opnum) == 3) {  /* double */
			desc = CDIMMD;
			vallen = 64;
		}
		else {		/* single */
			desc = CIMMD;
			vallen = 32;
		}
		break;
#endif
	case REGMD:
		if ( reg == PCREG ) {
			yyerror("Register mode illegal with register 15 (%pc).");
			return;
		}
		desc = ((CREGMD<<4)|reg);
		break;
	default:
		yyerror("Assembler error: invalid addressing mode");
		return;
	}
	generate(8,NOACTION,(long) desc, NULLSYM);
	switch (vallen) {
	case 0:
		return;
	case 8:
		exp_generate(8,NOACTION,*addr);
		break;
	case 16:
		if (UNEVALUATED(*addr))
			Generate(16,SWAP_B2,val,addr->symptr,1);
/* or			exp_generate(16,SWAP_B2,*addr); */
		else
			generate(16,NOACTION,swap_b2(val),addr->symptr);
		break;
	case 32:
		if (action)
			exp_generate(32,action,*addr);
		else if (UNEVALUATED(*addr))
			Generate(32,SWAP_B4,val,addr->symptr,1);
/* or			exp_generate(32,SWAP_B4,*addr); */
		else
			generate(32,NOACTION,swap_b4(val),addr->symptr);
		break;
	case 64:
       		/* addr->unevaluated == 0*/
		generate(32,NOACTION,swap_b4(val),addr->symptr);
		generate(32,NOACTION,swap_b4((long)(addr->fasciip)),addr->symptr);
		break;
	}
}


static long
swap_b2(val)
long val;
{
	long b1, b2;

	b1 = (val >> 8) & 0x00FFL;	/* 2nd low goes to low */
	b2 = (val << 8) & 0xFF00L;	/* low goes to high */
	return (b2|b1);
} /* swap_b2 */

long
swap_b4(val)
long val;
{
	long b1, b2, b3, b4;

	b1 = (val >> 24) & 0x000000FFL;	/* high goes to low */
	b2 = (val >>  8) & 0x0000FF00L;	/* 2nd high goes to 2nd low */
	b3 = (val <<  8) & 0x00FF0000L;	/* 2nd low goes to 2nd high */
	b4 = (val << 24) & 0xFF000000L;	/* low goes to high */
	return (b1|b2|b3|b4);
} /* swap_b4 */

optype(instype,opnum)
unsigned short instype;
unsigned char opnum;
{
/*
 * figure out operand type according to instruction type and operand number
 * return:	0=byte	1=half	2=word	3=double   4=double extended
 */

	switch (instype & (~IS25)) {

	case INSTRB:
		return 0;

	case INSTRH:
		return 1;

	case INSTRW:
		return 2;

#if	FLOAT
	case INSTRS:
		return 2;
	case INSTRD:
		return 3;
	case INSTRX:
		return 4;
	case INSTRSD:
		return (opnum==1) ? 2 : 3;
	case INSTRSX:
		return (opnum==1) ? 2 : 4;
	case INSTRDS:
		return (opnum==1) ? 3 : 2;
	case INSTRDX:
		return (opnum==1) ? 3 : 4;
	case INSTRXS:
		return (opnum==1) ? 4 : 2;
	case INSTRXD:
		return (opnum==1) ? 4 : 3;
#endif

	case INSTRBH:
		return (opnum==1) ? 0 : 1;

	case INSTRBW:
		return (opnum==1) ? 0 : 2;

	case INSTRHW:
		return (opnum==1) ? 1 : 2;

	case INSTRWH:
		return (opnum==1) ? 2 : 1;

	case INSTRWB:
		return (opnum==1) ? 2 : 0;

	case INSTRHB:
		return (opnum==1) ? 1 : 0;

	default:		/* should not happen, but just in case */
		return 2;	/* treat as word operand */
	}
} /* optype */

void
convgen(insptr,addr1,addr2)
struct mnemonic	*insptr;
register OPERAND
	*addr1,
	*addr2;
{
	/*
	 *
	 * movb[bh|bw|hw] and movt[wb|hb|wh] instructions;
	 *
	 * All are encoded as expand mode mappings except
	 * for movtXY with a register destination.
	 * These are encoded as follows:
	 *
	 *	movthb	X,%ry	-> andh3 &0xFF,X,%ry
	 *	movtwb	X,%ry	-> andw3 &0xFF,X,%ry
	 *	movtwh	X,%ry	-> movw  X,%ry ; movh %ry,%ry
	 *
	 */

	static OPERAND maskop = {NOTYPE,IMMD,0x01,ABS,0,NULLSYM,0xFFL,0};
	register unsigned char expand;
	struct mnemonic	*newins;		/* set up dummy operands for movtwb */
	char	*maskinst = "andw3";

	/* check for movtXY with a register dest. */
	if (addr2->type == REGMD)
		switch (insptr->tag & (~IS25)) {
		case INSTRHB:
			/* change instruction for movthb */
			maskinst = "andh3";
		/* FALLTHROUGH */
		case INSTRWB:
			newins = find_mnemonic(maskinst);
			generate(8,NOACTION,newins->opcode,NULLSYM);
			addrgen(newins,&maskop,NOTYPE,1); /* mask operand */
			addrgen(insptr,addr1,NOTYPE,2);
			addrgen(insptr,addr2,UBYTE,3);
			return;
		case INSTRWH:
			generate(8,NOACTION,insptr->opcode,NULLSYM);  /* movw */
			addrgen(insptr,addr1,NOTYPE,1);
			addrgen(insptr,addr2,NOTYPE,2);
			Genline = line;
			newins = find_mnemonic("movh");
			generate(8,NOACTION,newins->opcode,NULLSYM);
			addrgen(insptr,addr2,NOTYPE,1);
			addrgen(insptr,addr2,NOTYPE,2);
			return;
		} /* switch */

	generate(insptr->nbits,NOACTION,insptr->opcode,NULLSYM);
	addrgen(insptr,addr1,NOTYPE,1);
	switch (optype(insptr->tag,2)) {
		case 0: /* byte */
			expand = SBYTE;
			break;
		case 1: /* half */
			expand = SHALF;
			break;
		case 2: /* word */
			expand = SWORD;
			break;
	} /* switch */

	addrgen(insptr,addr2,expand,2);
} /* convgen */

void
testgen(insptr, addr1)
register	struct mnemonic		*insptr;
register	OPERAND	*addr1;
{
	/*
	 *	NOP2 NOP3
	 *
	 *		This routine will generate the filler bytes
	 *	for the above two nops.
	 */
	
	int	val;

	if ( addr1->newtype != NOTYPE ) {
		yyerror("Expand byte illegal with NOP2 or NOP3 instructions.");
		return;
	}

	if ( addr1->expspec != NULLSPEC ) {
		yyerror("Operand specifier illegal with NOP2 or NOP3 instructions.");
		return;
	}

	generate( insptr->nbits, NOACTION, insptr->opcode, NULLSYM);
	val = addr1->expval;

	if ( insptr->tag ==  INSTRB ) {
		if ((unsigned) val >= (1L << 8)) {
		      yyerror("Optional immediate for NOP2 larger than byte.");
		} else {
			generate( 8, NOACTION, val, NULLSYM);
		}
	} else {
		if ((unsigned) val >= (1L << 16)) {
		      yyerror("Optional immediate for NOP3 larger than halfword.");
		} else {
			generate(16, NOACTION, val, NULLSYM);
		}
	}
} /* testgen */

void
opt21gen(insptr,addr1,addr2,constant)
struct mnemonic	*insptr;
register OPERAND
	*addr1,
	*addr2;
short	constant;
{
	/*
	 * This function attempts to optimize certain dyadic instructions
	 * into monadics when the first operand is a particular
	 * immediate constant.
	 *
	 * The possible optimizations are:
	 *
	 * Dyadic	Constant	Monadic
	 *
	 * mov[bhw]	0		CLR[BHW]
	 * add[bhw]2	1		INC[BHW]
	 * sub[bhw]2	1		DEC[BHW]
	 * cmp[bhw]	0		TST[BHW]
	 *
	 *
	 * In all cases the dyadic opcode is stored in the low byte
	 * of the opcode field of the instruction table (pointed to by insptr)
	 * and the monadic opcode is stored in the 2nd low byte
	 *
	 */

	if (addr1->type==IMMD && addr1->exptype==ABS &&
		addr1->expval==(long)constant &&
		(insptr->tag & IS25) && spanopt)
	{
		/* generate monadic opcode */
		generate(8,NOACTION,MN_ALT(insptr->opcode),NULLSYM);
		addrgen(insptr,addr2,NOTYPE,2);
	}
	else {
		/* just do normal dyadic instruction */
		/* mask off 2nd low byte, but don't really have to */
		generate(8,NOACTION,MN_REG(insptr->opcode),NULLSYM);
		addrgen(insptr,addr1,NOTYPE,1);
		addrgen(insptr,addr2,NOTYPE,2);
	} /* else */
} /* opt21gen */

void
jmpopgen(insptr,opnd1)
struct mnemonic	*insptr;
OPERAND	*opnd1;
{
	struct mnemonic	*newins;
	long	opcd, altopcd;
	BYTE	nbits;
	int	action;
	SDI	*sdp;

	opcd = insptr->opcode;

	switch (insptr->tag) {
	case INSTRB: /* byte displacement */
		nbits = 8;
		if (opnd1->type != EXADMD)
			yyerror("Invalid operand");
		action = RELPC8;
		break;
	case INSTRH: /* halfword displacement */
		if (opnd1->type != EXADMD)
			yyerror("Invalid operand");
		nbits = 16;
		action = RELPC16;
		break;
	case IS25|INSTRW: /* IS25, word displacement */
		altopcd = MN_ALT(opcd);
		if (opnd1->type != EXADMD || UNEVALUATED(*opnd1)) {
			/* can't generate PC relative */
			if (altopcd != 0) {
				/* generate complementary branch */
				generate(8,NOACTION,altopcd,NULLSYM);
				generate(8,FNDBRLEN,0L,NULLSYM);
			}
			newins = find_mnemonic("JMP");
			generate(8,NOACTION,newins->opcode,NULLSYM);
			addrgen(insptr,opnd1,NOTYPE,1);
			return;
		}
		if ((sdp = shortsdi(opnd1->symptr, opnd1->expval)))
		{
			int	bits, action;
			bits = UBR_BSZ * BITSPBY;
			if (altopcd)
			{
				sdp->sd_itype = IT_CBR;
				action = CBRNOPT;
			}
			else
			{
				/*	opcode implicit for uncond jump
				 */

				opcd = 0;
				sdp->sd_itype = IT_UBR;
				action = UBRNOPT;
			}
			if (!spanopt)
			{
				sdp->sd_flags &= ~SDF_BYTE;
				bits = UBR_HSZ * BITSPBY;
			}
			generate(bits, action, opcd, sdp->sd_sym);
			return;
		}

		/* test for unconditional jump */
		if (altopcd != 0) {
			/* generate complementary branch */
			generate(8,NOACTION,altopcd,NULLSYM);
			generate(8,FNDBRLEN,0L,NULLSYM);
		}
		newins = find_mnemonic("JMP");
		generate(8,NOACTION,newins->opcode,NULLSYM);
		if (opnd1->symptr)
			PCREL_MODE(opnd1);
		addrgen(insptr,opnd1,NOTYPE,1);
		return;

	case INSTRW: /* word displacement */
			generate(8,NOACTION,opcd,NULLSYM);
			addrgen(insptr,opnd1,NOTYPE,1);
			return;
	default:
		aerror("assembler error; invalid operator type");
	} /* switch */

	generate(8,NOACTION,MN_REG(opcd),NULLSYM); /* not necessary to mask */
	exp_generate(nbits,action,*opnd1);
} /* jmpopgen */

void
jsbopgen(insptr,opnd1)
struct mnemonic	*insptr;
register OPERAND	*opnd1;
{
	long	opcd, altopcd;
	SDI	*sdp;
	int	bits;

	opcd = insptr->opcode;
	altopcd = MN_ALT(opcd);

	if (opnd1->type != EXADMD || UNEVALUATED(*opnd1)) {
		/* can't generate PC relative */
		generate(8,NOACTION,altopcd,NULLSYM);
		addrgen(insptr,opnd1,NOTYPE,1);
		return;
	}
	if ((sdp = shortsdi(opnd1->symptr, opnd1->expval)))
	{
		/*	opcode is implicit for BSBNOPT
		 */

		sdp->sd_itype = IT_JSB;
		bits = JSB_BSZ * BITSPBY;
		if (!spanopt)
		{
			bits = JSB_HSZ * BITSPBY;
			sdp->sd_flags &= ~SDF_BYTE;
		}
		generate(bits, BSBNOPT, 0L, sdp->sd_sym);
		return;
	}
	generate(8,NOACTION,altopcd,NULLSYM);
	addrgen(insptr,opnd1,NOTYPE,2);
	return;
}

void
callgen(insptr,imm,dest)
struct mnemonic	*insptr;
OPERAND	*imm, *dest;
{
	/*
	 *
	 * call	  imm,dest
	 *
	 *	is encoded as
	 *
	 * CALL	-(4*imm)(%sp),dest
	 *
	 */

	static OPERAND dispsp = {NOTYPE,DSPMD,SPREG,ABS,0,NULLSYM,1L,0};
	SDI	*sdp;
	int	bits;

if (imm->expval < MINARG  || imm->expval > MAXARG)
	aerror("Invalid argument to call instruction");
	
	dispsp.expval = - (4L * imm->expval );
	generate(8,NOACTION,insptr->opcode,NULLSYM); /* CALL opcode */
	addrgen(insptr,&dispsp,NOTYPE,1);

#ifdef CALLPCREL
	if (dest->type != EXADMD || UNEVALUATED(*dest))
	{
		addrgen(insptr,dest,NOTYPE,2);
		return;
	}
	if ((sdp = shortsdi(dest->symptr,dest->expval)))
	{
		sdp->sd_itype = IT_CALL;
		bits = CALL_BSZ * BITSPBY;
		if (!spanopt)
		{
			bits = CALL_HSZ * BITSPBY;
			sdp->sd_flags &= ~SDF_BYTE;
		}
		generate(bits, CALLNOPT, 0L, sdp->sd_sym);
		return;
	}
#endif
	addrgen(insptr,dest,NOTYPE,2);
	return;
}

void
loop1gen(insptr,index,incr,limit,dest)
struct mnemonic	*insptr;
OPERAND	*index, *incr, *limit, *dest;
{
	/*
	 *
	 * acX	index,incr,limit,dest
	 *	where X can be [jl|jlu|jle|jleu]
	 *
	 *	is encoded as
	 *
	 * addw2	incr,index
	 * cmpw 	index,limit
	 * X		dest
	 *
	 */

	struct mnemonic	*newins;

		/* addw2 */
	newins = find_mnemonic("addw2");
	opt21gen(newins,incr,index,1);	/* optimized "add" */
	pcintcheck(index);

		/* cmpw */
	Genline = line;
	newins = find_mnemonic("cmpw");
#if 0
	/* generate optimized "cmpw" and interchange operands */
	opt21gen(newins,limit,index,0);
#endif
	/* DONT generate optimized "cmpw" because of Interrupt
	 * After TSTW Chip Bug.
	 */
	generate(8,NOACTION,newins->opcode & 0xFFL,NULLSYM);
	addrgen(newins,limit,NOTYPE,1);
	addrgen(newins,index,NOTYPE,2);

		/* jump instr. */
	Genline = line;
	newins = find_mnemonic((insptr->name)+2);
	jmpopgen(newins,dest);
} /* loop1gen */

void
loop2gen(insptr,index,incr,dest)
struct mnemonic	*insptr;
OPERAND	*index, *incr, *dest;
{
	/*
	 *
	 * atjnz[bhw]	index,incr,dest
	 *
	 *	is encoded as
	 *
	 * addw2	incr,index
	 * TST[BHW]	*index
	 * jnz		dest
	 *
	 */

	struct mnemonic	*newins;

		/* addw2 */
	newins = find_mnemonic("addw2");
	opt21gen(newins,incr,index,1);	/* optimized "add" */

		/* TST[BHW] */
	Genline = line;
	generate(8,NOACTION,insptr->opcode,NULLSYM); /* TST opcode */

	/*
	 * To generate TST *index,
	 * must add a level of indirection to "index"
	 *
	 */

	switch(index->type) {

		case DSPMD:
		case EXADMD:
		case ABSMD:
			++(index->type); /* adds a level of indirection */
			break;
		case REGMD:
			index->type = DSPMD;
			index->exptype = ABS;
			index->expval = 0L;
			index->symptr = NULLSYM;
			break;
		default:
			yyerror("Invalid `index' operand of `atjnz'");
			break;
	} /* switch */

	addrgen(insptr,index,NOTYPE,1);

		/* jnz */
	Genline = line;
	newins = find_mnemonic("jnz");
	jmpopgen(newins,dest);
} /* loop2gen */

void
blockgen(insptr)
struct mnemonic	*insptr;
{
	/*
	 *
	 * movbl[bh]
	 *	is encoded as
	 *
	 * TSTW 	%r2
	 * BRB  	+12 (for half) or +14 (for byte)
	 * mov[bh]	(%r0),(%r1)
	 * NOP 
	 * addw2	&2,%r0 (for half) or INCW %r0 (for byte)
	 * addw2	&2,%r1 (for half) or INCW %r1 (for byte)
	 * DECW 	%r2
	 * BNEB 	-12 (for half) or -10 (for byte)
	 *
	 */

	struct mnemonic	*newins;
	short	byte;

	if( (insptr->tag & SHFT_MSK) == INSTRB)
		byte = YES;
	else
		byte = NO;

		/* TSTW */
	newins = find_mnemonic("TSTW");
	generate(8,NOACTION,newins->opcode,NULLSYM);	/* TSTW opcode */
	generate(8,NOACTION,(long)(CREGMD<<4|0x02),NULLSYM); /* %r2 */

		/* BRB */
	Genline = line;
	newins = find_mnemonic("BRB");
	generate(8,NOACTION,newins->opcode,NULLSYM);	/* unc. branch */
	if (byte == YES)
		generate(8,NOACTION,12L,NULLSYM);
	else
		generate(8,NOACTION,14L,NULLSYM);

		/* mov[bh] */
	Genline = line;
	generate(8,NOACTION,insptr->opcode,NULLSYM);	/* mov[bh] */
	generate(8,NOACTION,(long)(CREGDFMD<<4|0x00),NULLSYM); /* (%r0) */
	generate(8,NOACTION,(long)(CREGDFMD<<4|0x01),NULLSYM); /* (%r1) */

	/*
	 *  A NOP must ne inserted here because a write to memory 
	 *  was just generated.
	 */
	Genline = line;
	newins = find_mnemonic("NOP");
	generate(newins->nbits, NOACTION, newins->opcode, NULLSYM);

	Genline = line;
	if (byte == YES) {
		/* INCW - increment register 0 */
		/* INCW - increment register 1 */
	
		newins = find_mnemonic("INCW");
		generate(8,NOACTION,newins->opcode,NULLSYM);	/* INCW */
		generate(8,NOACTION,(long)(CREGMD<<4|0x00),NULLSYM); /* %r0 */
		Genline = line;
		generate(8,NOACTION,newins->opcode,NULLSYM);	/* INCW */
		generate(8,NOACTION,(long)(CREGMD<<4|0x01),NULLSYM); /* %r1 */
	}
	else {

		/* addw2 - increment register 0 */
		/* addw2 - increment register 1 */
		newins = find_mnemonic("addw2");
		generate(8,NOACTION,newins->opcode,NULLSYM);	/* addw2 */
		generate(8,NOACTION,2L,NULLSYM);
		generate(8,NOACTION,(long)(CREGMD<<4|0x00),NULLSYM); /* %r0 */
		Genline = line;
		generate(8,NOACTION,newins->opcode,NULLSYM);	/* addw2 */
		generate(8,NOACTION,2L,NULLSYM);
		generate(8,NOACTION,(long)(CREGMD<<4|0x01),NULLSYM); /* %r1 */
	}

		/* DECW */
	Genline = line;
	newins = find_mnemonic("DECW");
	generate(8,NOACTION,newins->opcode,NULLSYM);	/* DECW */
	generate(8,NOACTION,(long)(CREGMD<<4|0x02),NULLSYM); /* %r2 */

		/* BNEB */
	Genline = line;
	newins = find_mnemonic("BNEB");
	generate(8,NOACTION,newins->opcode,NULLSYM);	/* BNEB opcode */
	if (byte == YES)
		generate(8,NOACTION,-10L,NULLSYM);
	else
		generate(8,NOACTION,-12L,NULLSYM);
} /* blockgen */

void
pushopgen(insptr,opnd1)
struct mnemonic	*insptr;
OPERAND	*opnd1;
{
	/*
	 *
	 * push[zb][bh]	opnd1
	 *
	 *	is encoded as
	 *
	 * addw2	&4,%sp
	 * followed by one of the following:
	 *
	 *	if opnd1 does not use %sp
	 * mov[bh]	{X}opnd1,{sword}-4(%sp)
	 *
	 *	if opnd1=%sp
	 * subw3	&4,{X}%sp,{sword}-4(%sp)
	 *
	 *	if opnd1=expr(%sp)
	 * mov[bh]	{X}expr-4(%sp),{sword}-4(%sp)
	 *
	 *	if opnd1=*expr(%sp)
	 * mov[bh]	{X}*expr-4(%sp),{sword}-4(%sp)
	 *
	 *	where X = signed for push.b
	 *		  unsigned for push.h
	 */

	static OPERAND dispsp = {NOTYPE,DSPMD,SPREG,ABS,0,NULLSYM,-4L,0};
	static OPERAND immdop = {NOTYPE,IMMD,0x04,ABS,0,NULLSYM,4L,0};
	struct mnemonic	*newins;
	unsigned char expand;

		/* addw2 */
	newins = find_mnemonic("addw2");
	generate(8,NOACTION,newins->opcode,NULLSYM);
	addrgen(insptr,&immdop,NOTYPE,1);
	generate(8,NOACTION,(long)((CREGMD<<4)|SPREG),NULLSYM); /* operand %sp */

		/* decide on which second instruction to generate */
	switch (opnd1->type) {
	case REGMD:
		if (opnd1->reg == SPREG) {	/* if opnd1 is %sp then */
						/* generate subw3 instr */
			newins = find_mnemonic("subw3");
			generate(8,NOACTION,newins->opcode,NULLSYM);
			addrgen(insptr,&immdop,NOTYPE,1);
			generate(8,NOACTION,(long)((CREGMD<<4)|SPREG),NULLSYM);	/* oprnd %sp */
			addrgen(insptr,&dispsp,SWORD,3); /* oprnd {sword}-4(%sp) */
			pcintcheck(&dispsp);
			return;
		} else
			break;
	case REGDFMD:
	case DSPMD:
	case DSPDFMD:
		/* if register is %sp, subtract 4 from displacement */
		if (opnd1->reg == SPREG)
			opnd1->expval -= 4;
		break;
	} /* switch */

		/* mov[bh] opcode */
	generate(8,NOACTION,insptr->opcode,NULLSYM);
	switch (insptr->tag & (~IS25)) {
		case INSTRBW:
			expand = SBYTE;
			break;
		case INSTRHW:
			expand = UHALF;
			break;
		default:
			expand = NOTYPE;
	} /* switch */

	addrgen(insptr,opnd1,expand,1);
	addrgen(insptr,&dispsp,SWORD,2);
	pcintcheck(&dispsp);
} /* pushopgen */

void
shftopgen(insptr,opnd1,opnd2,opnd3)
struct mnemonic	*insptr;
OPERAND	*opnd1, *opnd2, *opnd3;
{
	unsigned char expand1, expand2;
	long	opcd, opcd2;

	opcd = insptr->opcode;
	if ((insptr->tag & IS25) &&
		(opnd1->type != REGMD) && (opnd1->type != IMMD))
	{
		expand2 = SWORD;
		/*
		 * The opcode field contains 2 8-bit opcodes.
		 * The lower opcode corresponds to the shift
		 * word opcode.  The higher opcode corresponds
		 * either to a shift word or a shift byte opcode.
		 * Since the shift length (opnd1) is a byte operand
		 * in IS25, the shift byte opcode will be used if
		 * there is one on the MAC32.  If there is no shift
		 * byte instruction, then the shift word will be used
		 * with an expand byte of signed-byte specified.
		 * Thus, test to see if the 2 opcodes are the same.
		 * If so, then the expand byte must be used.
		 */
		opcd2 = MN_REG(opcd); /* lower 8 bits */
		opcd = MN_ALT(opcd);
		if (opcd == opcd2)
			expand1 = SBYTE;
		else
			expand1 = NOTYPE;
	}
	else
		expand1 = expand2 = NOTYPE;

	generate(8,NOACTION,opcd,NULLSYM);
	addrgen(insptr,opnd1,expand1,1);
	addrgen(insptr,opnd2,expand2,2);
	addrgen(insptr,opnd3,NOTYPE,3);
} /* shftopgen */

short pswopt = NO;

void
flags(flag)
char flag;
{
	char errmsg[28];
	extern char dbg;
	switch(flag) {
	case 'G':
		pswopt = YES;
		break;
	case '#' : 
		(void) fprintf(stderr, "flags(#)\n");dbg = 1; break;
	default:
		(void) sprintf(errmsg,"Illegal flag (%c) - ignored",flag);
		werror(errmsg);
		break;
	}
}


