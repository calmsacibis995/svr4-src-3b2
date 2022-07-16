/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/program.c	1.29"
#include <stdio.h>
#include <sgs.h>
#include <ctype.h>
#include <string.h>
#include <systems.h>
#include <libelf.h>
#include "symbols.h"
#include "gendefs.h"
#include "as_ctype.h"
#include "instab.h"
#include "program.h"
#include "ind.out"
#include "typ.out"
#include "section.h"

extern int txtsec;
int datsec = -1;

extern void comment();
extern void	deflab();
extern long swap_b4();
extern int	Genline;
extern int pswcheck();
extern int mtoregcheck();
extern int abs();
extern void addrgen();
extern void pcintcheck();
extern void intaft2check();
extern void fpdtos();
extern void genstring();
extern int expr();
extern int fp_expr();
extern long Strtoul();
extern short spanopt;

#ifndef	lint
#	ifdef	CHIPFIX
		static char * SWA_CHIP = "@(#) CHIPFIX";
#	endif
#	if	M32RSTFIX
		static char * SWA_RST = "@(#) M32RSTFIX";
#	endif
#	if	ER10ERR
		static char * SWA_EXP = "@(#) ER10ERR";
#	endif
#	if	ER13WAR
		static char * SWA_BLDV = "@(#) ER13WAR";
#	endif
#	if	ER16FIX
		static char * SWA_16 = "@(#) ER16FIX";
#	endif
#	if	ER21WAR
		static char * SWA_21 = "@(#) ER21WAR";
#	endif
#	if	ER40FIX
		static char * SWA_40 = "@(#) ER40FIX";
#	endif
#	if	ER43ERR
		static char * SWA_43 = "@(#) ER43ERR";
#	endif
#endif	/* ifndef lint */


/* mnemonics[] is a table of machine instruction mnemonics used by yyparse().      */
/* These are accessed thru hashtab[].  NOTE: the hashing algorithm for the         */
/* mnemonics occurs in TWO places (mk_hashtabs() and find_mnemonic())              */
/* if changed, THE HASH ALGORITHM MUST BE UPDATED IN BOTH PLACES                   */
static struct mnemonic mnemonics[] = {
#include "mnm.out"
0
};

#define HASHSIZ 1009
static short	hashtab[HASHSIZ];

/* initializes the hash table for mnemonics */
void
mk_hashtab()
{
	register struct mnemonic	*curptr;
	register char			*cptr;
	register unsigned long		hval;
	register			i;

	for (i=0; i<HASHSIZ; i++) hashtab[i] = -1;
	for (curptr=mnemonics; curptr->name != 0; curptr++) {
		for (hval=0,cptr=curptr->name; *cptr; cptr++) {
			hval <<= 2;
			hval += *cptr;
		}
		hval += *--cptr << 5;
		hval %= HASHSIZ;
		curptr->next = hashtab[hval];
		hashtab[hval] = curptr - mnemonics;
	}
}


/* for use by routines other than yyparse(), typically on gencode.c */
struct mnemonic *
find_mnemonic(name)
	char		*name;
{
	register char	*tokptr;
	register int	instr_index;
	register unsigned	hval;

	tokptr = name;
	hval = *tokptr++;
	while (*tokptr) {
		hval <<= 2;
		hval += *tokptr++;
	}
	hval += *(tokptr-1) << 5;
	for (instr_index = hashtab[hval%HASHSIZ]; instr_index >= 0;
			instr_index = mnemonics[instr_index].next)
		if (!strcmp(mnemonics[instr_index].name, name))
			return(mnemonics+instr_index);
	return(NULL);
}

char dbg = 0;	/* -# flag */
static unsigned short action; /* action routine to be called during second pass */
extern unsigned short line;   /* current line number */
extern short bitpos;
extern long newdot; /* up-to-date value of "." */
extern int previous; /* previous section number */
extern symbol *dot;
extern FILE *fdin;
extern symbol *cfile;
extern struct scninfo *sectab;
extern int mksect();
extern void cgsect();
extern symbol *lookup(); /* for looking up labels */
static char as_version[] = "02.01";
static unsigned char expand;	/* expand byte to be generated */
static short memtoreg = 0;	/* Indicator of first instruction in the
				 * 2 instruction sequence that manifests
				 * the Interrupt After TSTW chip bug.
				 */
static unsigned linesize = BUFSIZ;  /* input buffer size - initially set to BUFSIZ */
static struct mnemonic *newins; /* for looking up a new instruction name */
extern short workaround, /* software work around disable flag */
	transition,	/* Assume translation of COFF-based syntax not needed */
	pswopt;	/* PSW optimization */


int need_mau = NO;		/* Assume mau is not required */
int warlevel = NO_AWAR;		/* Disable 32001 workarounds by default */
short wflag = NO;		/* Do not produce mau register warning */

#if	M32RSTFIX
extern short rstflag;
#endif

#if FLOAT
static unsigned long fpval[3];
#endif


char	*oplexptr;	/* set up by yyparse() for use in oplex() */

#define OPMAX 50
/* operstak[] is used to make operand list (operstak[0] is the first operand, */
/* operstak[1] the second, etc.). operstak[] is also used as an expression    */
/* stack by expr().                                                           */
static OPERAND operstak[OPMAX];
OPERAND *opertop;	/* points to first unused element in operstak[] */

