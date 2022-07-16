/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/addr2.c	1.16"
#include <stdio.h>
#include <libelf.h>
#include <elf.h>
#include <sys/elf_M32.h>
#include "gendefs.h"
#include "symbols.h"
#include "program.h"
#include "section.h"
#include "instab.h"


extern	unsigned int relent;
extern long newdot;
extern FILE *fdrel;
extern symbol *lookup();
extern void	getcode();
extern void 	codgen();
extern symbol *dot;

static long
swapb2(val)
	register long val;
{
	register long b1,b2;

	b1 = (val >> 8) & 0x00FFL;	/* 2nd low goes to low */
	b2 = (val << 8) & 0xFF00L;	/* low goes to high */
	return (b2|b1);
}

static long
swapb4(val)
	register long val;
{
	long b1,b2,b3,b4;

	b1 = (val >> 24) & 0x000000FFL;	/* high goes to low */
	b2 = (val >>  8) & 0x0000FF00L;	/* 2nd high goes to 2nd low */
	b3 = (val <<  8) & 0x00FF0000L;	/* 2nd low goes to 2nd high */
	b4 = (val << 24) & 0xFF000000L;	/* low goes to high */
	return (b1|b2|b3|b4);
}

/*ARGSUSED*/
static void
Swap_b2(code)
	register codebuf *code;
{
	code->cvalue = swapb2(code->cvalue);
}

/*ARGSUSED*/
static void
Swap_b4(code)
	register codebuf *code;
{
	code->cvalue = swapb4(code->cvalue);
}

/* Perform relocation.  The final value at the reference point
 * will depend on the type of relocation and the binding of the target
 * symbol.
 */

static void
relocat(sym,code,rtype)
register symbol *sym;
codebuf *code;
register BYTE rtype;
{
	char *rsym;
	prelent trelent;


	if (sym != NULLSYM) {
		if (LOCAL(sym) && UNDEF_SYM(sym)) {
			aerror("undefined local symbol");
			return;
		}
		switch(rtype) {
			case R_M32_32:
			case R_M32_32_S:
				if (ABS_SYM(sym)) {
					code->cvalue += sym->value;
					return;
				}
				if (TEMP_LABEL(sym)) {
					rsym = sectab[sym->sectnum].name;
					code->cvalue += sym->value;
				} else
					rsym = sym->name;
				break;
			case R_M32_PC32_S:
				if (ABS_SYM(sym)) {
					code->cvalue += sym->value +
						(newdot - dot->value);
					rsym = STN_UNDEF;
				} else if (sym->sectnum == dot->sectnum) {
					code->cvalue += sym->value - dot->value;
					return;
				} else if (TEMP_LABEL(sym)) {
					code->cvalue += sym->value + 
						(newdot - dot->value);
					rsym = sectab[sym->sectnum].name;
				} else {
					code->cvalue += newdot - dot->value;
					rsym = sym->name;
				}
				break;
			case R_M32_GOT32_S: 
			case R_M32_PLT32_S:
				if (ABS_SYM(sym))
					aerror("Invalid relocation");
				else {
					rsym = sym->name;
					code->cvalue = newdot - dot->value;
				}
				break;
			default:
				aerror("Invalid relocation type");
		}
	} else
		return;


	trelent.relval = newdot;
	trelent.relname = rsym;
	trelent.reltype = rtype;
	trelent.lineno = code->errline;
	(void) fwrite(&trelent,sizeof(prelent),1,fdrel);
	++relent;
}

static void
reldir32(code)
	register codebuf *code;
{
	relocat(code->csym,code,R_M32_32_S);
	code->cvalue = swapb4(code->cvalue);
}

static void
reldat32(code)
	register codebuf *code;
{
	relocat(code->csym,code,R_M32_32);
}

static void
relpc32(code)
	register codebuf *code;
{
	relocat(code->csym,code,R_M32_PC32_S);
	code->cvalue = swapb4(code->cvalue);
}

static void
relplt32(code)
	register codebuf *code;
{
	relocat(code->csym,code,R_M32_PLT32_S);
	code->cvalue = swapb4(code->cvalue);
}

static void
relgot32(code)
	register codebuf *code;
{
	relocat(code->csym,code,R_M32_GOT32_S);
	code->cvalue = swapb4(code->cvalue);
}

static void
savtrans(code)
register codebuf *code;
{
	static void resabs();

	if (code->csym != NULLSYM)
		resabs(code);
	if (code->cvalue < 0 || code->cvalue > 6)
		werror("Invalid number of registers in `sav/ret'");
	code->cvalue = (CREGMD << 4) + (FPREG - (int)(code->cvalue));
}

