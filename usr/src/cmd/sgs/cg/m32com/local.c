/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nifg:cg/mau/local.c	1.57"
/*	local.c - machine dependent stuff for front end
 *
 *		WE 32000
 *
 */

#include <signal.h> 
#include <memory.h>
#include <string.h>
#include "paths.h"		/* to get TMPDIR, directory for temp files */
#include "mfile1.h"
#include "mfile2.h"

#ifdef MAU
/* register numbers in the compiler and their external numbers:
**
**	comp	name
**	0-2	r0-r2
**	3-4	f0,f3
**	5-10	r3-8
**	11-12	f1-2
*/

#define	MAU_F1	11		/* register number for %f1 */
#define	MAU_F2	12		/* register number for %f2 */

/* bit vectors for register variables */

#define	REGREGS	(RS_BIT(5)|RS_BIT(6)|RS_BIT(7)|RS_BIT(8)|RS_BIT(9)|RS_BIT(10))
#define	MAUREGS	(RS_BIT(MAU_F1)|RS_BIT(MAU_F2))

#endif	/* ifdef MAU */

#ifndef	CPUREGHI
#define	CPUREGHI	TOTREGS	/* all registers are CPU registers */
#endif
extern int unlink();
static NODE *rewexc();
static int select();
static void jmplab(), makeheap(), walkheap();
#if 0
static void myexit();
#endif

/* int dsflag = 0;			/* symbol info initially enabled */
/* static int dlflag = 0;	/* line number info initially enabled */
int tmp_start = 0;		/*start of temp locations on stack*/
static char request[TOTREGS];	/*list of reg vars used by the fct.  Given on
				  either BEGF node or ENDF node */
static int biggest_returned;

#ifdef MAU
int r_caller[]={MAU_F1,MAU_F2,-1};
#else
int r_caller[]={-1};		/*list of caller save regs	*/
#endif	/* ifdef MAU */

#ifdef IMPREGAL
extern int sizeopt;		/* weight optimization info for size if set */
#endif

extern char *rnames[];		/* register names 	*/

/* location counters for PROG, ADATA, DATA, ISTRNG, STRNG, CDATA and CSTRNG */
static char *locnames[] =
{
#ifdef ELF_OBJ
/*PROG*/	"	.text\n",
/*ADATA*/	"	.data\n",
/*DATA*/	"	.data\n",
/*ISTRNG*/	"	.section	.data1\n",
/*STRNG*/	"	.section	.data1\n",
/*CDATA*/	"	.section	.rodata\n",	/* read-only data */
/*CSTRNG*/	"	.section	.rodata1\n"	/* read-only strings */

#else
/*PROG*/	"	.text\n",
/*ADATA*/	"	.section	.data1,\"w\"\n",
/*DATA*/	"	.data\n",
/*ISTRNG*/	"	.data	1\n",
/*STRNG*/	"	.data	1\n",
#	ifdef RODATA
/*CDATA*/	"	.section	.rodata,\"x\"\n",   /* read-only data */
/*CSTRNG*/	"	.section	.rodata,\"x\"\n"    /* read-only strings */
#	else
/*CDATA*/	"	.data\n",
/*CSTRNG*/	"	.data	1\n"
#	endif

#endif
};

#ifdef ELF_OBJ
# define FIRST_ISTR	"\t.section	.data1,\"aw\"\n"
# define FIRST_CSTR	"\t.section	.rodata1,\"a\"\n"
#endif

static char tmpfn[100];
static FILE *tmpfp;
extern char *tempnam();

/* for stack overflow exception handling: the margin of safety in
** estimating how much stack is left to use
*/
#define	STACKSLIP	52

FILE *fopen();
int proflag = 0;
int picflag = 0;

#if 0
static void
myexit(n)
{
	(void)unlink(tmpfn);
	if (n == 1)
		n = 51;
	exit(n);
}

void
getout()
{
	myexit(55);
}

/*extern int singflag;	/* flag for turning on single precision floating arith */

static void
catch_fpe()
{
    uerror("floating point constant folding causes exception");
    myexit( 1 );
}


void
beg_file()
{
	/* called as the very first thing by the parser to do machine
	 * dependent stuff
	 */
	register char *p, *s;
	char *tempnam();
 

	/* catch signals if they're not now being ignored */

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	    signal(SIGHUP, getout);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	    signal(SIGINT, getout);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	    signal(SIGQUIT, getout);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	    signal(SIGTERM, getout);
        /* catch floating error */

        if (signal(SIGFPE, SIG_IGN) != SIG_IGN)
            signal(SIGFPE, catch_fpe);

			/* note: double quotes already in ftitle... */
	p = ftitle + strlen( ftitle ) - 2;
	s = p - 14;	/* max name length */
	while ( p > s && *p != '"' && *p != '/' )
		--p;
	fprintf(outfile, "\t.file\t\"%.15s\n", p + 1 );
	(void)strcpy(tmpfn, tempnam( TMPDIR, "pcc2S" ));
	if(!(tmpfp= fopen(tmpfn, "w")))
		cerror("can't create string file\n");
}
#endif  /* 0 */

