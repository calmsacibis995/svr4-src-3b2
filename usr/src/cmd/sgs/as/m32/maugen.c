/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/maugen.c	1.4.1.2"

#include <stdio.h>
#include "systems.h"
#include "symbols.h"
#include "instab.h"
#include "program.h"
#include "maugen.h"
#include "gendefs.h"
#include "section.h"

#define	mk_word(rb1,rb2,rb3,mode,opcode,op1,op2,op3)	\
		(((rb1) << 20) | ((rb2) << 19) | ((rb3) << 18) | ((mode) << 15) |\
		((opcode) << 10) | ((op1) << 7) | ((op2) << 4) | (op3))


extern long newdot;
extern unsigned short line;
extern int Genline;
static OPERAND temp1 = { NOTYPE, DSPMD, SPREG, NULL, 0, NULL, -4L, NULL, NULL};
static OPERAND temp2 = { NOTYPE, DSPMD, SPREG, NULL, 0, NULL, -8L, NULL, NULL};
static int isize, rsize;
static int rdwri;
static int spopnop = 1;

static void branchgen();
static void stkop();
static void gen1();

extern int txtsec, datsec;

extern int counter;
extern symbol *dot;

extern short wflag;

static long flags[8] = {
	0x000000,	/* 0 */
	0x010000,	/* 1 */
	0x100000,	/* 2 */
	0x110000,	/* 3 */
	0x200000,	/* 4 */
	0x210000,	/* 5 */
	0x300000,	/* 6 */
	0x310000	/* 7 */
};

int  overlap();
static int  do_op();
static void fimmed();
static void fimmgen();
static void spopgen();
extern void addrgen();
extern long swap_b4();
extern symbol *lookup();
struct mnemonic *find_mnemonic();

void
fpjmpgen(insptr,addr,sense)
struct mnemonic *insptr;
OPERAND *addr;
long sense;
{
	extern void incrstk();
	extern void decrstk();
	extern void gen2();
	unsigned word;
	register int stksz = ((sense == 3) ? 8 : 4);
	int nbits;
/*
**	Create a temporary on the stack to hold the ASR.
**	Generate a RDASR instruction to the temporary.
*/
	incrstk(stksz);

	word = mk_word(RB1,RB2,RB3,ASR_RC,RD_ASR,OP1NOP,OP2NOP,OP3MEM(0x00));
	spopnop = 0;
	spopgen(0,MAU_WR,word,(OPERAND *)NULL,&temp1);
	spopnop = 1;

/*
**	Mask bits in ASR.
*/
	switch(sense) {
	case 1:
	case 2:
		stkop("ANDW2",flags[ insptr->opcode & 0x0f]);
		stkop("XORW2",flags[(insptr->opcode & 0xf0) >> 4]);
		break;
	case 3:
		gen2("MOVW",&temp1,&temp2);
		stkop("ANDW2",flags[ insptr->opcode & 0x0f]);
		stkop("XORW2",flags[(insptr->opcode & 0xf0) >> 4]);
		break;
	case 4:
		stkop("ANDW2",insptr->opcode & 0x8fffffff);
		break;
	}
/*
**	How big is our address?
*/
	counter = 0;
	addrgen(insptr,addr,NOTYPE,1);
	nbits = counter;
	counter = -1;

/*
**	B{E|NE}B nojump
*/
	switch (sense) {
	case 1:
		branchgen("BNEB",(long) (6 + nbits/8));
		break;
	case 3:
		branchgen("BEB",(long) (0x1d + nbits/8));
		gen2("MOVW",&temp2,&temp1);
		stkop("ANDW2",flags[(insptr->opcode & 0x0f00) >>  8]);
		stkop("XORW2",flags[(insptr->opcode & 0xf000) >> 12]);
		/* break left out intentionally!!! */
	/* FALLTHROUGH */
	case 2:
	case 4:
		branchgen("BEB",(long) (6 + nbits/8));
		break;
	}
/*
**	Deallocate temporary, and JUMP
*/
	
	if (addr->symptr) 	/* generate a PC relative jump */
		PCREL_MODE(addr);
	decrstk(stksz);
	gen1("JMP",addr);

/*
** nojump:
**
**	Deallocate temporary.
*/
	decrstk(stksz);
	return;
}

