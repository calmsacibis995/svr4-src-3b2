/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/GlobalDefs.c	1.6"

#include	<stdio.h>
#include	"defs.h"
#include	"OpTabTypes.h"
#include	"OperndType.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ALNodeType.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"FNodeTypes.h"
#include	"optim.h"


AN_Id A;			/* AN_Id of A condition code.	*/
AN_Id C;			/* AN_Id of C condition code.	*/
AN_Id N;			/* AN_Id of N condition code.	*/
AN_Id V;			/* AN_Id of V condition code.	*/
AN_Id X;			/* AN_Id of X condition code.	*/
AN_Id Z;			/* AN_Id of Z condition code.	*/
AN_Id i0;			/* AN_Id of address of immediate zero.	*/
AN_Id i1;			/* AN_Id of immediate one.	*/
NODE n0;			/* Node before the first real one.	*/
NODE ntail;			/* Node after the last real one.	*/
REF *lastref;			/* Pointer to last reference.	*/
FN_Id FuncId = {(FN_Id) NULL};	/* FN_Id of most recent function. */
boolean cflag = {TRUE};		/* Do CONSERVATIVE common tail merging.	*/
boolean hflag = {FALSE};	/* Peephole disable if TRUE.	*/
boolean misinp = {FALSE};	/* TRUE if MIS instructions detected in
				   the input file. */
TN_Id paft;
TN_Id pbef;
boolean sflag = {FALSE};	/* Want statistics on stderr if TRUE.	*/
boolean swflag = FALSE;		/* switch table appears in function */
char lextab[128];		/* Lex class table for scanning. */
int REGS[NVECTORS];
int RETREG[NVECTORS];
unsigned int ld_maxbit = 0;	/* # of GNAQs for live-dead analysis.	*/
unsigned int ld_maxword = 0;	/* # of words to fit ld_maxbit.	*/
int intpc = {-3};		/* % limit on inline expansion.	*/
int optmode = {ODEFAULT};	/* Optimization mode.	*/
unsigned int Avg_Expression;	/* Average expression length.	*/
unsigned int GnaqListSize;	/* Current size of GNAQ list.	*/
unsigned int Max_Ops;		/* Maximum operands in an instruction.	*/
unsigned int Nom_Instrs;	/* Initial instruction allocation. */
				/* STATISTICS COUNTERS.	*/
unsigned int LBLSrem = {0};	/* Number of labels removed.	*/
unsigned int LICMrem = {0};	/* Number of instructions moved outside loops.*/
unsigned int MaxGNAQNodes = {0};	/* Maximum size of GNAQ list.	*/
unsigned int PEEP1chg = {0};	/* Number of instructions changed by peep(1). */
unsigned int PEEP2chg = {0};	/* Number of instructions changed by peep(2). */
unsigned int PEEP3chg = {0};	/* Number of instructions changed by peep(3). */
unsigned int ndisc = {0};	/* Number of instructions discarded.	*/
unsigned int ninst = {0};	/* Number of instructions.	*/
int nmerge = 0;			/* redundant instructions */
int nrot = 0;			/* rotated loops. */
int nsave = 0;			/* branches saved */
int nunr = 0;			/* unreachable instructions */

enum CC_Mode ccmode = Transition;		/* compilation mode to detect ANSI volatiles */
boolean identflag = {FALSE};	/* Ident output flag.	*/
boolean Zflag = {FALSE};	/* TRUE if externs are nonvolatile. */


