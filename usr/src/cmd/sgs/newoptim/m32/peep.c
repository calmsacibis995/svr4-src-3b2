/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/peep.c	1.3"

/* peep.c
**
**	peephole improvement driver
**
**
** This module contains the driver for the peephole improver.
*/

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"OpTabTypes.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"optab.h"
#include	"optim.h"
#include	"optutil.h"

#define	DESTINATION	3
#define MAX_WINDOW	3

#define non_window(p)	(IsTxBr(p) || IsTxAux(p) || IsTxBlackBox(p))

	void
peep(peeppass)			/* Peephole optimization scanner.*/
unsigned int peeppass;		/* Peephole pass number. */

/* We scan backwards in order to combine moves			*/
/* following instructions into those instructions before they	*/
/* get separated by the address arithmetic optimizations.	*/
/* We then expand the window forward to get the			*/
/* multi-instruction windows.  If any optimization is performed,*/
/* we revert to the one instruction window at the	 	*/
/* first instruction below the last instruction not changed.	*/
/* We, therefore, stay in a region of code as long as there 	*/
/* are changes to be made.					*/

/* Pointers for scanning a function to find windows:		*/
/*								*/
/*	tn_before - text node previous to window		*/
/*	tn_first  - first text node of window			*/
/*	...							*/
/* 	tn_last	 - last text node of window			*/
/*	tn_after - next text node after window			*/
/*								*/
/* For consistency, we use both a "first" and a "last" pointer  */
/* for w1opt, even though only one is necessary. 		*/
/*								*/
/* For efficiency, we stop propagating information when there	*/
/* are no changes to be made.  But we have to be careful	*/
/* that we do not try to do this while scanning forward.	*/
/* That could result in not propagating information. 		*/
/* In order to keep track where we are, we keep a pointer to	*/
/* the furthest (backward) progress of tn_before.		*/