void
myflags(cp)
char *cp;
{
	while (*cp) {
		switch (*cp)
		{
		case 'k':	picflag = 1; break;
		default:	break;
		}
		++cp;
	}
}

void 
p2abort()
{
	if (tmpfp)
		(void)unlink(tmpfn);

	return;
}

static int lastalign;

locctr(l)		/* output the location counter */
{
#ifdef  ELF_OBJ
	static int first_istring = 1;
	static int first_cstring = 1;
#endif
	static int lastloc = UNK;
	int retval = lastloc;		/* value to return at end */
#if defined(RODATA) && !defined(ELF_OBJ)
	static int lasttmploc = UNK;
#endif
	curloc = l;
	if (curloc != lastloc) lastalign = -1;

	switch (l)
	{
	case CURRENT:
		return ( retval );
	case PROG:
		lastalign = -1;
		/* FALLTHRU */
	case ADATA:
	case CDATA:
	case DATA:
		if (lastloc == l)
			break;
		outfile = textfile;
		if (picflag) {
		    if (curloc == CDATA) {
			curloc = DATA;
			emit_str(locnames[DATA]);
		    }
		    else
			emit_str(locnames[l]);
		}
		else
		    emit_str(locnames[l]);
		break;

	case FORCE_LC(CDATA):
		if (lastloc == l)
			break;
		outfile = textfile;
		emit_str(locnames[CDATA]);
		break;

	case STRNG:
	case ISTRNG:
	case CSTRNG:
		/* output string initializers to a temporary file for now
		 * don't update lastloc
		 */
		if (lastloc == l)
			break;
#ifdef  ELF_OBJ
		outfile = textfile;
		if (curloc == CSTRNG && first_cstring)
		{
			emit_str(FIRST_CSTR);
			first_cstring = 0;
		}
		else if ( ((curloc == ISTRNG)||(curloc == STRNG)) && first_istring )
		{
			emit_str(FIRST_ISTR);
			first_istring = 0;
		}
		else
			emit_str(locnames[l]);
#else
		if (! tmpfp)
		{
			(void)strcpy(tmpfn, tempnam( TMPDIR, "pcc2S" ));
			if(!(tmpfp= fopen(tmpfn, "w")))
				cerror("can't create string file\n");
		}
		outfile = tmpfp;
#	ifdef RODATA
		if (lasttmploc == curloc)
			break;
		if (curloc == CSTRNG)
			fputs("	.section	.rodata,\"x\"\n",outfile);
		else
			fputs("	.data\n",outfile);
		lasttmploc = curloc;
#	endif
#endif
		break;

	case UNK:
		break;

	default:
		cerror( "illegal location counter");
	}

	lastloc = l;
	return( retval );		/* declare previous loc. ctr. */
}

NODE *
clocal(p)
NODE *p;
{
	register NODE *l, *r;


	if  (( p->in.strat & EXHONOR )
		&&(	(p->in.op == CALL)
			||(p->in.op == STCALL )
		)
				&&(((unsigned)p->stn.argsize/SZCHAR) > (STACKSLIP-36))
	)
	{
		return ( rewexc(p) );
	}
	if (!asgbinop(p->in.op) && p->in.op != ASSIGN)
		return (p);
	r = p->in.right;
	if (optype(r->in.op) == LTYPE)
		return (p);
	l = r->in.left;
	if (r->in.op == QUEST ||
		(r->in.op == CONV && l->in.op == QUEST) ||
		(r->in.op == CONV && l->in.op == CONV &&
		l->in.left->in.op == QUEST))
				/* distribute assigns over colons */
	{
		register NODE *pwork;
		register TWORD tp;
		NODE *tcopy();
		NODE *pcpy = tcopy(p), *pnew;
#ifndef NODBG
		extern int xdebug; 
		extern void e2print();

		if (xdebug)
		{
			fputs("Entering [op]=?: distribution\n",outfile);
			e2print(p);
		}
#endif
		tp = p->in.type;		/* save type of assign op */
		pnew = pcpy->in.right;
		while (pnew->in.op != QUEST)
			pnew = pnew->in.left;
		/*
		* pnew is top of new tree
		*/
		if ((pwork = p)->in.right->in.op == QUEST)
		{
			tfree(pwork->in.right);
			pwork->in.right = pnew->in.right->in.left;
			pnew->in.right->in.left = pwork;
			/* at this point, 1/2 distributed. Tree looks like:
			*		ASSIGN|ASGOP
			*	LVAL			QUEST
			*		EXPR1		COLON
			*			ASSIGN|ASGOP	EXPR3
			*		LVAL		EXPR2
			* pnew "holds" new tree from QUEST node
			*/
		}
		else
		{
			NODE *pholdtop = pwork;

			pwork = pwork->in.right;
			while (pwork->in.left->in.op != QUEST)
				pwork = pwork->in.left;
			tfree(pwork->in.left);
			pwork->in.left = pnew->in.right->in.left;
			pnew->in.right->in.left = pholdtop;
			/* at this point, 1/2 distributed. Tree looks like:
			*		ASSIGN|ASGOP
			*	LVAL			ANY # OF CONVs
			*			QUEST
			*		EXPR1		COLON
			*			ASSIGN|ASGOP	EXPR3
			*		LVAL		ANY # OF CONVs
			*			EXPR2
			* pnew "holds" new tree from QUEST node
			*/
		}
		if ((pwork = pcpy)->in.right->in.op == QUEST)
		{
			pwork->in.right = pnew->in.right->in.right;
			pnew->in.right->in.right = pwork;
			/*
			* done with the easy case
			*/
		}
		else
		{
			NODE *pholdtop = pwork;

			pwork = pwork->in.right;
			while (pwork->in.left->in.op != QUEST)
				pwork = pwork->in.left;
			pwork->in.left = pnew->in.right->in.right;
			pnew->in.right->in.right = pholdtop;
			/*
			* done with the CONVs case
			*/
		}
		p = pnew;
		p->in.type = tp;	/* type of Qnode is type of assignment */
#ifndef NODBG
		if (xdebug)
		{
			fputs("Leaving [op]=?: distribution\n",outfile);
			e2print(p);
		}
#endif
	}
	return(p);
}

