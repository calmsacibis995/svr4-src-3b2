//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libmachine/m32/Instr.C	1.13"
#include	"Instr.h"
#include	"Interface.h"
#include	"Itype.h"
#include	"Locdesc.h"
#include	"Place.h"
#include	"Process.h"
#include	<string.h>
#include 	"Symtab.h"
#include	"Symbol.h"

enum Opndtype	{
			none,
			disp8,
			disp16,
			bit32,
			moded,
		};

struct Instrdata	{
	char *		mnemonic;
	Opndtype	opnd1;
	Opndtype	opnd2;
	Opndtype	opnd3;
	Opndtype	opnd4;
};

struct Instrdata instrdata [256] = {
	{	"<none>",	none,	none,	none,	none	},	// 00
	{	"<none>",	none,	none,	none,	none	},	// 01
	{	"SPOPRD",	bit32,	moded,	none,	none	},	// 02
	{	"SPOPD2",	bit32,	moded,	moded,	none	},	// 03
	{	"MOVAW",	moded,	moded,	none,	none	},	// 04
	{	"<none>",	none,	none,	none,	none	},	// 05
	{	"SPOPRT",	bit32,	moded,	none,	none	},	// 06
	{	"SPOPT2",	bit32,	moded,	none,	none	},	// 07
	{	"RET",		none,	none,	none,	none	},	// 08
	{	"<none>",	none,	none,	none,	none	},	// 09
	{	"<none>",	none,	none,	none,	none	},	// 0a
	{	"<none>",	none,	none,	none,	none	},	// 0b
	{	"<none>",	none,	none,	none,	none	},	// 0c
	{	"<none>",	none,	none,	none,	none	},	// 0d
	{	"<none>",	none,	none,	none,	none	},	// 0e
	{	"<none>",	none,	none,	none,	none	},	// 0f

	{	"SAVE", 	moded,	none,	none,	none	},	// 10
	{	"<none>",	none,	none,	none,	none	},	// 11
	{	"<none>",	none,	none,	none,	none	},	// 12
	{	"SPOPWD",	bit32,	moded,	none,	none	},	// 13
	{	"EXTOP",	disp8,	none,	none,	none	},	// 14
	{	"<none>",	none,	none,	none,	none	},	// 15
	{	"<none>",	none,	none,	none,	none	},	// 16
	{	"SPOPWT",	bit32,	moded,	none,	none	},	// 17
	{	"RESTORE",	moded,	none,	none,	none	},	// 18
	{	"<none>",	none,	none,	none,	none	},	// 19
	{	"<none>",	none,	none,	none,	none	},	// 1a
	{	"<none>",	none,	none,	none,	none	},	// 1b
	{	"SWAPWI",	moded,	none,	none,	none	},	// 1c
	{	"<none>",	none,	none,	none,	none	},	// 1d
	{	"SWAPHI",	moded,	none,	none,	none	},	// 1e
	{	"SWAPBI",	moded,	none,	none,	none	},	// 1f

	{	"POPW",		moded,	none,	none,	none	},	// 20
	{	"<none>",	none,	none,	none,	none	},	// 21
	{	"SPOPRS",	bit32,	moded,	none,	none	},	// 22
	{	"SPOPS2",	bit32,	moded,	moded,	none	},	// 23
	{	"JMP",		moded,	none,	none,	none	},	// 24
	{	"<none>",	none,	none,	none,	none	},	// 25
	{	"<none>",	none,	none,	none,	none	},	// 26
	{	"<none>",	none,	none,	none,	none	},	// 27
	{	"TSTW",		moded,	none,	none,	none	},	// 28
	{	"<none>",	none,	none,	none,	none	},	// 29
	{	"TSTH",		moded,	none,	none,	none	},	// 2a
	{	"TSTB",		moded,	none,	none,	none	},	// 2b
	{	"CALL",		moded,	moded,	none,	none	},	// 2c
	{	"<none>",	none,	none,	none,	none	},	// 2d
	{	"BPT",		none,	none,	none,	none	},	// 2e
	{	"WAIT",		none,	none,	none,	none	},	// 2f

	{	"<none>",	none,	none,	none,	none	},	// 30
	{	"<none>",	none,	none,	none,	none	},	// 31
	{	"SPOP",		bit32,	none,	none,	none	},	// 32
	{	"SPOPWS",	bit32,	moded,	none,	none	},	// 33
	{	"JSB",		moded,	none,	none,	none	},	// 34
	{	"<none>",	none,	none,	none,	none	},	// 35
	{	"BSBH",		disp16,	none,	none,	none	},	// 36
	{	"BSBB",		disp8,	none,	none,	none	},	// 37
	{	"BITW",		moded,	moded,	none,	none	},	// 38
	{	"<none>",	none,	none,	none,	none	},	// 39
	{	"BITH",		moded,	moded,	none,	none	},	// 3a
	{	"BITB",		moded,	moded,	none,	none	},	// 3b
	{	"CMPW",		moded,	moded,	none,	none	},	// 3c
	{	"<none>",	none,	none,	none,	none	},	// 3d
	{	"CMPH",		moded,	moded,	none,	none	},	// 3e
	{	"CMPB",		moded,	moded,	none,	none	},	// 3f

