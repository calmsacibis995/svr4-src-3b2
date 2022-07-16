/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nifg:cg/mau/local2.c	1.82"
/*	local2.c - machine dependent grunge for back end
 *
 *		Bellmac32
 *			R. J. Mascitti
 */

# include "mfile2.h"
# include "string.h"
# define istnode(p) (p->in.op==REG && istreg(p->tn.rval))
# define STHRESH 6	/* max # of words to unroll copy loop */
# define BTHRESH 6	/* max # of words to unroll block move loop */

char *exname();
static void dbladrput(), blockmove(), blockcmp(), prep_offset(), rg_load(), 
	    hfselect(), fselect(), raisex(), testex(); 
/* static int strcall;	/* have we made a call to a structure function? */

static char * off_reg;	/* place to hold offset for fselect */

#ifdef VOL_SUPPORT
static int vol_opnd = 0;	/* volatile operand */
int cur_opnd = 1;		/* current operand  */
#define VOL_CLEAN()	{vol_opnd = 0; cur_opnd = 1;}
#endif

void
lineid(l, fn)			/* identify line l and file fn */
int l;
char *fn;
{
	fprintf(outfile,"#	line %d, file %s\n", l, fn);
}
#if 0  /*not be used */
void
eobl2()				/* end of function stuff */
{
	strcall = 0;
}
#endif /* 0 */

#ifdef MAU
/* the regout array maps a number for each value of rnames[] for
** debugging purposes.
*/
int regout[] =
{
	0,	/* CPU scratch	*/
	1,	/* CPU scratch  */
	2,	/* CPU scratch  */
	20,	/* MAU scratch	*/
	23,	/* MAU scratch	*/
	3,	/* CPU user	*/
	4,	/* CPU user     */
	5,	/* CPU user     */
	6,	/* CPU user     */
	7,	/* CPU user     */
	8,	/* CPU user     */
	21,	/* MAU user	*/
	22,	/* MAU user     */
};
#endif /* ifdef MAU */

char *rnames[]=
{
	"%r0",
	"%r1",
	"%r2",
#ifdef MAU
	"%f0",
	"%f3",
#endif	/* ifdef MAU */
	"%r3",
	"%r4",
	"%r5",
	"%r6",
	"%r7",
#ifdef MAU
	"%r8",
	"%f1",
	"%f2"
#else
	"%r8"
#endif	/* ifdef MAU */
};

static char * ccbranches[] =
{
	"je",
	"jne",
	"jle",
	"jl",
	"jge",
	"jg",
	"jleu",
	"jlu",
	"jgeu",
	"jgu"
};

static char * arithjump[] =
{
	"jz",
	"jnz",
	"jnpos",
	"jneg",
	"jnneg",
	"jpos",
	"jnpos",
	"jneg",
	"jnneg",
	"jpos"
};