static NODE *
rewexc(p)	/* rewrite stack overflow exceptable node */
NODE *p;
{
	register NODE *semiptr; /* pointer to ,OP node	*/
	register NODE *uop1ptr; /* pointer to UOP1 node	*/
	register NODE *iconptr; /* pointer to ICON node	*/

/* rewrite a tree rooted at an exceptable node which may cause a
** stack overflow exception. Cause the code produced to perform
** a run-time check of the expected new %sp against the stack
** bound ( "_stk_lim" ). If the bound will be exceeded, cause a runtime
** jump to (* _stk_ov)(), the exception handler. It is the 
** responsibility of the language (i.e. ADA) implementor to
** insure that _stk_lim and _stk_ov are defined and correct
** at runtime.
*/
#ifndef NODBG
	if( odebug>1 )
	{
		fprintf(outfile,"\nrewexc: initial tree was\n");
		e2print( p );
	}
#endif /* NODBG */
	semiptr= talloc();
	semiptr->in.op=SEMI;
	semiptr->in.type=p->in.type;
	semiptr->in.strat= ( LTOR | PAREN );
	semiptr->in.right=p;
	semiptr->in.left=(uop1ptr=talloc());

	uop1ptr->in.op=UOP1;
	uop1ptr->in.type=TVOID;
	uop1ptr->in.left=( iconptr=talloc());

	iconptr->tn.op=ICON;
	iconptr->tn.type=TULONG;
	iconptr->tn.lval= p->stn.argsize;
#ifndef NODBG
	if( odebug>1 )
	{
		fprintf(outfile,"\nrewexc: returned tree is\n");
		e2print( semiptr );
	}
#endif /* NODBG */
	return(semiptr);
}


/* Can object of type t go in a register?  rbusy is array
** of registers in use.  Return -1 if not in register,
** else return register number.  Also fill in rbusy[].
** op describes "address space" for register:  VAUTO, VPARAM, etc.
*/
/*ARGSUSED*/
cisreg(op, t, off, rbusy)		/* can type t go in reg? */
int op;					/* not used */
TWORD t;
OFFSET off;				/* offset, not used */
char  *rbusy;
{
	int i;			/* current register number */

#ifdef MAU
	/*
	* Only allow register basic types or pointers.
	* Someday, maybe allow register small-struct
	*/
	if (t & (TCHAR| TSHORT| TINT| TLONG|
		TUCHAR| TUSHORT| TUNSIGNED| TULONG| TPOINT| TPOINT2) )
	{
		/* Have a type we can put in a cpu register */
		for (i = CPUREGHI-1; i >= NRGS; --i)
			if (!rbusy[i])
			{
				rbusy[ i ] = 1;
				return ( i );
			}
		return (-1);	/* no register to allocate */

	}
	else 
	if ( t & (TFLOAT| TDOUBLE) )
	{
		/* Have a floating point type we can put in a MAU register */

		for (i = TOTREGS-1; i >= CPUREGHI; --i )
			if (!rbusy[ i ]) 
			{
				rbusy[i] = 1;
				return ( i );
			}	
		return (-1);	/* no register to allocate */
	}

	/* Bad type */
	return (-1);
	

#else
	/*
	* Only allow register basic types or pointers.
	* Someday, maybe allow register small-struct
	*/
	if (t & (TCHAR|	TSHORT|	TINT| TLONG| TUCHAR| TUSHORT| 
		 TUNSIGNED| TULONG| TFLOAT| TDOUBLE| TPOINT| TPOINT2))
	{
		/* Have a type we can put in a register */
		for (i = TOTREGS-1; i >= NRGS; --i ) {
			if (!rbusy[i]) 
			{
			/* i is okay if not double, or if i-1 is free. */
				if (! (t & TDOUBLE))  break;
				if ( i > NRGS && !rbusy[i-1]) break;
			}
		}

		/* If i >= NRGS, i is the register number to
		** allocate (or i-1 is, if t is TDOUBLE).
		*/

		/* If candidate is suitable number grab it, adjust rbusy[]. */
		if (i >= NRGS) 
		{
			rbusy[i] = 1;
			if ( t & TDOUBLE)
				rbusy[--i] = 1;
			return (i);
		}
	}

	/* Bad type or no register to allocate. */
	return(-1);

#endif	/* ifdef MAU */
}


