/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/RegId.c	1.3"

/************************************************************************/
/*				RegId.c					*/
/*									*/
/*		This file contains three functions: the first converts	*/
/*	an external register identifier (a pointer to a character	*/
/*	string) to an internal register identifier (a member of an	*/
/*	enumeration RegId).						*/
/*	The second converts an internal register identifier to an	*/
/*	external register identifier.					*/
/*	The third returns the register number for an internal		*/
/*	register identifier.						*/
/*									*/
/************************************************************************/

#include	<string.h>
#include	"defs.h"
#include	"Target.h"
#include	"OperndType.h"
#include	"RegId.h"

static char *ExtNames[] = {"%r0","%r1","%r2","%r3","%r4","%r5","%r6","%r7",
		"%r8","%fp","%ap","%psw","%sp","%pcbp","%isp","%pc",
		"%r9","%r10","%r11","%r12","%r13","%r14","%r15",
		"%r16","%r17","%r18","%r19","%r20","%r21","%r22","%r23",
		"%r24","%r25","%r26","%r27","%r28","%r29","%r30","%r31",
		":A:",":C:",":N:",":V:",":X:",":Z:",
		"%f0","%f1","%f2","%f3","%f4","%f5","%f6","%f7",
		"%s0","%s1","%s2","%s3","%s4","%s5","%s6","%s7",
		"%d0","%d1","%d2","%d3","%d4","%d5","%d6","%d7",
		"%x0","%x1","%x2","%x3","%x4","%x5","%x6","%x7",
		"mfpr","masr",
		""};
static unsigned short int Numbers[] = {0,1,2,3,4,5,6,7,
		8,9,10,11,12,13,14,15,
		9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,
		24,25,26,27,28,29,30,31,
		0,0,0,0,0,0,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,1,2,3,4,5,6,7,
		0,0,
		0};
static RegId IntIds[] = {CREG0,CREG1,CREG2,CREG3,CREG4,CREG5,CREG6,CREG7,
		CREG8,CFP,CAP,CPSW,CSP,CPCBP,CISP,CPC,
		CREG9,CREG10,CREG11,CREG12,CREG13,CREG14,CREG15,
		CREG16,CREG17,CREG18,CREG19,CREG20,CREG21,CREG22,CREG23,
		CREG24,CREG25,CREG26,CREG27,CREG28,CREG29,CREG30,CREG31,
		CCODE_A,CCODE_C,CCODE_N,CCODE_V,CCODE_X,CCODE_Z,
		MREG0,MREG1,MREG2,MREG3,MREG4,MREG5,MREG6,MREG7,
		MREG0,MREG1,MREG2,MREG3,MREG4,MREG5,MREG6,MREG7,
		MREG0,MREG1,MREG2,MREG3,MREG4,MREG5,MREG6,MREG7,
		MREG0,MREG1,MREG2,MREG3,MREG4,MREG5,MREG6,MREG7,
		MFPR,MASR,
		REG_NONE};
static RegId RegIds[] = {CREG0,CREG1,CREG2,CREG3,CREG4,CREG5,CREG6,CREG7,
			 CREG8,CREG9,CREG10,CREG11,CREG12,CREG13,CREG14,CREG15,
			CREG16,CREG17,CREG18,CREG19,CREG20,CREG21,CREG22,CREG23,
			 CREG24,CREG25,CREG26,CREG27,CREG28,CREG29,CREG30,CREG31
			};
	RegId
GetIntRegId(external)		/* Get internal form of register id from */
				/* the external form. */
char *external;			/* Pointer to the external form; */
				/* need not be NULL-terminated. */

{extern char *ExtNames[];	/* Array of external names for CPU registers.*/
 extern RegId IntIds[];		/* The internal identifiers. */
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 register unsigned int regno;	/* We sequentially search using this index. */
 /*extern int strlen();		** String length; in C(3) library. */
 /*extern int strncmp();	** Counted string compare; in C(3) library. */

 for(regno = 0; *ExtNames[regno] != EOS; regno++)	/* Sequential search. */
	{if(!strncmp(external,ExtNames[regno],strlen(ExtNames[regno])))
		return(IntIds[regno]);		/*Matched: return internal id.*/
	}
 fatal("GetIntRegId: unknown register id: '%s`.\n",external);
 /*NOTREACHED*/
}


	void
