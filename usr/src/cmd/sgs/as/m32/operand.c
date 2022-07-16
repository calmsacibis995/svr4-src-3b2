/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/operand.c	1.7"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "as_ctype.h"
#include "symbols.h"
#include "program.h"
#include "instab.h"
/*#include <sys/types.h>*/

extern char	*oplexptr;	/* set up for oplex() by yyparse() */
extern void yyerror();
extern void treeleaf();
extern void treenode();
extern OPERAND *merge();
extern int validfp();
extern symbol *dot;

static int expr1();
symbol *lookup();

static symbol *zero_symbol;
/* symbol to be used for the special case such as sym@PC where sym is ABSOLUTE.
 * This will result in a relocation entry being generated which has symbol
 * table index of 0
 */

extern OPERAND *opertop;/* bottom of expression stack & target for current operand */
static OPERAND	*tiptop;/* top of the expression stack, always starts at opertop */

/* expr1() return codes */
#define ERROR	-1
#define FNDEXPR  0
#define FNDFLOAT 1

/* for expression parsing by routines other than operand() (which calls expr1())  */
expr()
{

	tiptop = opertop;
	switch (expr1()) {
	case FNDEXPR  : return(*oplexptr);
	case FNDFLOAT : yyerror("expecting expression not float constant");
	default	      : return(-1);
	}
}


/* for use by routines other than operand() to parse floating point expressions */
fp_expr()
{

	tiptop = opertop;
	switch (expr1()) {
	case FNDFLOAT: return(*oplexptr);
	case FNDEXPR : if (opertop->fasciip != NULL) return(*oplexptr);
		       yyerror("expecting float constant not expression");
	default	     : return(-1);
	}
}


struct exptype { char *name; int typno; };
static struct exptype exptypes[] = {
			{"byte",	0x7},
			{"half",	0x6},
			{"sbyte",	0x7},
			{"shalf",	0x6},
			{"sword",	0x4},
			{"ubyte",	0x3},
			{"uhalf",	0x2},
			{"uword",	0x0},
			{"word",	0x4},
			0
		};

static find_exptype(name)
char *name;
{
	register struct exptype *curtype;

	for (curtype = exptypes; curtype->name != NULL; curtype++)
		if (!strcmp(name, curtype->name)) return(curtype->typno);
	return(NOTYPE);	/* expanded type not found */
}

/* operand tokens - used by operand(), expr1(), and oplex() */
#define	ID	0200
#define REG	0201
#define NUM	0202
#define FPNUM	0203
#define FREG	0204
#define SREG	0205
#define DREG	0206
#define XREG	0207