/* simple initialization code for integer types >= SZINT.  On the 3B2,3B5,3B20,
** this can only be an INT.  Do the obvious thing.
** We're handed an INIT node.
*/
void  
sincode(p)
NODE * p;
{
        int sz;
        if (p->in.op != INIT || ((p=p->in.left)->in.op != ICON && p->in.op != FCON))
            cerror("sincode:  got funny node\n");
        sz = gtsize ( p->in.type);
        if ( p->in.op == FCON)
        {
                fincode(p->fpn.dval, sz);
                return;
        }
                        /*Make sure we are aligned properly*/
        defalign(gtalign(p->in.type));

        switch ( sz )
	{
	case SZCHAR:
		fprintf(outfile,"\t.byte\t");
                break;
        case SZSHORT:
                fprintf(outfile,"\t.half\t");
                break;
        case SZINT:
		fprintf(outfile,"\t.word\t");
                break;
        case SZDOUBLE:
                fprintf(outfile,"\t.double\t");
                break;
        default:
                cerror("sincode: bad size for initializer");
        }
	if (picflag)
		p->in.strat &= ~(PIC_GOT|PIC_PLT|PIC_PC);
        acon(p);                            /* output constant appropriately */
        PUTCHAR('\n');
        inoff += sz;
        return;
}
 

#if 0  /* not be used */
void
incode( p, sz )
NODE *p;
{
	/* generate initialization code for assigning a constant c
	 * 	to a field of width sz
	 * we assume that the proper alignment has been obtained and sz < SZINT
	 * inoff is updated to have the proper final value
	 */
	if (inoff % SZINT == 0 )
		fprintf(outfile,"\t.word\t%d:%d", sz, p->tn.lval);
	else
		fprintf(outfile,",%d:%d", sz, p->tn.lval);
	inoff += sz;
	if (inoff % SZINT == 0)
		PUTCHAR('\n');
}
void 
vfdzero(n)		/* define n bits of zeros in a vfd */
int n;
{
	if( n <= 0 )
		return;
	if (inoff % SZINT == 0 )
		fprintf(outfile,"\t.word\t%d:0", n);
	else
		fprintf(outfile,",%d:0", n);
	inoff += n;
	if (inoff % SZINT == 0)
		PUTCHAR('\n');
}
#endif /* 0 */
void
fincode(d, sz)		/* floating initialization */
FP_DOUBLE d;
int sz;
{
#if defined(u3b) || defined(u3b5) || defined(u3b2) || (defined(uts) && defined(FP_EMULATE))
	union { FP_DOUBLE d; FP_FLOAT f; int i[2]; } cheat;

	if (sz == SZDOUBLE)
	{
		cheat.d = d;
		fprintf(outfile,"\t.word\t0x%x,0x%x\n", 
				(unsigned)cheat.i[0], (unsigned)cheat.i[1]);

	}
	else
	{
		cheat.f = FP_DTOF(d);
		fprintf(outfile,"\t.word\t0x%x\n", (unsigned)cheat.i[0]);
	}
#else
        fprintf(outfile,"\t.%s\t%.15e\n", sz == SZDOUBLE ? "double" : "float", d);
#endif
	inoff += sz;
}


char *
exname(p)			/* a name using external naming conventions */
char *p;
{
    return( p );
}
#if 0  /*not be used*/
static void
commdec(id)			/* generate a .comm from stab index id */
int id;
{
	cerror("commdec");
}

static
nalign(t)		/* figure alignment for type t */
TWORD t;
{
	int aln;

	if (ISPTR(t))
		aln = ALPOINT;
	else switch (BTYPE(t))
	{
	case CHAR:
	case UCHAR:
		aln = ALCHAR;
		break;
	case SHORT:
	case USHORT:
		aln = ALSHORT;
		break;
	case INT:
	case UNSIGNED:
	case ENUMTY:
		aln = ALINT;
		break;
	case LONG:
	case ULONG:
		aln = ALLONG;
		break;
	case FLOAT:
		aln = ALFLOAT;
		break;
	case DOUBLE:
		aln = ALDOUBLE;
		break;
	case STRTY:
	case UNIONTY:
		aln = ALSTRUCT;
		break;
	default:
		cerror("Confused type in nalign");
		/*NOTREACHED*/
	}
	return (aln / SZCHAR);
}

