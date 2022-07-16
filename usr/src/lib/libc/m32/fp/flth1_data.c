/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:fp/flth1_data.c	1.1"

/* This file contains definitions of constant tables that are used
 * by getflthw() (getflth1.c).  Currently the compiler is not able
 * to put const arrays in rodata with -KPIC; the code generator
 * does not have enough information to know when that is safe.
 * Therefore these tables are compiled separately without -KPIC.
 * When the compiler is changed this file can be removed.
 */

#include	"synonyms.h"
#include	<sys/signal.h>	/* for SIGFPE			*/
#include	<sys/psw.h>	/* for PSW fault-type defines	*/
#define	P754_NOFAULT	1	/* avoid generating extra code	*/
#include	<ieeefp.h>	/* floating-point data-types	*/

#define IS_NULL	(0)
#define IS_MEM	(1)
#define IS_REG	(2)
    struct mau_op {
	char	type;		/* IS_NULL, IS_MEM, or IS_REG	*/
	char	regno;		/* in [0,3] */
	char	format;		/* in [1,3] */
	char	rounding;	/* in [0,3] (0 if rounding is unknown*/
    };

#define	OP1	0x10	/* 1 byte opcodes			  */
#define	OP2	0x20	/* 2 byte opcodes: macro-rom, NOP2, EXTOP */
#define	OP3	0x30	/* 3 byte opcodes: NOP3			  */
#define	OP5	0x50	/* 5 byte opcodes: support processor	  */
const unsigned char _flth_op_table[ 256 ] = {
    0,		0,		OP5|1,		OP5|2,	/* 0x00-0x03 */
    OP1|2,	0,		OP5|1,		OP5|2,	/* 0x04-0x07 */
    OP1|0,	0,		0,		0,	/* 0x08-0x0b */
    OP1|2,	0,		0,		0,	/* 0x0c-0x0f */
    OP1|1,	0,		0,		OP5|1,	/* 0x10-0x13 */
    OP2|0,	0,		0,		OP5|1,	/* 0x14-0x17 */
    OP1|1,	0,		0,		0,	/* 0x18-0x1b */
    OP1|1,	0,		OP1|1,		OP1|1,	/* 0x1c-0x1f */

    OP1|1,	0,		OP5|1,		OP5|2,	/* 0x20-0x23 */
    OP1|1,	0,		0,		OP1|0,	/* 0x24-0x27 */
    OP1|1,	0,		OP1|1,		OP1|1,	/* 0x28-0x2b */
    OP1|2,	0,		OP1|0,		OP1|0,	/* 0x2c-0x2f */
    OP2|0,	0,		OP5|0,		OP5|1,	/* 0x30-0x33 */
    OP1|1,	0,		OP3|0,		OP2|0,	/* 0x34-0x37 */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0x38-0x3b */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0x3c-0x3f */

    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x40-0x43 */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x44-0x47 */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x48-0x4b */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x4c-0x4f */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x50-0x53 */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x54-0x57 */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x58-0x5b */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x5c-0x5f */

    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x60-0x63 */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x64-0x67 */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x68-0x6b */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x6c-0x6f */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x70-0x73 */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x74-0x77 */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x78-0x7b */
    OP1|0,	0,		OP3|0,		OP2|0,	/* 0x7c-0x7f */

    OP1|1,	0,		OP1|1,		OP1|1,	/* 0x80-0x83 */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0x84-0x87 */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0x88-0x8b */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0x8c-0x8f */
    OP1|1,	0,		OP1|1,		OP1|1,	/* 0x90-0x93 */
    OP1|1,	0,		OP1|1,		OP1|1,	/* 0x94-0x97 */
    0,		0,		0,		0,	/* 0x98-0x9b */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0x9c-0x9f */

    OP1|1,	0,		0,		0,	/* 0xa0-0xa3 */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0xa4-0xa7 */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0xa8-0xab */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0xac-0xaf */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0xb0-0xb3 */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0xb4-0xb7 */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0xb8-0xbb */
    OP1|2,	0,		OP1|2,		OP1|2,	/* 0xbc-0xbf */

    OP1|3,	0,		0,		0,	/* 0xc0-0xc3 */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xc4-0xc7 */
    OP1|4,	0,		OP1|4,		OP1|4,	/* 0xc8-0xcb */
    OP1|4,	0,		OP1|4,		OP1|4,	/* 0xcc-0xcf */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xd0-0xd3 */
    OP1|3,	0,		0,		0,	/* 0xd4-0xd7 */
    OP1|3,	0,		0,		0,	/* 0xd8-0xdb */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xdc-0xdf */

    OP1|1,	0,		0,		0,	/* 0xe0-0xe3 */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xe4-0xe7 */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xe8-0xeb */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xec-0xef */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xf0-0xf3 */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xf4-0xf7 */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xf8-0xfb */
    OP1|3,	0,		OP1|3,		OP1|3,	/* 0xfc-0xff */
    };