	{	"RGEQ",		none,	none,	none,	none	},	// 40
	{	"<none>",	none,	none,	none,	none	},	// 41
	{	"BGEH",		disp16,	none,	none,	none	},	// 42
	{	"BGEB",		disp8,	none,	none,	none	},	// 43
	{	"RGTR",		none,	none,	none,	none	},	// 44
	{	"<none>",	none,	none,	none,	none	},	// 45
	{	"BGH",		disp16,	none,	none,	none	},	// 46
	{	"BGB",		disp8,	none,	none,	none	},	// 47
	{	"RLSS",		none,	none,	none,	none	},	// 48
	{	"<none>",	none,	none,	none,	none	},	// 49
	{	"BLH",		disp16,	none,	none,	none	},	// 4a
	{	"BLB",		disp8,	none,	none,	none	},	// 4b
	{	"RLEQ",		none,	none,	none,	none	},	// 4c
	{	"<none>",	none,	none,	none,	none	},	// 4d
	{	"BLEH",		disp16,	none,	none,	none	},	// 4e
	{	"BLEB",		disp8,	none,	none,	none	},	// 4f

	{	"RCC",		none,	none,	none,	none	},	// 50
	{	"<none>",	none,	none,	none,	none	},	// 51
	{	"BCCH",		disp16,	none,	none,	none	},	// 52
	{	"BCCB",		disp8,	none,	none,	none	},	// 53
	{	"RGTRU",	none,	none,	none,	none	},	// 54
	{	"<none>",	none,	none,	none,	none	},	// 55
	{	"BGUH",		disp16,	none,	none,	none	},	// 56
	{	"BGUB",		disp8,	none,	none,	none	},	// 57
	{	"RCS",		none,	none,	none,	none	},	// 58
	{	"<none>",	none,	none,	none,	none	},	// 59
	{	"BCSH",		disp16,	none,	none,	none	},	// 5a
	{	"BCSB",		disp8,	none,	none,	none	},	// 5b
	{	"RLEQU",	none,	none,	none,	none	},	// 5c
	{	"<none>",	none,	none,	none,	none	},	// 5d
	{	"BLEUH",	disp16,	none,	none,	none	},	// 5e
	{	"BLEUB",	disp8,	none,	none,	none	},	// 5f

	{	"RVC",		none,	none,	none,	none	},	// 60
	{	"<none>",	none,	none,	none,	none	},	// 61
	{	"BVCH",		disp16,	none,	none,	none	},	// 62
	{	"BVCB",		disp8,	none,	none,	none	},	// 63
	{	"RNEQU",	none,	none,	none,	none	},	// 64
	{	"<none>",	none,	none,	none,	none	},	// 65
	{	"BNEH",		disp16,	none,	none,	none	},	// 66
	{	"BNEB",		disp8,	none,	none,	none	},	// 67
	{	"RVS",		none,	none,	none,	none	},	// 68
	{	"<none>",	none,	none,	none,	none	},	// 69
	{	"BVSH",		disp16,	none,	none,	none	},	// 6a
	{	"BVSB",		disp8,	none,	none,	none	},	// 6b
	{	"REQLU",	none,	none,	none,	none	},	// 6c
	{	"<none>",	none,	none,	none,	none	},	// 6d
	{	"BEH",		disp16,	none,	none,	none	},	// 6e
	{	"BEB",		disp8,	none,	none,	none	},	// 6f

	{	"NOP",		none,	none,	none,	none	},	// 70
	{	"<none>",	none,	none,	none,	none	},	// 71
	{	"NOP3",		none,	none,	none,	none	},	// 72
	{	"NOP2",		none,	none,	none,	none	},	// 73
	{	"RNEQ",		none,	none,	none,	none	},	// 74
	{	"<none>",	none,	none,	none,	none	},	// 75
	{	"BNEH",		disp16,	none,	none,	none	},	// 76
	{	"BNEB",		disp8,	none,	none,	none	},	// 77
	{	"RSB",		none,	none,	none,	none	},	// 78
	{	"<none>",	none,	none,	none,	none	},	// 79
	{	"BRH",		disp16,	none,	none,	none	},	// 7a
	{	"BRB",		disp8,	none,	none,	none	},	// 7b
	{	"REQL",		none,	none,	none,	none	},	// 7c
	{	"<none>",	none,	none,	none,	none	},	// 7d
	{	"BEH",		disp16,	none,	none,	none	},	// 7e
	{	"BEB",		disp8,	none,	none,	none	},	// 7f

	{	"CLRW",		moded,	none,	none,	none	},	// 80
	{	"<none>",	none,	none,	none,	none	},	// 81
	{	"CLRH",		moded,	none,	none,	none	},	// 82
	{	"CLRB",		moded,	none,	none,	none	},	// 83
	{	"MOVW",		moded,	moded,	none,	none	},	// 84
	{	"<none>",	none,	none,	none,	none	},	// 85
	{	"MOVH",		moded,	moded,	none,	none	},	// 86
	{	"MOVB",		moded,	moded,	none,	none	},	// 87
	{	"MCOMW",	moded,	moded,	none,	none	},	// 88
	{	"<none>",	none,	none,	none,	none	},	// 89
	{	"MCOMH",	moded,	moded,	none,	none	},	// 8a
	{	"MCOMB",	moded,	moded,	none,	none	},	// 8b
	{	"MNEGW",	moded,	moded,	none,	none	},	// 8c
	{	"<none>",	none,	none,	none,	none	},	// 8d
	{	"MNEGH",	moded,	moded,	none,	none	},	// 8e
	{	"MNEGB",	moded,	moded,	none,	none	},	// 8f