void
branch(n)			/* branch to label n or return */
int n;
{
	jmplab(n);
}
#endif  /* 0 */

void
defalign(n)			/* align to multiple of n */
int n;
{
	if ((n /= SZCHAR) > 1 && lastalign != n)
	     fprintf(outfile,"\t.align	%d\n", n);
	lastalign = n;
}
static void
jmplab(n)			/* produce jump to label n */
int n;
{
    fprintf(outfile,"	jmp	.L%d\n", n);
}

void
deflab(n)			/* label n */
int n;
{
	fprintf(outfile, ".L%d:\n", n);
}

#ifdef MAU
static int maxusrsize = 0;	/* stack size needed for MAU regs */
#endif /* ifdef MAU */


void
efcode(p)			/* wind up a function */
NODE *p;
{
	/*extern int maxtemp, maxarg;*/
	register int i;
	/*extern int strftn;	/* non-zero if function is structure function,
				** contains label number of local static value
				*/
	int n;
				/*Figure out the lowest requested register*/
	int stack,regs;
	
	if (p->in.name)
        	(void)memcpy(request, p->in.name, sizeof(request));

#ifdef MAU
	stack = p->tn.lval*SZCHAR + maxusrsize;
#else
	stack = p->tn.lval*SZCHAR;
#endif	/* ifdef MAU */

	/* figure out how many registers the return value will take 
	*/
	n = biggest_returned;

	deflab(retlab);

        for( i = NRGS; ( ! request[i] )  && i<CPUREGHI; ++i)
                ;
				/*How many registers to save/restore?*/
	regs = CPUREGHI - i ;
			/*If there are holes, make sure they will not
			  be restored. do this by copying them to the save area*/

	for ( ; i<CPUREGHI; ++i)
		if ( !request[i])
			fprintf(outfile,"\tmovw\t%%r%d,%d(%%fp)\n",i, -4*(CPUREGHI-i) );

	SETOFF(stack,ALSTACK);
			/*Define two macros:
				.R1 is the number of register variables
				.F1 is the number of automatics total;
				This includes temps generated by Nifty and cg*/
	fprintf(outfile,"	.set	.R%d,%d\n",ftnno,regs);
	fprintf(outfile,"	.set	.F%d,%d\n",ftnno,stack/SZCHAR);
			/*Now: do the return*/
	fprintf(outfile,"\tret\t&.R%d#%d\n",ftnno, n);
#ifdef MAU
	maxusrsize = 0;			/* reset stack size needed */
#endif	/* ifdef MAU */
}




#ifdef MAU
#if 0		/* not be used */
static int mauregs;	/* number of MAU register variables */
static void
savmau()		/* save active MAU user register variables */
{
	RST tempreg;	/* temporary for register bits */

	mauregs = 0;
	for ( tempreg = regvar; tempreg != 0; ) {
		RST regbit = RS_CHOOSE(tempreg);

		if ( regbit & MAUREGS)
			++mauregs;
		tempreg -= regbit;
	}
	switch(mauregs)
	{
	case 2:
		fprintf(outfile,"	mmovdd	%%f1,.F%d-%d(%%fp)\n",
			ftnno, 2*SZDOUBLE/SZCHAR );
                maxusrsize += SZDOUBLE;
	case 1:
		fprintf(outfile,"	mmovdd 	%%f2,.F%d-%d(%%fp)\n",
			ftnno, SZDOUBLE/SZCHAR );
                maxusrsize += SZDOUBLE;
	}
}