void
fpcnvfw(insptr,addr1,addr2)
struct mnemonic *insptr;
OPERAND *addr1;
OPERAND *addr2;
{
	unsigned word;
	int op3, type3, size3, rb3;

	rdwri = MAU_RD;
	op3 = do_op(insptr,addr2,OP3,&type3,&size3,&rb3);

	if (isize != size3 && size3 != -1)
		yyerror("wrong floating point conversion");
	size3 = isize;

	if (type3 == TMEM && size3 != 0) {
		rdwri = MAU_RD;
		op3 = OP3REG(0x03,0x02);
	}

	word = mk_word(RB1,RB2,rb3,insptr->roundmode,insptr->opcode,OP1MEM(0),OP2NOP,op3);
	spopgen(0,rdwri,word,addr1,addr2);

	if (type3 == TMEM && size3 != 0) {
		if (wflag)
			werror("Implicit use of %f3 MAU register");
		word = mk_word(RB1,RB2,RB3,ASR_RC,M_MOVE,OP1REG(0x03),OP2NOP,OP3MEM(size3));
		spopgen(rsize,MAU_WR,word,(OPERAND *)NULL,addr2);
	}
}

void
fpcnvtw(insptr,addr1,addr2)
struct mnemonic *insptr;
OPERAND *addr1;
OPERAND *addr2;
{
	unsigned word;
	int op1, type1,size1,rb1;

	rdwri = MAU_WR;
	op1 = do_op(insptr,addr1,OP1,&type1,&size1,&rb1);

	if (isize != size1 && addr1->type != FREGMD)
		yyerror("wrong floating point conversion");

	if (type1 == TMEM && size1 != 0) {
		if (wflag)
			werror("Implicit use of %f3 MAU register");
		word = mk_word(RB1,RB2,RB3,ASR_RC,M_MOVE,op1,OP2NOP,OP3REG(0x03,0x02));
		spopgen(isize,MAU_RD,word,addr1,(OPERAND *)NULL);
		op1 = OP1REG(0x03);
		type1 = TREG;
		rdwri = MAU_WR;
	}

	word = mk_word(rb1,RB2,RB3,insptr->roundmode,insptr->opcode,op1,OP2NOP,OP3MEM(0));
	spopgen(0,rdwri,word,addr1,addr2);
}