const char	_flth_op_size[16] = {
	1,1,1,1, /* 0-literal, 1-literal, 2-literal, 3-literal	*/
	1,1,1,1, /* 4-register,5-reg.def, 6-FP off,  7-AP off	*/
	5,5,3,3, /* 8-word dis,9-w.d.def, a-half dis,b-h.d. def */
	2,2,1,1, /* c-byte dis,d-b.d.def, e-expando, f-literal	*/
	};
const char  _flth_pc_op_size[16] = {
	1,1,1,1, /* 0-literal, 1-literal, 2-literal, 3-literal	*/
	5,3,2,5, /* 4-word im, 5-half im, 6-byte im, 7-absolute	*/
	5,5,3,3, /* 8-word dis,9-w.d.def, a-half dis,b-h.d. def	*/
	2,2,5,1, /* c-byte dis,d-b.d.def, e-abs.def, f-literal	*/
	};

const struct mau_op _flth_src[8] = {
	{ IS_REG,	0,	3,	0 },
	{ IS_REG,	1,	3,	0 },
	{ IS_REG,	2,	3,	0 },
	{ IS_REG,	3,	3,	0 },
	{ IS_MEM,	0,	1,	1 },
	{ IS_MEM,	0,	2,	2 },
	{ IS_MEM,	0,	3,	3 },
	{ IS_NULL,	0,	0,	0 } };
const struct mau_op _flth_dst[16] = {
	{ IS_REG,	0,	3,	1 },
	{ IS_REG,	1,	3,	1 },
	{ IS_REG,	2,	3,	1 },
	{ IS_REG,	3,	3,	1 },
	{ IS_REG,	0,	3,	2 },
	{ IS_REG,	1,	3,	2 },
	{ IS_REG,	2,	3,	2 },
	{ IS_REG,	3,	3,	2 },
	{ IS_REG,	0,	3,	3 },
	{ IS_REG,	1,	3,	3 },
	{ IS_REG,	2,	3,	3 },
	{ IS_REG,	3,	3,	3 },
	{ IS_MEM,	0,	1,	1 },
	{ IS_MEM,	0,	2,	2 },
	{ IS_MEM,	0,	3,	3 },
	{ IS_NULL,	0,	0,	0 } };
    
    /* This array gives the mapping from MAU opcode to fp_op.
     * Opcodes that should not get to format_data() are 'FP_ADD',
     * since they will have been caught by mau_opcode().
     */
const fp_op	_flth_op_map[32] = {
	FP_ADD,  FP_ADD,  FP_ADD,  FP_SUB,		/*  0- 3 */
	FP_DIV,  FP_REM,  FP_MULT, FP_CONV,		/*  4- 7 */
	FP_ADD,  FP_ADD,  FP_CMP,  FP_CMPT,		/*  8- b */
	FP_ABS,  FP_SQRT, FP_RNDI, FP_CONV,		/*  c- f */
	FP_CONV, FP_CONV, FP_CONV, FP_ADD,		/* 10-13 */
	FP_ADD,  FP_ADD,  FP_ADD,  FP_NEG,		/* 14-17 */
	FP_ADD,  FP_ADD,  FP_CMP,  FP_CMPT,		/* 18-1b */
	FP_SIN,  FP_COS,  FP_ATN,  FP_ADD,		/* 1c-1f */
	};

#define	K	(1024)
const struct expval {
	int	minexp;
	int	maxexp;
	int	bias;
	} _flth_expval[4] = {
	    { 0,		0,		0 }, /* ignore [0]   */
	    { 0 +              (16*K - 1) - (128 - 1),
		(256 - 2) +    (16*K - 1) - (128 - 1),
		3*(1<<6) },				/* single */
	    { 0 +              (16*K - 1) - (1*K - 1),
		(2*K - 2) +    (16*K - 1) - (1*K - 1),
		3*(1<<9) },				/* double */
	    { 0 +              (16*K - 1) - (16*K - 1),
		(32*K - 2) +   (16*K - 1) - (16*K - 1),
		3*(1<<13)}				/* d-x    */
	    };