static void
resmau()		/* restore MAU user register variables */
{
	switch( mauregs )
	{
	case 2:
		fprintf(outfile,"	mmovdd	.F%d-%d(%%fp),%%f1\n"
			,ftnno, 2*SZDOUBLE/SZCHAR );
	case 1:
		fprintf(outfile,"	mmovdd	.F%d-%d(%%fp),%%f2\n"
			,ftnno, SZDOUBLE/SZCHAR );
	}
}
#endif /* 0 */
/* for Fortran intrinsics: rsavemau() & rrestmau() - case 'V'
** and 'O' in local2.c zzzcode.
*/
void
rsavemau(p)
NODE * p;
{
	int reg1,reg2;

    	if ( p->in.name == 0 )
    	{
		reg1= request[MAU_F1];
		reg2= request[MAU_F2];
    	}
    	else
    	{
		reg1 = p->in.name[MAU_F1];
		reg2 = p->in.name[MAU_F2];
	}

	if (( reg1 )&&( reg2 ))
	/* save both regs */
	{
		fprintf(outfile,"\tmmovdd\t%%f1,.F%d-%d(%%fp)\n",
			ftnno, 2*SZDOUBLE/SZCHAR );
		fprintf(outfile,"\tmmovdd\t%%f2,.F%d-%d(%%fp)\n",
			ftnno, SZDOUBLE/SZCHAR );
                maxusrsize += ( 2 * SZDOUBLE );
	}
	else
	{
	    if ( reg1 )
		{
		fprintf(outfile,"\tmmovdd\t%%f1,.F%d-%d(%%fp)\n",
			ftnno, SZDOUBLE/SZCHAR );
                 maxusrsize += SZDOUBLE;
		}
	    if ( reg2 )
		{
		fprintf(outfile,"\tmmovdd\t%%f2,.F%d-%d(%%fp)\n",
			ftnno, SZDOUBLE/SZCHAR );
                maxusrsize += SZDOUBLE;
		}
	}
}
void
rrestmau(p)
NODE * p;
{
	int reg1,reg2;

    	if ( p->in.name == 0 )
    	{
		reg1= request[MAU_F1];
		reg2= request[MAU_F2];
    	}
    	else
    	{
		reg1 = p->in.name[MAU_F1];
		reg2 = p->in.name[MAU_F2];
	}

	if ( (reg1) && ( reg2 ) )
	/* restore both regs */
	{
		fprintf(outfile,"\tmmovdd\t.F%d-%d(%%fp),%%f1\n"
			,ftnno, 2*SZDOUBLE/SZCHAR );
		fprintf(outfile,"\tmmovdd\t.F%d-%d(%%fp),%%f2\n"
			,ftnno, SZDOUBLE/SZCHAR );
	}
	else
	{
	    if ( reg1 )
                {
                fprintf(outfile,"\tmmovdd\t.F%d-%d(%%fp),%%f1\n"
                        ,ftnno, SZDOUBLE/SZCHAR );
                }
            if ( reg2 )
                {
                fprintf(outfile,"\tmmovdd\t.F%d-%d(%%fp),%%f2\n"
                        ,ftnno, SZDOUBLE/SZCHAR );
                }
        }
}

#endif	/* ifdef MAU */
void
bycode(ch, loc)			/* byte ch into string location loc */
int ch, loc;
{
	if (ch < 0)		/* eos */
	{
		if (loc)
			putc('\n', outfile);
	}
	else
	{
		if ((loc % 10) == 0)
			emit_str("\n\t.byte\t");
		else
			putc(',', outfile);
		fprintf(outfile, "%d", ch);
	}
}
#if 0  /*not be used */
void 
zecode(n)			/* n words of 0 */
register int n;
{
	if (n <= 0)		/* this is possible, folks */
		return;
	fprintf(outfile,"	.zero	%d\n", (SZINT / SZCHAR) * n);
	inoff += n * SZINT;
}
#endif /* 0 */

#ifndef	INI_HSWITSZ
#define	INI_HSWITSZ 25
#endif

static struct sw heapsw_init[INI_HSWITSZ];

#ifndef STATSOUT
static
#endif
TD_INIT( td_heapsw, INI_HSWITSZ, sizeof(struct sw),
		0, heapsw_init, "heap switch table");
#define HSWITSZ (td_heapsw.td_allo)
#define heapsw ((struct sw *)(td_heapsw.td_start))