/* operand parser - returns 0 on success */
operand()
{
	register mode = 0;
	static int oplex();

	tiptop = opertop;
	opertop->newtype = NOTYPE;
	opertop->reg = 0;
	opertop->symptr = NULL;
	while (*oplexptr == '{') {
		register char *aheadptr;

		for (aheadptr = ++oplexptr; IDBEGIN(*aheadptr); aheadptr++) ;
		if (*aheadptr != '}') {
			yyerror("expecting '}'");
			return(-1);
		} 
		*aheadptr = '\0';
		if ((opertop->newtype = find_exptype(oplexptr)) == NOTYPE) {
			*aheadptr = '}';
			yyerror("Unknown expanded type operand");
			return(-1);
		}
		*aheadptr = '}';
		oplexptr = aheadptr + 1;
	}
	switch (*oplexptr) {
		case '(':
			if (oplexptr[1] != '%') {
				mode = EXADMD;
				break;
			}
			oplexptr++;
			if (oplex() != REG || *oplexptr++ != ')') return(-1);
			opertop->expspec = NULLSPEC;
			opertop->type = REGDFMD;
			return(*oplexptr);
		case '%':
			switch (oplex()) {
			case REG : opertop->type = REGMD; break;
			case FREG : opertop->type = FREGMD; break;
			case SREG : opertop->type = SREGMD; break;
			case DREG : opertop->type = DREGMD; break;
			case XREG : opertop->type = XREGMD; break;
			default : return(-1);
			}
			if (opertop->type != REGMD && opertop->newtype != NOTYPE)
				return(-1);
			opertop->expspec = NULLSPEC;
			return(*oplexptr);
		case '&':
			mode = IMMD;
			oplexptr++;
			break;
		case '*':
			switch (*++oplexptr) {
			case '%':
				oplexptr++;
				if (oplex() != REG) return(-1);
				opertop->expspec = NULLSPEC;
				opertop->type = REGDFMD;
				return(*oplexptr);
			case '$':
				mode = ABSDFMD;
				oplexptr++;
				break;
			default :
				mode = EXADDFMD;
				break;
			}
			break;
		case '$':
			mode = ABSMD;
			oplexptr++;
			break;
		default:	/* just expr */
			mode = EXADMD;
	}
	/* expr */
	switch (expr1()) {
	case FNDFLOAT:
		if (mode == IMMD && opertop->newtype == NOTYPE) {
			opertop->type = FPIMMD;
			opertop->expspec = NULLSPEC;
			return(*oplexptr);
		}
	/* FALLTHROUGH */
	case ERROR: return(-1);
	}
					
	/* (reg) */
	if (*oplexptr == '(') {
		oplexptr++;
		if (oplex() == REG && *oplexptr++ == ')') {
			if (mode == EXADDFMD) {
				opertop->type = DSPDFMD;
			} else if (mode == EXADMD) {
				opertop->type = DSPMD;
			} else return(-1);
			opertop->reg = (tiptop-1)->reg;
			if (*oplexptr == '<') {
				switch (*++oplexptr) {
				case 'B' : opertop->expspec = BYTESPEC; break;
				case 'H' : opertop->expspec = HALFSPEC; break;
				case 'W' : opertop->expspec = WORDSPEC; break;
				case 'L' : opertop->expspec = LITERALSPEC; break;
				case 'S' : opertop->expspec = SHORTSPEC; break;
				default : return(-1);
				}
				if (*++oplexptr != '>') return(-1);
				oplexptr++;
			}
			return(*oplexptr);
		} else return(-1);
	}
	if (mode == IMMD && *oplexptr == '<') {
		switch (*++oplexptr) {
		case 'B' : opertop->expspec = BYTESPEC; break;
		case 'H' : opertop->expspec = HALFSPEC; break;
		case 'W' : opertop->expspec = WORDSPEC; break;
		case 'L' : opertop->expspec = LITERALSPEC; break;
		case 'S' : opertop->expspec = SHORTSPEC; break;
		default : return(-1);
		}
		if (*++oplexptr != '>') return(-1);
		oplexptr++;
	} else opertop->expspec = NULLSPEC;
	opertop->type = (unsigned char) mode;
	if (*oplexptr == '@') {
		switch(*(++oplexptr)) {
			case 'P': if (*(++oplexptr) == 'C'){
					opertop->pic_rtype = PC;
				  }
				  else if ((*oplexptr == 'L') && 
					(*(++oplexptr) == 'T')) {
					opertop->pic_rtype = PLT;
				  }
				  else {
					yyerror("invalid PIC mode\n");
					oplexptr--;
				       }
				  break;
			case 'G': if ((*(++oplexptr) == 'O') &&
					(*(++oplexptr) == 'T')) {
					opertop->pic_rtype = GOT;
				}
				  else {
					yyerror("invalid PIC mode\n");
					oplexptr--;
				       }
				  break;
			default: yyerror("invalid PIC mode\n");
		}  /* switch */
		oplexptr++;
		/* when doing PIC relocation, absolute mode is mapped
		 * to displacment mode and absolute deferred mode is
		 * mapped to displacement deferred mode
		 */
		switch (opertop->type) {
			case EXADMD:
				opertop->type = DSPMD;
				break;
			case  EXADDFMD:
				opertop->type = DSPDFMD;
				break;
			default:
				yyerror("invalid PIC addressing mode");
		}
		if (!(opertop->reg))
			opertop->reg = PCREG; 	/* PC Relative */
		else
			yyerror("invalid PIC register");
		if (ABSOLUTE(*opertop) && opertop->pic_rtype == PC){
			opertop->expval = 0;
			opertop->symptr = zero_symbol;
		}
	}
	return(*oplexptr);
}