	{	"INCW",		moded,	none,	none,	none	},	// 90
	{	"<none>",	none,	none,	none,	none	},	// 91
	{	"INCH",		moded,	none,	none,	none	},	// 92
	{	"INCB",		moded,	none,	none,	none	},	// 93
	{	"DECW",		moded,	none,	none,	none	},	// 94
	{	"<none>",	none,	none,	none,	none	},	// 95
	{	"DECH",		moded,	none,	none,	none	},	// 96
	{	"DECB",		moded,	none,	none,	none	},	// 97
	{	"<none>",	none,	none,	none,	none	},	// 98
	{	"<none>",	none,	none,	none,	none	},	// 99
	{	"<none>",	none,	none,	none,	none	},	// 9a
	{	"<none>",	none,	none,	none,	none	},	// 9b
	{	"ADDW2",	moded,	moded,	none,	none	},	// 9c
	{	"<none>",	none,	none,	none,	none	},	// 9d
	{	"ADDH2",	moded,	moded,	none,	none	},	// 9e
	{	"ADDB2",	moded,	moded,	none,	none	},	// 9f

	{	"PUSHW",	moded,	none,	none,	none	},	// a0
	{	"<none>",	none,	none,	none,	none	},	// a1
	{	"<none>",	none,	none,	none,	none	},	// a2
	{	"<none>",	none,	none,	none,	none	},	// a3
	{	"MODW2",	moded,	moded,	none,	none	},	// a4
	{	"<none>",	none,	none,	none,	none	},	// a5
	{	"MODH2",	moded,	moded,	none,	none	},	// a6
	{	"MODB2",	moded,	moded,	none,	none	},	// a7
	{	"MULW2",	moded,	moded,	none,	none	},	// a8
	{	"<none>",	none,	none,	none,	none	},	// a9
	{	"MULH2",	moded,	moded,	none,	none	},	// aa
	{	"MULB2",	moded,	moded,	none,	none	},	// ab
	{	"DIVW2",	moded,	moded,	none,	none	},	// ac
	{	"<none>",	none,	none,	none,	none	},	// ad
	{	"DIVH2",	moded,	moded,	none,	none	},	// ae
	{	"DIVB2",	moded,	moded,	none,	none	},	// af

	{	"ORW2",		moded,	moded,	none,	none	},	// b0
	{	"<none>",	none,	none,	none,	none	},	// b1
	{	"ORH2",		moded,	moded,	none,	none	},	// b2
	{	"ORB2",		moded,	moded,	none,	none	},	// b3
	{	"XORW2",	moded,	moded,	none,	none	},	// b4
	{	"<none>",	none,	none,	none,	none	},	// b5
	{	"XORH2",	moded,	moded,	none,	none	},	// b6
	{	"XORB2",	moded,	moded,	none,	none	},	// b7
	{	"ANDW2",	moded,	moded,	none,	none	},	// b8
	{	"<none>",	none,	none,	none,	none	},	// b9
	{	"ANDH2",	moded,	moded,	none,	none	},	// ba
	{	"ANDB2",	moded,	moded,	none,	none	},	// bb
	{	"SUBW2",	moded,	moded,	none,	none	},	// bc
	{	"<none>",	none,	none,	none,	none	},	// bd
	{	"SUBH2",	moded,	moded,	none,	none	},	// be
	{	"SUBB2",	moded,	moded,	none,	none	},	// bf

	{	"ALSW3",	moded,	moded,	moded,	none	},	// c0
	{	"<none>",	none,	none,	none,	none	},	// c1
	{	"<none>",	none,	none,	none,	none	},	// c2
	{	"<none>",	none,	none,	none,	none	},	// c3
	{	"ARSW3",	moded,	moded,	moded,	none	},	// c4
	{	"<none>",	none,	none,	none,	none	},	// c5
	{	"ARSH3",	moded,	moded,	moded,	none	},	// c6
	{	"ARSB3",	moded,	moded,	moded,	none	},	// c7
	{	"INSFW",	moded,	moded,	moded,	moded	},	// c8
	{	"<none>",	none,	none,	none,	none	},	// c9
	{	"INSFH",	moded,	moded,	moded,	moded	},	// ca
	{	"INSFB",	moded,	moded,	moded,	moded	},	// cb
	{	"EXTFW",	moded,	moded,	moded,	moded	},	// cc
	{	"<none>",	none,	none,	none,	none	},	// cd
	{	"EXTFH",	moded,	moded,	moded,	moded	},	// ce
	{	"EXTFB",	moded,	moded,	moded,	moded	},	// cf