#define NEXTZZZCHAR     (*++(*ppc))
static int cmpflag = 0;		/* have we done a cmp? */
void  
zzzcode(pnode, ppc, q)		/* hard stuff escaped from templates */
NODE *pnode;
char **ppc;
OPTAB *q;			/* not used in m32 zzzcode */
{
 	
	int tempval;
	NODE *pl; 

#ifdef MAU
	NODE *pr;
	int o;
	static int ttmp , ctmp1, ctmp2;
#endif
	switch ( NEXTZZZCHAR )
	{
	case '?':
	/* Z? is for testing the offset of signed bitfield whether
	   it is zero or not.
	*/
	{
		extern int fldsz, fldshf;
		pl = getadr(pnode, ppc);
                fldsz = UPKFSZ(pl->tn.rval);
                fldshf = SZINT - fldsz - UPKFOFF(pl->tn.rval);
		if (32 - fldshf - fldsz)
		{
			while( NEXTZZZCHAR != 'E' )
                        {
                                if( **ppc == '\\' ) (void)NEXTZZZCHAR; /* hide next char */
                                if( !*ppc) return;
                        }
		}
		break;
	}

	case 'o':
	/* Zo is for outputing the comment line "#TYPE [SINGLE|DOUBLE]" for
	   floating point objects using cpu instructions, such as "mov" or
	   "push".  This is the interface to HALO optimizer.
	*/
		switch ( NEXTZZZCHAR ) {
		case 'f':
		    fprintf(outfile, "#TYPE	SINGLE\n");
		    break;
		case 'd':
		    fprintf(outfile, "#TYPE	DOUBLE\n");
		    break;
		default:
		    cerror("bad Zo");
		}
		break;

#ifdef MAU

	case 'R':
	/* read and change the rounding mode so that movsw and movdw will truncate */
	/* need two temp %fp offsets */
		ctmp1 =  freetemp( 1 ) / SZCHAR;
		ctmp2 =  freetemp( 1 ) / SZCHAR;
		fprintf(outfile, "\tmmovfa\t%d(%%fp)\n", ctmp1);
		fprintf(outfile, "\tandb3\t&0xc0,%d(%%fp),%d(%%fp)\n", ctmp1+1, ctmp2 );
		fprintf(outfile, "\torb2\t&0xc2,%d(%%fp)\n", ctmp1+1 );
		fprintf(outfile, "\tmmovta\t%d(%%fp)\n", ctmp1 );
		break;
		
	case 'r':
	/* restore old rounding mode */
		fprintf(outfile, "\tmmovfa\t%d(%%fp)\n", ctmp1 );
		fprintf(outfile, "\tandb2\t&0x3f,%d(%%fp)\n",ctmp1+1 );
		fprintf(outfile, "\torb2\t%d(%%fp),%d(%%fp)\n", ctmp2, ctmp1+1 );
		fprintf(outfile, "\torb2\t&0x2,%d(%%fp)\n", ctmp1+1 );
		fprintf(outfile, "\tmmovta\t%d(%%fp)\n", ctmp1 );
		break;

	case 'V':
	/* save mau user registers	*/
		rsavemau(pnode);
		break;

	case 'O':
	/* restore mau user registers	*/
		rrestmau(pnode);
		break;

	case 'd':
		switch ( NEXTZZZCHAR ) {
		case 'T':
		    ttmp =  freetemp( 2 ) / SZCHAR;
		    fprintf(outfile, "%d(%%fp)", ttmp );
		    break;

		case 't':
		    fprintf(outfile, "%d(%%fp)", ttmp+4 );
		    break;
		
		default:
		    cerror("bad Zd");
		}
		break;
 
	case 'T':
	/* get a temp %fp offset for intermediate storage for MAU instructions */
		ttmp =  freetemp( 1 )  / SZCHAR;
		/*FALLTHRU*/
	case 't':
		fprintf(outfile, "%d(%%fp)", ttmp );
		break;
		
	case 'F':
	/* used for FUNC shapes in 'CALL','UCALL', and 'ARG' templates
	*/
	{
		int didsave = 0;			
		pr = getadr( pnode, ppc );
		if (! (pnode->in.op == ASSIGN && pnode->in.left->in.op == RNODE))
		{
        	    rsavemau(pr);
		    didsave = 1;
		}
		fprintf(outfile,"	call	&%d,",pr-> stn.argsize/SZINT);
		pl= getl(pr);
		o = pl -> in.op;
		switch(o)
		{
		case REG:
			fprintf(outfile,"0(" );
			adrput( pl );
			PUTCHAR(')');
			break;
		case NAME:
		case TEMP:
		case VAUTO:
		case VPARAM:
			PUTCHAR('*');
			adrput( pl );
			break;
		case ICON:
			conput( pl );
			break;
			
		default:
			cerror("illegal ZF %c", **ppc);
			/* NOTREACHED */
		}
		PUTCHAR('\n');
		if (didsave)
        	    rrestmau(pr);

		break;
	}
#endif	/* ifdef MAU */

	case 'H':    
	/* put out word relative bit shift for insv and extv
	/* instruction.  Similar to case 'H' case in expand(),
	/* but no difference for different types
	*/
	    {
		extern int fldsz, fldshf;

		pl = getlr( pnode, NEXTZZZCHAR );
		if (pl->tn.op != FLD ) cerror( "bad FLD for ZH" );
		fldsz = UPKFSZ(pl->tn.rval);
		fldshf = SZINT - fldsz - UPKFOFF(pl->tn.rval);
		fprintf(outfile, "%d", fldshf );
	    }
		break;


	case 'D':	/* output address of second half of double */
		dbladrput( pl = getlr(pnode, NEXTZZZCHAR) );
		break;


	case 'B':
		bycode((int)pnode->in.left->tn.lval, 0);
		break;

	case 'b':
		++cmpflag;
		break;

	case 'C':
		fprintf(outfile,"%s\t.L%d\n",
		    cmpflag ? ccbranches[pnode->bn.lop - EQ] :
		    arithjump[pnode->bn.lop - EQ], pnode->bn.label);
		cmpflag = 0;
		break;

	case 'c':		/* # of word-length args */
		fprintf(outfile,"%d", pnode->stn.argsize / SZINT);
		break;

	case 'h':		/* deal with the BM32's weird treatment
				   of half-word compares */
		/* If a 16 bit positive constant is used, it will
		   not be sign extended, but the othe operand will */
		{
		register NODE *r, *l;
		int lchg = 0, rchg = 0;
		l = pnode ->in.left;
		if ((l->tn.op == ICON) &&
		    ((l->tn.type == TUSHORT) ||
		     (l->tn.type == TSHORT)) &&
		    (l->tn.lval > 0x7fff))  {
			lchg = 1;
		}
		r = pnode->in.right;
		if ((r->tn.op == ICON) &&
		    ((r->tn.type == TUSHORT) ||
		    (r->tn.type == TSHORT)) &&
		    (r->tn.lval > 0x7fff))  {
			rchg = 1;
		}
		if ((!rchg && !lchg) || (rchg && lchg))  {
			expand(pnode, FORCC, "cmph	AL,AR", q);
		} else if (rchg)  {
			expand(pnode, FORCC,
			    "movzhw	AL,A1\n	cmpw	A1,AR", q);
		} else {
			expand(pnode, FORCC,
			    "movzhw	AR,A1\n	cmpw	AL,A1", q);
		}
		break;
		}

	case 'k':
#ifdef	MAU
#define PEEKZZZCHAR	(*((*ppc)+1))
 	/* Output floating constant for INT_MAX or UINT_MAX:
	** For single precision:
 	** Zk.si:  output constant (INT_MAX); Zksi:  output address
 	** Zk.su:  output constant (UINT_MAX); Zksu:  output address
 	** Zk.siu: output both constants; Zksiu:  output both addresses 
	**
	** For double precision:
 	** Zk.di:  output constant (INT_MAX); Zkdi:  output address
 	** Zk.du:  output constant (UINT_MAX); Zkdu:  output address
 	** Zk.diu: output both constants; Zkdiu:  output both addresses 
 	*/
		{
 		static int labs[4];	/* [0],[2] for int; [1],[3] for uint */
 		static const char * const fmts[4] = {
	    		".L%d:	.word	0x4f000000\n",		/* int */
	    		".L%d:	.word	0x4f800000\n",		/* uint */
 	    		".L%d:	.word	0x41e00000,0x0\n",	/* int */
 	    		".L%d:	.word	0x41f00000,0x0\n",	/* uint */
 		};
 		int c;
 		int gendata = 0;
 		int didprefix = 0;
		int single_precision = 0;
	 
 		if ((c = NEXTZZZCHAR) == '.') {
			gendata = 1;
 	    		c = NEXTZZZCHAR;
		}
	
		switch ( c ) {
		case 's':
			single_precision = 1; 
		case 'd':
			break;
		default:
			cerror("bad Zk%c",c);
 		}
		c = NEXTZZZCHAR;
	 
 		for (;;) {
		    int index;
		    int lab;
 
		    switch( c ){
		    case 'i':   if (single_precision) 
				    index = 0; 
				else 	
				    index = 2;
				break;
            	    case 'u':   if (single_precision)
				    index = 1;
				else
				    index = 3; 
				break;
            	    default:    cerror("bad Zk[d|s]%c", c);
	    	    }
 	    
 	    	    lab = labs[index];
 	    	    if (gendata) {
 			if (lab == 0) {
 		    	    labs[index] = lab = getlab();
 		    	    if (!didprefix) {
 			        (void) locctr( FORCE_LC(CDATA) );
 			        emit_str("	.align	4\n");
		    	    }
 			    fprintf(outfile, fmts[index], lab);
 		    	    didprefix = 1;
 		    	}
 	    	    }
 	            else {
 			if (picflag)
 		    	    fprintf(outfile, ".L%d@PC", lab);
 		        else
			    fprintf(outfile, ".L%d", lab);
 	    	    }
 	    	    if (PEEKZZZCHAR == 'u')
 			c = NEXTZZZCHAR;
 	    	    else
			break;
 		}
 		if (didprefix)
 	    	    (void) locctr(PROG);
		break;
     		}

#else /*!MAU*/
		if (picflag)
		    emit_str("@PLT");
#endif
		break;

	case 'S':		/* STASG */
fprintf(outfile,"#STASG**************:\n");
		stasg(pnode, (NEXTZZZCHAR == '1') );
#ifdef VOL_SUPPORT
		VOL_CLEAN();
#endif
fprintf(outfile,"#End STASG^^^^^^^^^^^^^^:\n");
		break;

	case 's':		/* STARG */
fprintf(outfile,"#STARG**************:\n");
                starg(pnode);
#ifdef VOL_SUPPORT
		VOL_CLEAN();
#endif
fprintf(outfile,"#End STARG^^^^^^^^^^^^^^:\n");
		break;

	case 'M':		/*block move*/
		blockmove(pnode);
#ifdef VOL_SUPPORT
		VOL_CLEAN();
#endif
		break;

	case 'P':		/*block compare*/
		blockcmp(pnode);
		break;

	case 'f':
			/*arrange for the offset of this fselect to be
			  in a register. set off_reg to this register*/
		prep_offset(getadr(pnode,ppc));
		break;
	case 'A':		/*select from a frame*/
		fselect(getadr(pnode,ppc));
		break;
	case 'G':		/*select from a frame- high word*/
		hfselect(getadr(pnode,ppc));
		break;
	case 'a':	/*arrange for the high offset to be stored in off_reg*/
		rg_load(getadr(pnode,ppc),1);
		break;
	case 'p':	/*Print the high offset stored by 'h'*/
		expand( getadr(pnode,ppc), FOREFF, off_reg, (OPTAB *) 0);
		break;
	case 'e':	/*Test for numeric exception*/
		if ( pnode->in.strat & EXHONOR)
			testex(pnode);
		break;

	case 'E':	/*raise a numeric exception*/
		raisex(pnode);
		break;
	case 'x':	/*Print the vector location for the exception*/
		fprintf(outfile,"_exvect");
		if ( pnode->tn.lval == STACKOV)
			fprintf(outfile,"+4");
		break;
	case 'L':
#define MAXLAB  5
	        switch( tempval = NEXTZZZCHAR ) {
	            static int labno[MAXLAB+1]; /* saved generated labels */
	            int i, n;
	        case '.':                       /* produce labels */
	            /* generate labels from 1 to n */
	            n = NEXTZZZCHAR - '0';
	            if (n <= 0 || n > MAXLAB)
	                cerror("Bad ZL count");
	            for (i = 1; i <= n; ++i)
	                labno[i] = getlab();
	            break;
	
	        default:
	            cerror("bad ZL");
		    /*FALLTHRU*/
	        /* generate selected label number */
	        case '1':
	        case '2':
	        case '3':
	        case '4':          /* must have enough cases for MAXLAB */
	        case '5':
	            fprintf(outfile,".L%d", labno[tempval - '0']); 
	            break;
	        }         
	        break;



	default:
		cerror("botched Z macro %c", **ppc);
		/*NOTREACHED*/
	}
}
void
conput(p)			/* name for a known */
register NODE *p;
{
#ifdef VOL_SUPPORT
	if (p->in.strat & VOLATILE) vol_opnd |= cur_opnd;
#endif
	switch(p->in.op)
	{
	case ICON:
		acon(p);
		break;
	case REG:
		emit_str(rnames[p->tn.rval]);
		break;
        case UNINIT:
                acon(p);
                break;
        case CURCAP:
                emit_str("%sp");
                break;
        case CSE:
                {
                        struct cse *cs_ptr;
                        if ( (cs_ptr = getcse(p->csn.id)) == NULL)
                        {
                                e2print(p);
                                cerror("Uknown CSE id");
                        }
                        emit_str(rnames[cs_ptr->reg]);
                }
                break;
        case COPY:
        case COPYASM:
                if ( p->tn.name)
                        emit_str(p->tn.name);
                break;

	default:
		cerror("confused node in conput");
		/*NOTREACHED*/
	}
}
void
insput(p)			/* we don't have any */
NODE *p;
{
                        /*print the upper word of a two-word value.
                          used for stack frame funnies.  should really
                          be upput but that has been taken already*/
	int minusflag = 0;

        sideff = 0;
        while (p->in.op == FLD || p->in.op == CONV)
                p = p->in.left;

#ifdef VOL_SUPPORT
	if (p->in.strat & VOLATILE) vol_opnd |= cur_opnd;
#endif
        switch (p->in.op)
        {
        case NAME:
                if ( p->in.name == 0)
                        cerror("Bad name in insput");
                fprintf (outfile,"%s%s%ld",
                p->in.name, ((p->tn.lval+4) > 0 ? "+" : ""), (p->tn.lval+4));
                break;
        case REG:
                emit_str(rnames[p->tn.rval+1]);
                break;
        case TEMP:
                fprintf(outfile,"%ld(%%fp)", p->tn.lval + 4);
                break;
        case VAUTO:
                fprintf(outfile,"%ld(%%fp)", p->tn.lval + 4);
                break;
        case VPARAM:
                fprintf(outfile,"%ld(%%ap)", p->tn.lval + 4);
                break;
        case STAR:
		switch ( p->in.left->in.op )
		{
		case REG:
			fprintf(outfile,"4(%s)",
				rnames[ p->in.left->tn.rval ]);
			break;
		case MINUS:
                        minusflag = 1;
			/*FALLTHRU*/
                case PLUS:
                        if((p->in.left->in.left->in.op == REG)
                        &&(p->in.left->in.right->in.op == ICON))
                                {
                                /* change constant by 4 */
                                if (minusflag)
				   p->in.left->in.right->tn.lval -= 4;
				else
                                   p->in.left->in.right->tn.lval += 4;

                                upput(p);
                                /* restore constant to original value */
                                if (minusflag)
				   p->in.left->in.right->tn.lval += 4;
				else
                                   p->in.left->in.right->tn.lval -= 4;
                                }
                        else
                                cerror("insput: illegal subtree rooted at +/-");
                        break;
                default:
			PUTCHAR('*');
                	insput(p->in.left);
                	break;
		}
		break;
        case CURFRAME:
                fprintf(outfile,"%%fp");
                break;
        default:
                e2print(p);
                cerror("insput: illegal address");
                break;
        }

}
void
upput(p)			/* generate address incl. indirect, offset */
NODE *p;
{
	register NODE *pp;

	sideff = 0;
	pp =  p->in.left;

#ifdef VOL_SUPPORT
	if ((p->in.strat & VOLATILE) || (pp->in.strat & VOLATILE))
		vol_opnd |= cur_opnd;
#endif

again:
	switch (pp->tn.op)
	{
	case NAME:
	case TEMP:
	case VAUTO:
	case VPARAM:
		PUTCHAR('*');
		adrput(pp);
		break;

	case REG:
		emit_str("0(");
		conput(pp);
		PUTCHAR(')');
		break;

	case ICON:
		PUTCHAR('*');
		acon(pp);
		break;

	case STAR:
		PUTCHAR('*');
		pp = pp->in.left;
		goto again;

	case PLUS:
	case MINUS:
		if (pp->in.op == MINUS)
			PUTCHAR('-');
		acon(pp->in.right);
		PUTCHAR('(');
		conput(pp->in.left);
		PUTCHAR(')');
		break;
	
	default:
		cerror("upput: confused addressing mode matched");
		/*NOTREACHED*/
	}
}
void
adrput(p)		/* output address from p */
NODE *p;
{
	sideff = 0;
	while (p->in.op == FLD || p->in.op == CONV)
		p = p->in.left;

#ifdef VOL_SUPPORT
	if (p->in.strat & VOLATILE)  vol_opnd |= cur_opnd;
#endif

	switch (p->in.op)
	{
	case ICON:		 /* value of the constant */
		PUTCHAR('&');
		/*FALLTHRU*/
	case NAME:
		acon(p);
		break;
        case CURCAP:
        case CSE:
	case REG:
		conput(p);
		break;
	case STAR:
		upput(p);
		break;
	case TEMP:
		fprintf(outfile,"%ld(%%fp)", p->tn.lval );
		break;
	case VAUTO:
		fprintf(outfile,"%ld(%%fp)", p->tn.lval);
		break;
	case VPARAM:
		fprintf(outfile,"%ld(%%ap)", p->tn.lval);
		break;
        case CURFRAME:
                fprintf(outfile,"%%ap");
                break;
	default:
		cerror("adrput: illegal address");
		break;
	}
}

