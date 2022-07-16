/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/gendefs.h	1.4"
/*
 */
extern void Generate();
extern void yyerror();
extern void aerror();
extern void werror();
extern void exit();
extern int unlink();
extern int execv();
extern int wait();
extern int fork();


#define  LESS    -1
#define  EQUAL    0
#define  GREATER  1

#define  NO       0
#define  YES      1

#define NCPS	8	/* number of characters per symbol */
#define BITSPBY	8
#define BITSPOW	8
#define BITSPW	32
#define MIN16	(-32768L)
#define MAX16	32767L
#define MIN32	((long)0x80000000L)
#define MAX32	((long)0x7fffffffL)


#define OUTWTYPE	char
/* if the previous line changes the following line should be change to look like
 * #define	OUT(a,b)	(void) fwrite((char *)(&a), sizeof(OUTWTYPE), 1, b)
*/
#define	OUT(a,b)	(void) putc((a), b)
#define	MAX(a,b)	(a > b) ? a : b
#define GET_E_FLAGS 	(need_mau) ? EF_M32_MAU : 0

#define NBPW	32

#define SCTALIGN 4L /* byte alignment for sections */
#define TXTFILL	0x70L
#define FILL 0L
#define NULLVAL 0L
#define NULLSYM ((symbol *)NULL)

/* constants used in testing and flag parsing */

#define ARGERR	"Illegal argument - ignored\n"
#define TESTVAL	-2
#define NSECTIONS 20
#define NFILES	3	/* number of section files + other files */

/* minimum and maximum number of arguments allowed in is25 call instruction */
#define MINARG 0
#define MAXARG 65535

/* index of action routines in modes array */

typedef	enum
{
	NOACTION,	/* 0 */
	DEFINE,
	SETVAL,
	SETSCL,
	RELPC32,
	RELGOT32,	/* 5 */
	RELPLT32,
	ENDEF,
	_ACTION_14,	
	RELDIR32,
	RESABS,		/* 10 */
	RELPC8,	
	RELPC16,
	CBRNOPT,	
	BSBNOPT,
	FNDBRLEN,	/* 15 */
	SHIFTVAL,
	RELDAT32,
	SAVTRANS,
	DOTZERO,
	SWAP_B2,	/* 20 */
	SWAP_B4,
	UBRNOPT,
#ifdef	CALLPCREL
	CALLNOPT,
#endif
	NACTION
} ACTION;