void
genswitch(p, n)
register struct sw *p;
int n;
{
	/* p points to an array of structures, each consisting	*/
	/* of a constant value and a label. 			*/
	/* The first is >=0 if there is a default label; its	*/
	/* value is the label number. The entries p[1] to p[n]	*/
	/* are the nontrivial cases				*/

	register int i;
	register CONSZ j; 
	register unsigned long range;
	register int dlab, swlab;
	int piclab;

	if (p[n].sval < 0 || p[1].sval > 0)
	    range = p[n].sval-p[1].sval;
	else
	{	
	/*  The following 2 lines of code does unusual thing, which
	    is to enforce a 2's complement addition for subtraction.
	    This is to guard against boundary number subtraction 
	    overflow which would lead to undefined behavior - e.g.
	    0x10 - 0x80000000 (for 32 bit range).
	*/
	    range = (-1) - p[1].sval;
	    range += (unsigned long)p[n].sval + 1;
	}
	if (range != 0 && range <= (3 * n) && n >= 4)
	{				/* implement a direct switch */
		dlab = (p->slab >= 0) ? p->slab : getlab();
		if (p[1].sval)
#if defined(M32B) && defined(IMPSWREG)
			{
			fprintf(outfile,"\tsubw3\t&%ld,%s,%%r0\n",
				p[1].sval, rnames[swregno]);
			swregno = 0;
			}
#else
			fprintf(outfile,"\tsubw2\t&%ld,%%r0\n",
				p[1].sval);
#endif
		swlab = getlab();
#if defined(M32B) && defined(IMPSWREG)
		fprintf(outfile,"\tcmpw\t%s,&%lu\n\tjgu\t.L%d\n\tALSW3\t&2,%s,%%r0\n"
			, rnames[swregno], range, dlab,rnames[swregno]);
#else
		fprintf(outfile,"\tcmpw\t%%r0,&%d\n\tjgu\t.L%d\n\tALSW3\t&2,%%r0,%%r0\n",
			range, dlab);
#endif
		if (picflag) {
			fprintf(outfile, LABFMT,piclab = getlab());
			fprintf(outfile, ":\n\tmovaw	.L%d@PC,%%r1\n",piclab);
			fprintf(outfile, "\tmovaw	.L%d@PC,%%r2\n",swlab);
			fprintf(outfile, "\taddw2	%%r2,%%r0\n");
			fprintf(outfile,"\taddw2	0(%%r0),%%r1\n");
			fprintf(outfile,"\tJMP	0(%%r1)\n");
		}
		else
			fprintf(outfile,"\tjmp\t*.L%d(%%r0)\n", swlab);
			(void)locctr( FORCE_LC(CDATA) );			/* output table */
		defalign(ALPOINT);
		emit_str("#SWBEG\n");
		deflab(swlab);
		for (i = 1, j = p[1].sval; i <= n; ++j) {
			if (picflag)
			    fprintf(outfile,"	.word	.L%d-.L%d\n",
					(j == p[i].sval) ? p[i++].slab : dlab, piclab);
			else
			    fprintf(outfile,"	.word	.L%d\n",
			    		(j == p[i].sval) ? p[i++].slab : dlab );
		}
		emit_str("#SWEND\n");
		(void)locctr(PROG);
		if (p->slab < 0)
			deflab(dlab);
	}
	else if ( n > 8 )
	{
		heapsw[0].slab = dlab = p->slab >= 0 ? p->slab : getlab();
		makeheap( p, n );	/* build heap */
		walkheap( 1, n );	/* produce code */
		if( p->slab >= 0 )
			jmplab( dlab );
		else
			deflab( dlab );
	}
	else					/* simple switch code */
	{
		for (i = 1; i <= n; ++i)
#if defined(M32B) && defined(IMPSWREG)
			fprintf(outfile,"\tcmpw\t&%ld,%s\n\tje\t.L%d\n",
				p[i].sval, rnames[swregno], p[i].slab);
#else
			fprintf(outfile,"\tcmpw\t&%ld,%%r0\n\tje\t.L%d\n",
				p[i].sval, p[i].slab);
#endif
		if (p->slab >= 0)
			jmplab(p->slab);
	}
}


void
makeheap( p, n )
struct sw * p;				/* point at default label for
					** current switch table
					*/
int n;					/* number of cases */
{
    static void make1heap();

    if (n+1 > HSWITSZ)			/* make sure table is big enough */
	td_enlarge(&td_heapsw, n+1);

#ifdef STATSOUT
    if (td_heapsw.td_max < n) td_heapsw.td_max = n;
#endif

    make1heap( p, n, 1 );		/* do the rest of the work */
    return;
}

static void
make1heap( p, m, n )
	register struct sw *p;
{
	register int q = select( m );

	heapsw[n] = p[q];
	if( q > 1 )
		make1heap( p, q-1, 2*n );
	if( q < m )
		make1heap( p+q, m-q, 2*n+1 );
}
static int
select( m )
{
	register int l, i, k;

	for( i=1; ; i*=2 )
		if( (i-1) > m ) break;
	l = ((k = i/2 - 1) + 1)/2;
	return( l + (m-k < l ? m-k : l) );
}
static void 
walkheap( start, limit )
{
	int label;

	if( start > limit )
		return;
#if defined(M32B) && defined(IMPSWREG)
	fprintf( outfile, "\tcmpw\t%s,&%ld\n\tje\t.L%d\n",
		rnames[swregno], heapsw[start].sval, heapsw[start].slab );
#else
	fprintf( outfile, "\tcmpw\t%%r0,&%ld\n\tje\t.L%d\n",
		heapsw[start].sval, heapsw[start].slab );
#endif
	if( (2*start) > limit )
	{
		fprintf( outfile, "	jmp	.L%d\n", heapsw[0].slab );
		return;
	}
	if( (2*start+1) <= limit )
	{
		label = getlab();
		fprintf( outfile, "	jg	.L%d\n", label );
	}
	else
		fprintf( outfile, "	jg	.L%d\n", heapsw[0].slab );
	walkheap( 2*start, limit );
	if( (2*start+1) <= limit )
	{
		fprintf( outfile, ".L%d:\n", label );
		walkheap( 2*start+1, limit );
	}
}