GetExtRegId(internal,type,external,extsize)	/* Get external form of  */
					/* register id from the internal form.*/
RegId internal;			/* The internal form of CPU register id. */
OperandType type;		/* Data type of register. */
char *external;			/* Where to put the external form. */
unsigned int extsize;		/* Maximum allowed size for external form. */

{extern char *ExtNames[];	/* Pointers to the external names. */
 extern RegId IntIds[];		/* The internal identifiers. */
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 unsigned int offset;		/* Correction to get floating registers. */
 register unsigned int regno;	/* Sequential search index. */
 /*extern char *strncpy();	** Counted string copy; in C(3) library. */

 switch(type)					/* Get correct alias.	*/
	{case Tnone:
		offset = 0;
		endcase;
	 case Timm8:
	 case Tsbyte:
	 case Tbyte:
	 case Tubyte:
		offset = 0;
		endcase;
	 case Timm16:
	 case Tshalf:
	 case Thalf:
	 case Tuhalf:
		offset = 0;
		endcase;
	 case Timm32:
	 case Tsword:
	 case Tword:
	 case Tuword:
	 case T1word:
		offset = 0;
		endcase;
	 case Tsingle:
		offset = 8;
		endcase;
	 case T2word:
		offset = 0;
		endcase;
	 case Tdouble:
		offset = 16;
		endcase;
	 case T3word:
		offset = 0;
		endcase;
	 case Tdblext:
		offset = 24;
		endcase;
	 case Tdecint:
	 case Taddress:
	 case Tany:
		offset = 0;
		endcase;
	 default:
		fatal("GetExtRegId: invalid operand type (%u).\n",type);
		endcase;
	}
 for(regno = 0; IntIds[regno] != REG_NONE; regno++)	/* Search for it. */
	{if(internal == IntIds[regno])	/* Find a match? */
		{(void) strncpy(external,ExtNames[regno+offset],(int) extsize);
		 return;			/* Yes: copy it back. */
		}
	}
 fatal("GetExtRegId: invalid register id (%d).\n",internal);
 /*NOTREACHED*/
}
	unsigned int
GetRegNo(internal)		/* Get register number */
				/* from the internal form.*/
RegId internal;			/* The internal form of CPU register id. */

{extern unsigned short int Numbers[];	/* Pointers to the external names. */
 extern RegId IntIds[];		/* The internal identifiers. */
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 register unsigned int regno;	/* Sequential search index. */

 switch(internal)		/* Is it a status or control bit? */
	{case CCODE_A:			
	 case CCODE_C:
	 case CCODE_N:
	 case CCODE_V:
	 case CCODE_X:
	 case CCODE_Z:
	 case MFPR:				/* Rounding mode for MAU. */
		fatal("GetRegNo: Status or control reg id (%d).\n",internal);
		endcase;
	}
 for(regno = 0; IntIds[regno] != REG_NONE; regno++)	/* Search for it. */
	{if(internal == IntIds[regno])			/* Find a match? */
		return((int) Numbers[regno]);		/* Yes: return it. */
	}
 fatal("GetRegNo: invalid register id (%u).\n",internal);
 /*NOTREACHED*/
}


	RegId 
GetNextRegId(regid)		/* Get next register id */
RegId regid;			/* The internal form of CPU register id. */