#if DEBUG
static tree_visit(root)
EXPTREE *root;
{
	if (root == NULL) return;
	if (root->is_leaf) {
		if (root->t_leaf.symptr && root->t_leaf.value)
			(void) printf("(%s+%d) ",root->t_leaf.symptr->name,
				  root->t_leaf.value);
		else if (root->t_leaf.symptr)
			(void) printf("%s ",root->t_leaf.symptr->name);
		else
			(void) printf("%d ",root->t_leaf.value);
	} else {
		(void) printf("<%d> ",root->t_node.op);
		tree_visit(root->t_node.left);
		tree_visit(root->t_node.right);
	}
}

extern unsigned short line;
/*
expr1() calls expr2(), visits any tree that may have been built,
then returns what expr2() returned.
*/
static expr1()
{
	int retval;
	retval = expr2();
	(void) printf("%d:\t",line);
	if (UNEVALUATED(*opertop)) {
		tree_visit((EXPTREE *) opertop->symptr);
		(void) printf(" - tree\n");
	} else {
		if (opertop->symptr)
			(void) printf("(%s=%d + %d)\n",opertop->symptr->name,
				opertop->symptr->value,opertop->expval);
		else (void) printf("%d\n",opertop->expval);
	}
	return(retval);
}

static expr2()
#else
static expr1()
#endif /* DEBUG */
{
/*
Parse an expression and return:

	FNDFLT if a floating point constant was found
	FNDEXP for an expression
	ERROR if an error is encountered
*/
#define	EXPRERR(x)	{yyerror(x);\
			(tiptop-1)->expval = 0;\
			(tiptop-1)->symptr = NULL;\
			(tiptop-1)->exptype = ABS;}

	register op = 0, min = 0;

	for (;;) {
		while (*oplexptr == '-') {	/* unary minus */
			min ^= 1;
			oplexptr++;
		}
		switch (oplex()) {
		case FPNUM:
			if (tiptop != opertop+1) /* only [-]* <fpnum> allowed*/
				return(ERROR);
			if (min) {
				(tiptop-1)->expval |= ((unsigned) 1L << 31);
				*(--(tiptop-1)->fasciip) = '-';
			}
			return(FNDFLOAT);
		case '(':
			if (
#if DEBUG
				expr2()
#else
				expr1()
#endif
			!= FNDEXPR)
				return(ERROR);
			if (*oplexptr != ')') {
				yyerror("unbalanced parentheses");
				return(ERROR);
			}
			(tiptop-1)->fasciip = NULL;
			oplexptr++;
		/* FALLTHROUGH */
		case ID:
		case NUM:
			if (min) {
				min = 0;
				if ((tiptop-1)->fasciip != NULL)
					*(--(tiptop-1)->fasciip) = '-';
				/* expression must eventually be absolute */
				if (ABSOLUTE(*(tiptop-1))) {
					(tiptop-1)->expval = - (tiptop-1)->expval;
					break;
				} else if (RELOCATABLE(*(tiptop-1))) {
					EXPRERR("Illegal unary minus")
					break;
				}
				/* else build tree */
				if (UNDEFINED(*(tiptop-1)))
					treeleaf(tiptop-1);
				/* und operand has become uneval */
				if (UNEVALUATED(*(tiptop-1)))
					treenode(UMINUS_OP,(OPERAND *) NULL,tiptop-1,tiptop-1);
			}
			if (op) {	/* perform op */
				tiptop--;
				switch (op) {
				case '+':
					/* one side of expr must be absolute */
					if (ABSOLUTE(*tiptop)&&!UNEVALUATED(*(tiptop-1))) {
						/*abs+abs, rel+abs or und+abs*/
						(tiptop-1)->expval += tiptop->expval;
						break;
					} else if (ABSOLUTE(*(tiptop-1))&&!UNEVALUATED(*tiptop)) {
						/* abs+rel or abs+und */
						(tiptop-1)->expval += tiptop->expval;
						(tiptop-1)->symptr = tiptop->symptr;
						(tiptop-1)->exptype = tiptop->exptype;
						break;
					} else if (RELOCATABLE(*(tiptop-1)) && RELOCATABLE(*tiptop)) {
						EXPRERR("Illegal addition")
						break;
					}
					/* else build tree */
					if (UNDEFINED(*(tiptop-1)))
						/*und+rel or und+und or und+uneval*/
						treeleaf(tiptop-1);
					if (UNDEFINED(*tiptop))
						/*rel+und or und+und or uneval+und*/
						treeleaf(tiptop);
					/* und operands have become uneval */
					if (UNEVALUATED(*(tiptop-1))||UNEVALUATED(*tiptop)) {
						if (ABSOLUTE(*(tiptop-1))||ABSOLUTE(*tiptop))
							/*abs+uneval or uneval+abs*/
							(void) merge(tiptop-1,tiptop);
						else {
							if (!UNEVALUATED(*(tiptop-1)))
								/*rel+uneval*/
								treeleaf(tiptop-1);
							else if (!UNEVALUATED(*tiptop))
								/*uneval+rel*/
								treeleaf(tiptop);
							treenode(PLUS_OP,tiptop-1,tiptop,tiptop-1);
						}
					} else
						EXPRERR("Illegal addition")
					break;
				case '-':
					if (ABSOLUTE(*(tiptop-1)) && ABSOLUTE(*tiptop)) {
						/* abs-abs */
						(tiptop-1)->symptr = NULL;
						(tiptop-1)->exptype = ABS;
						(tiptop-1)->expval -= tiptop->expval;
						break;
					}
					else if (!UNEVALUATED(*(tiptop-1))&&ABSOLUTE(*tiptop)){
						/* rel-abs or und-abs */
						(tiptop-1)->expval -= tiptop->expval;
						break;
					}
					else if (ABSOLUTE(*(tiptop-1)) && RELOCATABLE(*tiptop)){
						/* abs-rel */
						EXPRERR("Illegal subtraction")
						break;
					}
					/* else build tree */
					if (UNDEFINED(*(tiptop-1)))
						/*und-rel, und-und, or und-uneval*/
						treeleaf(tiptop-1);
					if (UNDEFINED(*tiptop))
						/*abs-und, rel-und, und-und, or uneval-und*/
						treeleaf(tiptop);
					/* und operands have become uneval */
					if (UNEVALUATED(*(tiptop-1)) || UNEVALUATED(*tiptop)) {
						if (ABSOLUTE(*tiptop)){
							/*uneval-abs*/
							tiptop->expval = -tiptop->expval;
							(void) merge(tiptop-1,tiptop);
						} else {
							if (!UNEVALUATED(*(tiptop-1)))
							/*abs-uneval or rel-uneval*/
								treeleaf(tiptop-1);
							else if (!UNEVALUATED(*tiptop))
								/*uneval-rel*/
								treeleaf(tiptop);
							treenode(MINUS_OP,tiptop-1,tiptop,tiptop-1);
						}
					}
					else if ((tiptop-1)->symptr->sectnum==tiptop->symptr->sectnum) { 
						/* rel-rel */
						treeleaf(tiptop-1);
						treeleaf(tiptop);
						treenode(MINUS_OP,tiptop-1,tiptop,tiptop-1);
					} else
						EXPRERR("Illegal subtraction")
					break;
				case '*':
					if (ABSOLUTE(*(tiptop-1))&&ABSOLUTE(*tiptop)){
						/* abs*abs */
						(tiptop-1)->symptr = NULL;
						(tiptop-1)->exptype = ABS;
						(tiptop-1)->expval *= tiptop->expval;
						break;
					} else if (RELOCATABLE(*(tiptop-1)) || RELOCATABLE(*tiptop)){
						/* rel*abs, rel*rel, rel*und, rel*uneval,
						abs*rel, und*rel, or uneval*rel */
						EXPRERR("Illegal multiplication");
						break;
					}
					/* else build tree */
					if (UNDEFINED(*(tiptop-1)))
						/* und*abs, und*und, or und*uneval */
						treeleaf(tiptop-1);
					if (UNDEFINED(*tiptop))
						/* abs*und, und*und, or uneval*und */
						treeleaf(tiptop);
					if (UNEVALUATED(*(tiptop-1)) || UNEVALUATED(*tiptop)) {
						if (!UNEVALUATED(*(tiptop-1)))
							/* abs*uneval */
							treeleaf(tiptop-1);
						else if (!UNEVALUATED(*tiptop))
							/* uneval*abs */
							treeleaf(tiptop);
						treenode(MULT_OP,tiptop-1,tiptop,tiptop-1);
					}
					break;
				case '/':
					if (ABSOLUTE(*(tiptop-1))&&ABSOLUTE(*tiptop)){
						/* abs/abs */
						(tiptop-1)->symptr = NULL;
						(tiptop-1)->exptype = ABS;
						(tiptop-1)->expval /= tiptop->expval;
						break;
					} else if (RELOCATABLE(*(tiptop-1)) || RELOCATABLE(*tiptop)){
						/* rel/abs, rel/rel, rel/und, rel/uneval,
						abs/rel, und/rel, or uneval/rel */
						EXPRERR("Illegal division");
						break;
					}
					/* else build tree */
					if (UNDEFINED(*(tiptop-1)))
						/* und/abs, und/und, or und/uneval */
						treeleaf(tiptop-1);
					if (UNDEFINED(*tiptop))
						/* abs/und, und/und, or uneval/und */
						treeleaf(tiptop);
					if (UNEVALUATED(*(tiptop-1)) || UNEVALUATED(*tiptop)) {
						if (!UNEVALUATED(*(tiptop-1)))
							/* abs/uneval */
							treeleaf(tiptop-1);
						else if (!UNEVALUATED(*tiptop))
							/* uneval/abs */
							treeleaf(tiptop);
						treenode(DIVIDE_OP,tiptop-1,tiptop,tiptop-1);
					}
					break;
				}
				(tiptop-1)->fasciip = NULL;
			}
			break;
		default:
			return(ERROR);
		}
		switch(*oplexptr) {
		case '+': op = '+'; oplexptr++; break;
		case '-': op = '-'; oplexptr++; break;
		case '*': op = '*'; oplexptr++; break;
		case '/': op = '/'; oplexptr++; break;
		default: return(FNDEXPR);
		}
	}
}


