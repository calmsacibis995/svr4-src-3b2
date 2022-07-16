/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/debug.h	1.5"

					/* Mask definition for x flag.	 */

#define	XINCONV		0x00000001	/* Input conversion.	*/
#define	XOUTCONV	0x00000002	/* Output conversion.	*/
#define XTB_LD		0x00000004	/* live/dead table */
#define XTB_AD		0x00000008	/* address table */
#define XIL		0x00000010	/* in-line exp */
#define XIL_PUSH	0x00000020	/* in-line exp, pushes to moves */
#define XIL_INSERT	0x00000040	/* in-line exp, code insertion */
#define XIL_FP		0x00000080	/* in-line exp, fp arg and ret conv */
#define	XCSE		0x00000100	/* common subexpression elimination */
#define	XV_USED		0x00000200	/* value tracing */
#define	XV_SET		0x00000400	/* value tracing */
#define XVTRACE		(XV_USED|XV_SET)
#define	XLICM_1		0x00000800 	/* loop invariant code motion */
#define	XLICM_2		0x00001000 	/* loop invariant code motion */
#define	XLICM_3		0x00002000 	/* loop invariant code motion */
#define	XLICM_4		0x00004000 	/* loop invariant code motion */
#define	XLICM_5		0x00008000 	/* loop invariant code motion */
#define XLICM		(XLICM_1|XLICM_2|XLICM_3|XLICM_4|XLICM_5)
#define	XPCI_RL		0x00010000	/* PCI: remove labels */
#define	XPCI_BGR	0x00020000	/* PCI: Build Graph */
#define	XPCI_MBR	0x00040000	/* PCI: Merge Branches */
#define	XPCI_RUNR	0x00080000	/* PCI: Remove Unreachable Code */
#define	XPCI_COMT	0x00100000	/* PCI: Common Tail */
#define	XPCI_REORD	0x00200000	/* PCI: Reorder Blocks */
#define	XPCI_RMBR	0x00400000	/* PCI: Remove Branches */
#define	XPCI_LDA	0x00800000	/* PCI: Live Dead Analysis */
#define XPCI		(XPCI_RL|XPCI_BGR|XPCI_MBR|XPCI_RUNR|XPCI_COMT|XPCI_REORD|XPCI_RMBR|XPCI_LDA)
#define XPEEP_1		0x01000000	/* 1st pass peephole */
#define XPEEP_2		0x02000000	/* 2nd pass peephole */
#define XPEEP_3		0x04000000	/* 3rd pass peephole */
#define XPEEP		(XPEEP_1|XPEEP_2|XPEEP_3)
#define XGRA		0x10000000	/* global register allocation */

					/* for the various peephole optims */
#define PEEP100		0x00000001
#define PEEP101		0x00000002
#define PEEP102		0x00000004
#define PEEP103		0x00000008
#define PEEP104		0x00000010
#define PEEP107		0x00000020
#define PEEP109		0x00000040
#define PEEP111		0x00000080
#define PEEP200		0x00000100
#define PEEP202		0x00000200
#define PEEP203		0x00000400
#define PEEP204		0x00000800
#define PEEP207		0x00001000
#define PEEP209		0x00002000
#define PEEP210		0x00004000
#define PEEP220		0x00008000
#define PEEP221		0x00010000
#define PEEP222		0x00020000
#define PEEP227		0x00040000
#define PEEP228		0x00080000
#define PEEP241		0x00100000
#define PEEP242		0x00200000
#define PEEP244		0x00400000
#define PEEP245		0x00800000
#define PEEP246		0x01000000
#define PEEP300		0x10000000

#ifdef DEBUG
#define	DBdebug(level,mask)	DBDebug(level,(unsigned long int)mask)
#define	Xskip(mask)		XSkip((unsigned long int)mask)
#define Pskip(pmask)		PSkip((unsigned long int)pmask)
extern boolean DBDebug();
extern boolean XSkip();
extern boolean PSkip();
#else	/*DEBUG*/
#define	DBdebug(level,mask)	FALSE
#define	Xskip(mask)		FALSE
#define Pskip(pmask)		FALSE
#endif /*DEBUG*/
