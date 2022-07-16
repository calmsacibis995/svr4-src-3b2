/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/OpTab.c	1.3"

#include	"defs.h"
#include	"optab.h"
#include	"OpTabTypes.h"

#ifndef MACRO
	unsigned int
GetOpFirstOp(op_code)		/* Get index of first operand.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{extern struct opent optab[];	/* The operation code table.	*/

 return(optab[op_code].firstop);		/* Extract it from table. */
}
#endif

#ifndef MACRO
	boolean
IsOpGMove(op_code)		/* TRUE if opcode is GMove.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{
 if(op_code == G_MOV)
	return(TRUE);
 if(op_code == G_MMOV)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsOpDstSrc(op_code)		/* TRUE if Destination is also Source.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{extern struct opent optab[];	/* The operation code table.	*/

 if(optab[op_code].oflags & DSTSRCG)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsOpAux(op_code)		/* TRUE if opcode is auxiliary.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{
 if(op_code <= AUPPER)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsOpPseudo(op_code)		/* TRUE if opcode is pseudo.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{
 if(op_code <= PLOWER)
	return(FALSE);
 if(op_code >= PUPPER)
	return(FALSE);
 return(TRUE);
}
#endif

#ifndef MACRO
	boolean
IsOpGeneric(op_code)		/* TRUE if opcode is generic.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{
 if(op_code <= GLOWER)
	return(FALSE);
 if(op_code >= GUPPER)
	return(FALSE);
 return(TRUE);
}
#endif

#ifndef MACRO
	boolean
IsOpIs25(op_code)		/* TRUE if opcode is is25.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{extern struct opent optab[];	/* The operation-code table.	*/

 if(optab[op_code].oflags & I25OPC)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsOpCpu(op_code)		/* TRUE if opcode is CPU.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{extern struct opent optab[];	/* The operation-code table.	*/

 if(optab[op_code].oflags & CPUOPC)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsOpMIS(op_code)		/* TRUE if opcode is MIS.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{extern struct opent optab[];	/* The operation-code table.	*/

 if(optab[op_code].oflags & MISOPC)
	return(TRUE);
 return(FALSE);
}
#endif

#ifndef MACRO
	boolean
IsOpCompare(op_code)		/* TRUE if opcode is Compare.	*/
unsigned int op_code;		/* Op-code index of instruction.	*/

{extern struct opent optab[];	/* The operation-code table.	*/

 if(optab[op_code].oflags & CMP)
	return(TRUE);
 return(FALSE);
}
#endif
