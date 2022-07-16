/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/OperndType.c	1.3"

#include	<string.h>
#include	"defs.h"
#include	"OperndType.h"
#include	"ANodeTypes.h"

				/* The following must be in the same order */
				/* as the enumeration OperandTypes_E in */
				/* header file OperndType.h. */
static struct opty {
	char *name;
	char *sname;
	char size;
	OperandType type;
	} optytab[] = {

	{"none",	"n",	0,	Tnone},
	{"imm8",	"i8",	1,	Timm8},
	{"sbyte",	"sb",	1,	Tsbyte},
	{"byte",	"b",	1,	Tbyte},
	{"ubyte",	"ub",	1,	Tubyte},
	{"imm16",	"i16",	2,	Timm16},
	{"shalf",	"sh",	2,	Tshalf},
	{"half",	"h",	2,	Thalf},
	{"uhalf",	"uh",	2,	Tuhalf},
	{"imm32",	"i32",	4,	Timm32},
	{"sword",	"sw",	4,	Tsword},
	{"word",	"w",	4,	Tword},
	{"uword",	"uw",	4,	Tuword},
	{"1word",	"1w",	4,	T1word},
	{"single",	"s",	4,	Tsingle},
	{"2word",	"2w",	8,	T2word},
	{"double",	"d",	8,	Tdouble},
	{"3word",	"3w",	12,	T3word},
	{"dblext",	"x",	12,	Tdblext},
	{"decint",	"10",	18,	Tdecint},
	{"address",	"ad",	4,	Taddress},
	{"any",	"a",	0,	Tany},
	{"",	"",	0,	Tspec},
	{"",	"",	0,	Tbin},
	{"",	"",	0,	Tfp},
	};

	int
LSBOffset(mode,type)		/* Gives address offset for an address mode */
				/* and its type.	*/
AN_Mode mode;			/* Mode of address.	*/
OperandType type;		/* Type of address.	*/

{extern int TySize();		/* Size of a type.	*/
 extern void fatal();		/* Handles fatal errors.	*/

 switch(mode)
	{case Immediate:
	 case CPUReg:
	 case MAUReg:
	 case StatCont:
	 case Raw:
		endcase;
	 case AbsDef:
	 case DispDef:
		return(TySize(Taddress) - 1);
		/*NOTREACHED*/
		endcase;
	 case Absolute:
	 case Disp:
	 case PostDecr:
	 case PostIncr:
	 case PreDecr:
	 case PreIncr:
	 case IndexRegDisp:
	 case IndexRegScaling:
		return(TySize(type) - 1);
		/*NOTREACHED*/
		endcase;
	 default:
		fatal("LSBOffset: unknown mode (0x%8.8x).\n",mode);
		endcase;
	} /* END OF switch(mode)	*/
 return(0);
}


	int
TySize(type)			/* Return bytes required for item of type. */
OperandType type;		/* Type of item. */

{extern struct opty optytab[];	/* Decoding table. */

 return((int) optytab[(int) type].size);	/* Look it up in the table. */
}


	char *
GetSExtTypeId(type)		/* Return pointer to short type name string. */
OperandType type;		/* Type of operand. */

{extern struct opty optytab[];	/* Pointers to short type names. */

 if((int) type > (int) Tfp)			/* If illegal,	*/
	return("?");				/* give hint.	*/
 return(optytab[(int) type].sname);			/* Get it from table. */
}


	char *
GetExtTypeId(type)		/* Return pointer to type name string. */
OperandType type;		/* Type of operand. */

{extern struct opty optytab[];	/* Pointers to full type names. */

 if((int) type > (int) Tfp)			/* If illegal,	*/
	return("???");				/* give hint.	*/
 return(optytab[(int) type].name);			/* Get it from table. */
}


	OperandType
GetIntTypeId(type)		/* Return internal Operand Type. */
char *type;			/* Pointer to external form of operand type; */
				/* need not be NULL-terminated. */

{extern struct opty optytab[];	/* Names of types in internal form. */
 extern void fatal();		/* Handles fatal errors; in common. */
 register struct opty *p;	/* Lookup table index. */
 /*extern int strlen();		** String length; in C(3) library.	*/
 /*extern int strncmp();	** Counted string compare; in C(3) library.	*/

 for(p = &optytab[0]; p->name[0] != EOS; p++)	/* Look for it. */
	{if(strncmp(type,p->name,strlen(p->name)) == 0)
		return(p->type);
	}
 fatal("GetIntTypeId: unknown type:%s.\n",type);
 /*NOTREACHED*/
}
