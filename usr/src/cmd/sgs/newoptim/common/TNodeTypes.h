/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/TNodeTypes.h	1.5"

struct node			/* Structure of each text node. */
	{struct node *forw;	/* Forward link. */
	 struct node *back;	/* Backward link. */
	 AN_Id addrs[MAX_OPS];	/* Operand Address-Node-Identifiers. */
	 unsigned long int uniqueid;	/*unique identification for this node */
	 unsigned auditerr:1;
	 unsigned protected:1;
	 unsigned blackbox:1;	/* TRUE is instr to be a black box. */
	 unsigned RoundMode:3;	/* Rounding mode for mis instructions.	*/
	 unsigned DVectors:3;	/* Provides for up to (2**3)-1 NVECTORS. */
	 unsigned LVectors:3;	/* Provides for up to (2**3)-1 NVECTORS. */
	 unsigned StackPtrInc:1;
	 unsigned mark:1;	/* generic mark bit */
	 unsigned long int nlive[NVECTORS];
	 unsigned long int ndead[NVECTORS];
	 unsigned short op;	/*Operation code. */
	 unsigned char types[MAX_OPS];	/* Operand Types. */
	 unsigned char flags[MAX_OPS];	/* Operand flags. */
#ifdef USERTDATA
	USERTTYPE userdata;	/* user-defined data for this node */
#endif /* USERDATA */
	};
typedef struct node NODE;	/* (For compatability with old stuff.) */
typedef struct node *TN_Id;

				/* bits for operand flags. */
#define	VOLATILE	01
