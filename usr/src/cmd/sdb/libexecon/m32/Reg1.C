//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libexecon/m32/Reg1.C	1.6"

// Reg1.C -- register names and attributes, machine specific data (3B2)

#include "Reg.h"
#include "Itype.h"

RegAttrs regs[] = {

//	ref		name		size	flags	stype		offset
	
	REG_R0,		"%r0",		4,	0,	Suint4,		0,
	REG_R1,		"%r1",		4,	0,	Suint4,		0,
	REG_R2,		"%r2",		4,	0,	Suint4,		0,
	REG_R3,		"%r3",		4,	0,	Suint4,		0,
	REG_R4,		"%r4",		4,	0,	Suint4,		0,
	REG_R5,		"%r5",		4,	0,	Suint4,		0,
	REG_R6,		"%r6",		4,	0,	Suint4,		0,
	REG_R7,		"%r7",		4,	0,	Suint4,		0,
	REG_R8,		"%r8",		4,	0,	Suint4,		0,
	REG_FP,		"%fp",		4,	0,	Suint4,		0,
	REG_AP,		"%ap",		4,	0,	Suint4,		0,
	REG_PS,		"%psw",		4,	0,	Suint4,		0,
	REG_SP,		"%sp",		4,	0,	Suint4,		0,
	REG_PCBP,	"%pcbp",	4,	0,	Suint4,		0,
	REG_ISP,	"%isp",		4,	0,	Suint4,		0,
	REG_PC,		"%pc",		4,	0,	Suint4,		0,

/* 16, 17 unused */
	REG_PC,		"",		4,	0,	Suint4,		0,
	REG_PC,		"",		4,	0,	Suint4,		0,

/* MAU registers */

	REG_ASR,	"%asr",		4,	0,	Suint4,		0,
	REG_DR,		"%dr",		12,	FPREG,	Sxfloat,	0,

	REG_X0,		"%x0",		12,	FPREG,	Sxfloat,	0,
	REG_X1,		"%x1",		12,	FPREG,	Sxfloat,	0,
	REG_X2,		"%x2",		12,	FPREG,	Sxfloat,	0,
	REG_X3,		"%x3",		12,	FPREG,	Sxfloat,	0,

	REG_D0,		"%d0",		8,	FPREG,	Sdfloat,	0,
	REG_D1,		"%d1",		8,	FPREG,	Sdfloat,	0,
	REG_D2,		"%d2",		8,	FPREG,	Sdfloat,	0,
	REG_D3,		"%d3",		8,	FPREG,	Sdfloat,	0,

	REG_F0,		"%f0",		4,	FPREG,	Ssfloat,	0,
	REG_F1,		"%f1",		4,	FPREG,	Ssfloat,	0,
	REG_F2,		"%f2",		4,	FPREG,	Ssfloat,	0,
	REG_F3,		"%f3",		4,	FPREG,	Ssfloat,	0,

/* synonyms */

	REG_R9,		"%r9",		4,	0,	Suint4,		0,
	REG_R10,	"%r10",		4,	0,	Suint4,		0,
	REG_R11,	"%r11",		4,	0,	Suint4,		0,
	REG_R12,	"%r12",		4,	0,	Suint4,		0,
	REG_R13,	"%r13",		4,	0,	Suint4,		0,
	REG_R14,	"%r14",		4,	0,	Suint4,		0,
	REG_R15,	"%r15",		4,	0,	Suint4,		0,

// end marker
	REG_UNK,	0,		0,	0,	SINVALID,	0

};