void
fpcnvfu(insptr,addr1,addr2)
struct mnemonic *insptr;
OPERAND *addr1;
OPERAND *addr2;
{
	static void branchgen();
	unsigned word;
	int type3, size3 ,rb3;
	static unsigned long fpval[3] = {0,0,0};
	OPERAND caddr;
	int nbits;

	rdwri = MAU_RD;
	(void) do_op(insptr,addr2,OP3,&type3,&size3,&rb3);

	if (isize != size3 && size3 != -1)
		yyerror("wrong floating point conversion");
	size3 = isize;

	word = mk_word(RB1,RB2,RB3,insptr->roundmode,M_ITOF,OP1MEM(0),OP2NOP,OP3REG(0x03,0x02));
	spopgen(0,MAU_RD,word,addr1,(OPERAND *)NULL);

	if (wflag)
		werror("Implicit use of %f3 MAU register");
/*
**	Count the number of bits in addr2
*/
	if (type3 == TMEM) {
		counter = 0;
		addrgen(insptr,addr2,NOTYPE,1);
		nbits = counter;
		counter = -1;
	}
	else
		nbits = -8;

	branchgen("BGEB",(long) (15 + nbits/8));	/* bgeb next */

	if (isize == 0)
		fpval[0] = 0x4f800000;
	else if (isize == 1) {
		fpval[0] = 0x41f00000;
		fpval[1] = 0x0;
	}
	else {
		fpval[0] = 0x0000401f;
		fpval[1] = (unsigned long ) 0x80000000;
		fpval[2] = 0x0;
	}
	
	fimmed(fpval,&caddr,size3);

	word = mk_word(RB1,RB2,((type3 == TMEM) ? RB3: rb3),ASR_RC,
		M_ADD,OP1MEM(size3),OP2REG(0x03), 
		((type3 == TMEM) ? OP3MEM(size3) : OP3REG(addr2->reg,size3)));
	spopgen(size3,rdwri,word,&caddr,addr2);

	branchgen("BRB",(long) (8 + nbits/8));		/* brb end */
/* next: */
	word = mk_word(RB1,RB2,((type3 == TMEM) ? RB3: rb3),ASR_RC,
		M_MOVE,OP1REG(0x03),OP2NOP,
		((type3 == TMEM) ? OP3MEM(size3) : OP3REG(addr2->reg,size3)));
	spopgen(size3,((type3 == TREG) ? 0 : MAU_WR),word,(OPERAND *)NULL,addr2);
/* end: */
}

void
fpcnvtu(insptr,addr1,addr2)
struct mnemonic *insptr;
OPERAND *addr1;
OPERAND *addr2;
{
	unsigned word;
	int op1, type1,size1,rb1;
	int nbits;
	static unsigned long fpval [3] = {0x41f00000,0,0}; /* (2**32) double */
	static unsigned long fpval1[3] = {0x41e00000,0,0}; /* (2**31) double */
	OPERAND caddr;

	rdwri = 0;
	op1 = do_op(insptr,addr1,OP1,&type1,&size1,&rb1);

	if (size1 != isize && addr1->type != FREGMD)
		yyerror("wrong floating point conversion");
	
	if (wflag)
		werror("Implicit use of %f3 MAU register");

	word = mk_word(rb1,RB2,RB3,insptr->roundmode,M_RTOI,op1,OP2NOP,OP3REG(0x03,0x02));
	spopgen(isize,rdwri,word,addr1,(OPERAND *)NULL);

	counter = 0;
	addrgen(insptr,addr2,NOTYPE,1);
	nbits = counter;
	counter = -1;

	branchgen("BGB",(long) (5 + nbits/8));	/* if positive jump to next */
	gen1("CLRW",addr2);			/* 0 -> op2 */
	branchgen("BRB",(long) (0x29+nbits/8));	/* jmp to end */
/* next: */
	fimmed(fpval1, &caddr, 1);	/* compare(%f3, &(2**31)) */

	word = mk_word(RB1,RB2,RB3,ASR_RC,M_CMP,OP1REG(0x03),OP2MEM(0x01),OP3NOP);
	spopgen(1,MAU_RD,word,&caddr,(OPERAND *)NULL);

	branchgen("BLB",0x18);
	fimmed(fpval, &caddr, 1);	/* compare(%f3, &(2**32)) */

	word = mk_word(RB1,RB2,RB3,ASR_RC,M_CMP,OP1REG(0x03),OP2MEM(0x01),OP3NOP);
	spopgen(1,MAU_RD,word,&caddr,(OPERAND *)NULL);

	branchgen("BGEB",0xc);

	word = mk_word(RB1,RB2,RB3,ASR_RC,M_SUB,OP1MEM(0x01),OP2REG(0x03),OP3REG(0x03,0x02));
	spopgen(1,MAU_RD,word,&caddr,(OPERAND *)NULL);
/* ok: */
	word = mk_word(RB1,RB2,RB3,ASR_RC,M_FTOI,OP1REG(0x03),OP2NOP,OP3MEM(0x00));
	spopgen(0,MAU_WR,word,(OPERAND *)NULL,addr2);
/* end: */
}