{
 extern unsigned int GnaqListSize;	/* Current size of GNAQ list.	*/
 unsigned int NVectors;		/* Number of vectors to use.	*/
 extern void addrprint();	/* Prints address table.	*/
 AN_Id an_id;			/* Destination of possible stack incr.	*/
 boolean change_flag;		/* TRUE if change since previous branch. */
 extern void funcprint();	/* Prints a function.	*/
 extern boolean hflag;		/* TRUE to disable peephole entirely */
 unsigned long int lastlive[NVECTORS]; /* Live data for last node. */
 unsigned long int live[NVECTORS]; /* Variables that are live after an instr. */
 boolean new_change;		/* TRUE if change on last optimizer call. */
 extern TN_Id pbef;		/* Lower window limit for prbefore(). */
 extern TN_Id paft;		/* Upper limit for prbefore(). */
 extern void prafter();		/* Prints window after a change.	*/
 boolean same_flag;		/* TRUE if new and existing live are same. */
 unsigned long int set[NVECTORS]; /* Variables set by an instruction. */
 extern void sets();		/* Returns sets bits for an instruction. */
 extern TN_Id skipprof();	/* Skips profiling code.	*/
 TN_Id termination;		/* Don't optimize this node.	*/
 extern void textaudit();	/* Audits text nodes.	*/
 register TN_Id tn_after;	/* Id of text node after window. */
 TN_Id tn_before;		/* Id of textnode before window. */
 register TN_Id tn_first;	/* Id of first node of window. */
 TN_Id tn_furthest;		/* Id of furthest back that tn_before pointed.*/
 register TN_Id tn_last;	/* Id of last node of window. */
 unsigned long int used[NVECTORS]; /* Variables used by an instruction. */
 extern void uses();		/* Returns uses bits for an instruction. */
 int vector;			/* Index for scanning words of vectors. */
 int window_size;		/* Number of instructions in window. */
 extern boolean w1opt();	/* One instruction peephole optimizer. */
 extern boolean w2opt();	/* Two instruction peephole optimizer. */
 extern boolean w3opt();	/* Three instruction peephole optimizer. */

 if((Xskip(XPEEP_1)) && (peeppass == 1))
        return;
 else if((Xskip(XPEEP_2)) && (peeppass == 2))
        return;
 else if((Xskip(XPEEP_3)) && (peeppass == 3))
        return;
 if(hflag)			/* Suppress peephole? */
	return;

 NVectors = (GnaqListSize > (NVECTORS * sizeof(unsigned int) * B_P_BYTE)) ?
	(NVECTORS * sizeof(unsigned int) * B_P_BYTE) : GnaqListSize;
 NVectors = (NVectors + (sizeof(unsigned int) * B_P_BYTE) - 1) /
	(sizeof(unsigned int) * B_P_BYTE);

 if((DBdebug(3,XPEEP_1)) && (peeppass == 1))
	addrprint("peep: pass 1");
 else if((DBdebug(3,XPEEP_2)) && (peeppass == 2))
	addrprint("peep: pass 2");
 else if((DBdebug(3,XPEEP_3)) && (peeppass == 3))
	addrprint("peep: pass 3");

 if((DBdebug(1,XPEEP_1)) && (peeppass == 1))
	funcprint(stdout, "live/dead data before 1st peephole pass",2);
 else if((DBdebug(1,XPEEP_2)) && (peeppass == 2))
	funcprint(stdout, "live/dead data before 2nd peephole pass",2);
 else if((DBdebug(1,XPEEP_3)) && (peeppass == 3))
	funcprint(stdout, "live/dead data before 3rd peephole pass",2);


					/* Initialize window pointers. */
 tn_after = NULL;
 tn_last = GetTxPrevNode(tn_after);
 tn_first = tn_last;
 tn_before = GetTxPrevNode(tn_first);
					/* Initialize pointer to backward */
					/* progress of tn_before. */
 tn_furthest = tn_before;
					/* Keep a flag to remember whether a */
					/* change has been made since the */
					/* last branch.  */
 change_flag = FALSE;		
	 				/* Skip profiling and stack increment.*/
 termination = skipprof(GetTxNextNode((TN_Id) NULL)); 
 an_id = GetTxOperandAd(termination,DESTINATION);
 if((GetTxOpCodeX(termination) == G_ADD3) &&
		IsAdCPUReg(an_id) && (GetAdRegA(an_id) == CSP))
	termination = GetTxNextNode(termination);
 termination = GetTxPrevNode(termination);
					/* Scan backwards through the func. */
 while(tn_first != termination)
	{				/* Check whether we are in a */
					/* basic block. */
	 if(non_window(tn_first))
		{			/* Set pointers for one-instruction */
					/* window before tn_first. */
		 tn_after = tn_first;
		 tn_last = tn_before;
		 tn_first = tn_last;
		 tn_before = GetTxPrevNode(tn_first);
					/* Record furthest progress */
					/* of tn_before.*/
		 tn_furthest = tn_before;
		 			/* Don't propagate passed end of */
					/* block. */
		 change_flag = FALSE;
					/* Do one-instruction window. */
		 continue;
		}
					/* Loop through window sizes. */
	 for(window_size = 1;;window_size++)
		{
					/* Get lastlive. */
		 GetTxLive(tn_last,lastlive,NVectors);
					/* Initialize a flag to record */
					/* whether a change has been */
					/* made in the current window. */
		 new_change = FALSE;

					/* Do the optimizations. */
		 pbef = tn_before;	/* Set limits for prbefore, which */
		 paft = tn_after;	/* is called by w?opt(). */
		 switch(window_size)
			{case 1:
				new_change = w1opt(tn_first,peeppass);
				endcase;
			 case 2:
				new_change = w2opt(tn_first,tn_last,peeppass);
				endcase;
			 case 3:
				new_change = w3opt(tn_first,tn_last,peeppass);
				endcase;
			}
		 if(new_change)		/* Instruction(s) either removed */
					/* or modified. */
			{
			 if(((peeppass == 1) && DBdebug(0,XPEEP_1)) ||
					(peeppass == 2 && DBdebug(0,XPEEP_2))||
					(peeppass == 3 && DBdebug(0,XPEEP_3)))
				prafter(tn_before,tn_after,0);
			 if(DBdebug(4,XPEEP))
				textaudit("after peephole change");

					/* Reset pointers to one-instr. */
					/* window just below tn_after. */
					/* Lastlive stays the same. */
			 tn_last = GetTxPrevNode(tn_after);
			 tn_first = tn_last;
			 tn_before = GetTxPrevNode(tn_first);
					/* Set the change flag so the */
					/* live info will get propagated.*/
			 change_flag = TRUE;
					/* Mark progress of tn_before.*/
			 if(tn_furthest == tn_first)
				tn_furthest = tn_before;
					/* Put lastlive into tn_last node in */
					/* case previous tn_last was deleted */
					/* or moved. */
			 if(!non_window(tn_last))
			 	PutTxLive(tn_last,lastlive,NVectors);
					/* Go back to one instr. window. */
			 break;
			}
					/* If no larger window or can't */
					/* expand window, continue with next */
					/* one-instruction window. */
		 if(window_size == MAX_WINDOW || non_window(tn_after))
			{		/* Set pointer for one instruction */
					/* window preceeding "first". */
			 tn_after = tn_first;
			 tn_last = tn_before;
			 tn_first = tn_last;
			 tn_before = GetTxPrevNode(tn_first);
					/* Mark furthest progress of tn_after.*/
			 if(tn_furthest == tn_first)
				tn_furthest = tn_before;
					/* If no changes to propagate or if  */
					/* not in a window, go do next window.*/
			 if(!change_flag || non_window(tn_last))
				break;
					/* Compute new lastlive. */
			 GetTxLive(tn_after,lastlive,NVectors);
			 sets(tn_after,set,NVectors);
			 uses(tn_after,used,NVectors);
			 for(vector = 0; vector < NVectors; vector++)
				{lastlive[vector] &= ~set[vector];
				 lastlive[vector] |= used[vector];
				}
				/* Check whether are moving backward.*/
			 if(tn_furthest == tn_before)
				{	
					/* If the new lastlive is the same as */
					/* the existing live on tn_last, then */
					/* nothing is changing and we can */
					/* stop propagating. */
				 GetTxLive(tn_last,live,NVectors);
				 same_flag = TRUE;
				 for(vector = 0; vector < NVectors; vector++)
					if(lastlive[vector] != live[vector])
						same_flag = FALSE;
				 if(same_flag)
					{change_flag = FALSE;
					 break;
					}
				}
					/* Update live data. */
		 	 PutTxLive(tn_last,lastlive,NVectors);
					/* Go do one-instruction window. */
			 break;
			}
					/* Expand the window. */
		 tn_last = tn_after;
		 tn_after = GetTxNextNode(tn_last);
		 GetTxLive(tn_last,lastlive,NVectors);
		} /* END OF for(window_size = 1; ... */
	} /* END OF while(tn_first != termination) */

 if((DBdebug(2,XPEEP_1)) && (peeppass == 1))
	funcprint(stdout, "live/dead data after 1st peephole pass",2);
 else if((DBdebug(2,XPEEP_2)) && (peeppass == 2))
	funcprint(stdout, "live/dead data after 2nd peephole pass",2);
 else if((DBdebug(2,XPEEP_3)) && (peeppass == 3))
	funcprint(stdout, "live/dead data after 3rd peephole pass",2);

 return;
}
