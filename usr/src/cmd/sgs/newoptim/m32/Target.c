/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/Target.c	1.7"


#include	<stdio.h>
#include	<string.h>
#include	"defs.h"
#include	"OpTabTypes.h"
#include	"Target.h"

m32_target_cpu	cpu_chip	= (m32_target_cpu)CPU_CHIP;
m32_target_math	math_chip	= (m32_target_math)MATH_CHIP;

/*
 *	Target() is called to choose the target configurations, which
 *	controls, internally, argument type checking, instruction type checking,
 *	SWA generation,  etc.
 *	The intent here is to allow code to choose at run time what kind of silicon
 *	(i.e. 32A, 32B, BFO, MAU, etc.) is in the target by testing the global 
 *	variables set by this function, rather than choosing via the build-time 
 *	option, so that we can build a common binary when possible.
 *
 *	If Target() is not called, the set of defaults chosen at build time
 *	is used.  The defaults are chosen via the *preprocessor* symbols
 *	defined by the "make" variable TMAC.  
 *	If no defaults are chosen (because the "make" symbol TMAC was not 
 *	defined), then the default is ABWRMAC.
 */

	void
Target(targ)
char *targ;			/* Name of desired target.	*/
{extern void fatal();		/* Handles fatal errors.	*/
 boolean found;
 int	i;
 static struct {		/* Table which equates values of 	*/
	char		*s;	/* to run-time values for the global	*/
	m32_target_cpu	c;	/* 'target' variables.			*/
	m32_target_math	m;
 } table[] = {
		{ "ABWRMAC",	we32001,	we32fpe },
		{ "ABWORMAC",	we32001,	we32fpe },
		{ "BMAC",	we32100,	we32fpe },
		{ "BMAUMAC",	we32100,	we32106 },
		{ "BMFOMAC",	we32100,	we32206 },
		{ "BFO",	we32200,	we32fpe },
		{ "BFOMFO",	we32200,	we32206 },
		{ "CRISP",	weCRISP,	we32fpe },
		{ "CRISPMAU",	weCRISP,	we32106 },
		{ "CRISPMFO",	weCRISP,	we32206 },
		{ NULL,		we32001,	we32fpe } };	/* last! */


 if (targ != (char *)NULL && *targ != EOS){
	/* look for appropriate settings */
	found = FALSE;
 	for (i=0; table[i].s != NULL; i++)	
		if ( strcmp(table[i].s, targ) == 0 ) {
			cpu_chip = table[i].c;
			math_chip = table[i].m;
			found = TRUE;
			break;
		}
	if( !found )
		fatal("Target: target unknown: %s.\n",targ);
	}
 return;
}

/*
 * SetTargetMath() sets the floating-point modes (i.e. FPE or MAU).
 *  When MAU doesn't make sense or can mean 32106 or 32206, a best guess is made.
 *  The only way to precisely set the target configuration is by Target().
 */
	void
SetTargetMath( type )
int type;
{

	switch(type){
	case 'c':	/* compatibility mode */
		cpu_chip = we32001;
		math_chip = we32fpe;
		endcase;
	case 'f':
		math_chip = we32fpe;
		endcase;
	case 'm':
		if(math_chip == we32106 || math_chip == we32206)
			endcase;
		if(cpu_chip == we32100 || cpu_chip == weCRISP){
			/* assume that 32106 is what's wanted 
			 * and not 32206. 
			 */
			math_chip = we32106;
		}
		else if (cpu_chip == we32200)
			math_chip = we32206;
		else{	/* A chips => BMAUMAC */
			cpu_chip = we32100;
			math_chip = we32106;
		}
		endcase;
	}
}