void
fpcnvf10(insptr,addr1,addr2)
struct mnemonic *insptr;
OPERAND *addr1;
OPERAND *addr2;
{
	unsigned word;
	int op3, type3, size3, rb3;

	rdwri = MAU_RD;
	op3 = do_op(insptr,addr2,OP3,&type3,&size3,&rb3);

	if (isize != size3 && size3 != -1)
		yyerror("wrong floating point conversion");
	size3 = isize;

	if (type3 == TMEM && size3 != 2) {
		rdwri = MAU_RD;
		op3 = OP3REG(0x03,0x02);
		if (wflag)
			werror("Implicit use of %f3 MAU register");
	}

	word = mk_word(RB1,RB2,rb3,insptr->roundmode,M_DTOF,OP1MEM(0x02),OP2NOP,op3);
	spopgen(2,rdwri,word,addr1,addr2);

	if (type3 == TMEM && size3 != 2) {
		word = mk_word(RB1,RB2,rb3,ASR_RC,M_MOVE,OP1REG(0x03),OP2NOP,OP3MEM(size3));
		spopgen(size3,MAU_WR,word,(OPERAND *)NULL,addr2);
	}
}

void
fpcnvt10(insptr,addr1,addr2)
struct mnemonic *insptr;
OPERAND *addr1;
OPERAND *addr2;
{
	unsigned word;
	int op1, type1, size1,rb1;

	rdwri = MAU_WR;
	op1 = do_op(insptr,addr1,OP1,&type1,&size1,&rb1);

	if (isize != size1 && addr1->type != FREGMD)
		yyerror("wrong floating point conversion");

	if ((type1 == TMEM) && (size1 != 0x02 || overlap(addr1,12,addr2,12))) {
		if (wflag)
			werror("Implicit use of %f3 MAU register");
		word = mk_word(rb1,RB2,RB3,ASR_RC,M_MOVE,op1,OP2NOP,OP3REG(0x03,0x02));
		spopgen(isize,MAU_RD,word,addr1,(OPERAND *)NULL);
		op1 = OP1REG(0x03);
		rdwri = MAU_WR;
	}

	word = mk_word(rb1,RB2,RB3,insptr->roundmode,M_FTOD,op1,OP2NOP,OP3MEM(0x02));
	spopgen(2,rdwri,word,addr1,addr2);
}

void
fpmovto(insptr,addr)
struct mnemonic *insptr;
OPERAND *addr;
{
	unsigned word;
	int size1, type1, op1, rb1;

	op1 = do_op(insptr,addr,OP1,&type1,&size1,&rb1);

	word = mk_word(rb1,RB2,RB3,ASR_RC,insptr->opcode,op1,OP2NOP,OP3NOP);
	spopgen(isize,MAU_RD,word,addr,(OPERAND *)NULL);
}

void
fpmovfrom(insptr,addr)
struct mnemonic *insptr;
OPERAND *addr;
{
	unsigned word;
	int op3, type3, size3, rb3;

	op3 = do_op(insptr,addr,OP3,&type3,&size3,&rb3);

	word = mk_word(RB1,RB2,rb3,ASR_RC,insptr->opcode,OP1NOP,OP2NOP,op3);
	spopgen(isize,MAU_WR,word,(OPERAND *)NULL,addr);
}