static void
dbladrput(p)
NODE *p;
/* output address of word after p.  used for double moves */
{
	int minusflag = 0;

	while (p->in.op == FLD || p->in.op == CONV)
		p = p->in.left;

#ifdef VOL_SUPPORT
	if (p->in.strat & VOLATILE) vol_opnd |= cur_opnd;
#endif

	switch (p->in.op)
	{
	case NAME:
	    fprintf(outfile,"%s+%ld", p->in.name, p->tn.lval + 4);
	    if (picflag) {
		register flag = p->tn.strat;
		fprintf(outfile, "%s",
			((flag & PIC_GOT)?"@GOT":
				((flag & PIC_PLT)?"@PLT":
					((flag & PIC_PC)?"@PC":""))));
	    }
	    break;
	case REG:
	    emit_str(rnames[p->tn.rval + 1]);
	    break;
	case TEMP:
	    fprintf(outfile,"%ld(%%fp)", p->tn.lval+4);
	    break;
	case VAUTO:
	    fprintf(outfile,"%ld(%%fp)", p->tn.lval + 4);
	    break;
	case VPARAM:
	    fprintf(outfile,"%ld(%%ap)", p->tn.lval + 4);
	    break;
        case CURFRAME:
                fprintf(outfile,"%%fp");
                break;
        case STAR:
                switch( p->in.left->in.op )
                {
                case REG:
                        fprintf(outfile,"4(%s)", rnames[ p->in.left->tn.rval]);
                        break;
		case MINUS:
			minusflag = 1;
			/*FALLTHRU*/
                case PLUS:
                        if((p->in.left->in.left->in.op == REG)
			&&(p->in.left->in.right->in.op == ICON))
				{
				/* change constant by 4 */
				if (minusflag)
				   p->in.left->in.right->tn.lval -= 4;
				else
				   p->in.left->in.right->tn.lval += 4;

				adrput(p);
				/* restore constant to original value */
				if (minusflag)
				   p->in.left->in.right->tn.lval += 4;
				else
				   p->in.left->in.right->tn.lval -= 4;
				}
                        else
                                cerror("dbladrput: illegal subtree rooted at +/-");
                        break;
                default:
                        cerror("dbladrput: illegal subtree rooted at *");
                }
                break;

	default:
	    cerror("dbladrput:  illegal double address substitution");
	    /*NOTREACHED*/
	}
}