#ifdef M32B
NODE *
setswreg( p )
	NODE *p;
{
	swregno = 0;

#ifdef IMPSWREG
	if (p->in.op == ASSIGN && p->in.left->in.op == SNODE)
	{
		NODE *q = p->in.right;
		if( q->in.op == REG )
		{
			/* Overwrite in place so result node is
			** still p.
			*/
			swregno = q->tn.rval;
			p->in.left->in.op = FREE;
			*p = *q;
			q->in.op = FREE;
		}
	}
#endif
	return( p );
}

#endif



static int prot_nest=0; /*nesting level for protections*/
void
protect(p)
NODE *p;
{
                        /*Leave a marker telling optimizers (both LION
                          and pci, a.out optimizers) to keep
                          their paws off of this code. */
                        /*calls to this routine nest, i.e. two protect()s
                          require two unprot() calls before optimizers
                          will be let loose again*/
                        /*For 3b20: hard labels have a different syntax.
                          this is due to an optim bug that will remove
                          some labels even is #ASMs*/
        if ( (p->in.op == GENLAB) || (p->in.op == LABELOP) )
        {
                fprintf(outfile,"#HARD\t");
                fprintf(outfile,LABFMT, p->bn.label);
                fprintf(outfile,"\n");
        }
        else if ( !(prot_nest++))
        {
                fprintf(outfile,"%s\n", PROT_START);
        }
}
void  
unprot(p)
NODE *p;
{
                        /*label protection is the #HARD; no trailing
                          string needed*/
        if (p->in.op == GENLAB)
                return;

        switch( --prot_nest)
        {
        case -1:
                cerror("unprot() called without matching protect()");
		/*FALLTHRU*/
        case 0:
                fprintf(outfile,"%s\n", PROT_END);
        /*default: just decrement the prot_nest count*/
        }
}
int
p2done()
{
        char buf[BUFSIZ];
        int m;
	int err_flag = 0;
	if (tmpfp)
	{
        	(void)fclose(tmpfp);
		if ( ferror(tmpfp) )
			err_flag = 1;
        	if (!(tmpfp = fopen(tmpfn, "r")))
                	cerror("string file disappeared???");
#ifndef RODATA
		(void)locctr(DATA);
#endif
        	while ((m = fread(buf, 1, BUFSIZ, tmpfp)) != 0)  
                	fwrite(buf, 1, m, outfile);
        	(void)unlink(tmpfn);
	}
	if ( ferror(outfile) )
		err_flag = 1;
	return err_flag;
}
void
begf(p) /*called for BEGF nodes*/
NODE *p;
{
                        /*save the used registers*/
	if (p->in.name)
        	(void)memcpy(request, p->in.name, sizeof(request));
	else
		(void)memset(request, 0, sizeof(request));

        (void)locctr(PROG);
        retlab = getlab();              /*common return point*/
                        /*instances that need to generate seperate
                          prologs for every entry will want to bump
                         ftnno in bfcode, which is called for every entry.*/
        ftnno++;
        strftn = 0;
	biggest_returned = 0;
}

/*ARGSUSED*/
void bfdata(p) NODE *p; {}

void
myret_type(t)
TWORD t;
{
	switch (t)
	{
	case TVOID:
		break;
	case TDOUBLE:
	case TFPTR:
		biggest_returned = 2;
		break;
	default:
		if (!biggest_returned)
			biggest_returned = 1;
		break;
	}
}

#if 0

void
genladder( range_array, range_l, default_label)
struct case_range range_array[];
int range_l, default_label;
{
			/*Given the sorted array of case ranges, construct
			  a ladder that tests each case*/
	int around;
	int previous_upper_bound = range_array->lower_bound;
	register struct case_range *ranges;
	if (default_label == -1)
		around = getlab();
	else
		around = default_label;

	for (ranges = range_array; ranges < range_array + range_l; ++ranges)
	{
			/*If the case value is less than the lower bound,
			  skip to the "around" point*/
			/*But, skip this comparison if the previous
			  upper_bound was lower_bound - 1*/
	
		if (previous_upper_bound != (ranges->lower_bound - 1))
			fprintf(outfile,"\tcmpw\t%%r0,&%ld\n\tjl\t.L%d\n",
				ranges->lower_bound, around);
			/* If the case value is less than or 
			  equal than the upper bound,
			  skip to the case label*/

		/* Must have the second check if the first one was omitted,
		** or if the range has more than one value.
		*/
		if (   ranges->lower_bound != ranges->upper_bound
		    || previous_upper_bound == ranges->lower_bound - 1
		    )
		    fprintf(outfile, "\tcmpw\t%%r0,&%ld\n", ranges->upper_bound);

		fprintf(outfile,"\tjle\t.L%d\n", ranges->goto_label);

			/*Save the previous upper bound*/

		previous_upper_bound = ranges->upper_bound;

	}
			/*If we fall thru: if there is a default label,
			  jump to it; otherwise define the "around" case*/

	if (default_label != -1)
	{
		jmplab(default_label);
	}
	else
		deflab(around);
}

#endif