void
fpmovgen(insptr,addr1,addr2)
struct mnemonic *insptr;
OPERAND *addr1;
OPERAND *addr2;
{
	unsigned word;
	int op1, type1, size1, rb1;
	int op3, type3, size3, rb3;


	rdwri = 0;
	op1 = do_op(insptr,addr1,OP1,&type1,&size1,&rb1);
	op3 = do_op(insptr,addr2,OP3,&type3,&size3,&rb3);

	if (((size1 != isize) && (addr1->type != FREGMD)) ||
	    ((size3 != rsize) && (size3 != -1)))
		yyerror("wrong floating point conversion");

	if (type1 == TMEM && type3 == TMEM && (size1 != size3)) {
		if (wflag)
			werror("Implicit use of %f3 MAU register");
		word = mk_word(rb1,RB2,RB3,ASR_RC,M_MOVE,op1,OP2NOP,OP3REG(0x03,size3));
		spopgen(isize,MAU_RD,word,addr1,(OPERAND *)NULL);
		op1 = OP1REG(0x03);
		type1 = TREG;
		rdwri = MAU_WR;
	}

	word = mk_word(rb1,RB2,rb3,insptr->roundmode,insptr->opcode,op1,OP2NOP,op3);
	spopgen(((type1 == TMEM) ? isize : rsize),rdwri,word,addr1,addr2);
}

void
fptr1gen(insptr,addr1,addr2,addr3)
struct mnemonic *insptr;
OPERAND *addr1, *addr2, *addr3;
{
	unsigned word;
	int op1, size1, type1, rb1;
	int op2, size2, type2, rb2;
	int op3, size3, type3, rb3;
	int maxsize;

	rdwri = NULL;
	op1 = do_op(insptr,addr1,OP1,&type1,&size1,&rb1);
	op2 = do_op(insptr,addr2,OP2,&type2,&size2,&rb2);
	op3 = do_op(insptr,addr3,OP3,&type3,&size3,&rb3);

	maxsize = max(size1,size2);
/*
**	If addr1 and addr2 are both in memory, move addr1 to
**	mau register %f3, since there is no double read SPOP.
*/
	if (type1 == TMEM && type2 == TMEM) {
		if (wflag)
			werror("Implicit use of %f3 MAU register");
		word = mk_word(rb1,RB2,RB3,ASR_RC,M_MOVE,op1,OP2NOP,OP3REG(3,isize));
		spopgen(isize, MAU_RD, word, addr1, (OPERAND *)NULL);
		op1 = OP1REG(3);
		type1 = TREG;
		addr1 = (OPERAND *)NULL;
	}

	word = mk_word(rb1,rb2,rb3,insptr->roundmode,insptr->opcode,op1,op2,op3);

	if ((maxsize <= size3) || size3 == -1)
		spopgen(isize, rdwri, word, ((type1 == TMEM) ? addr1:addr2), addr3);
	else {
/*
**		MIXED PRECISION OPERATIONS NOT ALLOWED BY IEEE:
**		do operation into %f3 and convert.
*/
		word = mk_word(rb1,rb2,RB3,insptr->roundmode,
				insptr->opcode,op1,op2,OP3REG(3,maxsize));
		spopgen(isize, (rdwri & MAU_RD), word,
			((type1 == TMEM) ? addr1 : addr2), (OPERAND *)NULL);
		word = mk_word(RB1,RB2,rb3,ASR_RC,M_MOVE,OP1REG(3),OP2NOP,op3);
		spopgen(isize, (rdwri & MAU_WR), word, (OPERAND *)NULL, addr3);
	}
}

