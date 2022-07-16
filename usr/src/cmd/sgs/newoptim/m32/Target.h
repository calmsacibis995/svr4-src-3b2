/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/Target.h	1.1"


/*
 *	This file "breaks out" the target hardware #define
 *	into orthogonal #defines:
 *
 *	CPU identifiers:
 *		WE32001		32A module
 *		WE32100		32B chip
 *		WE32200		BFO chip
 *		WECRISP		CRISP chip
 *
 *	Floating-point hardware identifiers:
 *		WE32TRAP	trapping f.p. (obsolete)
 *		WE32FPE		function call f.p.
 *		WE32106		MAU
 *		WE32206		MAU Follow-on
 *
 *	Abbreviations:
 *		ABWORMAC =	32A without RESTORE workaround (function call)
 *		ABWRMAC =	32A with RESTORE workaround (function call)
 *		BMAC =		32B with no f.p. chip (function call)
 *		BMAUMAC =	32B with MAU chip (inline)
 *		BMFOMAC =	32B with MAU Follow On chip (inline)
 *		BFO =		32B Follow On with no f.p. chip (function call)
 *		BFOMFO =	32B Follow On with MAU Follow On chip (inline)
 *		CRISP =		CRISP with no f.p. chip (function call)
 *		CRISPMAU =	CRISP with MAU chip (inline)
 *		CRISPMFO =	CRISP with MAU Follow On chip (inline)
 */

#define	WE32001		1	/* the following #defines must be distinct */
#define	WE32100		2
#define	WE32200		3
#define	WECRISP		4
#define	WE32TRAP	5
#define	WE32FPE		6
#define	WE32106		7
#define	WE32206		8

#if defined(BMAC)
#define	CPU_CHIP	WE32100
#define	MATH_CHIP	WE32FPE
#endif

#if defined(BMAUMAC)
#define	CPU_CHIP	WE32100
#define	MATH_CHIP	WE32106
#endif

#if defined(BMFOMAC)
#define	CPU_CHIP	WE32100
#define	MATH_CHIP	WE32206
#endif

#if defined(BFO)
#define	CPU_CHIP	WE32200
#define	MATH_CHIP	WE32FPE
#endif

#if defined(BFOMFO)
#define	CPU_CHIP	WE32200
#define	MATH_CHIP	WE32206
#endif

#if defined(CRISP)
#define	CPU_CHIP	WECRISP
#define	MATH_CHIP	WE32FPE
#endif

#if defined(CRISPMAU)
#define	CPU_CHIP	WECRISP
#define	MATH_CHIP	WE32106
#endif

#if defined(CRISPMFO)
#define	CPU_CHIP	WECRISP
#define	MATH_CHIP	WE32206
#endif

#if !defined(CPU_CHIP)		/* ABWORMAC, ABWRMAC, and defaults */
#define	CPU_CHIP	WE32001
#define	MATH_CHIP	WE32FPE
#endif

typedef enum {
	we32001 = WE32001,
	we32100 = WE32100,
	we32200 = WE32200,
	weCRISP = WECRISP
} m32_target_cpu;

typedef enum {
	we32trap = WE32TRAP,
	we32fpe = WE32FPE,
	we32106 = WE32106,
	we32206 = WE32206
} m32_target_math;

extern m32_target_cpu	cpu_chip;
extern m32_target_math	math_chip;