	{	"LLSW3",	moded,	moded,	moded,	none	},	// d0
	{	"<none>",	none,	none,	none,	none	},	// d1
	{	"LLSH3",	moded,	moded,	moded,	none	},	// d2
	{	"LLSB3",	moded,	moded,	moded,	none	},	// d3
	{	"LRSW3",	moded,	moded,	moded,	none	},	// d4
	{	"<none>",	none,	none,	none,	none	},	// d5
	{	"<none>",	none,	none,	none,	none	},	// d6
	{	"<none>",	none,	none,	none,	none	},	// d7
	{	"ROTW",		none,	none,	none,	none	},	// d8
	{	"<none>",	none,	none,	none,	none	},	// d9
	{	"<none>",	none,	none,	none,	none	},	// da
	{	"<none>",	none,	none,	none,	none	},	// db
	{	"ADDW3",	moded,	moded,	moded,	none	},	// dc
	{	"<none>",	none,	none,	none,	none	},	// dd
	{	"ADDH3",	moded,	moded,	moded,	none	},	// de
	{	"ADDB3",	moded,	moded,	moded,	none	},	// df

	{	"PUSHAW",	moded,	none,	none,	none	},	// e0
	{	"<none>",	none,	none,	none,	none	},	// e1
	{	"<none>",	none,	none,	none,	none	},	// e2
	{	"<none>",	none,	none,	none,	none	},	// e3
	{	"MODW3",	moded,	moded,	moded,	none	},	// e4
	{	"<none>",	none,	none,	none,	none	},	// e5
	{	"MODH3",	moded,	moded,	moded,	none	},	// e6
	{	"MODB3",	moded,	moded,	moded,	none	},	// e7
	{	"MULW3",	moded,	moded,	moded,	none	},	// e8
	{	"<none>",	none,	none,	none,	none	},	// e9
	{	"MULH3",	moded,	moded,	moded,	none	},	// ea
	{	"MULB3",	moded,	moded,	moded,	none	},	// eb
	{	"DIVW3",	moded,	moded,	moded,	none	},	// ec
	{	"<none>",	none,	none,	none,	none	},	// ed
	{	"DIVH3",	moded,	moded,	moded,	none	},	// ee
	{	"DIVB3",	moded,	moded,	moded,	none	},	// ef

	{	"ORW3",		moded,	moded,	moded,	none	},	// f0
	{	"<none>",	none,	none,	none,	none	},	// f1
	{	"ORH3",		moded,	moded,	moded,	none	},	// f2
	{	"ORB3",		moded,	moded,	moded,	none	},	// f3
	{	"XORW3",	moded,	moded,	moded,	none	},	// f4
	{	"<none>",	none,	none,	none,	none	},	// f5
	{	"XORH3",	moded,	moded,	moded,	none	},	// f6
	{	"XORB3",	moded,	moded,	moded,	none	},	// f7
	{	"ANDW3",	moded,	moded,	moded,	none	},	// f8
	{	"<none>",	none,	none,	none,	none	},	// f9
	{	"ANDH3",	moded,	moded,	moded,	none	},	// fa
	{	"ANDB3",	moded,	moded,	moded,	none	},	// fb
	{	"SUBW3",	moded,	moded,	moded,	none	},	// fc
	{	"<none>",	none,	none,	none,	none	},	// fd
	{	"SUBH3",	moded,	moded,	moded,	none	},	// fe
	{	"SUBB3",	moded,	moded,	moded,	none	},	// ff
};

struct Instrdata op30data [] = {
	{	"MVERNO",	none,	none,	none,	none	},	// 3009
	{	"MOVBLW",	none,	none,	none,	none	},	// 3019
	{	"STREND",	none,	none,	none,	none	},	// 301f
	{	"STRCPY",	none,	none,	none,	none	},	// 3035
	{	"GATE",		none,	none,	none,	none	},	// 3061
};

int	addrmode[16][16]	=	{

	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,	// 0
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,	// 1
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,	// 2
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,	// 3
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 13,	// 4
	17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, -1, 17, 17, 17, 12,	// 5
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11,	// 6
	 9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  1,	// 7
	 7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7, -1,  7,  7,  7,  7,	// 8
	 8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8, -1,  8,  8,  8,  8,	// 9
	 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5, -1,  5,  5,  5,  5,	// a
	 6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6, -1,  6,  6,  6,  6,	// b
	 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, -1,  3,  3,  3,  3,	// c
	 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, -1,  4,  4,  4,  4,	// d
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,  2,	// e
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,	// f

};

enum Modetype	{
			notused,	// invalid
			ab,		// Absolute
			ab_df,		// Absolute deferred
			bd,		// Byte displacement
			bd_df,		// Byte displacement deferred
			hd,		// Halfword displacement
			hd_df,		// Halfword displacement deferred
			wd,		// Word displacement
			wd_df,		// Word displacement deferred
			ap_so,		// AP short offset
			fp_so,		// FP short offset
			bi,		// Byte immediate
			hi,		// Halfword immediate
			wi,		// Word immediate
			plit,		// Positive literal
			nlit,		// Negative literal
			r,		// Register
			r_df,		// Register deferred
			ex_op,		// Extended-operand type
		};