/* Performs floating point remainder operation-- rounding specification is applied
** to the last move SPOP
*/
void
fptr2gen(insptr,addr1,addr2,addr3)
struct mnemonic *insptr;
OPERAND *addr1, *addr2, *addr3;
{
	extern void incrstk();
	extern void decrstk();
	unsigned word;
	int op1, type1, size1, rb1;
	int op2, type2, size2, rb2;
	int op3, type3, size3, rb3;
	long savedot;

	op1 = do_op(insptr,addr1,OP1,&type1,&size1,&rb1);
	op2 = do_op(insptr,addr2,OP2,&type2,&size2,&rb2);
	op3 = do_op(insptr,addr3,OP3,&type3,&size3,&rb3);

/* 
**	MOVE op2 to %f3
*/
	if (wflag)
		werror("Implicit use of %f3 MAU register"); 

	word = mk_word(rb2,RB2,RB3,ASR_RC,M_MOVE,op2,OP2NOP,OP3REG(3,0x02));
	spopgen(isize,((type2 == TMEM) ? MAU_RD : NULL),word,addr2,(OPERAND *)NULL);

/*
**	Create temporary for the ASR on the stack.
**	if addr1 is relative to %sp, adjust the displacement here.
**	(no need to un-adjust, as we will not reference it again).
*/
	incrstk(4);
	if (((addr1->type == DSPMD)	||
	     (addr1->type == DSPDFMD)	||
	     (addr1->type == REGDFMD))	&&
	     (addr1->reg  == SPREG))
		addr1->expval -= 4;

/*
** loop:
*/
	savedot = newdot;
/*
**	Generate op1 <rem> reg3 -> reg3
*/
	word = mk_word(rb1,RB2,RB3,ASR_RC,insptr->opcode,op1,OP2REG(0x03),OP3REG(0x03,0x02));
	spopgen(isize, ((type1 == TMEM) ? MAU_RD : NULL), word, addr1, (OPERAND *)NULL);

/*
**	Copy the ASR to our temporary.
*/
	word = mk_word(RB1,RB2,RB3,ASR_RC,RD_ASR,OP1NOP,OP2NOP,OP3MEM(0));
	spopnop = 0;
	spopgen(0, MAU_WR, word, (OPERAND *)NULL, &temp1);
	spopnop = 1;

	stkop("ANDW2",0x20);			/* Isolate Partial Remainder bit */
	branchgen("BNEB",savedot-newdot);	/* bneb	loop # if != 0 */
	decrstk(4);			/* deallocate temporary */

	word = mk_word(RB1,RB2,rb3,insptr->roundmode,M_MOVE,OP1REG(0x03),OP2NOP,op3);
	spopgen(isize, ((type3 == TMEM) ? MAU_WR : NULL), word, (OPERAND *)NULL, addr3);

	return;
}

/*
**	Calculate type (TMEM, TNOP, or TREG) of operand.
**	Calculate size (0=NOP, 1=SINGLE, 2=DOUBLE, 3=EXTENDED) of operand.
**	Return operand specifier, properly shifted.
*/

static int
do_op(insptr,addr,opnum,type,size,regbank)
struct mnemonic *insptr;
OPERAND *addr;
int opnum, *type, *size, *regbank;
{
/*
**	Calculate default size of operand according to instuction type.
**	This is used as the size if the operand is in memory or in a
**	general sized register.
*/
	extern int optype();

	isize = optype(insptr->tag,1) - 2;
	rsize = optype(insptr->tag,(unsigned char) opnum) - 2;

	if (addr)
		switch (addr->type) {
		case FREGMD:
			*size = ((opnum == OP3) ? -1 : 0);
			*type = TREG;
			break;
		case SREGMD:
			*size = 0;
			*type = TREG;
			break;
		case DREGMD:
			*size = 1;
			*type = TREG;
			break;
		case XREGMD:
			*size = 2;
			*type = TREG;
			break;
		default:
			*size = rsize;
			*type = TMEM;
			break;
		}
	else {
		*size = -1;
		*type = TNOP;
	}

	switch (opnum) {
	case OP1:
	case OP2:
		switch (*type) {
		case TMEM:
			*regbank = 0;
			rdwri |= MAU_RD;
			return (*size | 0x04);
		case TREG:
			*regbank = ((int) addr->reg)  >> 2 ;
			return ((int)addr->reg & 3);
		case TNOP:
			*regbank = 0;
			return (0x07);
		}
	/* FALLTHROUGH */
	case OP3:
		switch (*type) {
		case TMEM:
			*regbank = 0;
			rdwri |= MAU_WR;
			return (*size | 0x0C);
		case TREG:
			*regbank = ((int) addr->reg) >> 2;
			if (*size != -1)
				return (((int) addr->reg & 3) | (*size << 2));
			else
				return (((int ) addr->reg & 3) | (rsize << 2));
		case TNOP:
			*regbank = 0;
			return (0x0F);
		}
	}
/*NOTREACHED*/
}