void
acon(p)			/* print out a constant */
register NODE *p;
{
	register long off;

	if (p->in.name == (char *) 0)	/* constant only */
	{
		if (p->in.op == ICON &&
		    (p->in.type == TCHAR || p->in.type == TUCHAR))
			p->tn.lval &= 0xff;
		fprintf(outfile,"%ld", p->tn.lval);
	}
	else if ( (off = p->tn.lval) == 0 )	/* name only */
		fprintf(outfile,"%s", p->in.name);
	else if ( off > 0 )			/* name + offset */
		fprintf(outfile,"%s+%ld", p->in.name, off);
	else					/* name - offset */
		fprintf(outfile,"%s%ld", p->in.name, off);

	if (picflag) {
		register flag = p->tn.strat;
		fprintf(outfile, "%s",
			((flag & PIC_GOT)?"@GOT":
				((flag & PIC_PLT)?"@PLT":
					((flag & PIC_PC)?"@PC":""))));
	}
}


/*ARGSUSED*/
int
special(sc,p)
NODE *p;
{
	cerror("special shape used");
	return (INFINITY);
}

void
end_icommon()
{
	(void)locctr(locctr(UNK));
}

int
getlab()
{
        static int labels = 1;
        return labels++;
}

void
defnam(p)
NODE *p;
{
        CONSZ flags;
        char *name;             /*define this symbol*/
        int size;
        int alignment;
        name = p->tn.name;
        flags = p->tn.lval;
        size = p->tn.rval;

        alignment = gtalign(p->in.type);
        if (flags & EXTNAM)
		fprintf(outfile,"\t.globl	%s\n", exname(name));
        if (flags & COMMON)
        {
                /*make sure the size is a multiple of the alignment.
                  This is because ld uses the size to determine the alignment
                  of unresolved .comms*/
                if ( size % alignment)
                {
                   size += (alignment - (size % alignment));
                }
                fprintf(outfile,
                "\t.comm	%s,%d", exname(name), size/SZCHAR);
#ifdef  ELF_OBJ
		fprintf(outfile, ",%d", alignment/SZCHAR);
#endif
		emit_str("\n");
        }
#ifndef ELF_OBJ 
        else if (flags & ICOMMON)
        {
                int curloc;
                char *ex;
                if ( (curloc = locctr(CURRENT)) != DATA && curloc != CDATA)
                        cerror(
                        "initialized common not in data: not implemented");
                ex = exname(name);
                fprintf(outfile,"\t.icomm       %s,.data\n", ex);
        	defalign(alignment);
		fprintf(outfile,"%s:\n", ex);
        }
#endif
	else if (flags & LCOMMON)
	{
#ifdef  ELF_OBJ
		fprintf(outfile, "\t.local	%s\n", name);
		fprintf(outfile, "\t.comm	%s,%d,%d\n", name, size/SZCHAR, alignment/SZCHAR);
#else
		fprintf(outfile,
		"\t.lcomm\t%s,%d,%d\n", exname(name), size/SZCHAR, alignment/SZCHAR);
#endif
	}
        else if (flags & DEFINE)
	{
        	defalign(alignment);
                fprintf(outfile,"%s:\n", exname(name));
	}
}
/* Print out "type" and "size" information of a variable.
 * There is no information printed out for block statics.
 */
void
definfo(p)
NODE * p;
{
#ifdef  ELF_OBJ
	int type = p->tn.rval;
	CONSZ size = p->tn.lval;
	char *name = p->tn.name;

	/* print out "type" information - function or object */
	if (type & NI_FUNCT) {
	    /* make sure in a right section for output "size" info later. */
	    (void)locctr(PROG);
	    fprintf(outfile, "\t.type	%s,@function\n", name);
	}
	else if ( (type & NI_OBJCT) && (type & (NI_GLOBAL|NI_FLSTAT)) )
		   fprintf(outfile, "\t.type	%s,@object\n", name);

	/* print out "size" information for function or object */
	if (! (type & NI_BKSTAT) ) {
	    if (!size)
		/* this may be used for function size.
		 * it should be directed to the right location counter.
		 */
		fprintf(outfile, "\t.size	%s,.-%s\n", name, name);
	    else
		fprintf(outfile, "\t.size	%s,%ld\n", name, size/SZCHAR);
	}
#endif
}