struct Modedata {
	Modetype	type;
	char *		format;
	int		size;
};

Modedata	modedata[] = {
	{	notused,"",		0	},	// invalid
	{	ab,	"$%#x",		4	},	// Absolute
	{	ab_df,	"*$%#x",	4	},	// Absolute deferred
	{	bd,	"%#x(%s)",	1	},	// Byte disp.
	{	bd_df,	"*%#x(%s)",	1	},	// Byte disp. deferred
	{	hd,	"%#x(%s)",	2	},	// Halfword disp.
	{	hd_df,	"*%#x(%s)",	2	},	// Halfword disp. def
	{	wd,	"%#x(%s)",	4	},	// Word disp.
	{	wd_df,	"*%#x(%s)",	4	},	// Word disp. deferred
	{	ap_so,	"%#x(%%ap)",	0	},	// AP short offset
	{	fp_so,	"%#x(%%fp)",	0	},	// FP short offset
	{	bi,	"&%#x",		1	},	// Byte immediate
	{	hi,	"&%#x",		2	},	// Halfword immediate
	{	wi,	"&%#x",		4	},	// Word immediate
	{	plit,	"&%#x",		0	},	// Positive literal
	{	nlit,	"&%#x",		0	},	// Negative literal
	{	r,	"%s",		0	},	// Register
	{	r_df,	"(%s)",		0	},	// Register deferred
	{	ex_op,	"",		-1	},	// Extended-operand type
};

char * exoptype[16] =	{
				"{uword}",
				0,
				"{uhalf}",
				"{ubyte}",
				"{sword}",
				0,
				"{shalf}",
				"{sbyte}",
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
			};

char *	rname[16] =	{
				"%r0",
				"%r1",
				"%r2",
				"%r3",
				"%r4",
				"%r5",
				"%r6",
				"%r7",
				"%r8",
				"%fp",
				"%ap",
				"%psw",
				"%sp",
				"%pcbp",
				"%isp",
				"%pc",
			};


char *
Instr::sym_deasm(Iaddr address)
{
	Symtab *	symtab;
	Symbol		symbol;
	static char	lbuf[200];
	long		offset;
	Locdesc		locdesc;
	Place		place;
	Iaddr		addr;
	
	if ( (symtab = process->find_symtab( address )) == 0 )
	{
		::sprintf( lbuf, "%#x", address );
		return lbuf;
	}
	symbol = symtab->find_entry(address);
	if ( symbol.isnull() )
	{
		::sprintf( lbuf, "%#x", address );
		return lbuf;
	}
	else if ( symbol.locdesc( locdesc ) == 0 )
	{
		addr = symbol.pc( an_lopc );
		offset = address - addr;
		if ( offset != 0 )
		{
			::sprintf( lbuf, "%s+%#x", symbol.name(), offset );
			return lbuf;
		}
		else
			return symbol.name();
	}
	place = locdesc.place( 0, 0 );
	if ( place.isnull() || place.kind == pRegister )
	{
		return symbol.name();
	}
	offset = address - place.addr;
	if ( offset == 0 )
	{
		return symbol.name();
	}
	else
	{
		::sprintf( lbuf, "%s+%#x", symbol.name(), offset );
		return lbuf;
	}
}

char
Instr::get_byte()
{
	char	c;

	c = byte[i++];
	return c;
}

short
Instr::get_short()
{
	char *	p;
	short	hwrd;

	p = (char *) &hwrd;
	p[1] = byte[i++];
	p[0] = byte[i++];
	return hwrd;
}

long
Instr::get_long()
{
	char *	p;
	long	word;

	p = (char *) &word;
	p[3] = byte[i++];
	p[2] = byte[i++];
	p[1] = byte[i++];
	p[0] = byte[i++];
	return word;
}

Instrdata *
Instr::get_opcode( char * x )
{
	opcode = x[0];
	if ( opcode != 0x30 )
	{
		i = 1;
		return instrdata + opcode;
	}
	else
	{
		i = 2;
		switch ( x[1] )
		{
			case 0x09:	return op30data;
			case 0x19:	return op30data + 1;
			case 0x1f:	return op30data + 2;
			case 0x35:	return op30data + 3;
			case 0x61:	return op30data + 4;
			default:	return 0;
		}
	}
}

static char	buf[256];
char dis_buf[256];

int
Instr::parse_opnd( Opndtype optype )
{
	int		mmmm, rrrr;
	int		n;
	char		descbyte;

	switch ( optype )
	{
		case none:
			return 1;
		case disp8:
			i += 1;
			return 1;
		case disp16:
			i += 2;	
			return 1;
		case bit32:
			i += 4;	
			return 1;
		case moded:
			descbyte = get_byte();
			mmmm = descbyte >> 4;
			rrrr = descbyte & 0xf;
			if ( (n = addrmode[mmmm][rrrr]) == -1 )
			{
				printe("malformed instruction at %#x\n",addr);
				return 0;
			}
			else if ( (mmmm != 14) || (rrrr == 15) )
			{
				i += modedata[n].size;
				return 1;
			}
			descbyte = get_byte();
			mmmm = descbyte >> 4;
			rrrr = descbyte & 0xf;
			if ( (n = addrmode[mmmm][rrrr]) == -1 )
			{
				printe("malformed instruction at %#x\n",addr);
				return 0;
			}
			else if ( (mmmm != 14) || (rrrr == 15) )
			{
				i += modedata[n].size;
				return 1;
			}
			else
			{
				printe("malformed instruction at %#x\n",addr);
				return 0;
			}
		default:
			return 0;
	}
}