static void
spopgen(insize,rw,word,raddr,waddr)
int insize;
int rw;
unsigned word;
OPERAND *raddr, *waddr;
{
	void er40gen();
	static struct mnemonic *spop_table[4][3];
	struct mnemonic *ins;

	Genline = line;

	if (spop_table[0][0] == NULL) {
		spop_table[NULL][0] = find_mnemonic("SPOP");
		spop_table[NULL][1] = find_mnemonic("SPOP");	/* not used */ 
		spop_table[NULL][2] = find_mnemonic("SPOP");	/* not used */ 

		spop_table[MAU_RD][0] = find_mnemonic("SPOPRS");
		spop_table[MAU_RD][1] = find_mnemonic("SPOPRD");
		spop_table[MAU_RD][2] = find_mnemonic("SPOPRT");

		spop_table[MAU_WR][0] = find_mnemonic("SPOPWS");
		spop_table[MAU_WR][1] = find_mnemonic("SPOPWD");
		spop_table[MAU_WR][2] = find_mnemonic("SPOPWT");

		spop_table[MAU_RD|MAU_WR][0] = find_mnemonic("SPOPS2");
		spop_table[MAU_RD|MAU_WR][1] = find_mnemonic("SPOPD2");
		spop_table[MAU_RD|MAU_WR][2] = find_mnemonic("SPOPT2");
	}

	ins = spop_table[rw][insize];

	generate(ins->nbits, NOACTION,   ins->opcode, NULLSYM);
	generate(	 32, NOACTION,swap_b4((long) word), NULLSYM);

	if ((rw & MAU_RD) && raddr &&
	   !((1<<raddr->type)&(FPDEST&(~MEM)))) 	/* Read mem, !FP Reg. */
		switch (raddr->type) {
		case FPIMMD:
			fimmgen(raddr,insize);
			break;
		case DECIMMD:
			fimmgen(raddr,4);
			break;
		case IMMD:
			fimmgen(raddr,3);
			break;
		default:
			addrgen(ins,raddr,NOTYPE,1);
			break;
		}

	if ((rw & MAU_WR) && waddr &&
	   !((1<<waddr->type)&(FPDEST&(~MEM)))) { /* Write mem, !FP Reg. */
		addrgen(ins,waddr,NOTYPE,2);
#if	ER40FIX
		if (spopnop)
			er40gen();
#endif
	}
}

static void
fimmed(fpval,addr,size)
unsigned long fpval[3];
OPERAND *addr;
int size;
{
	extern void ckalign();
	extern void fillbyte();
	extern int mksect();
	extern void cgsect();
	long savedot;
	int i;

	if (sectab[dot->sectnum].flags & SHF_EXECINSTR) {
		if (datsec < 0)
			datsec = mksect(lookup(_DATA,INSTALL), (SHF_ALLOC | SHF_WRITE), (long) SHT_PROGBITS);
		cgsect(datsec);
	}
	else
		yyerror("Floating point instruction not in .text section");

	fillbyte();
	ckalign(4L);
	savedot = newdot;
	for (i = 0; i <= size; i++)
		generate(32, RELDAT32, fpval[i], NULLSYM);

	cgsect(txtsec);

	addr->newtype = NOTYPE;
	addr->type = DSPMD;
	addr->reg = PCREG;
	addr->pic_rtype = PC;
	addr->exptype =  REL;
	addr->symptr = lookup(".data",INSTALL);
	addr->unevaluated = EVALUATED;
	addr->expval = savedot;
	addr->expspec = NULLSPEC;
}

