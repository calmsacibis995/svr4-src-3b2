/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/OpTabDefs.h	1.2"

#ifndef MACRO
extern unsigned GetOpFirstOp();
extern boolean IsOpIs25();
extern boolean IsOpCpu();
extern boolean IsOpMIS();
extern boolean IsOpCompare();
extern boolean IsOpDstSrc();
#else
extern struct opent optab[];
#define	GetOpFirstOp(opc)	((unsigned int) optab[opc].firstop)
#define	IsOpIs25(opc)	((optab[opc].oflags & I25OPC) ? TRUE : FALSE)
#define	IsOpCpu(opc)	((optab[opc].oflags & CPUOPC) ? TRUE : FALSE)
#define	IsOpMIS(opc)	((optab[opc].oflags & MISOPC) ? TRUE : FALSE)
#define	IsOpCompare(opc)	((optab[opc].oflags & CMP) ? TRUE : FALSE)
#define	IsOpDstSrc(opc)	((optab[opc].oflags & DSTSRCG) ? TRUE : FALSE)
#endif /*MACRO*/

#ifndef MACRO
extern boolean IsOpAux();
extern boolean IsOpPseudo();
extern boolean IsOpGeneric();
extern boolean IsOpGMove();
#else
#define	IsOpAux(opc)	((opc) < AUPPER ? TRUE : FALSE)
#define	IsOpPseudo(opc)	((PLOWER < (opc) && (opc) < PUPPER) \
					? TRUE : FALSE)
#define	IsOpGeneric(opc)	((GLOWER < (opc) && (opc) < GUPPER) \
					? TRUE : FALSE)
#define	IsOpGMove(opc)	(((opc) == G_MOV || (opc) == G_MMOV) ? \
					TRUE : FALSE)
#endif /*MACRO*/

				/* MACRO to run through all useful operands. */
#define ALLOP(tn_id,op) \
	op = GetOpFirstOp(GetTxOpCodeX(tn_id)); op < Max_Ops; op++