{extern m32_target_cpu cpu_chip;	/* Identity of target cpu chip.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 extern m32_target_math math_chip;	/* Identity of target math chip. */
 RegId ri;

 switch(regid)
	{case CREG0:	ri = CREG1;	endcase;
	 case CREG1:	ri = CREG2;	endcase;
	 case CREG2:	ri = CREG3;	endcase;
	 case CREG3:	ri = CREG4;	endcase;
	 case CREG4:	ri = CREG5;	endcase;
	 case CREG5:	ri = CREG6;	endcase;
	 case CREG6:	ri = CREG7;	endcase;
	 case CREG7:	ri = CREG8;	endcase;
	 case CREG8:	ri = CREG9;	endcase;
	 case CREG9:	ri = CREG10;	endcase;
	 case CREG10:	ri = CREG11;	endcase;
	 case CREG11:	ri = CREG12;	endcase;
	 case CREG12:	ri = CREG13;	endcase;
	 case CREG13:	ri = CREG14;	endcase;
	 case CREG14:	ri = CREG15;	endcase;
	 case CREG15:
		ri = (cpu_chip == we32200) ? CREG16 : REG_NONE;
		endcase;
	 case CREG16:
		ri = (cpu_chip == we32200) ? CREG17 : REG_NONE;
		endcase;
	 case CREG17:
		ri = (cpu_chip == we32200) ? CREG18 : REG_NONE;
		endcase;
	 case CREG18:
		ri = (cpu_chip == we32200) ? CREG19 : REG_NONE;
		endcase;
	 case CREG19:
		ri = (cpu_chip == we32200) ? CREG20 : REG_NONE;
		endcase;
	 case CREG20:
		ri = (cpu_chip == we32200) ? CREG21 : REG_NONE;
		endcase;
	 case CREG21:
		ri = (cpu_chip == we32200) ? CREG22 : REG_NONE;
		endcase;
	 case CREG22:
		ri = (cpu_chip == we32200) ? CREG23 : REG_NONE;
		endcase;
	 case CREG23:
		ri = (cpu_chip == we32200) ? CREG24 : REG_NONE;
		endcase;
	 case CREG24:
		ri = (cpu_chip == we32200) ? CREG25 : REG_NONE;
		endcase;
	 case CREG25:
		ri = (cpu_chip == we32200) ? CREG26 : REG_NONE;
		endcase;
	 case CREG26:
		ri = (cpu_chip == we32200) ? CREG27 : REG_NONE;
		endcase;
	 case CREG27:
		ri = (cpu_chip == we32200) ? CREG28 : REG_NONE;
		endcase;
	 case CREG28:
		ri = (cpu_chip == we32200) ? CREG29 : REG_NONE;
		endcase;
	 case CREG29:
		ri = (cpu_chip == we32200) ? CREG30 : REG_NONE;
		endcase;
	 case CREG30:
		ri = (cpu_chip == we32200) ? CREG31 : REG_NONE;
		endcase;
	 case CREG31:	ri = REG_NONE;	endcase;
	 case CCODE_A:	ri = REG_NONE;	endcase;
	 case CCODE_C:	ri = REG_NONE;	endcase;
	 case CCODE_N:	ri = REG_NONE;	endcase;
	 case CCODE_V:	ri = REG_NONE;	endcase;
	 case CCODE_X:	ri = REG_NONE;	endcase;
	 case CCODE_Z:	ri = REG_NONE;	endcase;
	 case MREG0:	ri = MREG1;	endcase;
	 case MREG1:	ri = MREG2;	endcase;
	 case MREG2:	ri = MREG3;	endcase;
	 case MREG3:
		ri = (math_chip == we32206) ? MREG4 : REG_NONE;
		endcase;
	 case MREG4:
		ri = (math_chip == we32206) ? MREG5 : REG_NONE;
		endcase;
	 case MREG5:
		ri = (math_chip == we32206) ? MREG6 : REG_NONE;
		endcase;
	 case MREG6:
		ri = (math_chip == we32206) ? MREG7 : REG_NONE;
		endcase;
	 case MREG7:	ri = REG_NONE;	endcase;
	 default: 
		fatal("GetNextRegId: invalid register id (%u).\n", regid );
		/*NOTREACHED*/
	}
 return( ri );
}
	RegId 
GetPrevRegId(regid)		/* Get previous register id */
RegId regid;			/* The internal form of CPU register id. */