static void
resabs(code)
register codebuf *code;
{
	register symbol *sym;

	if ((sym = code->csym) != 0)
		switch (sym->sectnum) {
		case SHN_ABS:
			code->cvalue += sym->value; /* sym must be non-null */
			return;
		case SHN_UNDEF:
			yyerror("Undefined symbol in absolute expression");
			return;
		default:
			yyerror("Relocatable symbol in absolute expression");
			return;
		}
}


static void
relpc8(code)
	register codebuf *code;
{
	register symbol *sym;
	register long val;

	val = code->cvalue;
	if ((sym = code->csym) != NULLSYM)
		if (sym->sectnum != dot->sectnum && !(ABS_SYM(sym)))
			aerror("relpc8: reference to symbol in another section");
		else
			val += sym->value;
	if ((val -= dot->value) >= (1L << 7) || val < -(1L << 7))
		aerror("relpc8: offset out of range");
	code->cvalue = val;
}

static void
relpc16(code)
	register codebuf *code;
{
	register symbol *sym;
	register long val;

	val = code->cvalue;
	if ((sym = code->csym) != NULLSYM)
		if (sym->sectnum != dot->sectnum && !(ABS_SYM(sym)))
			aerror("relpc16: reference to symbol in another section");
		else
			val += sym->value;
	if ((val -= dot->value) >= (1L << 15) || val < -(1L << 15))
		aerror("relpc16: offset out of range");
	code->cvalue = swapb2(val);
}

static void
cbrnopt(code)
	register codebuf *code;
{
	register symbol *sym;
	long	opcd;
	register SDI	*sdp;

	sdp = Sdi;
	for (;;)
	{
		if (++sdp < Sdibox->b_end)
			if (sdp->sd_itype != IT_LABEL)
				break;
			else
				continue;
		if ((Sdibox = Sdibox->b_link) == 0)
			aerror("no cbr boxes");
		sdp = Sdibox->b_buf - 1;
	}
	Sdi = sdp;
	sym = code->csym;	/* all sdi's have symbols */

	opcd = code->cvalue;
	if (sdp->sd_flags & SDF_BYTE)
	{
		codgen(8, opcd);
		code->cnbits = 8;
		code->cvalue = sym->value + sdp->sd_off - dot->value;
	}
	else if (sdp->sd_flags & SDF_HALF)
	{
		codgen(8, opcd - 1);
		code->cnbits = 16;
		code->cvalue = swapb2(sym->value + sdp->sd_off - dot->value);
	}
	else
	{
		codgen(8, (long) MN_ALT(opcd));
		codgen(8, 8L);		/* length of branch */
		dot->value = newdot;	/* resynchronize */
		codgen(8, JMPOPCD);
#if 0
		codgen(8, (long)CABSMD); /* descriptor for operand */
#endif
		codgen(8, (long)(CDSPMD << 4 | PCREG)); /* descriptor for operand */
		code->cnbits = 32;
		code->cvalue = sdp->sd_off;
#if 0
		reldir32(code);
#endif
		relpc32(code);
	}
}

static void
ubrnopt(code)
	register codebuf	*code;
{
	register symbol	*sym;
	register SDI	*sdp;

	sdp = Sdi;
	for (;;)
	{
		if (++sdp < Sdibox->b_end)
			if (sdp->sd_itype != IT_LABEL)
				break;
			else
				continue;
		if ((Sdibox = Sdibox->b_link) == 0)
			aerror("no ubr boxes");
		sdp = Sdibox->b_buf - 1;
	}
	Sdi = sdp;
	sym = code->csym;	/* all sdi's have symbols */

	if (sdp->sd_flags & SDF_BYTE)
	{
		codgen(8, BRBOPCD);
		code->cnbits = 8;
		code->cvalue = sym->value + sdp->sd_off - dot->value;
	}
	else if (sdp->sd_flags & SDF_HALF)
	{
		codgen(8, BRHOPCD);
		code->cnbits = 16;
		code->cvalue = swapb2(sym->value + sdp->sd_off - dot->value);
	}
	else
	{
		codgen(8, JMPOPCD);
#if 0
		codgen(8, (long)CABSMD);
#endif
		codgen(8, (long)(CDSPMD << 4 | PCREG)); /* descriptor for operand */
		code->cnbits = 32;
		code->cvalue = sdp->sd_off;
		relpc32(code);
	}
}