Instr::Instr( Process * p )
{
	process = p;
}

int
Instr::get_text( Iaddr pc )
{
	if ( process == 0 )
	{
		return 0;
	}
	else if ( pc == 0 )
	{
		return 0;
	}
	else if ( pc == addr )
	{
		return 1;
	}
	else
	{
		addr = pc;
		return (process->read( addr, 25, byte ) > 0);
	}
}


Iaddr
Instr::retaddr( Iaddr pc )
{
	Instrdata *	inst;

	if ( get_text( pc ) == 0 )
	{	
		return 0;
	}
	else if ( (inst = get_opcode( byte )) == 0 )
	{
		return 0;
	}
	else if ( opcode != 0x2c )
	{
		return 0;
	}
	else if ( parse_opnd( inst->opnd1 ) == 0 )
	{
		return 0;
	}
	else if ( parse_opnd( inst->opnd2 ) == 0 )
	{
		return 0;
	}
	else
	{
		return addr + i;
	}
}

Iaddr
Instr::next_instr( Iaddr pc )
{
	Instrdata *	inst;
	char *		s;
	char		opbyte[2];

	if ( get_text( pc ) == 0 )
	{	
		return 0;
	}
	if ( (s = process->text_nobkpt( pc )) == 0 )
	{
		opbyte[0] = byte[0];
		opbyte[1] = byte[1];
	}
	else
	{
		opbyte[0] = s[0];
		opbyte[1] = byte[1];
	}
	if ( (inst = get_opcode( opbyte )) == 0 )
	{
		return 0;
	}
	else if ( parse_opnd( inst->opnd1 ) == 0 )
	{
		return 0;
	}
	else if ( parse_opnd( inst->opnd2 ) == 0 )
	{
		return 0;
	}
	else if ( parse_opnd( inst->opnd3 ) == 0 )
	{
		return 0;
	}
	else if ( parse_opnd( inst->opnd4 ) == 0 )
	{
		return 0;
	}
	else
	{
		return addr + i;
	}
}

int
Instr::make_moded_opnd( Modedata & mdata, char desc_byte, int symbolic )
{
	int		rrrr;
	char		*name, string[40], temp[40];
	char		c;
	short		hwrd;
	long		word;

	rrrr = desc_byte & 0xf;
	switch ( mdata.type )
	{
		case notused:
			printe("internal error: ");
			printe("notused taken in Instr::make_moded_opnd\n");
			return 0;
		case bd:
		case bd_df:
			c = get_byte();
			::sprintf( string, mdata.format, c, rname[rrrr] );
			::strcat( dis_buf, string);
			break;
		case hd:
		case hd_df:
			hwrd = get_short();
			::sprintf( string, mdata.format, hwrd, rname[rrrr] );
			::strcat( dis_buf, string);
			break;
		case wd:
			word = get_long();
			::sprintf( string, mdata.format, word, rname[rrrr] );
			if ( symbolic )
			{
				name = sym_deasm( (Iaddr)word );
				::strcat( dis_buf, name );
			}
			break;
		case wd_df:
			word = get_long();
			::sprintf( string, mdata.format, word, rname[rrrr] );
			break;
		case ap_so:
		case fp_so:
			::sprintf( string, mdata.format, rrrr );
			::strcat( dis_buf, string);
			break;
		case bi:
			c = get_byte();
			::sprintf( string, mdata.format, c );
			::sprintf( temp, "%#x", c);
			::strcat( dis_buf, temp);
			break;
		case hi:
			hwrd = get_short();
			::sprintf( string, mdata.format, hwrd );
			::sprintf( temp, "%#x", hwrd );
			::strcat( dis_buf, temp);
			break;
		case wi:
			word = get_long();
			::sprintf( string, mdata.format, word );
			if ( symbolic )
			{
				name = sym_deasm( (Iaddr)word );
				::strcat( dis_buf, name );
			}
			break;
		case ab:
			word = get_long();
			::sprintf( string, mdata.format, word );
			if ( symbolic )
			{
				name = sym_deasm( (Iaddr) word );
				::strcat( dis_buf, name );
			}
			break;
		case ab_df:
			word = get_long();
			::sprintf( string, mdata.format, word );
			break;
		case r:
		case r_df:
			::sprintf( string, mdata.format, rname[rrrr] );
			::strcat( dis_buf, string);
			break;
		case plit:
			::sprintf( string, mdata.format, desc_byte );
			::sprintf( temp, "%#x", desc_byte );
			::strcat( dis_buf, temp);
			break;
		case nlit:
			::sprintf( string, mdata.format, ( -1 - rrrr )  );
			::sprintf( temp, "%#x", ( -1 - rrrr )  );
			::strcat( dis_buf, temp);
			break;
		case ex_op:
			printe("internal error: ");
			printe("ex_op taken in Instr::make_moded_opnd\n");
			return 0;
		default:
			printe("internal error: ");
			printe("unknown type in Instr::make_moded_opnd\n");
			return 0;
	}
	::strcat( buf, string );
	return 1;
}

