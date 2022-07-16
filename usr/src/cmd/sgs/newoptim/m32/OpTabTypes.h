/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/OpTabTypes.h	1.2"

#define	MAX_OPS	4		/* Maximum operands in an instruction.	*/

struct opent			/* The operation code table.	*/
	{char *oname;		/* Opcode string.	*/
	 unsigned long oflags;	/* Opcode description flags.	*/
	 short oopcode;		/* Associated opcode for
					branches:		reverse opcode
					is25 non-branches:	assoc CPU opcode
					generic opcodes:	default opcode
					non-generic opcodes:	generic opcode
					other:			OLOWER	*/
	 short onongens;	/* Index for associated non-generic opcodes. */
	 unsigned short ouseregs;	/* Registers used.	*/
	 unsigned short osetregs;	/* Registers set.	*/
	 unsigned char otype[MAX_OPS];	/* Operand types.	*/
	 unsigned char firstop;	/* Index of first operand.	*/
	 unsigned char osrcops;	/* Source operands.	*/
	 unsigned char odstops;	/* Destination operands.	*/
	 unsigned char ouseccs;	/* Used conditional codes.	*/
	 unsigned char osetccs;	/* Set conditional codes.	*/
	};

				/* opent.oflags values */
				/* auxilliary flags (bits 3-6) */
#define NOFLAG	0x0
#define MISOPC	0x8	/* MIS opcode */
#define CPUOPC	0x10	/* 32000 opcode */
#define SPOPC	0x20	/* support processor opcode */
#define I25OPC	0x40	/* IS25 opcode */
				/* flags used in all tables (bits 11-17) */
#define PRET	0x800		/* procedure return opcode */
#define	UNCBR	0x1000		/* unconditional branch */
#define	CBR	0x2000		/* cond branch (branch is UNCBR or CBR) */
#define	REV	0x4000		/* reversible branch */
#define	ALLMATH	0x8000		/* arithmetic plus logical instructions */
#define ARITH	0x10000		/* arithmetic instructions */
#define	CMP	0x20000		/* compare */
				/* flags used in internal opcodes (bits 18-24)*/
#define ZERO0	0x40000		/* identity generation, e.g., mulw &0,O */
#define IDENT0	0x80000		/* zero identity opcodes, e.g., addw &0,O */
#define IDENT1	0x100000	/* one identity opcodes, e.g., mulw &1,O */
#define DSTSRCG 0x200000	/* dst == src in generic opcodes */
#define CCNZ00	0x400000	/* logical plus field instructions */
#define CCNZCV	0x800000	/* additive arithmetic instructions */
#define CCNZ0V	0x1000000	/* non-additive arithmetic instructions */
				/* flags for MIS instructions */
#define IUMR0	0x2000000	/* never implicitly uses a MAU reg */
#define IUMR1	0x4000000	/* sometimes implicitly uses a MAU reg */
#define IUMR2	0x8000000	/* always implicitly uses a MAU reg */

				/* opent.osrcops and opent.odstops values */
#define OOPR0	0x1
#define OOPR1	0x2
#define OOPR2	0x4
#define OOPR3	0x8

		/* opent.ouseccs and opent.osetccs values (internal use only) */
# define OCCCPUC	0x1
# define OCCCPUN	0x2
# define OCCCPUV	0x4
# define OCCCPUX	0x8
# define OCCCPUZ	0x10
# define OCCMAUASR	0x20

				/* opent.ouseregs and opent.osetregs values */
# define OREG0		0x1
# define OREG1		0x2
# define OREG2		0x4
# define OREG3_8	0x8
# define OREGFP		0x10
# define OREGAP		0x20
# define OREGPSW	0x40
# define OREGSP		0x80
# define OREGPCBP	0x100
# define OREGISP	0x200
# define OREGPC		0x400
# define OREGASR	0x800
# define OREGMFPR	0x1000
# define OREGENAQS	0x2000	/* ENAQS	*/
# define OREGSENAQS	0x4000	/* SENAQS	*/
# define OREGSNAQS	0x8000  /* SNAQS	*/
# define OREGALL	(OREG0|OREG1|OREG2|OREG3_8|OREGFP|OREGAP|OREGPSW|OREGSP|OREGPCBP|OREGISP|OREGPC|OREGASR|OREGMFPR|OREGENAQS|OREGSENAQS|OREGSNAQS)

struct htabent {
	struct opent *op;
	struct htabent *next;
	};
# define HTABSZ	1024
