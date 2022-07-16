/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/sequence.c	1.9"

#include	<stdio.h>
#include	"defs.h"
#include	"LoopTypes.h"
#include	"OpTabTypes.h"
#include	"OperndType.h"
#include	"RegId.h"
#include	"RoundModes.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"olddefs.h"
#include	"ALNodeType.h"
#include	"ALNodeDefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"

	void
sequence1()			/* Sequences pass 1 optimizations.	*/

{extern void inconv();		/* Converts input into internal form.	*/
 extern void pveval();		/* Evaluates pseudo-variables.	*/
 extern void translate();	/* Translate src arch label refs.	*/

 pveval();				/* Evaluate pseudo-variables. */

 inconv();				/* input conversion */

 translate();				/* translate label ref's (for src arch). */

 return;
}
	void
sequence2(ldtab)		/* Sequences pass 2 optimizations.	*/
struct ldent ldtab[];		/* The Live-Dead Table for GRA.		*/

{extern void Estim();		/* Sets address node estimators.	*/
 extern FN_Id FuncId;		/* FN_Id of current function.		*/
 extern void SetRegEstim();	/* Sets register estimators.		*/
 extern boolean Zflag;		/* TRUE if externs are nonvolatile.	*/
 boolean moved;			/* TRUE if LICM moved instrs out of loops */
 extern void bldgr();		/* Build flowgraph; in this file.	*/
 extern void chkauto();		/* Eliminates auto area if no more autos. */
 extern void comtail();		/* Common tail merging.			*/
 extern void cse();		/* Common Subexpression Elimination.	*/
 extern void gra();		/* Defined in func.c. 			*/
 extern void ilfunc();		/* Defined in func.c. 			*/
 extern void ldanal();		/* Does live-dead analysis.		*/
 extern void ldinit();		/* Prepares for live-dead analysis.	*/
 extern boolean loop();		/* Top of loop invariant code motion.	*/
 extern void mrgbrs();		/* Merges branches.			*/
 extern void outconv();		/* Converts to cpu instructions.	*/
 extern void peep();		/* Defined in peep.c.		 	*/
 extern void remLOOP();		/* Removes #LOOP nodes.			*/
 extern void reord();		/* Reorders loops.			*/
 extern void rer();		/* Eliminates branches to rets.		*/
 extern void rmbrs();		/* Removes branches to branches.	*/
 extern void rmlbls();		/* Removes useless labels.		*/
 extern void rmunrch();		/* Removes unreachable code.		*/
 extern void value();		/* Defined in value.c. */

				/* INLINE EXPANSION.	*/
 ilfunc();
				/* consolidate alias info. */
 DoFnAlias(FuncId);

				/* COMMON SUBEXPRESSION ELIMINATION */
 SetRegEstim("CM");			/* Set register estimators.*/
 SortAdGnaq((unsigned int) NAQ, 0);	/* Sort NAQs. */
 ldinit();
 ldanal();
 cse();	
				/* VALUE TRACE.	*/
 Estim();				/* Set address estimators. */
 SetRegEstim("M");			/* Set register estimators.*/
 SortAdGnaq((unsigned int) NAQ |
	    (unsigned int) SNAQ, 0);	/* Sort NAQs. */
 value();				/* Do value tracing.	*/

				/* LOOP INVARIANT CODE MOTION.*/
 Estim();				/* Set address estimators. */
 SortAdGnaq((unsigned int) NAQ |
	    (unsigned int) SNAQ |
	    (unsigned int) ENAQ |
	    (unsigned int) SENAQ |
	    (unsigned int) SV, LICM_SIZE);	/* Sort GNAQS.	*/
 moved = loop();

				/* get ready for the rest of opts */
 if(moved)				/* If we've moved instructions out */
					/* of loops, we need to re-compute */
					/* address payoff estimates.       */
					/* Note that loopclean only changes*/
					/* text size and NOT the number of */
					/* runtime references of addresses.*/
	Estim();				/* Recompute estimators. */

 SetRegEstim("CM");			/* Set register estimators. */
 if(Zflag)				/* Sort address nodes.*/
	SortAdGnaq((unsigned int) NAQ |	
		   (unsigned int) SNAQ |
		   (unsigned int) ENAQ |
		   (unsigned int) SENAQ, 0);	
 else					
	SortAdGnaq((unsigned int) NAQ |
		   (unsigned int) SNAQ,0);

 remLOOP();				/* Rm loop indicator nodes. */

				/* PORTABLE CODE IMPROVER. */
 ldinit();				/* Initialize for l/d analysis. */

 if(GetTxNextNode((TN_Id) NULL) != NULL)	/* If text nodes, */
	{
#ifdef ISLABREF
	 rmlbls(TRUE);			/* Remove useless labels. */
#else
	 rmlbls();
#endif
	 bldgr(TRUE);			/* Build flow graph and call bboptim */
	 mrgbrs();			/* Merge branches to branches */
	 rmunrch(FALSE);		/* remove unreachable code, don't
					** preserve block/node connectivity.	*/
	 comtail();			/* Remove common tails.	*/
	 reord();			/* Reorder code.	*/
	 rmbrs();			/* Remove redundant branches. */
#ifdef ISLABREF
	 rmlbls(FALSE);			/* Remove useless labels. */
#else
	 rmlbls();
#endif
	 ldanal();			/* Perform live/dead analysis.*/
	} 

	
 peep(1);			/* PEEPHOLE optimization peeppass 1. */
			
 peep(2);			/* PEEPHOLE optimization peeppass 2. */		
			
 gra(ldtab);			/* GLOBAL REGISTER ALLOCATION.	*/ 

 peep(3); 			/* PEEPHOLE optimization peeppass 3. */

 rer(); 			/* Remove extra returns. */

 chkauto();			/* Eliminate auto stack (if no more autos). */

 UndoFnAlias(FuncId);		/* undo aliasing */

 outconv();			/* convert to cpu instructions for output. */

 return;
}