int
Instr::make_opnd( Opndtype optype, int ord, int symbolic )
{
	int		mmmm, rrrr, tttt;
	char		descbyte;
	int		n;
	char *		s;
	char		string[40], * name;
	char		c;
	short		hwrd;
	long		word;

	if ( optype == none )
		return 1;
	else if ( ord == 1 )
		::strcat( buf, "\t" );
	else {
		::strcat( buf, "," );
		::strcat( dis_buf, "," );
	}

	switch ( optype )
	{
		case none:
			return 1;
		case disp8:
			c = get_byte();
			::sprintf( string, "%#x", c );
			::strcat( buf, string );
			if ( symbolic )
			{
				word = c;
				if ( word & 0x80 ) word |= ~0xff;
				::sprintf( string, "%#x", addr + word );
				::strcat( dis_buf, string);
			}
			return 1;
		case disp16:
			hwrd = get_short();
			::sprintf( string, "%#x", hwrd );
			::strcat( buf, string );
			if ( symbolic )
			{
				::sprintf( string, "%#x", addr + hwrd );
				::strcat( dis_buf, string);
			}
			return 1;
		case bit32:
			word = get_long();
			::sprintf( string, "%#x", word );
			::strcat( buf, string );
			name = sym_deasm( (Iaddr)word );
			if (symbolic ) ::strcat( dis_buf, name );
			return 1;
		case moded:
			descbyte = get_byte();
			mmmm = descbyte >> 4;
			rrrr = descbyte & 0xf;
			if ( (n = addrmode[mmmm][rrrr]) == -1 )
			{
				printe("malformed instruction at %#x\n",addr);
				return 0;
			}
			else if ( (mmmm != 14) || (rrrr == 15) )
			{
				return make_moded_opnd( modedata[n], descbyte,
							symbolic );
			}
			tttt = rrrr;
			descbyte = get_byte();
			mmmm = descbyte >> 4;
			rrrr = descbyte & 0xf;
			if ( (s = exoptype[tttt]) == 0 )
			{
				printe("malformed instruction at %#x\n",addr);
				return 0;
			}
			else if ( (n = addrmode[mmmm][rrrr]) == -1 )
			{
				printe("malformed instruction at %#x\n",addr);
				return 0;
			}
			else if ( (mmmm != 14) || (rrrr == 15) )
			{
				::strcat( buf, s );
				::strcat( dis_buf, s);
				return make_moded_opnd( modedata[n], descbyte,
							symbolic );
			}
			else
			{
				printe("malformed instruction at %#x\n",addr);
				return 0;
			}
		default:
			printe("wierd optype %#x in make_opnd()\n",optype);
			return 0;
	}
}

int
Instr::make_mnemonic( char * mnemonic, int symbolic )
{
	Symtab *	symtab;
	Symbol		symbol,srcsym;
	Source		source;
	long		line, offset;
	char		lbuf[30];
	Iaddr		lnaddr;

	if ( mnemonic == 0 )
	{
		printe("internal error: ");
		printe("null first argument to Instr::make_mnemonic()\n");
		return 0;
	}
	else if ( !symbolic )
	{
		::strcpy( buf, "\t" );
		::strcat( buf, mnemonic );
		return 1;
	}
	else if ( (symtab = process->find_symtab( addr )) == 0 )
	{
		::strcpy( buf, "\t" );
		::strcat( buf, mnemonic );
		::strcpy( dis_buf, "\t[" );
		return 1;
	}
	symbol = symtab->find_entry( addr );
	if ( symbol.isnull() )
	{
		::strcpy( buf, "\t" );
		::strcat( buf, mnemonic );
		::strcpy( dis_buf, "\t[" );
		return 1;
	}
	::strcpy( buf, "(" );
	::strcat( buf, symbol.name() );
	if ( symtab->find_source( addr, srcsym ) == 0 )
	{
		offset = addr - symbol.pc( an_lopc );
		if ( offset != 0 )
		{
			::sprintf( lbuf, "+%#x", offset );
			::strcat( buf, lbuf );
		}
		::strcat( buf, ")\t" );
		::strcat( buf, mnemonic );
		::strcpy( dis_buf, "\t[" );
		return 1;
	}
	else if ( srcsym.source( source ) == 0 )
	{
		offset = addr - symbol.pc( an_lopc );
		if ( offset != 0 )
		{
			::sprintf( lbuf, "+%#x", offset );
			::strcat( buf, lbuf );
		}
		::strcat( buf, ")\t" );
		::strcat( buf, mnemonic );
		::strcpy( dis_buf, "\t[" );
		return 1;
	}
	source.pc_to_stmt( addr, line, &lnaddr );
	if ( line == 0 )
	{
		offset = addr - symbol.pc( an_lopc );
		if ( offset != 0 )
		{
			::sprintf( lbuf, "+%#x", offset );
			::strcat( buf, lbuf );
		}
		::strcat( buf, ")\t" );
		::strcat( buf, mnemonic );
		::strcpy( dis_buf, "\t[" );
		return 1;
	}
	offset = addr - lnaddr;
	if ( offset != 0 )
	{
		::sprintf( lbuf, ":%d+%#x", line, offset );
		::strcat( buf, lbuf );
		::strcat( buf, ")\t" );
		::strcat( buf, mnemonic );
		::strcpy( dis_buf, "\t[" );
		return 1;
	}
	else
	{
		::sprintf( lbuf, ":%d", line );
		::strcat( buf, lbuf );
		::strcat( buf, ")\t" );
		::strcat( buf, mnemonic );
		::strcpy( dis_buf, "\t[" );
		return 1;
	}
}

