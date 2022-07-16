/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/m32/Reg1.h	1.3"

//
// NAME
//	Reg1.h
//
// ABSTRACT
//	Register names and indices (machine dependent)
//
// DESCRIPTION
//	Each machine has its own set of register names.  This header
//	give the names for the 3B2.  It is included by "Reg.h"
//

#ifndef REG1_H
#define REG1_H

/* IU registers */

#define	REG_R0		0
#define	REG_R1		1
#define	REG_R2		2
#define	REG_R3		3
#define	REG_R4		4
#define	REG_R5		5
#define	REG_R6		6
#define	REG_R7		7
#define	REG_R8		8
#define	REG_R9		9
#define	REG_R10		10
#define	REG_R11		11
#define	REG_R12		12
#define	REG_R13		13
#define	REG_R14		14
#define	REG_R15		15

/* synonyms */

#define	REG_FP		REG_R9
#define	REG_AP		REG_R10
#define	REG_PS		REG_R11
#define	REG_SP		REG_R12
#define	REG_PCBP	REG_R13
#define	REG_ISP		REG_R14
#define	REG_PC		REG_R15

/* 16, 17 unused */

/* MAU registers */

#define	REG_ASR		18	/* arbitrary */
#define	REG_DR		19	/* arbitrary */

#define	REG_X0		20	/* used by comp */
#define	REG_X1		21	/* used by comp */
#define	REG_X2		22	/* used by comp */
#define	REG_X3		23	/* used by comp */

#define	REG_D0		24	/* rest are arbitrary */
#define	REG_D1		25
#define	REG_D2		26
#define	REG_D3		27

#define	REG_F0		28
#define	REG_F1		29
#define	REG_F2		31
#define	REG_F3		32


#endif /* REG1_H */