int
canbereg(t)
TWORD t;
{
	register TWORD *typ ;
			/*can this type be a register variable?*/
			/*can't use sw/case becuase some types are identical*/
	static TWORD oktypes[]= {
		TCHAR , TSHORT , TINT , TLONG , TUCHAR , TUSHORT ,
		TUNSIGNED , TULONG , TFLOAT , TDOUBLE , TPOINT , TPOINT2,
		TFPTR, 0};
	for ( typ = oktypes; *typ; ++typ)
	{
		if ( t == *typ)
			return 1;
	}
	return 0;
}

int
tyreg(t)
TWORD t;
{
			/*Given a type, return:
			  0 if it cannot be a register variable.
			  #regs to hold it if it can be a reg variable*/
	if ( !canbereg(t))
		return 0;
	else
		return szty(t);
}

char *
reglst(t)
TWORD t;
{
			/*return a list of registers that can be used for
			  register variables*/
	static char list[TOTREGS];
	register int r;
	char canbe = canbereg(t);
	int size = tyreg(t);

	for(r=0;r<TOTREGS;++r)
		list[r]=0;

#ifdef MAU
	if ((t==TFLOAT)||(t==TDOUBLE))
	{
		list[10]=1;
		list[11]=1;
	}
	else
	{
		for ( r=0; r<CPUREGHI; ++r)
#else
	{
		for ( r=0; r<TOTREGS; ++r)
#endif	/* ifdef MAU */
		{
			list[r] = (canbe
				&& (r >= NRGS)
					&& ( r+size <= TOTREGS) ); 
		}
	}
	return list;
}

rgsave(request)
register char *request;
{
			/*Figure out how many register we can easily save.
			  This is the largest contiguous bunch starting
			  at TOTREGS and remaining >NRGS.*/
	int rgs=0;
	register int i;
			/*we can efficiently save a block starting at TOTREGS*/
			/*Find the lowest set big*/

#ifdef MAU
	for( i = NRGS; ( ! request[i] )  && i<CPUREGHI; ++i)
		;
	rgs = CPUREGHI - i;
	for ( ; i < CPUREGHI; ++i)
#else
	for( i = NRGS; ( ! request[i] )  && i<TOTREGS; ++i)
		;

	rgs = TOTREGS - i;

			/*Make sure the rest are set*/
	for ( ; i < TOTREGS; ++i)
#endif	/* if def MAU */
		request[i] = 1;

	return rgs;
}

/* supply the costing functions that will be called by lion.
	These numbers have NOT been tuned and are just guesses at the moment*/
	/*First; the cost of doing an arbitrary tree*/
#ifdef  LION  /* turn off the stuff for LION  */
static int oneline = 10;	/*cost of one template*/
static int costld = 20;	/*load a register*/
static int costst = 20;	/*store a register*/
static int usediff = 5;	/*Diff between register and mem use*/
static int setdiff = 5;	/*Diff between register and mem set*/
static int default_cost = 5;
#define NOT_SET -1
int op_costs[DSIZE];

void
local_init()
{
	int i;

			/* initialize cost array	*/
	for(i=0; i<DSIZE; ++i)
	{
		op_costs[i] = NOT_SET;
	}

	op_costs[NAME]     = 20;
	op_costs[ICON]     =  5;
	op_costs[FCON]     = 10;
	op_costs[CALL]     = 60;
	op_costs[VAUTO]    = 10;
	op_costs[VPARAM]   = 10;
	op_costs[UNARY MINUS]= 20;
	op_costs[COMOP]    =  0;
	op_costs[CM]       =  0;
	op_costs[SEMI]     =  0;
	op_costs[OROR]     = 10;	/* "||" logical op */
	op_costs[ANDAND]   = 10;	/* "&&" logical op */
	op_costs[QUEST]    = 10;	/* "?" op */
	op_costs[OR]       = 20;	/* "|" op */
	op_costs[AND]      = 20;	/* "&" op */
	op_costs[MOD]      = 20;	/* "%" op */
	op_costs[ASG MOD]  = 25;	/* "%=" op */
	op_costs[PLUS]	   = 20;	/* "+" op */
	op_costs[ASG PLUS] = 30;	/* "+=" op */
	op_costs[ASG MINUS]= 30;	/* "-=" op */
	op_costs[MINUS]    = 20;	/* "-" op */
	op_costs[DIV]      = 65;	/* "/" op */
	op_costs[ASG DIV]  = 75;	/* "/=" op */
	op_costs[MUL]      = 40;	/* "*" op */
	op_costs[ASG MUL]  = 50;	/* "*=" op */
	op_costs[JUMP]     = 10;
	op_costs[STAR]     = 10;
	op_costs[LS]	   = 20;	/* "<<" op */
	op_costs[RS]       = 20;	/* ">>" op */
	op_costs[EQ]       = 20;	/* "==" op */
	op_costs[NE]       = 20;	/* "!=" op */
	op_costs[LE]       = 20;	/* "<=" op */
	op_costs[GE]       = 20;	/* ">=" op */
	op_costs[GT]       = 20;        /* ">" op */                        
        op_costs[LT]       = 20;        /* "<" op */                        
        op_costs[INCR]     = 25;	/* "++" op */
	op_costs[DECR]     = 25;	/* "--" op */

	/* set remaining costs to default value	*/
	for(i=0; i<DSIZE; ++i)
	{
		if(op_costs[i] == NOT_SET)
			op_costs[i] = default_cost;
	}
}

read_costs(costfile)
FILE * costfile;
{
	char namebuf[80];
	int value;
	int i;

			/* initialize cost array	*/
	for(i=0; i<DSIZE; ++i)
		op_costs[i] = NOT_SET;

	while( fscanf(costfile, "%s %d", namebuf, &value) == 2)
	{
		if(strcmp(namebuf,"oneline")==0) oneline = value;
		else if(strcmp(namebuf,"costld")==0) costld = value;
		else if(strcmp(namebuf,"costst")==0) costst = value;
		else if(strcmp(namebuf,"usediff")==0) usediff = value;
		else if(strcmp(namebuf,"setdiff")==0) setdiff = value;
		else if(strcmp(namebuf,"default")==0) default_cost = value;
		else
		{
			for (i=0; i<DSIZE; ++i)
			{
				if(strcmp(namebuf, opst[i])==0)
				{
					op_costs[i] = value;
					break;
				}
			}
			if( i >= DSIZE)
			{
				fprintf(stderr,"Bad field: %s\n", namebuf);
				cerror("Error in costs file\n");
			}
		}
	}

	/* set remaining costs to default value	*/
	for(i=0; i<DSIZE; ++i)
	{
		if(op_costs[i] == NOT_SET)
			op_costs[i] = default_cost;
	}
}

print_costs()
{
	int i;

	for ( i = 0; i < DSIZE ; ++i )
	{
		fprintf(outfile,"%s\t%d\n",opst[i],op_costs[i]);
	}
}

unsigned int
costex(p)
register NODE *p;
{
	register int cst=0;
			/*for max safety, make a copy of the tree*/
	return count_costs(p);
}
			/*Return the cost of memory refs in the tree*/
static int
count_costs(p)
NODE *p;
{
	int cost = 0;
	if (!p)
		return 0;
	switch(optype(p->in.op))
	{
	case BITYPE:
		cost = count_costs(p->in.right);
		/*FALLTHRU*/
	case UTYPE:
		cost += count_costs(p->in.left);
		break;
	};
	return cost + op_costs[p->in.op];
}

unsigned int
cst_ld(t)
TWORD t;
{
	return costld;
}
unsigned int
cst_st(t)
TWORD t;
{
	return costst;
}
unsigned int
use_diff(t)
TWORD t;
{
	return usediff;
}
unsigned int
set_diff(t)
TWORD t;
{
	return setdiff;
}
#endif  /* LION */

			/*For clever block moves: a list of opcodes,
			  their alignments, an the amount they move*/
static struct {
	char * opcode;	/*the opcode that does the move*/
	int alignment;	/* the alignment neccessary for the move*/
	int count;	/* the number of bytes moved*/
} opcodes[] = {
		{ "movw",	ALINT,		(SZINT/SZCHAR)	},
		{ "movh",	ALSHORT,	(SZSHORT/SZCHAR) },
		{ "movb",	ALCHAR,		1}
	};
#define LASTOP (sizeof(opcodes)/sizeof(opcodes[0])) /*end of the opcodes table*/

static void 
blockmove(pnode)
NODE *pnode;
{
			/*Do a block move.  from and to are scratch
			  registers;  count is either a scratch register
			  or a constant.*/
	NODE *pcount, *pfrom, *pto;	/*node pointers */
	int lastlab = getlab();
	int looplab = getlab();
	int align = gtalign(pnode->in.type);	/*promised alignment*/
	char *opcode = "movb";	/*opcode to move with*/
	int move_size = 1;	/*number of bytes moved each time*/
			/*This can be either a block move, or
			  VLRETURN node*/
			/*For block moves, the count may or may not
			  be a scratch register; if not, a1 is available
			  for a copy;
			/*For VLRETURN, the count must be in a scratch reg;
			  A1 contains the "to" adress (%fp)*/
#ifdef VOL_SUPPORT
	int bk_vol_opnd = 0;
#endif
	switch ( pnode->in.op)
	{
	case BMOVE:
	case BMOVEO:
#ifdef VOL_SUPPORT
		if (pnode->in.strat & VOLATILE) bk_vol_opnd |= VOL_OPND2;
#endif
		pcount = pnode->in.left;
		pfrom = pnode->in.right->in.right;
		pto = pnode->in.right->in.left;
		break;
	case VLRETURN:
		pcount = pnode->in.right;
		pfrom = pnode->in.left;
		pto = &resc[0];		/*A1*/
		break;
	default:
		cerror("Bad node passed to ZM macro");
	};

			/*First, try to be smart about short structures*/
			/*We can do this if the count is a constant ,
			  the move is properly aligned,
			  and the size of the move is small enough*/

	if ( pcount->tn.op == ICON && pcount->tn.name == 0)
	{
		int i;
			/*How many bytes to move?*/
		int move_count = pcount->tn.lval;
			/*First, find out the largest opcode consistent with
			  the alignment*/
		for ( i=0; opcodes[i].alignment > align; ++i)
		{
			if ( i >= LASTOP)
				cerror("blockmove: bad alignment");
		}
			/*Is it small enough to unroll?*/
		if ( move_count / opcodes[i].count < BTHRESH)
		{
			int done;
			move_size = opcodes[i].count;
			for ( done = 0; done < move_count; )
			{
				/*Must we change op size?*/
				while ( done+move_size > move_count)
				{
					if ( i >= LASTOP)
						cerror("blockmove: bad move count");
					move_size = opcodes[++i].count;
				}
				fprintf(outfile, "\t%s\t%d(%%r%d),%d(%%r%d)\n",
				opcodes[i].opcode, done,
				pfrom->tn.rval, done, pto->tn.rval);
#ifdef VOL_SUPPORT
				if (bk_vol_opnd) fprintf(outfile,"#VOL_OPND\t2\n");
#endif
				done += move_size;
			}
			return;
		}
			/*Can't unroll. Can we use a different opcode?*/
			/*Can use an opcode if the number of bytes we move
			  is a multiple of the number of bytes the opcode
			moves.*/
		for ( ; move_count % opcodes[i].count ; ++i)
		{
			if ( i >= LASTOP)
				cerror("blockmove: Bad move count (rolled)");
		}
		opcode = opcodes[i].opcode;
		move_size = opcodes[i].count;
	}


			/*Can't be clever- must use a loop*/
			/*at this point, "opcode" is to be used to move
			  "move_size" bytes at a time*/

		/*Figure out the count*/
		/*If not in a reg, move it there*/
	if ( pcount->in.op != REG)
	{
		if (pnode->in.op == VLRETURN)
			cerror("Blockmove: VLRETURN count not in register"); 
		expand( pcount, FOREFF, "\tmovw	A.,A1", (OPTAB *) 0);
		emit_str("\n");
		pcount = &resc[0];
	}

		/*Generate the loop*/

	fprintf(outfile,"\tcmpw\t&0,%%r%d\n", pcount->tn.rval );
	fprintf(outfile,"\tjz\t.L%d\n",lastlab);
	deflab(looplab);
	fprintf(outfile,"\t%s\t0(%%r%d),0(%%r%d)\n",
		opcode,pfrom->tn.rval, pto->tn.rval );
#ifdef VOL_SUPPORT
	if (bk_vol_opnd) fprintf(outfile,"#VOL_OPND\t2\n");
#endif
	fprintf(outfile,"\taddw2\t&%d,%%r%d\n", move_size, pfrom->tn.rval );
	fprintf(outfile,"\taddw2\t&%d,%%r%d\n", move_size, pto->tn.rval );
	fprintf(outfile,"\tsubw2\t&%d,%%r%d\n", move_size, pcount->tn.rval );
        fprintf(outfile,"\tjpos\t.L%d\n", looplab );
	deflab(lastlab);

}
/*ARGSUSED*/
static void
blockcmp(pnode)
NODE *pnode;
{
#if 0   /* turn off this function because it is not used by ANSI C and C++ */
	int lastlab = getlab();
	int looplab = getlab();
	NODE *count, *from, *to;
		/*Stolen from the STASG code.*/
		/*All three inputs are in scratch registers*/
	from = pnode->in.right->in.right;
	to = pnode->in.right->in.left;

		/*Figure out the count*/
	count = pnode->in.left;

			/*generate the loop*/
	deflab(looplab);
        fprintf(outfile,"\tcmpw\t&0,%s\n",rnames[count->tn.rval]);
        fprintf(outfile,"\tjz\t.L%d\n",lastlab);
	fprintf(outfile,"\tcmpb\t0(%s),0(%s)\n",
		rnames[from->tn.rval], rnames[to->tn.rval]);
        fprintf(outfile,"\tjne\t.L%d\n",lastlab);
        fprintf(outfile,"\tsubw2\t&1,%s\n",rnames[count->tn.rval]);
        fprintf(outfile,"\taddw2\t&1,%s\n",rnames[from->tn.rval]);
        fprintf(outfile,"\taddw2\t&1,%s\n",rnames[to->tn.rval]);
	fprintf(outfile,"\tjmp\t.L%d\n",looplab);
	deflab(lastlab);
#endif
}
/*ARGSUSED*/
static void
prep_offset(p)
NODE *p;
{
#if 0   /* turn off this function because it is not used by ANSI C and C++ */
			/*temp is a select node. arrange for the proper pointer
			  to be in a register, and save the "expand" string
			  in off_reg.*/
	int upper;
	if ( p->in.op != FSELECT)
		cerror("Bad arg to Zf macro");
			/*remember whether we want the upper(%fp) or lower(%ap)
			  half */
	upper = (p->in.right->in.op != VPARAM);
	rg_load(p->in.left,upper);
#endif
}
/*ARGSUSED*/
static void
rg_load(p,upper)
NODE *p;
int upper;		/*flag: upper half*/
			/*load a register with the appropriate half of a
			  frame pointer*/
{
#if 0   /* turn off this function because it is not used by ANSI C and C++ */
			/*if already in a register, do nothing but remember*/
	if ( p->in.op == REG )
	{
		off_reg = (upper ? "IL" : "AL" );
		return;
	}
			/*Otherwise, must move it into a register.*/
	off_reg = "A1";
	if ( upper)
	{
		expand(p, FOREFF, "	movw	I.,A1\n", (OPTAB *)0);
	}
	else
	{
		expand(p, FOREFF, "	movw	A.,A1\n", (OPTAB *)0);
	}
#endif
}
/*ARGSUSED*/
static void
fselect(p)
NODE *p;
{
#if 0   /* turn off this function because it is not used by ANSI C and C++ */
				/*off_reg contains the expand string of the
				  pointer (done by Zf macro).
				  right is temp/auto/param with offset*/
	if ( p->in.op != FSELECT)
		cerror("Bad ZF macro argument");
	fprintf(outfile,"%d(", p->in.right->tn.lval);
	/*
	** here, all we really want to say is 
	** "expand( p, FOREFF, off_reg, (OPTAB *) 0);". However, if 
	** we're honoring exceptions, expand() calls ex_before() and we
	** will get a NOP printed in the middle of the (MAU) instruction
	** operand list. The following method is faster anyway ...
	*/
	if ( off_reg[0] == 'A' )
		adrput(getlr(p,off_reg[1]));
	else if ( off_reg[0] == 'I' )
		insput(getlr(p,off_reg[1]));
	else
		cerror("CG function fselect(): bad value for off_reg"); 
	fprintf(outfile,")");
#endif
}
/*ARGSUSED*/
static void
hfselect(p)
NODE *p;
{
#if 0   /* turn off this function because it is not used by ANSI C and C++ */
				/*select the high word of a double.
				  off_reg contains the expand string of the
				  pointer (done by Zf macro).
				  right is temp/auto/param with offset*/
	if ( p->in.op != FSELECT)
		cerror("Bad ZF macro argument");
	fprintf(outfile,"%d(", p->in.right->tn.lval + 4);
	/*
	** here, all we really want to say is 
	** "expand( p, FOREFF, off_reg, (OPTAB *) 0);". However, if 
	** we're honoring exceptions, expand() calls ex_before() and we
	** will get a NOP printed in the middle of the (MAU) instruction
	** operand list. The following method is faster anyway ...
	*/
	if ( off_reg[0] == 'A' )
		adrput(getlr(p,off_reg[1]));
	else if ( off_reg[0] == 'I' )
		insput(getlr(p,off_reg[1]));
	else
		cerror("CG function hfselect(): bad value for off_reg"); 
	fprintf(outfile,")");
#endif
}
/*ARGSUSED*/
static void raisex(p)
NODE *p;
{
#if 0   /* turn off this function, because it is only for exception handler
	   of ADA  
	*/
			/*Raise this exception*/
	fprintf(outfile,"\n\tjsb\t*_exvect\n");
#endif
}
/*ARGSUSED*/
static void testex(p)
NODE *p;
{
#if 0   /* turn off this function, because it is only for exception handler
	   of ADA  
	*/
			/*Generate code to test for numeric exception;
			  if the overflow bit is set, branch to the handler.*/
			/*NOTE: This does NOT WORK for integer multiply, since
			  the 3B20 does not set the bit correctly. On the assump-
			  tion that this will be fixed, we go ahead and do this
			  test.*/
	int skiplabel;
	skiplabel = getlab();
	fprintf(outfile,"\n\tjvc	");
	fprintf(outfile,LABFMT, skiplabel);
	fprintf(outfile,"\n\tjsb\t*_exvect\n");
	deflab(skiplabel);
#endif
}
void
bfcode(p)			/* begin function code. a is array of n stab */
NODE *p;
{
	int temp; 

	fprintf(outfile,"	save	&.R%d\n", ftnno);
#if 0   /* turn off this function, because it is only for exception handler
	   of ADA  
	*/
	if ( p -> in.strat & EXHONOR )
	{
		int label1;
		fprintf(outfile,"	subw3	%%sp,_stk_lim,%%r0\n");
		fprintf(outfile,"	cmpw	&.F%d,%%r0\n",ftnno);
		fprintf(outfile,"	jle	.L%d\n", label1 = getlab() );
		fprintf(outfile,"	call	&0,*_stk_ov\n");
		deflab( label1 );
	}
#endif
	if (proflag)
	{
		(void)locctr(DATA);
		temp = getlab();
		emit_str("\t.align\t4\n");
		deflab(temp);
		emit_str("\t.word	0\n");
		(void)locctr(PROG);
		if (picflag) {
		    fprintf(outfile,"	movaw	.L%d@PC,%%r0\n",temp);
		    emit_str("	jsb	*_mcount@GOT\n");
		}
		else {
		    fprintf(outfile,"	movw	&.L%d,%%r0\n",temp);
		    emit_str("	jsb	_mcount\n");
		}
	}
	fprintf(outfile,"	addw2	&.F%d,%%sp\n", ftnno);
                        /*If this entry returns structure function, get
                          pointer from %r2 and save on the stack*/
        if (p->in.type == TSTRUCT)
        {
                        /*Save place for structure return on stack*/
                strftn = 1;
                fprintf(outfile,"	movw	%s,%d(%%fp)\n",rnames[AUXREG],str_spot);
        }
}

#define	HONOR		1
#define IGNORE		2
#define DONT_KNOW	3
/*ARGSUSED*/
void
ex_before(p, goal, q)
NODE *p;	/* pointer to root node of sub-tree 	*/
int goal;	/* NOT USED - required by definition	*/
OPTAB *q;	/* NOT USED - required by definition	*/
{
#if 0   /* turn off this function, because it is only for exception handler
	   of ADA  
	*/
	static int state = DONT_KNOW;

#ifdef MAU
	if ( state == HONOR )
	{
	/* because of a late hardware exception on integer overflow, a fault
	** caused by a SPOP ( MAU instruction ) might be ambiguous. see
	** MR ub85-35104. In lieu of eliminating the delay in in the
	** integer overflow trap or placing workarounds in the kernal,
	** place a NOP before any SPOP which follows an exceptable node.
	*/
		if ((( p->in.type == TDOUBLE ) ||( p->in.type == TFLOAT ))
			&&( p->in.op != CONV ))
			fprintf( outfile, "\tNOP\n");
	}
#endif	/* ifdef MAU */

/* Produce run-time code for SIGFPE exception handling.
** Set oe bit with iotrap{on|off} to indicate whether you're
** honoring SIGFPE exceptions. iotrap{on|off} must be loaded at
** run time. Labels
** are always "DONT_KNOW" because they may be jumped to from
** a node which does or does not honor exceptions.
*/
	if( p->in.op == GENLAB )
	{
		state = DONT_KNOW;
	}
	else if(( p->in.strat & EXHONOR ) && ( state != HONOR ))
	{
		fprintf(outfile,"%s\n\tcall\t&0,iotrapon\n%s\n",ASM_COMMENT,ASM_END);
		state = HONOR;
	}
	else if(( p->in.strat & EXIGNORE ) && ( state != IGNORE ))
	{
		fprintf(outfile,"%s\n\tcall\t&0,iotrapoff\n%s\n",ASM_COMMENT,ASM_END);
		state = IGNORE;
	}
#endif
}

#ifdef VOL_SUPPORT

/* output volatile operand information at the end of an instruction */
void
vol_instr_end()
{
	int opnd;
	int first = 1;

	for (opnd=0; vol_opnd; ++opnd)
	{
	    if (vol_opnd & (1<<opnd))
	    {
		   /* first time output the information for the instruction */
		   if ( first )			
		   {
			PUTS("#VOL_OPND	");
		   	first = 0;
		   }
		   else
			PUTCHAR(',');

		   vol_opnd &= ~(1<<opnd);	/* clean up the checked operand bit */
		   fprintf(outfile, "%d", opnd+1);
	    }
	}
	if ( !first ) PUTCHAR('\n');
	VOL_CLEAN();	/* reset the initial values for bookkeeping variables */
}

#endif /* VOL_SUPPORT */


/* Routines to support HALO optimizer. */

#ifdef	OPTIM_SUPPORT

#ifndef	INI_OIBSIZE
#define	INI_OIBSIZE 100
#endif

static char oi_buf_init[INI_OIBSIZE];	/* initial buffer */
static
TD_INIT(td_oibuf, INI_OIBSIZE, sizeof(char), 0, oi_buf_init, "optim support buf");

#define	OI_NEED(n) if (td_oibuf.td_used + (n) > td_oibuf.td_allo) \
			td_enlarge(&td_oibuf, td_oibuf.td_used+(n)) ;
#define	OI_BUF ((char *)(td_oibuf.td_start))
#define	OI_USED (td_oibuf.td_used)

/* Produce comment for loop code. */

char *
oi_loop(code)
int code;
{
    char * s;

    switch( code ) {
    case OI_LSTART:	s = "#LOOP	BEG\n"; break;
    case OI_LBODY:	s = "#LOOP	HDR\n"; break;
    case OI_LCOND:	s = "#LOOP	COND\n"; break;
    case OI_LEND:	s = "#LOOP	END\n"; break;
    default:
	cerror("bad op_loop code %d", code);
    }
    return( s );
}


/* Analog of adrput, but this one takes limited address modes (leaves
** only) and writes to a buffer.  It returns a pointer to just past the
** end of the buffer.
*/
static void
sadrput(p)
NODE * p;				/* node to produce output for */
{
    int n;

    /* Assume need space for auto/param at a minimum. */
    /*      % n ( % fp) NUL */
    OI_NEED(1+8+1+1+2+1+1);

    switch( p->tn.op ){
    case VAUTO:		OI_USED += sprintf(OI_BUF+OI_USED, "%ld(%%fp)",
								p->tn.lval);
			break;
    case VPARAM:	OI_USED += sprintf(OI_BUF+OI_USED, "%ld(%%ap)",
								p->tn.lval);
			break;
    case NAME:		n = strlen(p->tn.name);
			OI_NEED(n+1);
			(void) strcpy(OI_BUF+OI_USED, p->tn.name);
			OI_USED += n;
			if (p->tn.lval != 0) {
			    OI_NEED(1+8+1);
			    OI_USED += sprintf(OI_BUF+OI_USED, "%s%ld",
					(p->tn.lval > 0 ? "+" : ""),
					p->tn.lval);
			}
			if (picflag) {
			    register flag = p->tn.rval;
			    OI_NEED(4+1);
			    OI_USED += sprintf(OI_BUF+OI_USED, "%s",
				((flag & NI_GLOBAL)?"@GOT":
				    ((flag & (NI_FLSTAT|NI_BKSTAT))?"@PC":"")));
			}
			break;
    default:
	cerror("bad op %d in sadrput()", p->in.op);
    }
    return;
}

#ifndef	TLDOUBLE
#define	TLDOUBLE TDOUBLE
#endif

/* Note that the address of an object was taken. */

char *
oi_alias(p)
NODE * p;
{
    BITOFF size = (p->tn.type & (TVOID|TSTRUCT)) ? 0 : gtsize(p->tn.type);

    OI_USED = 0;			/* start buffer */
    /*	    #ALIAS\t	*/
    OI_NEED(1+   5+1+1);
    OI_USED += sprintf(OI_BUF, "#ALIAS	");
    sadrput(p);
    /*	    \t% n\tFP\n */
    OI_NEED(1+1+8+1+2+1+1);
    (void) sprintf(OI_BUF+OI_USED, "	%ld%s\n", size/SZCHAR,
		(long)(p->tn.type & (TFLOAT|TDOUBLE|TLDOUBLE)) ? "	FP" : "");
    return( OI_BUF );
}

/* Produce #REGAL information for a symbol. */

char *
oi_symbol(p, class)
NODE * p;
int class;
{
    char * s_class;

    switch( class ) {
    case OI_AUTO:	s_class = "AUTO"; break;
    case OI_PARAM:	s_class = "PARAM"; break;
    case OI_EXTERN:	s_class = "EXTERN"; break;
    case OI_EXTDEF:	s_class = "EXTDEF"; break;
    case OI_STATEXT:	s_class = "STATEXT"; break;
    case OI_STATLOC:	s_class = "STATLOC"; break;
    default:
	cerror("bad class %d in op_symbol", class);
    }

    OI_USED = 0;			/* initialize */
    /*		#REGAL\t 0\tSTATLOC\t	*/
    OI_NEED(	1+   5+1+1+1+     7+1+1 );
    OI_USED += sprintf(OI_BUF, "#REGAL	0	%s	", s_class);
    sadrput(p);
    /*	    \t% n\tFP\n */
    OI_NEED(1+1+8+1+2+1+1);
    (void) sprintf(OI_BUF+OI_USED, "	%d%s\n", gtsize(p->tn.type)/SZCHAR,
		(p->tn.type & (TFLOAT|TDOUBLE|TLDOUBLE)) ? "	FP" : "");
    return( OI_BUF );
}

#endif	/* def OPTIM_SUPPORT */