static void
bsbnopt(code)
	register codebuf *code;
{
	register symbol *sym;
	register SDI	*sdp;

	sdp = Sdi;
	for (;;)
	{
		if (++sdp < Sdibox->b_end)
			if (sdp->sd_itype != IT_LABEL)
				break;
			else
				continue;
		if ((Sdibox = Sdibox->b_link) == 0)
			aerror("no bsb boxes");
		sdp = Sdibox->b_buf - 1;
	}
	Sdi = sdp;
	sym = code->csym;	/* all sdi's have symbols */

	if (sdp->sd_flags & SDF_BYTE)
	{
		codgen(8, BSBBOPCD);
		code->cnbits = 8;
		code->cvalue = sym->value + sdp->sd_off - dot->value;
	}
	else if (sdp->sd_flags & SDF_HALF)
	{
		codgen(8, BSBHOPCD);
		code->cnbits = 16;
		code->cvalue = swapb2(sym->value + sdp->sd_off - dot->value);
	}
	else
	{
		codgen(8, JSBOPCD);		/* generate long form */
		codgen(8, (long)CABSMD);	/* operand descriptor */
		code->cnbits = 32;
		code->cvalue = sdp->sd_off;
		reldir32(code);
	}
}

#ifdef	CALLPCREL
static void
callnopt(code)
register codebuf *code;
{
	register symbol *sym;
	register SDI	*sdp;

	sdp = Sdi;
	for (;;)
	{
		if (++sdp < Sdibox->b_end)
			if (sdp->sd_itype != IT_LABEL)
				break;
			else
				continue;
		if ((Sdibox = Sdibox->b_link) == 0)
			aerror("no call boxes");
		sdp = Sdibox->b_buf - 1;
	}
	Sdi = sdp;
	sym = code->csym;	/* all sdi's have symbols */

	if (sdp->sd_flags & SDF_BYTE)
	{
		codgen(8, (long)CBPCREL);
		code->cnbits = 8;
		code->cvalue = sym->value + sdp->sd_off - dot->value;
	}
	else if (sdp->sd_flags & SDF_HALF)
	{
		codgen(8, (long)CHPCREL);
		code->cnbits = 16;
		code->cvalue = swapb2(sym->value + sdp->sd_off - dot->value);
	}
	else
	{
		codgen(8, (long)CABSMD);	/* operand descriptor */
		code->cnbits = 32;
		code->cvalue = sdp->sd_off;
		reldir32(code);
	}
}
#endif

static void
fndbrlen(code)
register codebuf *code;
{
	static int oplen();
	long opcd;

	getcode(code);
	opcd = code->cvalue;
	codgen(8,(long)(3 + oplen(code)));
	dot->value = newdot; /* resynchronize */
	codgen(8,opcd);
	/* now return with descriptor of operand in "code" */
}

static 
oplen(code)
register codebuf *code;
{
	static int opndlen[2][16] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 3, 3, 2, 2, 0, 1}, /*  < 15 */
		{1, 1, 1, 1, 5, 3, 2, 5, 5, 5, 3, 3, 2, 2, 5, 1}  /* == 15 */
	};
	register int desc;

	getcode(code);
	desc = (int)(code->cvalue);
	return(opndlen[(desc & 0xF) == 0xF][desc >> 4]);
}

static void
shiftval(code)
register codebuf *code;
{
	resabs(code); /* symbol must be absolute */
	if (code->cvalue < 1L || code->cvalue > 31L)
		yyerror("Bit position out of range");
	code->cvalue = swapb4(1L << code->cvalue); /* shift and swap */
}

extern void
	define(),
	setval(),
	setscl(),
	endef(),
	dotzero();


void (*(modes[CB_AMASK+1]))() = {
/*0*/	0,		/* NOACTION */
	define,		/* DEFINE */
	setval,		/* SETVAL */
	setscl,		/* SETSCL */
	relpc32,	/* RELPC32 */
/*5*/	relgot32,	/* RELGOT32 */
	relplt32,	/* RELPLT32 */
	endef,		/* ENDEF */
	0,		/* not used */
	reldir32,	/* RELDIR32 */
/*10*/	resabs,		/* RESABS */
	relpc8,		/* RELPC8 */
	relpc16,	/* RELPC16 */
	cbrnopt,	/* CBRNOPT */
	bsbnopt,	/* BSBNOPT */
/*15*/	fndbrlen,	/* FNDBRLEN */
	shiftval,	/* SHIFTVAL */
	reldat32,	/* RELDAT32 */
	savtrans,	/* SAVTRANS */
	dotzero,	/* DOTZERO */
/*20*/	Swap_b2,	/* SWAP_B2 */
	Swap_b4,	/* SWAP_B4 */
	ubrnopt,	/* UBRNOPT */
#ifdef	CALLPCREL
	callnopt,	/* CALLNOPT */
#endif
	0,		/* NACTION */
};