char *
Instr::deasm ( Iaddr pc, int symbolic )
{
	Instrdata *	inst;
	char *		s;
	char		opbyte[2];

	if ( get_text( pc ) == 0 )
	{	
		return 0;
	}
	if ( (s = process->text_nobkpt( addr )) == 0 )
	{
		opbyte[0] = byte[0];
		opbyte[1] = byte[1];
	}
	else
	{
		opbyte[0] = s[0];
		opbyte[1] = byte[1];
	}
	if ( (inst = get_opcode( opbyte )) == 0 )
	{
		return 0;
	}
	else if ( make_mnemonic( inst->mnemonic, symbolic ) == 0 )
	{
		return 0;
	}
	else if ( make_opnd( inst->opnd1, 1, symbolic ) == 0 )
	{
		return 0;
	}
	else if ( make_opnd( inst->opnd2, 2, symbolic ) == 0 )
	{
		return 0;
	}
	else if ( make_opnd( inst->opnd3, 3, symbolic ) == 0 )
	{
		return 0;
	}
	else if ( make_opnd( inst->opnd4, 4, symbolic ) == 0 )
	{
		return 0;
	}
	else if ( symbolic )
	{
		::strcat(dis_buf,"]");
		::strcat(buf, dis_buf);
		return buf;
	}
	else
	{
		return buf;
	}
}

int
Instr::is_bkpt( Iaddr pc )
{
	Instrdata *	inst;

	if ( get_text( pc ) == 0 )
	{	
		return 0;
	}
	else if ( (inst = get_opcode( byte )) == 0 )
	{
		return 0;
	}
	else if ( opcode == 057 )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

#define JMPLEN		6	// length of a JMP instruction
#define JMPOPCOD	0x24	// JMP instruction opcode


//
// translate branch table address to the actual function address
//
Iaddr
Instr::brtbl2fcn( Iaddr addr )
{
	Instrdata *	inst;
	Iaddr		retval;

	if ( get_text( addr ) == 0 )
		return 0;
	if ( (inst = get_opcode( byte )) == 0 )
		return 0;
	if ( opcode != 0x24 )	// JMP instruction
		return addr;
	i++;			// skip addressing mode byte
	retval = get_long();
	return retval;
}

//
// translate  a function address to the adress of the corresponding 
// branch table slot
//
Iaddr
Instr::fcn2brtbl( Iaddr addr, int offset)
{
	return ((addr  & 0xffff0000 ) + (offset-1) * 6);
}

//
//
//
Iaddr
Instr::adjust_pc()
{
	return process->getreg(REG_PC);
}

//
//
//
Iaddr
Instr::fcn_prolog(Iaddr pc, short )
{
	return pc;
}

Iaddr
Instr::jmp_target( Iaddr pc )
{
	Instrdata *	inst;
	int		mmmm, rrrr;
	int		n;
	char		descbyte;
	Iaddr		vaddr;
	long		word;
	Itype		itype;

	if ( get_text( pc ) == 0 )
	{	
		return 0;
	}
	else if ( (inst = get_opcode( byte )) == 0 )
	{
		return 0;
	}
	else if ( opcode != 0x24 )
	{
		return 0;
	}
	descbyte = get_byte();
	mmmm = descbyte >> 4;
	rrrr = descbyte & 0xf;
	if ( (n = addrmode[mmmm][rrrr]) == -1 )
	{
		printe("malformed instruction at %#x\n",addr);
		return 0;
	}
	switch( modedata[n].type )
	{
		case notused:
			printe("internal error: ");
			printe("notused taken in Instr::jmp_target\n");
			return 0;
		case bd:
		case bd_df:
			break;
		case hd:
		case hd_df:
			break;
		case wd:
			word = get_long();
			vaddr = word + process->getreg( rrrr );
			return vaddr;
		case wd_df:
			word = get_long();
			vaddr = word + process->getreg( rrrr );
			if ( process->read( vaddr, Saddr, itype ) == 0 )
				return 0;
			else
				return itype.iaddr;
		case ap_so:
		case fp_so:
			break;
		case bi:
			break;
		case hi:
			break;
		case wi:
			break;
		case ab:
			break;
		case ab_df:
			break;
		case r:
		case r_df:
			break;
		case plit:
			break;
		case nlit:
			break;
		case ex_op:
			return 0;
		default:
			printe("internal error: ");
			printe("unknown type in Instr::jmp_target\n");
			return 0;
	}
	return 0;
}
