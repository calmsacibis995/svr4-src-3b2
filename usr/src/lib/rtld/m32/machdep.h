/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:m32/machdep.h	1.2"

/* 3B2 machine dependent macros, constants and declarations */

#include <sys/sys3b.h>

extern int _sys3b	ARGS((int, unsigned long *));

/* object file macros */
#define ELF_TARGET_M32 1
#define M_MACH	EM_M32
#define M_CLASS	ELFCLASS32
#define M_DATA	ELFDATA2MSB
#define M_FLAGS	_flags

/* page size */

#define PAGESIZE	_syspagsz

/* segment boundary */

#define SEGSIZE		0x2000	/* 8k */

/* macro to truncate to previous page boundary */

#define PTRUNC(X)	((X) & ~(PAGESIZE - 1))

/* macro to round to next page boundary */

#define PROUND(X)	(((X) + PAGESIZE - 1) & ~(PAGESIZE - 1))

/* macro to round to next segment boundary */

#define SROUND(X)	(((X) + SEGSIZE - 1) & ~(SEGSIZE - 1))

/* macro to round to next double word boundary */

#define DROUND(X)	(((X) + sizeof(double) - 1) & ~(sizeof(double) - 1))


/* WE32100 instruction encodings - used for procedure linkage table entries */

/* opcodes: */

#define JMP	0x24
#define JSB	0x34
#define	PUSHW	0xa0

/* operand descriptors */

#define	IMM_MODE	0x4f	/* word immediate &foo      */
#define	ABS_MODE	0x7f	/* absolute address $sym    */
#define	DISPL_MODE	0x8f 	/* word displacement n(%pc) */

/* generic bit mask */

#define MASK(N)	((1 << (N)) -1)

/* is V in the range supportable in N bits ? */

#define IN_RANGE(V, N)  ((-(1 << ((N) - 1))) <= (V) && (V) < (1 << ((N) - 1)))

/* macro to determine if relocation is a PC-relative type */

#define PCRELATIVE(T)	((T) == R_M32_PC32_S)

/* default library search directory */

#define LIBDIR	"/usr/lib"
#define LIBDIRLEN	8

/* /dev/zero */
#define DEV_ZERO "/dev/zero"
