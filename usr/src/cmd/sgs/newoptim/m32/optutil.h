/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/optutil.h	1.4"

/* optutil.h */

/* peephole and function optimization utilities (optutil.c) */

extern int getbit();		/* get exponent if value is power of 2 */
extern void updlive();		/* update live/dead information */
extern boolean setslivecc();	/* does it set live condition codes */
extern boolean notaffected();	/* is one address notaffected by another */
extern boolean legalgen();	/* checks for generic being real instruction */
extern TN_Id skipprof();	/* skip label, SAVE, and profiling code */
extern boolean chktyp();	/* checks types for merging preceding move */
extern boolean IsDeadAd();	/* is address dead following text node */
extern boolean IsOpUses();	/* is address used by opcode */
extern AN_Id GetP();		/* get addresss node of destination */
extern void PutP();		/* put address node in destination */

