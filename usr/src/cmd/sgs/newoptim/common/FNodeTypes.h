/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/FNodeTypes.h	1.3"

#define	MaxLocalName	6

struct Function			/* Structure of a function node.	*/
	{AN_Id name;		/* Name of the function.	*/
	 struct Function *next;	/* FN_Id of next function node, if any.	*/
	 struct Function *prev;	/* FN_Id of previous function node, if any.*/
	 unsigned long int calls;/* 1st level call instructions. */
	 unsigned long int expansions; /* Actual expansions at all levels. */
	 unsigned long int instructions; /* Number of instructions. */
	 unsigned long int LocalSize;	/* Size of local area in bytes.	*/
	 unsigned int NumReg;	/* Number of registers used by function. */
	 unsigned int RVRegs;	/* Number of registers used to return values.*/
	 char LocalName[MaxLocalName];
	 unsigned auditerr:1;		/* 1 if error reported 		*/
	 unsigned blackbox:1;		/* 1 if node is black box.	*/
	 unsigned LocalSizeFlag:1;	/* 1 if LocalSize present.	*/
	 unsigned candidate:1;		/* 1 if func is cand for in-line */
	 unsigned defined:1;		/* 1 if function defined.	*/
	 unsigned library:1;		/* 1 if function in a library.	*/
	 unsigned nodblflg:1;		/* 1 if NODBL flag seen.	*/
	 unsigned setjmp:1;		/* 1 if function calls setjmp.  */
	 unsigned MISconvert:1;		/* 1 if function contains #TYPE.*/
	 unsigned PAlias:1;		/* 1 if function param stack's aliased. */

	 			/* expansion candidate data */

	 AN_Id begin;		/* Begin of hidden address node list.	*/
	 TN_Id start;		/* TN_Id of first node of function.	*/
	 TN_Id finish;		/* TN_Id of last node of function.	*/
	 ALN_Id abegin;		/* Begin of alias list.			*/
	 unsigned long int expansion_limit; /* Limit on 1st level expansions.*/
	 unsigned long int cand_loc_sz; /* Candiate local size. */
	};

typedef struct Function *FN_Id;