/* yyparse() token types */
#define ENDOFLINE	0
#define LABEL		1
#define MNEMONIC	2
#define OPNDID		3
#define OPNDSTR		5
#define OPND		6

/* optional floating point rounding modes */
#define FP_RN 		4
#define FP_RP 		5
#define FP_RM 		6
#define FP_RZ		7
#define NO_ROUND	0

static short mode = 0;


/* for yyparse level tokens, i.e. LABEL's, MNEMONIC's, and OPERAND's */
static struct token {
	char	*ptr;
	char	type;
	short	numopnds;
	} tokens[BUFSIZ/2];

/*
yyparse() is the high level parser for assembly source it calls operand() for operand
parsing.  yyparse consists of a main loop (one time thru for each assembly line) 
containing two phases:

lexing - chopping the line up into labels, mnemonics, and operands.
	 identifier and string operands are recognized directly;
	 all operands have their whitespace removed.

processing - the labels are processed.  mnemonics are looked up.
	     a switch on the instruction index permits tailored
	     parsing and processing of the statement.  otherwise
	     operand() is called in a loop to parse the operands.
	     the operand types are checked.  a switch executes the 
	     statement semantics provided for the mnemonic on "ops.in".
*/
void
yyparse()
{
	void bss();
	static char *getline();
	static void fill();
	static void ck_nonneg_absexpr();
	void ckalign();
	void fillbyte();
	extern void opnd_dmp();
	extern void remember_set_or_size();
	extern void atodi();
	extern void testgen();
	extern int operand();
	extern int atofs();
	extern int atofd();
	extern int atofx();

	char *linebuf;		 /* contains line being parsed */

	if ((linebuf = (char *) malloc(BUFSIZ)) == NULL) /* initialize line buffer */
		aerror("cannot malloc line buffer");

newline:
	while (linebuf = getline(linebuf,fdin))  {	/* line loop */
		line++;

{ /* lex the line */
	register struct token *nexttoken;/* points to free location for next token */
	register char *start, *end, *suffix, *ahead = linebuf;
	register struct token *mnemtoken;

	nexttoken = tokens;
anotherstmt: 
	for (;;) { /* parse labels until we find something else */
		while (WHITESPACE(*ahead)) ahead++;
		if (IDBEGIN(*ahead)) {
			start = ahead;
			while (IDFOLLOW(*++ahead)) ;
			end = ahead;
			while (WHITESPACE(*ahead)) ahead++;
			if (*ahead == ':') {	/* label */
				nexttoken->type = LABEL;
				nexttoken++->ptr  = start;
				ahead++;
				*end = 0;
			} else if (WHITESPACE(*end) || !OPRCHAR(*ahead)) {
				/* mnemonic */
				mnemtoken = nexttoken;
				nexttoken->type = MNEMONIC;
				suffix=end-3;
				if (*suffix++ == '.') {
					if ((*suffix == 'r') || (*suffix == 'R'))
						switch(*++suffix) {
							case 'n': case 'N':
								mode = FP_RN;
								end -= 3; break;
							case 'p': case 'P':
								mode = FP_RP;
								end -= 3; break;
							case 'm': case 'M':
								mode = FP_RM;
								end -= 3; break;
							case 'z': case 'Z':
								mode = FP_RZ;
								end -= 3; break;
							default: ;
						}
				} else mode = NO_ROUND;  
				/* any errors will be detected when the line is processed and the instruction is looked up */
				nexttoken++->ptr  = start;
				break;
			} else {
				yyerror("no white space between mnemonic & operand");
				goto newline;
			}
		} else
			switch (*ahead) {
			case '#' : case '\0': case '\n':
				nexttoken->type = ENDOFLINE;
				goto process;
			case ';' :
				ahead++;
				break;
			default:
				yyerror("statement syntax");
				goto newline;
			}
	}
	/*scan operands; recognize id's and strings directly; take out white space*/
	mnemtoken->numopnds = 0;
	if (OPRCHAR(*ahead)) for (;;) {
		mnemtoken->numopnds++;
		*end = 0;	/* close preceding mnemonic or operand */
		start = ahead;
		if (IDBEGIN(*ahead)) {	/* try for an id */
			while (IDFOLLOW(*++ahead)) ;
			end = ahead;
			while (WHITESPACE(*ahead)) ahead++;
			if (!OPRCHAR(*ahead)) {
				nexttoken->type = OPNDID;
				goto got_opnd;
			}
		} else if (*ahead == '"') {	/* try for a string */
			while (*++ahead)
				if (*ahead == '"') {
					end = ++ahead;
					while (WHITESPACE(*ahead)) ahead++;
					if (!OPRCHAR(*ahead)) {
						nexttoken->type = OPNDSTR;
						goto got_opnd;
					}
				}
			if (*ahead == '\0') {
				yyerror("bad string");
				goto newline;
			}
		} else {
			while (HOPRCHAR(*ahead)) ahead++;
			end = ahead;
		}
		while (OPRCHAR(*ahead))
		/* remove any whitespace but be careful not to combine   */
		/* tokens previously separated by white space. we assume */
		/* that this is a sytax error.                           */
			if (WHITESPACE(*ahead))
				ahead++;
			else if (end == ahead) {
				end++; ahead++;
			} else if (HOPRCHAR(*(ahead-1)) ||
				/* no combining problem since previous
				   character was not a white space character */
				   end == start ||
				/* no combining problem since this is first
				   character to be copied */
				   SINGCHAR(*(end-1)) || SINGCHAR(*ahead))
				/* no combining problem since at least one
				   of the characters always stands alone */
				*end++ = *ahead++;
			else {
				yyerror("operand syntax");
				goto newline;
			}
		nexttoken->type = OPND;
got_opnd:
		nexttoken++->ptr  = start;
		if (*ahead != ',')
			break;
		while (WHITESPACE(*++ahead)) ;
		if (!OPRCHAR(*ahead)) {
			yyerror("missing operand after ','");
			goto newline;
		}
	}
		
	switch (*ahead) {
	case '#' : case '\n': case '\0':
		*end = 0;
		nexttoken->type = ENDOFLINE;
		goto process;
	case ';' :
		*end = 0;
		ahead++;
		goto anotherstmt;
	default:
		yyerror("statement syntax");
		goto newline;
	}
} /* end lexing */

process:
{ /* process the line */
register struct token *curtoken = tokens;

while(curtoken->type != ENDOFLINE) {
	if (curtoken->type == LABEL) {
		register symbol	*symptr;

		if (dbg) (void) printf("LAB %s\n",curtoken->ptr);
		symptr = lookup(curtoken->ptr, INSTALL);
		if (! UNDEF_SYM(symptr) && !COMMON(symptr)
		      || BIT_IS_ON(symptr,SET_SYM)) {
			yyerror("error: multiply defined label");
			(void) fprintf(stderr, "\t\t... \"%s\"\n", curtoken->ptr);
		}
		symptr->sectnum = dot->sectnum;
		if (!BIT_IS_ON(symptr,BOUND) && !COMMON(symptr))
			symptr->binding = STB_LOCAL;
		if (TEMP_LABEL(symptr))
			symptr->flags &= ~GO_IN_SYMTAB;
		symptr->value = newdot;
		if (spanopt && TEXT_SYM(dot))
			deflab(symptr);
		curtoken++;
	} else { /* MNEMONIC */
		register struct mnemonic *instrptr;
		register curopnd, numopnds = curtoken->numopnds; 

		if (dbg) {
			int i; 

			(void) printf("MNM %s",curtoken->ptr);
			for (i=1; i<=curtoken->numopnds; i++)
				(void) printf(" %s",curtoken[i].ptr);
			(void) printf("\n");
		}
		if ((instrptr = find_mnemonic(curtoken->ptr)) == NULL) {
			yyerror("invalid instruction name");
			(void) fprintf(stderr, "\t\t... \"%s\"\n", curtoken->ptr);
			goto instr_done;
		}
		if (mode != NO_ROUND)  /* instruction should be MAU floating point */
			switch(instrptr->tag & ~IS25) {
				case INSTRB: case INSTRH: case INSTRW: 
				case INSTRBH: case INSTRBW: case INSTRHW:
				case INSTRWH: case INSTRWB: case INSTRHB:
					yyerror("invalid rounding specification");
					break;
				default: 
					;
			}	
		instrptr->roundmode = mode;

		/* processing operands */
		opertop = operstak;
		/* switch for statements having parsing intermingled with semantics */
		switch(instrptr->index) {

#define CHEKID(num)	if (curtoken[num].type != OPNDID) {\
				yyerror("expecting identifier");\
				(void) fprintf(stderr,"\t\t... not \"%s\"\n",\
					curtoken[num].ptr);\
				goto instr_done;\
			}

#define CHEKONEID	if (numopnds != 1) {\
				yyerror("expecting 1 operand");\
				(void) fprintf(stderr,"\t\t... not %d\n",numopnds);\
				goto instr_done;\
			}\
			CHEKID(1)

#define CHEKNUMOP(min)	if (numopnds < min) { \
				yyerror("statement missing operand(s)");\
				goto instr_done;\
			}

#define GETID(n)	(curtoken[n].ptr)

#define GETEXPR(num)	oplexptr = curtoken[num].ptr;\
			switch (expr()) { \
			case 0: break; \
			default: yyerror("expression syntax error");\
				 (void) fprintf(stderr,"\t\t... in \"%s\"\n",\
						curtoken[num].ptr);\
			case -1: yyerror("expression syntax error");\
				 (void) fprintf(stderr,"\t\t... in \"%s\"\n",\
						curtoken[num].ptr);\
				goto instr_done;\
			}

#define GETFP(num)	oplexptr = curtoken[num].ptr;\
			switch (fp_expr()) { \
			case 0: break; \
			default: yyerror("floating point syntax error");\
				 (void) fprintf(stderr,"\t\t... in \"%s\"\n",\
						curtoken[num].ptr);\
			case -1: yyerror("floating point syntax error");\
				 (void) fprintf(stderr,"\t\t... in \"%s\"\n",\
						curtoken[num].ptr);\
			 	 goto instr_done;\
			}


case PSSECTION :	/* .section <id>[,<string>][,<type>] */
	{ register char *s;
	  register int att = 0;
	  register long type = SHT_NULL;
	  char has_attributes = NO;
	  char has_type = NO;
	  CHEKID(1)
	  switch (numopnds) {
		case 1: break;
		case 2: if (curtoken[2].type == OPNDSTR) 
				has_attributes = YES;
			else
				has_type = YES;
			break;
		case 3: if (curtoken[2].type != OPNDSTR) {
				yyerror("expecting a string");
				(void) fprintf(stderr,"\t\t... not \"%s\"\n",curtoken[2].ptr);
				goto instr_done;
			}
			has_attributes = has_type = YES;
			break;
		default: {
			yyerror("'.section' takes one, two or three operands");
			(void) fprintf(stderr,"\t\t... not %d\n",numopnds);
			goto instr_done;
			}
		}
		if (has_attributes) {

			s = (curtoken[2].ptr)++;
			s[strlen(s)-1] = '\0';
			s++;
			if (transition) {  /* translating COFF-based code */
				werror(".section has obsolete semantics in transition mode");
				while (*s) switch(*s++) {
					case 'a':
						att |= SHF_ALLOC;
						if (type && (type != SHT_PROGBITS))
							werror("section type has already been set");
						else
							type = SHT_PROGBITS;
						break;
					case 'b':
						/* zero initialized block */
						att |= SHF_ALLOC | SHF_WRITE ;
						if (type && (type != SHT_NOBITS))
							werror("section type has already been set");
						else
							type = SHT_NOBITS;
						break;
					case 'c': /* copy */
					case 'd': /* dummy */
					case 'n': /* noload */
					case 'o': /* overlay */
					case 'l': /* lib */
					case 'i': /* info */
						if (type && (type != SHT_PROGBITS))
							werror("section type has already been set");
						else
							type = SHT_PROGBITS;
						break;
					case 'x':
						/* executable */
						if (type && (type != SHT_PROGBITS))
							werror("section type has already been set");
						else
							type = SHT_PROGBITS;
						if (strcmp(GETID(1),".rodata")==0)
							att |= SHF_ALLOC;
						else
							att |= SHF_ALLOC | SHF_EXECINSTR;
						break;
					case 'w':
						/* writable */
						att |= SHF_ALLOC | SHF_WRITE;
						if (type && (type != SHT_PROGBITS))
							werror("section type has already been set");
						else
							type = SHT_PROGBITS;
						break;
					default:
					yyerror("invalid section attribute");
					(void) fprintf(stderr,"\t\t... %s\n",s);
					break;
					} 
			} else 	{ /* no translation */
				while (*s) switch(*s++) {
					case 'a':
						att |= SHF_ALLOC;
						break;
					case 'x':
					/* executable */
						att |=  SHF_EXECINSTR;
						break;
					case 'w':
						/* writable */
						att |=  SHF_WRITE;
						break;
					default:
						yyerror("invalid section attribute");
						(void) fprintf(stderr,"\t\t... %s\n",s);
					break;
				}
			}
		}
		if (has_type) {

			s = GETID(numopnds);
			if (s[0] == '@') {
				if (strcmp(s, "@progbits") == 0)
					type = SHT_PROGBITS;
				else if (strcmp(s, "@nobits") == 0)
					type = SHT_NOBITS;
				else if (strcmp(s, "@symtab") == 0)
					type = SHT_SYMTAB;
				else if (strcmp(s, "@strtab") == 0)
					type = SHT_STRTAB;
				else if (strcmp(s, "@note") == 0)
					type = SHT_NOTE;
				else if (isxdigit(*++s))  {
					char *ahead=s+1;

					if ((*ahead == 'X' || *ahead == 'x')
					    && *s != '0') {
						yyerror("invalid section type");
						goto instr_done;
					}
					while (isxdigit(*++ahead));
					if (HOPRCHAR(*ahead))
						yyerror("invalid section type");
					type = (unsigned long) Strtoul(s, (char **)NULL,0);
				}
				else yyerror("invalid section type");
			} else yyerror("invalid section type");
		}
			
	cgsect(mksect(lookup(curtoken[1].ptr, INSTALL),att,type));
	}
	goto instr_done;
case PSBSS :{	/*   .bss <id>,<expr>,<expr>   */
	register symbol *symptr;
	CHEKNUMOP(3)
	CHEKID(1)
	GETEXPR(2) opertop++;
	GETEXPR(3)
	symptr = lookup(GETID(1), INSTALL);
	if (operstak[1].expval == 0 || 
		((operstak[1].expval - 1) & operstak[1].expval) != 0)
	{
		yyerror(".bss alignment has invalid value");
		(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
		goto instr_done;
	}
	ck_nonneg_absexpr(operstak[0],".bss size");
	ck_nonneg_absexpr(operstak[1],".bss alignment");
	if (!UNDEF_SYM(symptr) && !COMMON(symptr)) {
		yyerror("multiply defined label in '.bss'");
		(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
		goto instr_done;
	}
	bss(symptr, operstak[0].expval,operstak[1].expval);
	goto instr_done;
	}
case PSDEF :	/*   .def <id>   */
	if (transition) {
		register symbol *symptr;
		symptr = lookup(GETID(1), INSTALL);
		symptr->binding = STB_LOCAL;
		CHEKONEID
		generate(0,DEFINE,NULLVAL, symptr);
		werror("obsolete symbolic debugging statement");
	} else
		yyerror("obsolete symbolic debugging statement");
	goto instr_done;

case PSDIM :	/*   .dim <expr>[,<expr>]*   */
		/* obsolete COFF debugging statement */
	if (!transition)
		yyerror("obsolete symbolic debugging statement");
	goto instr_done;
case PSGLOBL : {	/*   .globl <id> [, <id.]*   */
	register symbol *symptr;
	CHEKNUMOP(1)
	for (curopnd = 1; curopnd <= numopnds; curopnd++) {
		CHEKID(curopnd);
		symptr = lookup(GETID(curopnd), INSTALL);
		if ((BIT_IS_ON(symptr,BOUND))  && (!GLOBAL(symptr))){
			yyerror("can not override binding for symbol");
			(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
			goto instr_done;
			}
		symptr->binding = STB_GLOBAL;
		symptr->flags |= BOUND | GO_IN_SYMTAB;
		
		
	} /* for */
	goto instr_done;
} /* case */
case PSWEAK : {	/*   .weak <id>   */
	register symbol *symptr;
	CHEKNUMOP(1)
	for (curopnd = 1; curopnd <= numopnds; curopnd++) {
		CHEKID(curopnd);
		symptr = lookup(GETID(curopnd), INSTALL);
		if ((BIT_IS_ON(symptr,BOUND)) && (!WEAK(symptr))){
			yyerror("can not override binding for symbol");
			(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
			goto instr_done;
			}
		symptr->binding = STB_WEAK;
		symptr->flags |= BOUND | GO_IN_SYMTAB;
		
	} /* for */
	goto instr_done;
} /* case */
case PSFILE :	/*   .file <string>   */
case PSIDENT :	/*   .ident <string>   */
case PSVERS :	/*   .version <string> */
case PSSTRING :	/*   .string <string> */
	if (numopnds != 1) {
		yyerror("expecting 1 operand");
		(void) fprintf(stderr,"\t\t... not %d\n",numopnds);
		goto instr_done;
	}
	if (curtoken[1].type != OPNDSTR) {
		yyerror("expecting a string");
		(void) fprintf(stderr,"\t\t... not \"%s\"\n",curtoken[1].ptr);
		goto instr_done;
	}
	(curtoken[1].ptr)++;
	(curtoken[1].ptr)[strlen(curtoken[1].ptr)-1] = '\0';
	if (instrptr->index == PSIDENT) {
		comment(curtoken[1].ptr);
		goto instr_done;
	}
	if (instrptr->index == PSVERS) {
		if (strcmp(curtoken[1].ptr,as_version) > 0) {
			yyerror("inappropriate assembler version");
			(void) fprintf(stderr,
				"\t\tcompiler expected %s or greater, have %s\n",
				curtoken[1].ptr,as_version);
		}
		goto instr_done;
	}
	if (instrptr->index == PSSTRING) {
		genstring(curtoken[1].ptr);
		goto instr_done;
	}

	/* DFILE */
	if (cfile != NULL) 
		yyerror("only 1 '.file' allowed");
	else {
		cfile = (symbol *) malloc(sizeof(symbol));
		cfile->name = malloc(strlen(curtoken[1].ptr) +1);
		(void) strcpy(cfile->name, curtoken[1].ptr);
	}
	cfile->binding = STB_LOCAL;
	cfile->flags = (~TYPE_SET) | (~SIZE_SET) | GO_IN_SYMTAB;
	cfile->sectnum = SHN_ABS;
	cfile->value = 0;
	cfile->size = 0;
	cfile->type = STT_FILE;
	goto instr_done;

case PSCOMM :	/*   .comm <id>,<expr> [,<expr>]  */
case PSSET :	/*   .set <id>,<expr>   */
	{ register symbol *symptr;
	CHEKNUMOP(2)
	CHEKID(1)
	GETEXPR(2)
	symptr = lookup(GETID(1), INSTALL);
	if (instrptr->index == PSCOMM) {
		ck_nonneg_absexpr(operstak[0],".comm size");
		if (numopnds == 3) {  /* optional alignment given */
			opertop++;
			GETEXPR(3);
			if (operstak[1].expval == 0 || 
				((operstak[1].expval - 1) & operstak[1].expval) != 0)
			{
				yyerror(".comm alignment has invalid value");
				(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
				goto instr_done;
			}
			ck_nonneg_absexpr(operstak[1],".comm alignment");
			if (!COMMON(symptr) && symptr->align < operstak[1].expval)
				symptr->align = (short) operstak[1].expval;
		} else if (numopnds == 2)
		  /* no alignment given; give default alignment */
                        symptr->align = 4;
		else  
			yyerror("'.comm' takes 2 or 3 operands");
		if (! BIT_IS_ON(symptr,SIZE_SET) && !(COMMON(symptr) 
	    	       && operstak[0].expval < symptr->size))
			symptr->size = operstak[0].expval;
		if (BIT_IS_ON(symptr,SET_SYM)) {
			yyerror("error: multiply defined symbol");
			(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
		}
		if ((UNDEF_SYM(symptr)) ) 
			symptr->sectnum = SHN_COMMON;
		if (!(BIT_IS_ON(symptr, BOUND)))
			symptr->binding = STB_GLOBAL;
		symptr->type = STT_OBJECT;
		goto instr_done;
	}
	/* DSET */
	if (symptr == NULL)
		aerror("Unable to define identifier");
	if (symptr == dot) {
		if (operstak[0].exptype == UNDEF)
/* or:		if (UNDEFINED(operstak[0]) || UNEVALUATED(operstak[0])) */
			yyerror("expression cannot be evaluated");
		else if ((operstak[0].symptr != NULL) &&
			 (symptr->sectnum != operstak[0].symptr->sectnum))
			yyerror("Incompatible section numbers");
		else {
			long incr;

			if ((incr = operstak[0].symptr->value + operstak[0].expval - symptr->value) < 0)
				yyerror("Cannot decrement '.'");
			else
				fill(incr);
		}
	} else {
		if (!UNDEF_SYM(symptr) && !(BIT_IS_ON(symptr,SET_SYM))) {
			yyerror("error: multiply defined symbol");
			(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
		}
		if (!(BIT_IS_ON(symptr,BOUND))) {
			symptr->binding = STB_LOCAL;
			symptr->flags &= ~GO_IN_SYMTAB;
		}
		if (ABSOLUTE(operstak[0])) {
			symptr->value = operstak[0].expval;
			symptr->sectnum = SHN_ABS;
		} else if (RELOCATABLE(operstak[0])) {
			symptr->value = operstak[0].expval;
			if (operstak[0].symptr == dot) {
				symptr->value += operstak[0].symptr->value;
				symptr->sectnum = operstak[0].symptr->sectnum;
			} else 
				/* may want to remember to pass size 
				 * information if given later
				 */
				remember_set_or_size(symptr,operstak[0].symptr,
					     SETTO_SYM,REMEMBER_SET);
		} else if (UNEVALUATED(operstak[0])) 
			remember_set_or_size(symptr,operstak[0].symptr,
					     SETTO_XPTR,REMEMBER_SET);	
		else  /* UNDEFINED(operstak[0]) */ {
			remember_set_or_size(symptr,operstak[0].symptr,
					     SETTO_SYM,REMEMBER_SET);
			symptr->value = operstak[0].expval;
		}
	symptr->flags |= SET_SYM;
	}
	goto instr_done; }
case PSTAG :	/*   .tag <id>   */
	if (transition)
		werror("obsolete symbolic debugging statement");
	else
		aerror("obsolete symbolic debugging statement");
	goto instr_done;
#if	FLOAT
case PSDECINT: {
	 int i;

	CHEKNUMOP(1)
	ckalign(4L);
	for (curopnd = 1; curopnd <= numopnds; curopnd++) {
		GETEXPR(curopnd)
		if (operstak[0].fasciip == NULL) {
			yyerror("expecting decimal constant");
			goto instr_done;
		}
		atodi(operstak[0].fasciip, fpval);
		for (i=0; i<=2; i++)
			generate(32, RELDAT32, fpval[i], NULLSYM);
		dot->value = newdot; /* synchronize */
		Genline = line;
	}
	ckalign(4L);
}
	goto instr_done;
case PSDECFP :
	CHEKNUMOP(1)
	ckalign(4L);
	for (curopnd = 1; curopnd <= numopnds; curopnd++) {
		GETFP(curopnd)
		dot->value = newdot; /* synchronize */
		Genline = line;
	}
	ckalign(4L);
	goto instr_done;
case PSEXT : 		/* .ext floatval [,floatval]* */
case PSDOUBLE :		/* .double floatval [,floatval]* */
case PSFLOAT : { 	/* .float floatval [,floatval]* */
	register width;
	int spctype;

	CHEKNUMOP(1)
	switch (instrptr->index) {
	case PSEXT:
		spctype = 3*NBPW;
		break;
	case PSDOUBLE:
		spctype = 2*NBPW;
		break;
	case PSFLOAT:
		spctype = NBPW;
		break;
	}
	ckalign(4L);
	for (curopnd = 1; curopnd <= numopnds; curopnd++) {
		GETFP(curopnd)
		width = spctype;
		if (bitpos + width > spctype)
			yyerror("Expression crosses field boundary");
		switch (spctype) {
		case 32:	/* single fp number */
		   if (atofs(operstak[0].fasciip,fpval))
			yyerror("fp conversion error");
		   generate(32, RELDAT32, fpval[0], NULLSYM);
		   break;
		case 64:	/* double fp number */
		   if (atofd(operstak[0].fasciip,fpval))
			yyerror("fp conversion error");
		   generate(32, RELDAT32, fpval[0], NULLSYM);
		   generate(32, RELDAT32, fpval[1], NULLSYM);
		   break;
		case 96:	/* double extended fp number */
		   if (atofx(operstak[0].fasciip,fpval))
			yyerror("fp conversion error");
		   generate(32, RELDAT32, fpval[0], NULLSYM);
		   generate(32, RELDAT32, fpval[1], NULLSYM);
		   generate(32, RELDAT32, fpval[2], NULLSYM);
		   break;
		}
		dot->value = newdot; /* syncronize */
		Genline = line;
	}
	ckalign(4L);
}
	goto instr_done;
#endif
case PSBYTE :	/*   .byte [<num>:]<expr>[,[<num>:]<expr>]*   */
case PSHALF :	/*   .half [<num>:]<expr>[,[<num>:]<expr>]*   */
case PSWORD :	/*   .word [<num>:]<expr>[,[<num>:]<expr>]*   */
case PS2BYTE :	/*   .2byte [<num>:]<expr>[,[<num>:]<expr>]*   */
case PS4BYTE :	/*   .4byte [<num>:]<expr>[,[<num>:]<expr>]*   */
	{ register width; int spctype;
	switch (instrptr->index) {
	case PSBYTE : spctype = NBPW/4; break;
	case PSHALF : spctype = NBPW/2; ckalign(2L); break;
	case PSWORD : spctype = NBPW; ckalign(4L); break;
	case PS2BYTE : spctype = NBPW/2; break;
	case PS4BYTE : spctype = NBPW; break;
	}
	CHEKNUMOP(1)
	for (curopnd = 1; curopnd <= numopnds; curopnd++) {
		oplexptr = curtoken[curopnd].ptr;
		switch (expr()) {
		case ':':
			oplexptr++;
			opertop++;
			if (expr()) {
				yyerror("expression error");
				(void) fprintf(stderr,"\t\t... in \"%s\"\n",
					curtoken[curopnd].ptr);
				goto instr_done;
			}
			if (operstak[0].symptr != NULL || 
			    operstak[0].expval < 0)
				yyerror("bad field width specifier");
			width = operstak[0].expval;
			if (bitpos + width > spctype)
				yyerror("expression crosses field boundary");
			action = (width == NBPW) ? RELDAT32 : RESABS;
			exp_generate(width,(operstak[1].symptr!=NULLSYM)*action,
				operstak[1]);
			opertop = operstak;
			break;
		case '\0':
			width = spctype;
			if (bitpos + width > spctype)
				yyerror("expression crosses field boundary");
			/* Figure out which action routine to use   */
			/* in case there is an unresolved symbol.   */
			/* If a full word is being used, then       */
			/* a relocatable may be specified.          */
			/* Otherwise it is restricted to being an   */
			/* absolute (forward reference).            */
			action = (width == NBPW) ? RELDAT32 : RESABS;
			exp_generate(width,(operstak[0].symptr!=NULLSYM)*action,
				operstak[0]);
			break;
		default:
			yyerror("expression error");
			goto instr_done;
		}
		dot->value = newdot; /* syncronize */
		Genline = line;
	}
	switch (instrptr->index) {
	case PSBYTE : 
	case PS2BYTE : 
	case PS4BYTE : fillbyte(); break;
	case PSHALF : fillbyte(); ckalign(2L); break;
	case PSWORD : fillbyte(); ckalign(4L); break;
	}
	goto instr_done;
	}
case PSLOCAL : /* .local <id> [,id]*  */ {
	register symbol *symptr;

	CHEKNUMOP(1)
	for (curopnd = 1; curopnd <= numopnds; curopnd++) {
		CHEKID(curopnd);
		symptr = lookup(GETID(curopnd), INSTALL);
		if ((BIT_IS_ON(symptr,BOUND))  && (!LOCAL(symptr))){
			yyerror("can not override binding for symbol");
			(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
			goto instr_done;
			}
		symptr->binding = STB_LOCAL;
		symptr->flags |= BOUND | GO_IN_SYMTAB;
	}  /* for */
	goto instr_done;
	}
case PSSIZE : /* .size <id>, <expr> */
	{ register symbol *symptr;
	if (numopnds == 1) { /* COFF based debugging statement */
		if (transition) {
			GETEXPR(1)
		} else
			yyerror("'.size' takes 2 operands");
		goto instr_done;
	}
	CHEKID(1);
	GETEXPR(2);
	symptr = lookup(GETID(1),INSTALL);
	if (BIT_IS_ON(symptr,SIZE_SET))
	        yyerror("symbol size has previously been set");
	if (ABSOLUTE(operstak[0])) {
		if (operstak[0].expval < 0)
			yyerror(".size argument has negative value");
		if (operstak[0].expval == 0)
			yyerror(".size argument has invalid value");
		symptr->size = operstak[0].expval;
	} else if (RELOCATABLE(operstak[0]))
		yyerror("Expression in '.size' cannot be relocatable");
	else if (UNEVALUATED(operstak[0]))
		remember_set_or_size(symptr,operstak[0].symptr,SETTO_XPTR,REMEMBER_SIZE);
	else  /* (UNDEFINED(operstak[0])) */ {
		remember_set_or_size(symptr,operstak[0].symptr,SETTO_SYM,REMEMBER_SIZE);
		symptr->size = operstak[0].expval;
	} 
	symptr->flags |= SIZE_SET;
	goto instr_done;
	}

case PSTYPE : /* .type <id>, <string> */ {
	register symbol *symptr;
	char *type;
	BYTE temp;
	if (numopnds == 1) { /* COFF based debugging statement */
		if (transition) {
			GETEXPR(1)
		} else
			yyerror("'.type' takes 2 operands");
		goto instr_done;
	}
	CHEKID(1);
	type = GETID(2);
	symptr = lookup(GETID(1), INSTALL);
	if (strcmp(type,"@no_type") == 0)
		temp = STT_NOTYPE;
	else if (strcmp(type,"@object") == 0)
		temp = STT_OBJECT;
	else if (strcmp(type,"@function") == 0)
		temp = STT_FUNC;
	else	{
		yyerror("unknown symbol type for");
		(void) fprintf(stderr,"\t\t...  %s\n",symptr->name);
	}
	if (BIT_IS_ON(symptr,TYPE_SET)) {
	/* symbol already has a type make sure there is no conflict */
		if ( symptr->type != temp) {
			yyerror("symbol type conflict for");
			(void) fprintf(stderr,"\t\t... %s\n",symptr->name);
		}
		goto instr_done;
	}
	symptr->flags |= TYPE_SET;
	symptr->type = temp;
	goto instr_done;
	}	
case TESTOP1 :
	oplexptr = curtoken[1].ptr;
	if (numopnds == 0) {
		operstak[0].type = IMMD;
		operstak[0].newtype = NOTYPE;
		operstak[0].expval = 0;
	} else if (numopnds != 1) {
		yyerror("expecting 0 or 1 operands");
		goto instr_done;
	} else if (operand()) {
		yyerror("operand error");
		goto instr_done;
	}
	if (!(1<<operstak[0].type & TIMMD)) {
		yyerror("operand type mismatch");
		goto instr_done;
	}
	testgen(instrptr, &(operstak[0]));
	memtoreg = 0;
	goto instr_done;

		}
		/* parse operands & check their types */
		{ register unsigned long *opndtype = instrptr->opndtypes;

		for (curopnd = 1; curopnd <= numopnds; curopnd++) {
			oplexptr = curtoken[curopnd].ptr;
			if (operand()) { /* operand error */
				yyerror("operand error");
				(void) fprintf(stderr,"\t\t... operand #%d of \"%s\", \"%s\"\n", curopnd, curtoken->ptr, curtoken[curopnd].ptr);
				goto instr_done;
			}
			if (*opndtype == 0) {
				yyerror("too many operands");
				(void) fprintf(stderr,"\t\t... \"%s\" takes %d; %d were present\n",
					curtoken->ptr, curopnd-1, numopnds);
				goto instr_done;
			}

/* 32000 specific kludge - type DECIMMD's look just like some IMMD's.  If the  */
/* instruction wants a DECIMMD and the operand was an IMMD, we promote it here */
			if (*opndtype & (1<<DECIMMD) && opertop->type == IMMD) {
				if (opertop->fasciip != NULL) /* decimal constant */
					opertop->type = DECIMMD;
			}

			if (!(*opndtype & (1<<opertop->type))) {
				yyerror("operand type mismatch");
				(void) fprintf(stderr,"\t\t... operand #%d of \"%s\": \"%s\"\n", curopnd, curtoken->ptr, curtoken[curopnd].ptr);
				goto instr_done;
			}
			opertop++;
			opndtype++;
			if (dbg) opnd_dmp(operstak+curopnd-1);
		}
		if (*opndtype != 0) {
			yyerror("too few operands");
			(void) fprintf(stderr,"\t\t... only %d present for \"%s\"\n",
					numopnds, curtoken->ptr);
			goto instr_done;
		}
		}

		/* semantic switch */
		switch(instrptr->index) {
#include "sem.out"
		}
instr_done:
		/* end of statement semantics */
		Genline = line;
		dot->value = newdot;	/* synchronize */

		curtoken += numopnds + 1;
	}
}
} /* end line processing */

	}
	/* end of program semantics */
}

static void
fill(nbytes)
long nbytes;
{
	long fillval;

	fillval = (TEXT_SYM(dot))	? ((TXTFILL<<8)|TXTFILL)
					: ((FILL<<8)|FILL);
	while (nbytes >= 2) {
		generate(2*BITSPBY,0,fillval,NULLSYM);
		nbytes -= 2;
	}
	if (nbytes)
		generate(BITSPBY,0,fillval,NULLSYM);
	
} /* fill */

void
ckalign(size)
long size;
{
	long mod;
	if (size != 0 && size != 1) {
		if ((mod = newdot % size) != 0) 
			fill(size - mod);
		if (size > sectab[dot->sectnum].addralign)
			sectab[dot->sectnum].addralign = size;
	}
} /* ckalign */

void
fillbyte()
{
	if (bitpos)
		generate(BITSPBY-bitpos,0,FILL,NULLSYM);
} /* fillbyte */



static void
ck_nonneg_absexpr(expr,name)
OPERAND expr;
char *name;
{
	char errmsg[100];

  if (!ABSOLUTE(expr)) {
	(void) sprintf(errmsg,"%s not absolute",name);
	yyerror(errmsg);
  }
  if (expr.expval < 0) {
	(void) sprintf(errmsg,"%s has negative value",name);
	yyerror(errmsg);
  }
} /* ck_nonneg_absexpr */

static char *
getline(linebuf,fdin)
char *linebuf;
FILE * fdin;
{
	register int c;
	register int index = 0;

	while ((c = getc(fdin)) != EOF && c != '\n') {
		if (index == linesize) {
			linesize *= 2;
			if ((linebuf = realloc(linebuf, linesize)) == NULL)
				yyerror("cannot allocate line buffer");
		}
		linebuf[index++] = (char) c;
	}
	if (c == '\n')
		linebuf[index] = (char) c;
	if (c == EOF)
		return NULL;
	else
		return linebuf;
}