{extern m32_target_cpu cpu_chip;	/* Identity of target cpu chip.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 extern m32_target_math math_chip;	/* Identity of target math chip. */
 RegId ri;

 switch(regid)
	{case CREG0:	ri = REG_NONE;	endcase;
	 case CREG1:	ri = CREG0;	endcase;
	 case CREG2:	ri = CREG1;	endcase;
	 case CREG3:	ri = CREG2;	endcase;
	 case CREG4:	ri = CREG3;	endcase;
	 case CREG5:	ri = CREG4;	endcase;
	 case CREG6:	ri = CREG5;	endcase;
	 case CREG7:	ri = CREG6;	endcase;
	 case CREG8:	ri = CREG7;	endcase;
	 case CREG9:	ri = CREG8;	endcase;
	 case CREG10:	ri = CREG9;	endcase;
	 case CREG11:	ri = CREG10;	endcase;
	 case CREG12:	ri = CREG11;	endcase;
	 case CREG13:	ri = CREG12;	endcase;
	 case CREG14:	ri = CREG13;	endcase;
	 case CREG15:	ri = CREG14;	endcase;
	 case CREG16:
		ri = (cpu_chip == we32200) ? CREG15 : REG_NONE;
		endcase;
	 case CREG17:
		ri = (cpu_chip == we32200) ? CREG16 : REG_NONE;
		endcase;
	 case CREG18:
		ri = (cpu_chip == we32200) ? CREG17 : REG_NONE;
		endcase;
	 case CREG19:
		ri = (cpu_chip == we32200) ? CREG18 : REG_NONE;
		endcase;
	 case CREG20:
		ri = (cpu_chip == we32200) ? CREG19 : REG_NONE;
		endcase;
	 case CREG21:
		ri = (cpu_chip == we32200) ? CREG20 : REG_NONE;
		endcase;
	 case CREG22:
		ri = (cpu_chip == we32200) ? CREG21 : REG_NONE;
		endcase;
	 case CREG23:
		ri = (cpu_chip == we32200) ? CREG22 : REG_NONE;
		endcase;
	 case CREG24:
		ri = (cpu_chip == we32200) ? CREG23 : REG_NONE;
		endcase;
	 case CREG25:
		ri = (cpu_chip == we32200) ? CREG24 : REG_NONE;
		endcase;
	 case CREG26:
		ri = (cpu_chip == we32200) ? CREG25 : REG_NONE;
		endcase;
	 case CREG27:
		ri = (cpu_chip == we32200) ? CREG26 : REG_NONE;
		endcase;
	 case CREG28:
		ri = (cpu_chip == we32200) ? CREG27 : REG_NONE;
		endcase;
	 case CREG29:
		ri = (cpu_chip == we32200) ? CREG28 : REG_NONE;
		endcase;
	 case CREG30:
		ri = (cpu_chip == we32200) ? CREG29 : REG_NONE;
		endcase;
	 case CREG31:
		ri = (cpu_chip == we32200) ? CREG30 : REG_NONE;
		endcase;
	 case CCODE_A:	ri = REG_NONE;	endcase;
	 case CCODE_C:	ri = REG_NONE;	endcase;
	 case CCODE_N:	ri = REG_NONE;	endcase;
	 case CCODE_V:	ri = REG_NONE;	endcase;
	 case CCODE_X:	ri = REG_NONE;	endcase;
	 case CCODE_Z:	ri = REG_NONE;	endcase;
	 case MREG0:	ri = REG_NONE;	endcase;
	 case MREG1:	ri = MREG0;	endcase;
	 case MREG2:	ri = MREG1;	endcase;
	 case MREG3:	ri = MREG2;	endcase;
	 case MREG4:
		ri = (math_chip == we32206) ? MREG3 : REG_NONE;
		endcase;
	 case MREG5:
		ri = (math_chip == we32206) ? MREG4 : REG_NONE;
		endcase;
	 case MREG6:
		ri = (math_chip == we32206) ? MREG5 : REG_NONE;
		endcase;
	 case MREG7:
		ri = (math_chip == we32206) ? MREG6 : REG_NONE;
		endcase;
	 default: 
		fatal("GetPrevRegId: invalid register id (%u).\n", regid );
		/*NOTREACHED*/
	}
 return( ri );
}
	RegId
GetRegId(number)		/* Get RegId of cpu register from number. */
int number;			/* Register number.	*/

{extern RegId RegIds[];		/* Conversion table.	*/
 extern void fatal();		/* Handles fatal errors.	*/

 if((number < 0) || (number > 31))		/* Validate argument.	*/
	fatal("GetRegId: invalid argument (%d).\n",number);

 return(RegIds[number]);
}