static void
stkop(op,mask)
char *op;
long mask;
{
	struct mnemonic *tmpins;

	Genline = line;
	tmpins = find_mnemonic(op);
	generate(tmpins->nbits,NOACTION,tmpins->opcode,NULLSYM);

	generate(8,NOACTION,(long)CIMMD,NULLSYM);
	generate(32,NOACTION,swap_b4(mask),NULLSYM);

	generate(8,NOACTION,(long)((CBDSPMD<<4)|SPREG),NULLSYM);
	generate(8,NOACTION,-4,NULLSYM);
}

static void
branchgen(op,disp)
char *op;
long disp;
{
	struct mnemonic *tmpins;

	Genline = line;
	tmpins = find_mnemonic(op);
	generate(tmpins->nbits,NOACTION,tmpins->opcode,NULLSYM);
	generate(8,NOACTION,disp,NULLSYM);
}

static void
gen1(op,addr1)
char *op;
OPERAND *addr1;
{
	struct mnemonic *tmpins;

	Genline = line;
	tmpins = find_mnemonic(op);
	generate (tmpins->nbits, NOACTION, tmpins->opcode, NULLSYM);
	addrgen(tmpins,addr1,NOTYPE,1);
}

static void
fimmgen(addr,op_size)
OPERAND *addr;
int op_size;
{
	extern int atofx();
	extern void atodi(); 
	extern int atofs();
	extern int atofd();
	unsigned long fpval[3];

	switch(op_size) {
	case 0:		/* 32 bit (single) */
		if (atofs(addr->fasciip,fpval))
			yyerror("fp conversion error");
		break;
	case 1:		/* 64 bit (double) */
		if (atofd(addr->fasciip,fpval))
	 		yyerror("fp conversion error");
		break;
	case 2:		/* 96 bit (double extended) */
		if (atofx(addr->fasciip,fpval))
			yyerror("fp conversion error");
		break;
	case 3:		/* word immediate */
		fpval[0] = addr->expval;
		op_size = 0;
		break;
	case 4:		/* decimal immediate */
		atodi(addr->fasciip,fpval);
		op_size = 2;
		break;
	}
	fimmed(fpval, addr, op_size);
	generate(8,NOACTION,(long)(CABSMD),NULLSYM);
	exp_generate(32,RELDIR32,*addr);
	return;
}

int
overlap(addr1,size1,addr2,size2)
OPERAND *addr1;
int size1;
OPERAND *addr2;
int size2;
{
	if ((addr1->type == IMMD) || (addr2->type == IMMD))
		return(NO);

	switch(addr2->type) {
	case REGMD:
		if (addr1->type == REGMD)
			if (addr1->reg == addr2->reg)
				return(YES);
		if (((addr1->type == REGDFMD) ||
		     (addr1->type == DSPMD) ||
		     (addr1->type == DSPDFMD)) &&
		      addr1->reg == addr2->reg)
			return(YES);
		return(NO);
	case REGDFMD:
		if (addr1->type == DSPMD) {
			if (addr1->reg == addr2->reg)
				if ((size2 < addr1->expval) ||
				    (addr1->expval + size1 < 0))
					return(NO);
			return(YES);
		}
		return(YES);
	case DSPMD:
		if (addr1->type == REGDFMD) {
			if (addr1->reg == addr2->reg)
				if ((size1 < addr2->expval) ||
				    (addr2->expval + size1 < 0))
					return(NO);
			return(YES);
		}
		else if (addr1->type == DSPMD) {
			if (addr1->reg == addr2->reg)
				if ((addr2->expval + size2 < addr1->expval) ||
				    (addr1->expval + size1 < addr2->expval))
					return(NO);
		}
		return(YES);
	case ABSMD:
		if (addr1->type == ABSMD) {
			if ((addr1->expval + size1 < addr2->expval) ||
			    (addr2->expval + size2 < addr1->expval))
				return(NO);
		}
		return(YES);
	}
	return(YES);
}
