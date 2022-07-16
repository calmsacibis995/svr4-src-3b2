/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/in.c	1.8"

#include	<ctype.h>
#include	<stdio.h>
#include	<memory.h>
#include	<string.h>
#include	"defs.h"
#include	"debug.h"
#include	"OpTabTypes.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"OperndType.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"
#include	"optim.h"
#include	"optutil.h"

/* parseop:  Parse an operand and return an address node in *anode. */
	void
parseop(operand,dtype,ptype,anode) 
char *operand; 			/* pointer to null term operand string */
OperandType dtype;		/* default operand type */
OperandType *ptype;		/* pointer to operand type */
AN_Id *anode;			/* place to return address node id */

{extern OperandType GetIntTypeId();  /* Gets operand type; in Mach.Dep. */
 extern void fatal();		/* Handles fatal errors; in common.  */
 register char *breg,	/* ptr to text of reg 1  */
               *breg2;	/* ptr to text of reg 2  */
 register char	*s;	/* current parse point in operand */
 char *rcurly,		/* ptr to right curly brace/bracket            */
      *p = operand,	/* another working pointer                     */
      *ad_expr = NULL;	/* ptr to expression component of opnd         */
 int expr_length;	/* length of expression component of opnd	*/
 boolean md_abs = FALSE, /* set for absolute indicated by $            */
         md_def = FALSE, /* set for deferred indicated by *            */
         md_imm = FALSE; /* set for immediate indicated by &           */
 OperandType t = dtype;  /* holds derived operand type                 */
 RegId rg1 = REG_NONE,	/* leftmost register  */
       rg2 = REG_NONE;	/* rightmost register, if any */
 AN_Mode md = Undefined;	/* derived address mode of operand */
 extern unsigned int praddr();	/* Prints addresses.	*/

						/* Look for register.	*/
 if((breg = s = strchr(p, '%')) != NULL){
	switch( *++s ) {
		case 'r':
			switch( *++s ) {
			case '1':
			case '2':
				switch( *++s ) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					md = CPUReg;		
					rg1 = GetIntRegId(breg);
					++s;
					break;
				default:
					md = CPUReg; 
					rg1 = GetIntRegId(breg);
					break;
				}
				break;
			case '3':
				switch(*++s)
					{case '0':
					 case '1':
						md = CPUReg;
						rg1 = GetIntRegId(breg);
						++s;
						endcase;
					 default:
						md = CPUReg;
						rg1 = CREG3;
						endcase;
					}
				endcase;
			case '0':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				md = CPUReg;       
				rg1 = GetIntRegId(breg);
				++s;
				break;
			}
			break;
		case 'f':
			switch( *++s ) {
			case 'p':
				md = CPUReg; 
				rg1 = CFP;
				++s;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				md = MAUReg;
				rg1 = GetIntRegId(breg);
				++s;
				break;
			}
			break;
		case 'a':
			switch( *++s ) {
			case 'p':
				md = CPUReg;
				rg1 = CAP;
				++s;
				break;
			}
			break;
		case 'p':
			switch( *++s ) {
			case 's':
				switch( *++s ) {
				case 'w':
					md = CPUReg;  
					rg1 = CPSW;  
					++s;
					break;
				}
				break;
			case 'c':
				switch( *++s ) {
				case 'b':
					switch( *++s ) {
					case 'p':
						md = CPUReg;
						rg1 = CPCBP; 
						++s;
						break;
					}
					break;
				default:
					md = CPUReg;
					rg1 = CPC;   
					break;
				}
				break;
			}
			break;
		case 's':
			switch( *++s ) {
			case 'p':
				md = CPUReg;
				rg1 = CSP;
				++s;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				md = MAUReg;   
				rg1 = GetIntRegId(breg);
				t = Tsingle;
				++s;
				break;
			}
			break;
		case 'i':
			switch( *++s ) {
			case 's':
				switch( *++s ) {
				case 'p':
					md = CPUReg;
					rg1 = CISP;  
					++s;
					break;
				}
				break;
			}
			break;
		case 'd':
			switch( *++s ) {
			case '0':
			case '1':
			case '2':
			case '3':
				md = MAUReg;  
				rg1 = GetIntRegId(breg);
				t = Tdouble;
				++s;
				break;
			}
			break;
		case 'x':
			switch( *++s ) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				md = MAUReg;  
				rg1 = GetIntRegId(breg);
				t = Tdblext;
				++s;
				break;
			}
			break;
		}
		if( md == Undefined ) fatal( "parseop:  Undefined Mode\n" );
		while( isspace( *s ) ) ++s;

		/* check for comma -> Index Reg Disp */
		if( *s == ',' ) {
			++s;
			if(DBdebug(4,XINCONV))
				fprintf(stdout,
					"par: comma found (index disp)\n");
			while( isspace(*s) ) ++s;
			breg2 = s;
			if(DBdebug(4,XINCONV))
				fprintf(stdout,"breg2=%s\n",breg2);
			if( *s++ != '%' )
				fatal("parseop:  expected % after comma\n");
			if( *s++ != 'r' )
				fatal("parseop:  expected %r after comma\n");
			if( !isdigit(*s) )
				fatal("parseop:  expected digit after %r\n");
			if( *s == '1' )
				if( isdigit( *(s+1) ) ) ++s;
			++s;
			md = IndexRegDisp;
			rg2 = GetIntRegId(breg2);
			while( isspace(*s) ) ++s;
			if( *s != ')' ) fatal("parseop: missing right paren\n");
		}
		/* check for Index Reg Scaling */
		if( *s == '[' ){
			if(DBdebug(4,XINCONV))
				{fprintf(stdout,
					"parseop: bracket found (index scal)\n");
				 fprintf(stdout,"breg2=%s\n",s);
				}
			++s;
			while( isspace(*s) ) ++s;
			breg2 = s;
			if( *s++ != '%' )
				fatal("parseop:  expected % after [\n");
			if( *s++ != 'r' )
				fatal("parseop:  expected %r after [\n");
			if( !isdigit(*s) )
				fatal("parseop:  expected digit after %r\n");
			if( *s == '1' )
				if( isdigit( *(s+1) )) ++s;
			++s;
			md = IndexRegScaling;
			rg2 = GetIntRegId(breg2);
			rcurly = strchr(breg2,']');
			if(rcurly==NULL) 
				fatal("parseop:  missing right brackt\n");
			while( isspace(*s) ) ++s;
			if( *s++ != ']' )
				fatal("parseop:  garbage in operand\n");
			while( isspace(*s) ) ++s;
			if( *s != '\0' )
				fatal("parseop:  garbage after ]\n");
		}

		/* check for register displacement */
		if( *s == ')' ) {
			while( isspace( *(breg-1) ) && breg > operand ) --breg;
			if( breg > operand ) --breg;
			if( *breg != '(' ) 
				fatal("parseop:  missing left parenthesis\n");
			while( isspace( *(breg-1) ) && breg > operand ) --breg;
			/* mode becomes Disp, unless we already know 
			 * that it's IndexRegDisp.
			 */
			if( md != IndexRegDisp ) md = Disp;
			/* check for pre-incr or pre-decr */
			/* fprintf(stdout,"breg=%s\n",breg);*/
			if( *(breg-1)=='+' && breg>operand){
				--breg;
				if(DBdebug(4,XINCONV))
		fprintf(stdout,"parseop: found pre-incr in %s\n",breg);
				if(md==Disp) md=PreIncr;
				else fatal("parseop: illegal pre-incr\n");
				while(isspace(*(breg-1))&&breg>operand) --breg;
			}
			else if( *(breg-1)=='-' && breg> operand){
				--breg;
				if(DBdebug(4,XINCONV))
		fprintf(stdout,"parseop: found pre-decr in %s\n",breg);
				if(md==Disp) md=PreDecr;
				else fatal("parseop: illegal pre-decr\n");
				while(isspace(*(breg-1))&&breg>operand) --breg;
			}
			++s;
			while( isspace( *s ) ) ++s;
		}
		/* check for +, -, or garbage after ) */
		if( *s == '+'){
			if(DBdebug(4,XINCONV))
		fprintf(stdout,"found post-incr in %s\n",breg);
			++s;
			if(md==Disp) md=PostIncr;
			else fatal("parseop:  illegal post-incr\n");
			while( isspace(*s) ) ++s;
		}
		else if( *s == '-'){
			if(DBdebug(4,XINCONV))
		fprintf(stdout,"found post-decr in %s\n",breg);
			++s;
			if(md==Disp) md=PostDecr;
			else fatal("parseop:  illegal post-decr\n");
			while( isspace(*s) ) ++s;
		}

		if( *s != '\0' ) {
			fatal( "parseop:  Extraneous characters at end of line: %s\n",s );
		}
	}
	else {
		breg = strchr( p, '\0' );
		while( isspace( *( breg - 1 ) ) && breg > operand ) --breg;
	}

	/* check for expand byte */
	s = operand;
	if( *s == '{' ) {
		/* adjust pointers */
		s++;
		while( isspace( *s ) ) s++;
		if((rcurly = memchr(s,'}',breg - s)) == NULL) 
			fatal( "parseop:  curly braces don't match\n" );
		while( isspace( *( rcurly - 1 ) ) && rcurly > s ) rcurly--;

		/* get internal type from string s with (rcurly-s) chars */
		t = GetIntTypeId(s);

		/* type conversions */
		if( t == Tubyte ) t = Tbyte;
		if( t == Tshalf ) t = Thalf;
		if( t == Tsword ) t = Tword;

		/* adjust pointers */
		s = rcurly;
		while( isspace( *s ) ) s++;
		s++;
		while( isspace( *s ) ) s++;
	}

	/* check for deferred */
	if( *s == '*' ) {
		/* md |= A_DEF; */
		md_def = TRUE;
		++s;
		while( isspace ( *s ) ) ++s;
	}

	/* check for absolute */
	if( *s == '$' ) {
		/* md |= A_ABS; */
		md_abs = TRUE;
		++s;
		while( isspace ( *s ) ) ++s;
	}

	/* check for immediate */
	if( *s == '&' ) {
		/* md |= A_IMMED; */
		md_imm = TRUE;
		++s;
		while( isspace ( *s ) ) ++s;
	}
	/* build a mode */
	if(md_abs){
		if(md_imm) fatal( "parseop:  Illegal mode (abs and imm)\n" );
		if(md != Undefined) 
			fatal( "parseop:  Illegal mode (using abs)\n" );
		if(md_def) md = AbsDef;
		else       md = Absolute;
	}
	else if(md_def){
		if(md_imm) fatal( "parseop:  Illegal mode (def and imm)\n" );
		if(md == Disp) md=DispDef;
		else if(md == CPUReg) md=Disp;
		else if(md == Absolute) md=AbsDef;
		else if(md == Undefined) md=AbsDef;
		else fatal( "parseop:  Illegal mode (using def)\n" );
	}
	else if(md_imm){
		if(md!=Undefined) fatal("parseop:  Illegal mode (using imm)\n");
		md = Immediate;
	}
	if(md == Undefined) md = Absolute;
					/* return type */
	*ptype = t;
					/* get expression */
	ad_expr = s;
	expr_length = breg - s;
	*breg = EOS;
	if(DBdebug(4,XINCONV))
		fprintf(stdout,"ad_expr set to >%s<\n",ad_expr);

					/* return address id */
	switch(md)
		{case Absolute: 
			if(PIC_flag && strchr(ad_expr,AtChar))
			    /* PIC code, convert to mode Disp,
			       pc relative address */
			    *anode = GetAdDisp(t,ad_expr,CPC);
			else
			    *anode = GetAdAbsolute(t,ad_expr); 
			endcase;
		 case AbsDef: 
			if(PIC_flag && strchr(ad_expr,AtChar))
			    /* PIC code, convert to mode DispDef:
			       pc relative address */
			    *anode = GetAdDispDef(ad_expr,CPC);
			else
			    *anode = GetAdAbsDef(ad_expr);
			endcase;
	 	 case CPUReg:
			if(expr_length)
				fatal("parseop: expr illegal for CPU reg.\n");
			*anode = GetAdCPUReg(rg1);
			endcase;
	 	 case StatCont:
			fatal("parseop: StatCont mode illegal on input.\n");
			endcase;
	 	 case Disp:
			*anode = GetAdDisp(t,ad_expr,rg1);
			endcase;
	 	 case DispDef:
			*anode = GetAdDispDef(ad_expr,rg1);
			endcase;
	 	 case Immediate:
			*anode = GetAdImmediate(ad_expr);
			endcase;
	 	 case IndexRegDisp:
			*anode = GetAdIndexRegDisp(t,ad_expr,rg1,rg2);
			endcase;
		 case IndexRegScaling:
			*anode = GetAdIndexRegScaling(t,
				ad_expr,rg1,rg2); 
			endcase;
		 case MAUReg:
			if(expr_length)
				fatal("parseop: expr illegal for MAU reg.\n");
			*anode = GetAdMAUReg(rg1);
			endcase;
		 case PostDecr:
			*anode = GetAdPostDecr(t,ad_expr,rg1);
			endcase;
		 case PostIncr:
			*anode = GetAdPostIncr(t,ad_expr,rg1); 
			endcase;
		 case PreDecr:
			*anode = GetAdPreDecr(t,ad_expr,rg1); 
			endcase;
		 case PreIncr:
			*anode = GetAdPreIncr(t,ad_expr,rg1); 
			endcase;
		 default:
			fatal("parseop: unknown mode (0x%x)\n",md); 
			endcase;
		} /* END OF switch(md) */

	if(DBdebug(4,XINCONV))
		{fprintf(stdout,"#parsed address: ");
		 (void)praddr(*anode,t,stdout);
		 fprintf(stdout,"\n");
		}

 return;
}