/* lexical analyzer for operand tokens */
static
oplex()
{
	register char	*ahead = oplexptr;
	register char	tmpchar;

	switch (*ahead) {
	case 'A': case 'B': case 'C': case 'D': case 'E':
	case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O':
	case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y':
	case 'Z': case 'a': case 'b': case 'c': case 'd':
	case 'e': case 'f': case 'g': case 'h': case 'i':
	case 'j': case 'k': case 'l': case 'm': case 'n':
	case 'o': case 'p': case 'q': case 'r': case 's':
	case 't': case 'u': case 'v': case 'w': case 'x':
	case 'y': case 'z': case '_': case '.':
	{
		register symbol *symptr;

		while (IDFOLLOW(*++ahead)) ;
		if (tmpchar = *ahead) {
			*ahead = '\0';
			symptr = lookup(oplexptr, INSTALL);
			*ahead = tmpchar;
		} else
			symptr = lookup(oplexptr, INSTALL);
		oplexptr = ahead;
		if ((tiptop->exptype = get_sym_exptype(symptr)) == ABS) {
			/* symbol is absolute */
			tiptop->expval = symptr->value;
			zero_symbol = symptr;
			tiptop->symptr = NULL;
		} else {
			/* symbol is relocatable, undefined, or unevaluated */
			tiptop->symptr = symptr;
			tiptop->expval = 0;
			tiptop->pic_rtype = 0;
							
		}
		tiptop->fasciip = NULL;
		tiptop->unevaluated = 0;
		tiptop++;
		return(ID);
	}
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	{
		register long val;
		register base;
		register char c;

		if ((val = *ahead - '0') == 0) {
			c = *++ahead;
			if (c == 'x' || c == 'X') {
				base = 16;
				val = 0;
				while (HEXCHAR(c = *++ahead)) {
					val <<= 4;
					if ('a' <= c && c <= 'f')
						val += 10 + c - 'a';
					else if ('A'<= c && c <= 'F')
						val += 10 + c - 'A';
					else	
						val += c - '0';
					}
			} else {
				if (c == '.' || c == 'E' || c == 'e')
					base = 10;	/* fp number */
				else if (OCTCHAR(c)) {
					base = 8;
					val = c - '0';
					while(OCTCHAR(c = *++ahead)) {
						val <<= 3;
						val += c - '0';
					}
				} else {	/* just 0 */
					val = 0;
				}
			}
		} else {
			base = 10;
			while (DECCHAR(c = *++ahead)) {
				val *= 10;
				val += c - '0';
			}
		}
		/* check if its a fp number */
		if ((base == 10) && (c == '.' || c == 'E' || c == 'e')) {
			do {
				if (c == 'E' || c == 'e') {
					/* exp for fp number, next char might
					   be a sign (PLUS/MINUS) so get it here */
					if (((c = *++ahead) != '+') && (c != '-'))
						ahead--;
				}
			} while (isdigit(c = *++ahead) || isalpha(c));

			*ahead = '\0';
			if (validfp(oplexptr)) {
				tiptop->fasciip = oplexptr;
				*(oplexptr = ahead) = c;
				tiptop->symptr = NULL;
				tiptop->unevaluated = 0;
				tiptop++;
				return(FPNUM);
			} else {
				yyerror("invalid floating point constant");
				(void) fprintf(stderr, "\t\t... float constant: \"%s\"\n", oplexptr);
				*(oplexptr = ahead) = c;
				return(-1);
			}
		} else {
			*ahead = '\0';
			tiptop->fasciip = oplexptr;
			*(oplexptr = ahead) = c;
			tiptop->exptype = ABS;
			tiptop->expval = val;
			tiptop->symptr = NULL;
			tiptop->unevaluated = 0;
			tiptop++;
			return(NUM);
		}
	}
	case '%' : { register int regcode = REG;
		switch (*++oplexptr){
		case 'r':
			switch(*++oplexptr) {
			case '0': tiptop->reg = 0; break;
			case '1': switch (*++oplexptr) {
				  case '0' : tiptop->reg = 10; break;
				  case '1' : tiptop->reg = 11; break;
				  case '2' : tiptop->reg = 12; break;
				  case '3' : tiptop->reg = 13; break;
				  case '4' : tiptop->reg = 14; break;
				  case '5' : tiptop->reg = 15; break;
				  default  : oplexptr--; tiptop->reg = 1; break;
				  }
				  break;
			case '2': tiptop->reg = 2; break;
			case '3': tiptop->reg = 3; break;
			case '4': tiptop->reg = 4; break;
			case '5': tiptop->reg = 5; break;
			case '6': tiptop->reg = 6; break;
			case '7': tiptop->reg = 7; break;
			case '8': tiptop->reg = 8; break;
			case '9': tiptop->reg = 9; break;
			default : goto badreg;
			}
			break;
		case 'a':
			if (*++oplexptr == 'p')
				tiptop->reg = 10;
			else
				goto badreg;
			break;
		case 'f':
			switch (*++oplexptr) {
			case 'p' : tiptop->reg = 9; break;
			case '0' : tiptop->reg = 0; regcode = FREG; break;
			case '1' : tiptop->reg = 1; regcode = FREG; break;
			case '2' : tiptop->reg = 2; regcode = FREG; break;
			case '3' : tiptop->reg = 3; regcode = FREG; break;
			case '4' : tiptop->reg = 4; regcode = FREG; break;
			case '5' : tiptop->reg = 5; regcode = FREG; break;
			case '6' : tiptop->reg = 6; regcode = FREG; break;
			case '7' : tiptop->reg = 7; regcode = FREG; break;
			default  : goto badreg;
			}
			break;
		case 's':
			switch (*++oplexptr) {
			case 'p' : tiptop->reg = 12; break;
			case '0' : tiptop->reg = 0; regcode = SREG; break;
			case '1' : tiptop->reg = 1; regcode = SREG; break;
			case '2' : tiptop->reg = 2; regcode = SREG; break;
			case '3' : tiptop->reg = 3; regcode = SREG; break;
			case '4' : tiptop->reg = 4; regcode = SREG; break;
			case '5' : tiptop->reg = 5; regcode = SREG; break;
			case '6' : tiptop->reg = 6; regcode = SREG; break;
			case '7' : tiptop->reg = 7; regcode = SREG; break;
			default  : goto badreg;
			}
			break;
		case 'x':
			switch (*++oplexptr) {
			case '0' : tiptop->reg = 0; regcode = XREG; break;
			case '1' : tiptop->reg = 1; regcode = XREG; break;
			case '2' : tiptop->reg = 2; regcode = XREG; break;
			case '3' : tiptop->reg = 3; regcode = XREG; break;
			case '4' : tiptop->reg = 4; regcode = XREG; break;
			case '5' : tiptop->reg = 5; regcode = XREG; break;
			case '6' : tiptop->reg = 6; regcode = XREG; break;   
			case '7' : tiptop->reg = 7; regcode = XREG; break;
			default  : goto badreg;
			}
			break;
		case 'd':
			switch (*++oplexptr) {
			case '0' : tiptop->reg = 0; regcode = DREG; break;
			case '1' : tiptop->reg = 1; regcode = DREG; break;
			case '2' : tiptop->reg = 2; regcode = DREG; break;
			case '3' : tiptop->reg = 3; regcode = DREG; break;
			case '4' : tiptop->reg = 4; regcode = DREG; break;
			case '5' : tiptop->reg = 5; regcode = DREG; break;
			case '6' : tiptop->reg = 6; regcode = DREG; break;
			case '7' : tiptop->reg = 7; regcode = DREG; break;
			default  : goto badreg;
			}
			break;
		case 'i':
			if (*++oplexptr != 's' || *++oplexptr != 'p')
				goto badreg;
			tiptop->reg = 14; break;
		case 'p':
			switch(*++oplexptr) {
			case 'c' : if (oplexptr[1] == 'b' && oplexptr[2] == 'p') {
					oplexptr += 2;
					tiptop->reg = 13;
					break;
				   }
				   tiptop->reg = 15;
				   break;
			case 's' : if (*++oplexptr != 'w') goto badreg;
				   tiptop->reg = 11; break;
			default  : goto badreg;
			}
			break;
		default:
badreg:
			yyerror("invalid register name");
			return(-1);
		}
		tiptop++;
		if (IDFOLLOW(*++oplexptr)) goto badreg;
		return(regcode);
		}

	case '\0': return(*oplexptr);
	default  : return(*oplexptr++);
	}
}


