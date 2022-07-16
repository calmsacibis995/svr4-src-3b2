/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/defs.h	1.6"
enum boolean_E {FALSE=0,TRUE=1};
typedef enum boolean_E boolean;

/* lex items */
#define	COMMA		','
#define	DOT		'.'
#define	EOS		'\0'
#define	NEWLINE		'\n'
#define	SPACE		' '
#define	TAB		'\t'
#define ComChar		'#'
#define AtChar		'@'
#define	FindWhite(p)	while(!isspace(*(p)) && *(p) != EOS) (p)++;
#define	SkipWhite(p)	while(isspace(*(p))) (p)++;
#define _E_O_I		01
#define _E_O_L		02
#define _E_O_Lbl	04
#define eoi(c)		(lextab[(c)] & _E_O_I)
#define eol(c)		(lextab[(c)] & _E_O_L)
#define eolbl(c)	(lextab[(c)] & _E_O_Lbl)

#define	endcase	break

#define EBUFSIZE	127
#define	MAXFILENAME	128
#define	MAX_LSLINE	255
#define MAXLONGDIG	10	/* max decimal digits for a long */

#ifdef PROFILE
#define	STATIC			/* NULL for profiling.	*/
#else
#define	STATIC	static		/* Can be NULL if profiling. */
#endif	/*PROFILE*/

#ifdef	MALLOC
char *Malloc();
void Free();
#else
#define Malloc	malloc
#define Free	free
#endif	/*MALLOC*/

#define	B_P_BYTE	8

#define	clr_bit(array,bit)	(array[(bit) / (sizeof(unsigned long int) * \
					B_P_BYTE)] &= \
				~(1 << ((bit) % (sizeof(unsigned long int) * \
					B_P_BYTE))))
#define	get_bit(array,bit)	(array[(bit) / (sizeof(unsigned long int) * \
					B_P_BYTE)] & \
				(1 << ((bit) % (sizeof(unsigned long int) * \
					B_P_BYTE))))
#define	set_bit(array,bit)	(array[(bit) / (sizeof(unsigned long int) * \
					B_P_BYTE)] |= \
				(1 << ((bit) % (sizeof(unsigned long int) * \
					B_P_BYTE))))

#define Min(a,b)	((a) > (b) ? (b) : (a))

enum CC_Mode {Transition, Ansi, Conform};
int PIC_flag;