/* print out an operand */
void
opnd_dmp(opndptr)
OPERAND *opndptr;
{
	switch (opndptr->type) {
	case IMMD:
		(void) printf("IMMD\t");
		(void) printf("expval = %d\t",(int) opndptr->expval);
		break;
	case FPIMMD:
		(void) printf("FPIMMD\t");
		(void) printf("expval = %d\t",(int) opndptr->expval);
		break;
	case ABSMD:
		(void) printf("ABSMD\t");
		(void) printf("symptr = 0x%x\t",(int)  opndptr->symptr);
		(void) printf("expval = %d\t",(int) opndptr->expval);
		break;
	case ABSDFMD:
		(void) printf("ABSDFMD\t");
		(void) printf("symptr = 0x%x\t",(int)  opndptr->symptr);
		(void) printf("expval = %d\t",(int) opndptr->expval);
		break;
	case EXADMD:
		(void) printf("EXADMD\t");
		(void) printf("symptr = 0x%x\t",(int)  opndptr->symptr);
		(void) printf("expval = %d\t",(int) opndptr->expval);
		break;
	case EXADDFMD:
		(void) printf("EXADDFMD\t");
		(void) printf("symptr = 0x%x\t",(int)  opndptr->symptr);
		(void) printf("expval = %d\t",(int)  opndptr->expval);
		break;
	case DSPMD:
		(void) printf("DSPMD\t");
		(void) printf("symptr = 0x%x\t",(int)  opndptr->symptr);
		(void) printf("expval = %d\t",(int)  opndptr->expval);
		(void) printf("reg = %d\t", opndptr->reg);
		break;
	case DSPDFMD:
		(void) printf("DSPDFMD\t");
		(void) printf("symptr = 0x%x\t",(int)  opndptr->symptr);
		(void) printf("expval = %d\t",(int)  opndptr->expval);
		(void) printf("reg = %d\t", opndptr->reg);
		break;
	case REGMD:
		(void) printf("REGMD\t");
		(void) printf("reg = %d\t", opndptr->reg);
		break;
	case REGDFMD:
		(void) printf("REGDFMD\t");
		(void) printf("reg = %d\t", opndptr->reg);
		break;
	case FREGMD:
		(void) printf("FREGMD\t");
		(void) printf("reg = %d\t", opndptr->reg);
		break;
	case SREGMD:
		(void) printf("SREGMD\t");
		(void) printf("reg = %d\t", opndptr->reg);
		break;
	case DREGMD:
		(void) printf("DREGMD\t");
		(void) printf("reg = %d\t", opndptr->reg);
		break;
	case XREGMD:
		(void) printf("XREGMD\t");
		(void) printf("reg = %d\t", opndptr->reg);
		break;
	default:
		(void) printf("UNKNOWN ");
		break;
	}
	(void) printf("\n");
}
