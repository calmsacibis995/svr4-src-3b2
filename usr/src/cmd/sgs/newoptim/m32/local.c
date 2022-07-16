/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/local.c	1.33"

#include	<ctype.h>
#include	<stdio.h>
#include	<malloc.h>
#include	<memory.h>
#include	<string.h>
#include	"defs.h"
#include	"debug.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"RoundModes.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"ALNodeType.h"
#include	"ALNodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"
#include	"TNodeDefs.h"
#include	"optab.h"
#include	"optim.h"
#include	"optutil.h"
#include	"storclass.h"
#include	"ar.h"
#include	"Target.h"
#include	"CSections.h"

#define	IN	1	/* Operand in (,).	*/
#define	OUT	0	/* Operand not in (,).	*/
#define	DESTINATION	3

#define NLAB	32

#define MakeRawNode(S,list,t)	{ SkipWhite(S);\
				if(*S == EOS)\
					fatal("parse_pseudo:bad format: %s\n",lp);\
				list = MakeTxNodeBefore((TN_Id) NULL,pop);\
				an_id = GetAdRaw(S);\
				PutTxOperandAd(list,0,an_id);\
				PutTxOperandType(list,0,t); }

#define MakeAbsNode(S,list,t)	{ SkipWhite(S);\
				if(*S == EOS)\
					fatal("parse_pseudo:bad format: %s\n",lp);\
				an_id = GetAdAbsolute(t,S);\
				list = MakeTxNodeBefore((TN_Id) NULL,pop);\
				PutTxOperandAd(list,0,an_id);\
				PutTxOperandType(list,0,t); }

#define	InLibrary(F)		((F) != stdin)

enum Section section, prev_section; /* Control section.	*/
char *getline();		/* Gets a line of assembler input. */

static unsigned long int lineno = IDVAL;
static struct pvent *pvhead;	/* pseudo-variable list head */

boolean zflag = FALSE;		/* debug flag for in--line expansion */

extern char lextab[];
static boolean infunc = FALSE;
static boolean inswitch = FALSE;
struct liblist {
	struct liblist *next;
	char *libname;
	};
static struct liblist lib0;
static struct liblist *liblast = &lib0;
static char oldarlab[NLAB];
static char newarlab[NLAB];

	/* Private function declarations. */
STATIC void do_libsa();		/* Handles src archive libraries. */
STATIC FILE *libopen();		/* Opens a src arch library.	*/
STATIC void maketrans();	/* Makes translation nodes.	*/
STATIC boolean memhead();	/* Is this a member header in src arch?	 */
STATIC void parse_alias();	/* Handles alias comments.	*/
STATIC void parse_ops();	/* Handles parsing of operands. */
STATIC void parse_regal();	/* Parses a #REGAL line. */
STATIC OperandType parse_type();/* Gets the type of an object.	*/
STATIC void pass1();		/* First pass: parse, and build text nodes. */
STATIC long int pvsave();	/* Stashes away the value of a pseudo var. */
STATIC void skipmem();		/* Skips a non-candidate arch member.	*/

	void
do_optim(ldtab)
struct ldent ldtab[];		/* The Live-Dead Table for GRA.	*/

{
 FILE *Tfile;		/* Stream pointer of text intermediate file. */
 STATIC void do_libsa();	/* Handles src srchive libraries. */
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/
 extern void ildecide();	/* Defined in func.c. */
 extern void ilinit();		/* Defined in func.c. */
 extern void ilsummary();	/* Prints summary of in-line expansion. */
 extern void init();		/* Initializes portable code improver.	*/
 char *s;
 extern enum Section section;	/* Type of current control section.	*/
 extern void wrapup();
 STATIC void pass1();		/* Pass 1: parse & construct nodes. */
 extern void pass2();		/* Pass 2: perform optimizations. */
 extern int unlink();		/* Remove directory entry (2). */

						/* Pass one. */
			/* Scan for expansion candidates in same file. */
 s = tempnam("/usr/tmp","optmT");		/*Make text intermediate file.*/
 if(s == NULL)
	fatal("do_optim: couldn't make tempname.\n");
 Tfile = fopen(s,"w+");				/* Open it.	*/
 if(Tfile == NULL)				/* Did it open?	*/
	fatal("do_optim: couldn't open file: %s (%d).\n",s,errno);
 if(unlink(s) == -1)				/* Unlink right away.	*/
	fatal("do_optim: couldn't unlink file: %s (%d).\n",s,errno);
 free(s);			/* Release space malloced by tempnam.*/
 section = CStext;		/* .text control section. */
 ilinit();
 pvhead = NULL;
 pass1(stdin,Tfile,0);			/* Parse compiler output file.*/

 if(lib0.next != NULL)				/* Handle source archives. */
	do_libsa(lib0.next);

						/* Pass two.	*/
 if(ferror(Tfile))
	{perror("optim");
	 fatal("do_optim(after pass1): I/O error occurred in Tfile\n");
	}
 rewind(Tfile);
 section = CStext;
 init();
 ildecide();
 pvhead = NULL;
 pass2(Tfile,ldtab);			/* Process intermediate file. */
 ilsummary();
 wrapup();
 if(ferror(Tfile))
	{perror("optim");
	 fatal("do_optim(after pass2): I/O error occurred in Tfile\n");
	}
 if(fclose(Tfile) == EOF)			/* Close intermediate file. */
	fatal("do_optim(after pass2): can't close Tfile : (%d).\n",errno);
 return;
}
	STATIC void
pass1(infile,Tfile,inlib)
FILE *infile;
FILE *Tfile;
	/* Stream pointer of text. */
boolean inlib;

{
 extern void init();		/* Initializes portable code improver.	*/
 extern void parse_debug();	/* Reads the .debug directives. (Debug_info.c */
 extern void init_debug();
 STATIC void parse_com();	/* Handles special comments. */
 STATIC void parse_instr();	/* Handles normal instructions. */
 STATIC char *parse_label();
 STATIC void parse_pseudo();	/* Handles pseudo instructions. */
 register char *s;		/* temporary string ptr */
 TN_Id tn_id;
 static debugging_flag=0;



 while ((s = getline(infile)) != NULL ){
    if (section == CSdebug) {
	if(!debugging_flag) {
	    debugging_flag=1;
 	    init_debug(infile);
	}
	parse_debug(s);		/* Hook to handle an entire debug
				   section. */
	continue;
    }
    else switch(*s){
	 case ComChar:
		/* check for special comments. */
		parse_com(s,infile);
		continue;	/* finished comment processing, goto next line */
	 case EOS:			/* Ignore empty line. */
		continue;
	 case SPACE:			/* Normal instructions, fall thru */
	 case TAB:
		endcase;
	 default:			/* Label. */
		if (section == CSline) continue; /* don't print line info */
		s=parse_label(s,infile); /* return what's left */
		if (*s != SPACE ) return; /* Must have been in library
					     and at end of function */
		s = s+1;
		endcase;
	} /* end of switch(*s) */

	/* Handle more usual case of line beginning with space/tab.	*/
	SkipWhite(s);

	switch(*s){
	case EOS:
	case ComChar:
    	/* line of white spaces, comment or just a label */
	    endcase;
	case DOT:		/* Start of pseudo-op */
    	parse_pseudo(s,Tfile,inlib);
	    endcase;
	default:		/* Normal instruction */
    	if (section != CStext)
      	    printf("\t%s\n",s);
    	else			/* Normal instruction in text.*/
	    parse_instr(s,infile);
    	endcase;
	} /* switch */
} /* END OF while */

DoExtAlias();		/* Make aliased global REGALs nonNAQs. */
	    
if (! inlib ) /* Save any remaining text nodes */
    for(ALLN(tn_id)) WriteTxNode(tn_id,Tfile);
init();
pvhead = NULL;
DelTxNodes((TN_Id) NULL,(TN_Id) NULL);
section = CStext;
}
	STATIC void
parse_com(s,infile)
register char *s;
FILE *infile;
{
 extern LoopType GetIntLoopType();	/* Gets loop type; in common.	*/
 extern OperandType GetIntTypeId();	/* Gets Operand type; in common. */
 AN_Id an_id;
 extern int atoi();			/* Converts ASCII to int. (3C). */
 extern void fatal();
 char *getline();
 LoopType ltype;
 extern void newlab();			/* Returns a new label string. */
 char newlabel[EBUFSIZE+1];	/* Place for a new label. */
 TN_Id node;
 extern int optmode;			/* -Ksd or -Ksz	*/
 STATIC void parse_alias();		/* Parses a #ALIAS line. */
 STATIC void parse_regal();		/* Parses a #REGAL line. */
 extern void Xdebug();			/* Sets Xmask according to the #DEBUG line (in debug.c) */
 extern boolean swflag;
 register char *t;

	switch(*++s){
	case 'A':	/* #ALIAS, #ASM or #ASMEND ? */
		if(strncmp(s,"ALIAS",5) == 0){
			parse_alias(s+5);
			endcase;
		}
		else if(strncmp(s,"ASM",3) != 0) 
			endcase;
		if(strncmp(s+3,"END",3) == 0)
			/* since #ASM processing chews up everything to #ASMEND ... */
			fatal("parse_com: unbalanced asm.\n");
		/* here if #ASM<stuff> */
		s = "#ASM";
		do{
			if (section != CStext)
				printf("%s\n",s);
			else {
				node = MakeTxNodeBefore((TN_Id) NULL,ASMS);
				PutTxOperandAd(node,0,GetAdRaw(s));
				PutTxBlackBox(node,TRUE);
			}
			 if(strncmp(s,"#ASMEND",7) == 0)
				break;
		} while((s = getline(infile)) != NULL);

		if(s == NULL)
			fatal("parse_com: unbalanced ASM.\n");
		endcase;
	 case 'D':	/* #DEBUG ? */
		if(strncmp(s,"DEBUG",5) != 0) endcase;
		s += 5;
		SkipWhite(s);
		Xdebug(atoi(s));
		endcase;
	 case 'L':	/* #LOOP ? */
		if(strncmp(s,"LOOP",4) != 0) endcase;
		node = MakeTxNodeBefore((TN_Id) NULL,LOOP);
		s += 4;		/* enter it into the list. */
		SkipWhite(s);	/* Skip white space.	*/
		PutTxLoopDepth(node,0);
		PutTxLoopFlag(node,FALSE);
		PutTxLoopSerial(node,0);
		ltype = GetIntLoopType(s);
		PutTxLoopType(node,ltype);
		if(ltype == Condition && optmode != OSPEED)
					/* this label is necessary to prevent	 */
					/* value tracing across the body of loop */
					/* thus ensuring the correctness of 	 */
					/* loopclean() (don't bother in speed mode) */
			{newlab(newlabel,".O",sizeof(newlabel));
			 an_id = GetAdAbsolute(Tbyte,newlabel);
			 PutAdLabel(an_id,TRUE);
			 node = MakeTxNodeBefore((TN_Id)NULL,LABEL);
			 PutTxOperandAd(node,0,an_id);
			 PutTxOperandType(node,0,Tubyte);
			}
		endcase;
	 case 'R':	/* #REGAL ? */
		if(strncmp(s,"REGAL",5) != 0) endcase;
		parse_regal(s+5);	/*Read NAQ info in #REGAL stmt*/
		endcase;
	 case 'S':	/* #SWBEG or #SWEND ? */
		if(strncmp(s,"SWBEG",5) == 0)
			inswitch = 	/* Now in a switch table. */
			swflag = 
			TRUE;
		else if(strncmp(s,"SWEND",5) == 0)
			inswitch = FALSE;	/*Out of switch table.*/
		endcase;
	 case 'T':	/* #TYPE ? */
		if(strncmp(s,"TYPE",4) != 0) endcase;
		node = MakeTxNodeBefore((TN_Id) NULL,TYPE);
		s += 4;
		SkipWhite(s);	/* Skip white space.	*/
		for(t = s; *t != EOS ; t++)	/*Conv. to lower*/
			*t = tolower( *t );	/* case: WHY?? */
		PutTxOperandType(node,0,GetIntTypeId(s));
		endcase;
	 case 'V':	/* #VOL_OPND ? */
		if(strncmp(s,"VOL_OPND", 8) != 0) endcase;
		s += 8;
		SkipWhite(s);
		if((node = GetTxPrevNode((TN_Id)NULL)) == NULL)
			endcase;
		while( *s != EOS ){
			/* set volatile bit in operand */
			PutTxOperandVol(node,*s - '1',TRUE);
			s++;
			SkipWhite(s);
			if( *s == ',' ){ 
				s++;
				SkipWhite(s);
			}
		}
		endcase;
	} /* end of switch(*++s) */
}
	STATIC void
parse_regal( p ) 		/* read #REGAL comments */
register char *p;

{
 extern FN_Id FuncId;		/* FN_Id of current function.	*/
 extern int TySize();
 AN_Id anode;
 extern enum CC_Mode ccmode;	/* cc -X? */
 OperandType dtype;
 extern void fatal();
 STATIC OperandType parse_type();
 extern void parseop(); 	/* Parses an operand.	*/
 register char *q;
 enum {Unknown,Auto,Extdef,Extern,Param,Statext,Statloc} rt;
 OperandType type;

	/* the formats recognized are:
	 * 1) #REGAL <wt> NODBL
	 * 2) #REGAL <wt> {AUTO,PARAM,EXTERN,EXTDEF,STATEXT,STATLOC} <name> <size> [FP]
	 * where <wt> is currently ignored.
	 */
			/* skip over estimator, we don't use it any more */
	SkipWhite(p);
	FindWhite(p);

			/* scan to storage class and read it */
	SkipWhite(p);
	rt = Unknown;
	switch(*p){
	case 'A':
		if(strncmp(p,"AUTO",4) == 0)
			rt = Auto;
		endcase;
	case 'E':
		if(strncmp(p,"EXTDEF",6) == 0)
			rt = Extdef;
		else if(strncmp(p,"EXTERN",6) == 0)
			rt = Extern;
		endcase;
	case 'N':
		if(strncmp(p,"NODBL",5) == 0){
			PutFnNODBL(FuncId,TRUE);
			return;
		}
		endcase;
	case 'P':
		if(strncmp(p,"PARAM",5) == 0)
			rt = Param;
		endcase;
	case 'S':
		if(strncmp(p,"STATEXT",7) == 0)
			rt = Statext;
		else if(strncmp(p,"STATLOC",7) == 0)
			rt = Statloc;
		endcase;
	}
	if( rt == Unknown )
		fatal( "parse_regal:  illegal #REGAL type:\n\t%s\n",p);

			/* scan to name and delimit it */
	FindWhite(p);
	SkipWhite(p);
	q = p;
	FindWhite(q);
	*q = EOS;

			/* get type of object.	*/
	dtype = parse_type(++q);

			/* Parse address.	*/
 	parseop( p, dtype, &type, &anode);

	switch(rt)
	{case Auto:
	 case Param:
		PutAdGnaqType(anode,NAQ);	/* Update address node.	*/
		if(ccmode == Transition)	/* In transition mode,	*/
			PutAdSafe(anode,TRUE);	/* Node is SAFE.	*/
		endcase;
	 case Extern:
		if(ccmode != Transition)
			PutAdGnaqType(anode,ENAQ);
						/* mode == Transition: !safe */
						/* mode != Transition: already safe */
		endcase;
	 case Extdef:
		PutAdGnaqType(anode,ENAQ);
		if(ccmode == Transition)	/* In transition mode,	*/
			PutAdSafe(anode,TRUE);	/* Node is SAFE.	*/
		endcase;
	 case Statext:
		PutAdGnaqType(anode,SENAQ);
		if(ccmode == Transition)	/* In transition mode,	*/
			PutAdSafe(anode,TRUE);	/* Node is SAFE.	*/
		endcase;
	 case Statloc:
		PutAdGnaqType(anode,SNAQ);
		if(ccmode == Transition)	/* In transition mode,	*/
			PutAdSafe(anode,TRUE);	/* Node is SAFE.	*/
		endcase;
	}
 PutAdGNAQSize(anode,(unsigned short int)TySize(dtype));
 if(IsFP(dtype))
	PutAdFP(anode,YES);			/* Is FP.	*/
 else
	PutAdFP(anode,NO);			/* Is NOT FP.	*/
 return;
}
	STATIC void
parse_alias(s)
register char *s;

{extern FN_Id FuncId;
 AN_Id anode;
 OperandType dtype;
 char *name;
 STATIC OperandType parse_type();
 extern void parseop();
 OperandType type;

 SkipWhite(s);
 name = s;
 FindWhite(s);
 *s++ = EOS;
 dtype = parse_type(s);
 parseop(name, dtype, &type, &anode);
 if(infunc){	/* inside function, insert into alias list for this function. */
	PutFnAlias(FuncId, anode);
	if(IsAdArg(anode))	/* if parameter aliased,
				 * flag it in function node.
				 */
		PutFnPAlias(FuncId, TRUE);
 }
 else		/* else put in global list */
	PutExtAlias(anode);
}
	STATIC char *
parse_label(s, infile) /* called when s points to a label */
register char *s;
FILE *infile;
{
    extern enum CC_Mode ccmode;
    extern void fatal();
    extern void newlab();	/* Returns a new label string. */
    long atol();
    extern void put_FS_entry();	/* Save function start source line info,
				   in DebugInfo.c */
    register char *t; /* temporary string */
    unsigned short int LblType;	/* Type of label needed.	*/
    AN_Id an_id;
    
    if ( (*s == '.') && (*(s+1) != '.') ) /* Compiler generated label
				         which is not a tag (..) */
        LblType = LABEL; /* Make it soft, i.e., optimizer can delete it. */
    else
        LblType = HLABEL;	/* Seems to be a hard label */
    for(t = s; !eolbl(*t); ++t) /* Find ':' */
        ;
    if(InLibrary(infile) && (*t == '/'))
        /* At beginning of next archive member, */
        return t;		/* just quit. */
    
    if(*t == EOS)		/* error if ':' not found */
        fatal("parse_label: bad input format\n%s\n",s);
    *t = SPACE;		/* change ':' to SPACE */
    while( isspace(t[-1]) ) /* find end of label symbol */
        --t;
    *t = EOS; /* now s is the string value of the label */
	    	
    if (section == CStext) { /* Here for .text labels. */
        if(LblType == HLABEL) 
	    if(strncmp(s,"..LN",4)==0) { /* lineno label */
	        s += 4;
	        lineno=(unsigned) atol(s);
	        *t=SPACE;
	        return(t);	/* Eat the label */
            }
	    else if (strncmp(s,"..FS",4)==0) { /* Label marking
				the first source line entry for
				this function. */
		s += 4;
		put_FS_entry(atol(s));
	        *t=SPACE;
	        return(t);	/* Eat the label */
	    }
			/* Append label node. */
        { /* local block for lastnode */
	    register  TN_Id lastnode;	
	    /* TN_Id of last node inserted.	*/
	    lastnode = MakeTxNodeBefore((TN_Id) NULL,LblType);
	    an_id = GetAdAbsolute(Tbyte,s);
	    PutAdLabel(an_id,TRUE);
	    PutTxOperandAd(lastnode,0,an_id);
	    PutTxOperandType(lastnode,0,Tubyte);
        } /* block */
    }
    else {
	an_id = GetAdAbsolute(Tbyte,s);	
	if(ccmode == Transition)	/* Make */
	PutAdSafe(an_id,TRUE);	/* safe address. */
	if(section == CSrodata)
	    PutAdRO(an_id,TRUE);	/* Mark it readonly. */
	if(InLibrary(infile)){		/* Archive library.  */
	    /* rewrite label to prevent possible clash.  */
	    strncpy(oldarlab,s,sizeof(oldarlab));
	    newlab(newarlab,".I",sizeof(newarlab));
	    s = newarlab;
	}
	printf("%s:\n",s);
    }
    *t = SPACE;
    return t;
}
	STATIC OperandType
parse_type(s)
register char *s;

{OperandType dtype;
 extern void fatal();
 int len;
 extern long strtol();	/* Converts ASCII string to long; in C(3) lib.*/
 char *t;

 SkipWhite(s);
			/* read length in bytes */
 len = strtol( s, &t, 0 );
 if(s == t)
	fatal("parse_type: missing length\n");
 s = t;
	
			/* read floating point indicator */
 SkipWhite(s);
 if( (s[0] != EOS && s[0] == 'F') &&
     (s[1] != EOS && s[1] == 'P') ){
	switch( len ) { 
	case 12:dtype = Tdblext; break;
	case 8: dtype = Tdouble; break;
	case 4: dtype = Tsingle; break;
	default: fatal( "parse_type:  unknown FP length %d\n",len);
	}
 }
 else {
	switch( len ) { 
	case 4: dtype = Tword; break;
	case 2: dtype = Thalf; break;
	case 1: dtype = Tbyte; break;
	case 0: dtype = Tbyte; break;
	default: fatal( "parse_type:  unknown length %dL\n",len);
	}
 }
 return(dtype);
}
	STATIC void
 /* Parse pseudo-ops. There are 3 categories of directives: */
 /* 1. section control directives */
 /* 2. directives independent of the current section */
 /* 3. directives dependent on the current section */
parse_pseudo(s,Tfile,inlib)
register char *s;		/* points at pseudo-op */
FILE *Tfile;			/* Stream pointer of text intermediate file. */
boolean inlib;

{
 extern void addref();		/* Add text reference to reference list. */
 extern void fatal();		/* Fatal abort.	*/
 extern void fatalinit();	/* Lets fatal know current filename.	*/
 extern void init();
 extern FN_Id FuncId;		/* FN_Id of most recently seen function. */
 extern unsigned int plookup();	/* Looks up opcodes; defined in lookup.c. */
 extern boolean swflag;		/* Reset when we see end of function in
				   parse_pseudo(). */
 AN_Id an_id;			/* AN_Id of address of operand. */
 TN_Id lastnode, tn_id;
 char *lp;			/* pointer to beginning of pseudo */
 STATIC void maketrans();	/* Makes translation nodes.	*/
 STATIC long int pvsave();
 void sequence1(), funcdata(), ilcandidate();
 register unsigned int pop;	/* pseudo-op code */
 char savechar;			/* saved char that follows pseudo */
 extern enum Section prev_section; /* Type of previous control section. */
 extern enum Section section;	/* Type of current control section.	*/
 char *sname;			/* Section name.	*/
 STATIC enum Section tmp_section; /* for swaps */

 lp = s;
 FindWhite(s);					/* Find white space.	*/
 savechar = *s;					/* Mark end of pseudo-op. */
 *s = EOS;
 pop = plookup(lp);				/* Identify pseudo-op. */
 *s = savechar;					/* Restore saved character. */

 switch(pop){	/* dispatch on pseudo-op */

 case PLOWER:	/* pseudo-op not found */
   fatal("parse_pseudo: illegal directive in \"%s\".\n",lp);
   endcase;

 /* 1. The following directives keep track of the current and previous */
 /* sections.  They are written out at the beginning of the file in */
 /* the same order (before any code in the .text section), regardless */
 /* of original placement in the assembly file. */
 case PS_DATA:
   prev_section = section;
   section = CSdata;
   goto printit;
   /* NOTREACHED */
   endcase;
 case PS_PREVIOUS:
   tmp_section=section;
   section = prev_section;
   prev_section=tmp_section;
   goto printit;
   /* NOTREACHED */
   endcase;
 case PS_SECTION:
   SkipWhite(s);
   for(sname = s; *s != EOS; ++s)
     if(*s == COMMA || isspace(*s)) break;
   savechar = *s;
   *s = EOS;
   prev_section = section;
   if(strcmp(sname, ".rodata") == 0)
     section = CSrodata;
   else if(strcmp(sname, ".data1") == 0)
     section = CSdata1;
   else if(strcmp(sname, ".data") == 0)
     section = CSdata;
   else if(strcmp(sname, ".text") == 0)
     section = CStext;
   else if(strcmp(sname, ".debug") == 0) {
	if(inlib)
	    fatal("parse_pseudo(): debugging info not allowed in archive.\n");
	section=CSdebug;
   }
   else if(strcmp(sname, ".line") == 0) {
	if(inlib)
	    fatal("parse_pseudo(): debugging info not allowed in archive.\n");
	section=CSline;
   }
   else	/* unknown section */
     section = CSother;
   *s = savechar;
   goto printit;
   /* NOTREACHED */
   endcase;
 case PS_TEXT:
   prev_section = section;
   section = CStext;
   goto printit;	/* We must print this out, even though we put out */
			/* text nodes at the end of the file.  We won't be */
			/* able to keep track of previous sections otherwise. */
   /* NOTREACHED */
   endcase;

 /* 2. The following directives are written out to the beginning */
 /* of the file in the same order (before any code in the .text section),*/
 /* regardless of original placement in the assembly file.  ASSUMPTION: */
 /* THE FOLLOWING DIRECTIVES MAY NOT REFER TO THE LOCATION COUNTER (.) IF */
 /* THEY ARE ORIGINALLY PLACED IN THE .text SECTION. */
 case PS_BSS:
 case PS_COMM:
   goto printit;
   /* NOTREACHED */
   endcase;
 case PS_FILE:
   SkipWhite(s);
   fatalinit(s);
   if(!inlib)
     goto printit;
   endcase;
 case PS_GLOBL:
   if(inlib) {
     if ( section != CStext )
       fatal("parse_pseudo: global data not permitted in source library.\n");
   }
   else goto printit; /* don't print .globl's in archive */
   endcase;
 case PS_IDENT:
 case PS_LOCAL:
   goto printit;
   /* NOTREACHED */
   endcase;
 case PS_SET:
   SkipWhite(s);	/* Skip to arguments.	*/
   /* ASSUMPTION: */
   /* 	.set .F..... */
   /* CAN ONLY BE USED TO SET THE LOCAL SIZE OF A FUNCTION. */
   if((*s == DOT) && (*(s+1) == 'F'))
     {if(FuncId==NULL)
	 fatal("parse_pseudo(): illegal .set .F..\n");
      PutFnLocSz(FuncId,(unsigned long int) pvsave(s));
      PutFnLocNm(FuncId,pvhead->name);
      if(!inlib)
	goto printit;
    }
   /* ASSUMPTION: */
   /* 	.set .R..... */
   /* CAN ONLY BE USED TO SET THE NUMBER OF SAVED REGISTERS OF A FUNCTION. */
   else if((*s == DOT) && (*(s+1) == 'R')) {
      if(FuncId==NULL)
	 fatal("parse_pseudo():  illegal .set .R..\n");
      PutFnNumReg(FuncId,(unsigned int) pvsave(s));
   }
   else
     {(void) pvsave(s);
      if(!inlib)
	goto printit;
    }
   endcase;
 case PS_TYPE: if(!inlib) goto printit;
   endcase;
 case PS_VERSION:
 case PS_WEAK:
   goto printit;
   /* NOTREACHED */
   endcase;

 /* 3. The following directives must be inserted into the text nodes list */
 /* in the same relative position, if the current section is the .text section. */
 /* Otherwise, they can be written out to the beginning of the file in*/
 /* the same order. */
 default:

   switch(section) {

   case CStext:
     switch(pop) {		/* Dispatch on pseudo-op in .text */
     case PS_ALIGN:
       MakeRawNode(s,lastnode,Tbyte);	/* Why Tbyte ?? */
       endcase;
     case PS_BYTE:
       MakeRawNode(s,lastnode,Tbyte);
       endcase;
     case PS_HALF:
     case PS_2BYTE:
       MakeRawNode(s,lastnode,Thalf);
       endcase;
     case PS_4BYTE:
     case PS_WORD: 
       MakeRawNode(s,lastnode,Tword);
       endcase;
     case PS_FLOAT:
       MakeRawNode(s,lastnode,Tsingle);
       endcase;
     case PS_DOUBLE:
       MakeAbsNode(s,lastnode,Tdouble); /* Why not MakeRawNode() ?? */
       endcase;
     case PS_EXT:
       MakeRawNode(s,lastnode,Tdblext);
       endcase;
     case PS_ZERO:
       fatal("parse_pseudo: .zero illegal in .text.\n");
       endcase;
     case PS_SIZE:
	/* When the optimizer sees the .size instruction
	   in .text it takes that as the end of the function's
	   code.  This assumption ought to be spelled out
	   someplace other than this comment. */

	    { /* for now this code is unneeded */

	    char * s1;
	    char * fname;
	    int l;
	    SkipWhite(s);
	    if(FuncId==(FN_Id)NULL || FuncId->name == (AN_Id)NULL)
		fname=""; /* Avoid problems if not in function */
	    else fname=FuncId->name->key.K.string+1;
	    /* FuncId->name->key.K.string is the string
	       name of the function.  Maybe should write
	       an access macro for this? */
	    l=2*strlen(fname)+3;
	    if ( ( (s1=malloc(l+1)) != NULL ) && /* one more for null */
	        strcpy(s1,fname) &&
	        strcat(s1,",.-") &&
	        strcat(s1,fname) ) {

		if (  strcmp(s1,s) == 0 ) {
		    free(s1);
		}
		else {
       		    /* MakeRawNode(s,lastnode,Tnone);*/	/* in case of ".size fn, . - fn" */
	  /* This is some kind of .size in text unknown to us,
	     perhaps for a global array? */
		    goto printit;
   /* NOTREACHED */
		    break;
		}
	    }
	    else
	        fatal("parse_psuedo(): parsing .size.\n");

	    }
	    sequence1();
	    if (! inlib ) {
		for(ALLN(tn_id)) /* Save text from pass 1. */
		    WriteTxNode(tn_id,Tfile);
		/* End the list.	*/
	        tn_id = MakeTxNodeBefore((TN_Id) NULL,TAIL);
		WriteTxNode(tn_id,Tfile);
		DelTxNode(tn_id);
	    }
	    funcdata();
	    ilcandidate();
	    PutFnPrivate(FuncId);
	    init();
	    pvhead = NULL;
	    swflag = FALSE;		/* reset switch table flag */
	    infunc = FALSE;		/* leaving function. */
	    DelTxNodes((TN_Id) NULL,(TN_Id) NULL);
	    DelAdTransNodes();
       if(! inlib )
            MakeRawNode(s,lastnode,Tbyte);
       /* Tbyte, because we want the operand to print, but
	  the assumption is that it is of the form fn,.-fn */
       endcase;
     } /* END OF switch(pop) in .text */
     endcase;	/* of case CStext: */
   case CSline:
       return; /* don't print line info: it will be recreated later */
   default:
     switch(pop) {		/* Dispatch on pseudo-op in non-.text section */
     case PS_ALIGN:
     case PS_SIZE:
       goto printit;
   /* NOTREACHED */
       endcase;
     case PS_BYTE:
     case PS_HALF:
     case PS_2BYTE:
     case PS_4BYTE:
     case PS_FLOAT:
     case PS_DOUBLE:
     case PS_EXT:
     case PS_ZERO:
       if (inlib)
	 maketrans(pop,s);
       goto printit;
   /* NOTREACHED */
       endcase;
     case PS_WORD: 
	if (inswitch) {

	/* Jump table.  We must keep track of all references from other */
	/* sections to labels in .text, so that we do not remove them later. */
	/* ASSUMPTION:  EXCEPT FOR TAG LABELS,  LOCAL LABELS IN THE .TEXT */
	/* SECTION ARE ONLY REFERENCED FROM WITHIN THE .TEXT SECTION OR */
	/* FROM JUMP TABLES. */

	SkipWhite(s);
	if(PIC_flag) {
	    char * temp;
	    if(temp=strchr(s,'-')) {
		*temp ='\0';
		an_id = GetAdAbsolute(Tbyte,s); 
		addref(an_id);
		*temp = '-';
		s = ++temp;
		SkipWhite(s);
		an_id = GetAdAbsolute(Tbyte,s); 
		addref(an_id);
	    }
	    else
		fatal("parse_psuedo(): illegal PIC jump table.\n");
	}
	else {
	    an_id = GetAdAbsolute(Tbyte,s); /*Need an_id of it.*/
	    addref(an_id);	/* Add the reference. */
	}
    } /* if (inswitch) */
       if(inlib)
	 maketrans(pop,s);
       goto printit;
   /* NOTREACHED */
       endcase;
     } /* END OF switch(pop) in default */

   } /* END OF switch(section) */

 } /* END OF switch(pop) */
 return;
 printit:
	printf("\t%s\n",lp);
}
	STATIC void
maketrans(pop,s)		/* create translation node for data labels */
unsigned int pop;
char *s;			/* points to pass the pseudo op. */
{
 AN_Id an_id;
 extern enum CC_Mode ccmode;	/* cc -X?	*/
 int nitems;			/* number of items in a .word list */
 OperandType type;		/* type of addr nodes.	*/
 extern void fatal();
 extern long strtol();		/* Converts ASCII string to long; in C(3) lib.*/

 if(newarlab[0] == EOS)		/* nothing to translate */
	return;

 switch(pop){
 case PS_BYTE:
	type = Tbyte;
	endcase;
 case PS_HALF:
 case PS_2BYTE:
	type = Thalf;
	endcase;
 case PS_WORD:
 case PS_4BYTE:
 case PS_FLOAT:
	SkipWhite(s);
	for(nitems = 1; *s != EOS; ++s)
		if( *s == ',' )
			++nitems;
	switch(nitems){
	case 1:	type = T1word;	endcase;
	case 2:	type = T2word;	endcase;
	case 3:	type = T3word;	endcase;
	default:
		fatal("translate: too many items after %s\n",oldarlab);
	}
	endcase;
 case PS_DOUBLE:
	type = Tdouble;
	endcase;
 case PS_EXT:
	type = Tdblext;
	endcase;
 case PS_ZERO:
	SkipWhite(s);
	switch(strtol(s,&s,0)){
	default:
	case 1:  type = Tbyte;	endcase;
	case 2:  type = Thalf;	endcase;
	case 4:  type = T1word;	endcase;
	case 8:  type = T2word;	endcase;
	case 12: type = T3word;	endcase;
	}
	endcase;
 default:
	fatal("maketrans: illegal directive passed to maketrans\n");
	endcase;
 }
 an_id = GetAdAbsolute(type, newarlab);
 if(ccmode == Transition)	/* In transition mode,	*/
	PutAdSafe(an_id, TRUE);	/* set MMIO safety.	*/
 if(section == CSrodata)	/* In rodata,		*/
	PutAdRO(an_id, TRUE);	/* mark node read-only.	*/
 (void)GetAdTranslation(type, oldarlab, an_id);
 newarlab[0] = EOS;		/* Only do it once. */
}
	STATIC void
parse_instr(s,infile)
register char *s;		/* points to the first char of mnemonic. */
FILE *infile;			/* input file.				 */
				/* line looks like: "<ws>mnemonic<ws>opn1,...,opnN" */
{			
 extern void mark_func();	/* In DebugInfo.c: enter FuncId into
				   debug info. */
 extern FN_Id FuncId;		/* FN_Id of most recently seen function. */
 extern void fatal();
 TN_Id lastnode;
 extern unsigned int lookup();	/* Looks up opcodes; in this file. */
 extern boolean misinp;		/* TRUE if MIS instructions detected.	*/
 extern unsigned int ninst;	/* Number of instructions processed.	*/
 unsigned short int opn;	/* Operation code index.	*/
 extern struct opent optab[];	/* The operation-code table.	*/
 STATIC void parse_ops();	/* Handles parsing of operands. */
 TN_Id prev;
 register char *t;

			/* Skip over instruction mnemonic. */
	t = s;
	FindWhite(t);
	*t = EOS;	/* demarcate with EOS*/

	opn = lookup(s);
	if(opn == OLOWER)
		 fatal("parse_instr: unknown opcode (%s).\n",s);

	if((optab[opn].oflags & SPOPC) || (opn == EXTOP)){ 
		 /* treat SPOP instructions (not MIS, just SPOPs )
		  * and EXTOP like ASMS */
		*t = SPACE;
		--s;	/* back over the white space. */
		lastnode = MakeTxNodeBefore((TN_Id) NULL,ASMS);
		PutTxOperandAd(lastnode,0,GetAdRaw(s));
		PutTxBlackBox(lastnode,TRUE);
	}
	else{
		 /* Create new node at end. */
		lastnode = MakeTxNodeBefore((TN_Id) NULL,opn);
		ninst += 1;
		s = t + 1;
		SkipWhite(s);	/* Skip to first operand, and parse. */
		parse_ops(s,0,lastnode);
				/* Remember function name.	*/
		if((opn == ISAVE) || (opn == SAVE)){
			prev = GetTxPrevNode(lastnode);
			if(prev == NULL)
				fatal("parse_instr: missing function name\n");
			FuncId = GetFnId(GetTxOperandAd(prev,0));
			infunc = TRUE;		/* entering a function. */
			if(InLibrary(infile))	/* mark fcns from archive. */
				PutFnLibrary(FuncId, TRUE);
			else
				mark_func(FuncId); /* Fix the debug info entry */
		}
		else if(IsOpMIS(opn)) {	/* Check for MIS code in input.	*/
			misinp = TRUE;
		}
	}
	PutTxUniqueId(lastnode,lineno);
	lineno = IDVAL;
}
	STATIC void
parse_ops(s,opn,tn_id)
register char *s;
unsigned int opn;		/* Operand counter. */
TN_Id tn_id;			/* Node under consideration. */

{
 extern FN_Id FuncId;		/* FN_Id of current function.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 AN_Id an_id;			/* AN_Id for this operand.	*/
 register int c;
 extern void fatal();
 unsigned int operand;		/* Operand counter.	*/
 extern struct opent optab[];	/* The operation code table.	*/
 extern void parseop();		/* Parses an operand; in Machine Dep.	*/
 unsigned int rvregs;
 register int state;		/* Keeps track of (,) state.	*/
 char *t;
 OperandType tdflt[MAX_OPS];
 OperandType type;		/* Type of operand.	*/

 for(operand = 0; operand < Max_Ops; operand++)
	tdflt[operand] =
		(OperandType) optab[GetTxOpCodeX(tn_id)].otype[operand];

 if(tdflt[0] == Tnone)		/* 0 operand instructions */
	return;

 while(*s)
	{SkipWhite(s);			/* Skip leading whitespace. */
	 t = s;				/* Remember effective operand start */

					/* An operand of the form '(%r,%r)' */
					/* contains a ',', so we need to scan */
					/* until a ',' outside of the operand */
	 state = OUT;
	 for(c = *s; !(state == OUT && c == COMMA) && 
			c != EOS &&
			c != ComChar; c = *++s)
		switch(c)
			{case '(':
				state = IN;
				endcase;
		 	 case ')':
				state = OUT;
				endcase;
			}

	 switch(c)
		{case ComChar:		/* Get return value register count */
			if(IsTxRet(tn_id))
				{rvregs = s[1] - '0';		/*Conv dig. to binary.*/
				 PutFnRVRegs(FuncId,rvregs);	/* Save for pass 2. */
				}
			*s = EOS;
			endcase;
		 case EOS:
			endcase;
		 default:
			*s++ = EOS;
			endcase;
		}
	 if(tdflt[opn] == Tnone)
		fatal("parse_ops: too many operands for instruction %s\n",
			optab[GetTxOpCodeX(tn_id)].oname);
	 parseop(t,tdflt[opn],&type,&an_id);
	 if(type != tdflt[opn]) 
		for(operand = opn + 1; operand < Max_Ops; operand++)
			if(tdflt[operand] != Tnone)
				tdflt[operand] = type;
	 PutTxOperandAd(tn_id,opn,an_id);
	 PutTxOperandType(tn_id,opn,type);
	 ++opn;
	}	/* END OF while(*s) */

 if((opn < Max_Ops) && (tdflt[opn] != Tnone))
	fatal("op: missing operand for opcode %s\n",
		 optab[GetTxOpCodeX(tn_id)].oname);
}
	char *
getline(file)
FILE *file;		

{
 register char *s;
 register int c;
 static char *p0 = NULL;		/* Beginning of line buffer. */
 static char *pn = NULL;		/* End of line buffer.	*/
 extern char *ExtendCopyBuf();
 extern void fatal();

	/* each line of input can contain multiple instructions separated
	 * by ';' as a line terminator,
	 * and return each line as a string terminated by EOS.
	 * the static variables p0 & pn keeps track of beginning & end of
	 * line buffer.
	 */

	if( p0 == NULL )	/* initialize buffer */
		{p0 = Malloc(MAX_LSLINE+1);
		 if(p0 == NULL)
			fatal("getline: out of buffer space\n");
		 pn = p0 + MAX_LSLINE;
		}
	/* read until end of instruction */
	s = p0;
	while( (c = getc(file)) != EOF )
		{if(s >= pn)
			s = ExtendCopyBuf(&p0,&pn,(unsigned)2*(pn-p0+1));
		 if(eoi(c))
			{switch(c)
				{case ';':
				 case EOS:
				 case NEWLINE:
					*s = EOS;
					return(p0);
				 case ComChar:
					*s++ = (char)c;
					endcase;
				}
			 /* here if ComChar, read to end of line */
			 while((c = getc(file)) != EOF)
				{if(s >= pn)
					s = ExtendCopyBuf(&p0,&pn,(unsigned)2*(pn-p0+1));
				 if(eol(c))
					{*s = EOS;
					 return(p0);
					}
				 else
					*s++ = (char)c;
				}
			 /* premature EOF */
			 if(s > p0)
				{*s = EOS;
				 return(p0);
				}
			 return(NULL);
			}
		 else
			*s++ = (char)c;
		}
	/* EOF */
	if(s > p0)	/* premature */
		{*s = EOS;
		 return(p0);
		}
	return(NULL);
}
	STATIC void
do_libsa(libp)			/* Processes src archives.   */
struct liblist *libp;		/* list of arcive libraries. */
{
 FILE *libfile;			/* stream pointer of a src arch library	*/
 register char *lp;		/* pointer to line of input from sa lib	*/
 long home;			/* file offset to beginning of member	*/
 char *s;
 char *fname;			/* pointer to saved file name.	*/
 extern void fatal();		/* handles fatal error messages.	*/
 extern void fatalinit();	/* initializes file name for err mesgs.	*/
 extern char *fatalsave();	/* saves file name for error messages.	*/
 char *getline();	/* gets a line of assembler.	*/
 extern int iliscalled();	/* answers "has function been called?"	*/
 STATIC FILE *libopen();	/* opens an archive library file.	*/
 STATIC boolean memhead();	/* answers "is this a member header?"	*/
 extern void init();		/* initializes PCI.	*/
 STATIC void pass1();		/* pass 1, does parsing of input.	*/
 STATIC void skipmem();		/* skips over a sa member function.	*/

 lextab['/'] |= _E_O_Lbl;	/* for scanning member header.	*/

 fname = fatalsave();		/* save file name for error messages.	*/

 for(; libp != NULL; libp = libp->next)
 {
	libfile = libopen(libp->libname);
	while((lp = getline(libfile)) != NULL)
	{
		if(memhead(lp)){	/* if first or empty member.	*/
			home = ftell(libfile);
			continue;
			}
					/* look for '.globl<ws>fname'	*/
		SkipWhite(lp);
		if(strncmp(lp,".globl",6) == 0)
		{
			FindWhite(lp);
			SkipWhite(lp);
			s = lp;
			FindWhite(lp);
			*lp = EOS;	/* s -> fname	*/
			if(iliscalled(s) == 1)	/* if called, pull it in.	*/
			{
				if(fseek(libfile,home,0) != NULL)
					fatal("do_libsa: fseek failed for '%s(%s)'\n",
						libp->libname, s);
				section = CStext;
				init();
				pvhead = NULL;
				pass1(libfile, (FILE *)NULL, 1);
			}
			else			/* otherwise, skip to next one.	*/
				skipmem(libfile);
			home = ftell(libfile);	/* reset home offset */
		}
	}
				
 }
 fatalinit(fname);	/* restore file name for error messages.	*/
}
	STATIC void
skipmem(f)
FILE *f;
{
 register char *s;
 char *getline();
 STATIC boolean memhead();

 while((s = getline(f)) != NULL)
	if(memhead(s))
		return;
}

	STATIC boolean
memhead(s)
register char *s;
{
 if(isspace(*s))
	return FALSE;
 for( ; !eolbl(*s); ++s)
	;
 if( *s == '/' )
	return TRUE;
 return FALSE;
}
	STATIC FILE *
libopen(arnam)			/* Open source library file.	*/
char *arnam;

{FILE *af;
 char buf[SARMAG + 1];
 /*extern FILE *fopen();	 * CLIB file open.	*/
 extern void fatal();

 af = fopen(arnam,"r");
 if(af == NULL)
	fatal("libopen: cannot open source archive file %s\n",arnam);
 if((fread(buf,1,sizeof(char) * SARMAG,af) != sizeof(char) * SARMAG) ||
		strncmp(buf, ARMAG, SARMAG) != 0)
	fatal("%s not in archive format\n",arnam);
 return (af);
}

	void
addlib(name)			/* add a library to the list.	*/
char *name;
{
 struct liblist *p;
 extern void fatal();
				/* get new list item.	*/
 p = (struct liblist *) Malloc(sizeof(struct liblist));
 if(p == NULL)
	fatal("addlib: Malloc failed.\n");
 p->next = NULL;
 p->libname = name;
				/* append to list.	*/
 liblast->next = p;
 liblast = p;
}
	void
yyinit()

{
 extern enum Section section;	/* Type of current control section.	*/

 section = CStext;
 lextab[':'] |= _E_O_Lbl;
 lextab[ComChar] |= _E_O_I;
 lextab[';'] |= _E_O_I;
 lextab[NEWLINE] |= (_E_O_I|_E_O_L);
 lextab[EOS] |= (_E_O_I|_E_O_L|_E_O_Lbl);
 return;
}
	STATIC long int
pvsave(s)
register char *s;	 /* save pseudo-variable value in .set */
	
{extern void fatal();		/* Fatal abort.	*/
 extern char *getspace();
 /*extern char *memcpy();	** Memory to memory copy; in C(3) library.*/
 struct pvent *pv;
 extern long strtol();		/* Converts ASCII string to long; in C(3) lib.*/
 register char *t;
 int l;

 /* s points to first nonwhite character after '.set'.
  */
 pv = GETSTR( struct pvent );		/* create and link list entry */
 pv->next = pvhead;
 pvhead = pv;
 t = s;
 while(*t != ',' && *t != EOS)
	++t;
 if( *t == EOS ) 
	fatal("pvsave: improperly formatted '.set'.\n" );
 pv->value = strtol( t + 1, (char **) NULL, 0 ); /* get value */
 
 while( isspace( t[-1] ) ) 	/* get variable name */
	--t;	
 l = t - s;
 pv->name = memcpy( getspace((unsigned) (l + 1)), s, l );
 pv->name[l] = EOS;
 return( pv->value );
}
	void
pveval()			 /* Evaluate pseudo-variables */

{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 static char *Mbpveval = {"pveval: before evaluation of pseudo variables"};
 extern void addrprint();	/* Prints address table.	*/
 register AN_Id an_id;
 char *expression;		/* expression part of address.	*/
 extern void fatal();		/* Handles fatal errors: in common.	*/
 extern void funcprint();	/* Prints a function.	*/
 AN_Mode mode;			/* Mode of an address node.	*/
 register unsigned int operand;
 struct pvent *pv;
 RegId regA;			/* Place for register A, if any.	*/
 RegId regB;			/* Place for register B, if any. */
 register TN_Id tn_id;
 OperandType type;		/* Type of an operand.	*/
 char value[16+1];		/* Place for ASCII version of pv->value. */

 if(DBdebug(3,XINCONV))				/* Print address table	*/
	addrprint(Mbpveval);			/* if  wanted.	*/
 if(DBdebug(1,XINCONV))				/* Print function	*/
	funcprint(stdout,Mbpveval,0);		/* if wanted.	*/

 for(ALLN(tn_id))
	{if(IsTxLabel(tn_id))			/* No point evaluating labels.*/
		continue;			/* It is a label: skip it. */
	 for(operand = 0; operand < Max_Ops; operand++)
		{if((type = GetTxOperandType(tn_id,operand)) == Tnone)
			break;
		 an_id = GetTxOperandAd(tn_id,operand);
		 if(IsAdNumber(an_id))		/*No point evaluating numbers.*/
			continue;		/* This one is numeric.	*/
		 if(IsAdLabel(an_id))		/* No point evaluating labels.*/
			continue;		/* This one is a label.	*/
		 expression = GetAdExpression(type,an_id);
		 for(pv = pvhead; pv != NULL; pv = pv->next)
			{if(strcmp(expression,pv->name) == 0)
						/* Get mode of node. */
				{if(sprintf(value,"%d",pv->value) >=
						sizeof(value))
					fatal("pveval: value too large (%d).\n",
						pv->value);
				 switch((mode = GetAdMode(an_id)))
					{case Absolute:
					    an_id = GetAdAbsolute(type,value);
						endcase;
					 case AbsDef:
						an_id = GetAdAbsDef(value);
						endcase;
					 case CPUReg:
					    fatal("pveval: logic error 1.\n");
						endcase;
					 case StatCont:
					    fatal("pveval: logic error 2.\n");
						endcase;
					 case Disp:
						regA = GetAdRegA(an_id);
						an_id = GetAdDisp(type,value,
							regA);
						endcase;
					 case DispDef:
						regA = GetAdRegA(an_id);
						an_id = GetAdDispDef(value,
							regA);
						endcase;
					 case Immediate:
						an_id = GetAdImmediate(value);
						endcase;
					 case IndexRegDisp:
						regA = GetAdRegA(an_id);
						regB = GetAdRegB(an_id);
						an_id = GetAdIndexRegDisp(type,
							value,regA,regB);
						endcase;
					 case IndexRegScaling:
						regA = GetAdRegA(an_id);
						regB = GetAdRegB(an_id);
						an_id = GetAdIndexRegScaling(type,
								value,
								regA,
								regB);
						endcase;
					 case MAUReg:
					    fatal("pveval: logic error 3.\n");
						endcase;
					 case PostDecr:
						regA = GetAdRegA(an_id);
						an_id = GetAdPostDecr(type,
							value,regA);
						endcase;
					 case PostIncr:
						regA = GetAdRegA(an_id);
						an_id = GetAdPostIncr(type,
							value,regA);
						endcase;
					 case PreDecr:
						regA = GetAdRegA(an_id);
						an_id = GetAdPreDecr(type,
							value,regA);
						endcase;
					 case PreIncr:
						regA = GetAdRegA(an_id);
						an_id = GetAdPreIncr(type,
							value,regA);
						endcase;
					 default:
					  fatal("pveval:unknown mode (0x%x).\n",
							mode);
						endcase;
					} /* END OF switch(GetAdMode(an_id)) */
				 PutTxOperandAd(tn_id,operand,an_id);
				 break;
				} /*END OF if(strcmp(expression,pv->name)==0)*/
			} /*END OF for(pv = pvhead; pv != NULL; pv = pv->next)*/
		} /* END OF for(operand = 0; operand < Max_Ops; operand++) */
	} /* END OF for(ALLN(tn_id)) */

 if(DBdebug(2,XINCONV))
	funcprint(stdout,"pveval: after evaluation of pseudo variables",0);

 return;
}
	void
translate()			/* Translate data label references.	*/
				/* By now, all labels have been rewritten,
				 * and translation nodes created, but refs
				 * to these labels in text nodes still need
				 * translation.
				 */
{
 extern FN_Id FuncId;
 register TN_Id tn_id;
 register AN_Id an_id;
 register AN_Id an0;
 AN_Mode m;
 long int offset;
 unsigned short op;
 int operand;
 OperandType type;

 if(!IsFnLibrary(FuncId))	/* if not src library member,	*/
	return;			/* just return.			*/

 for(ALLN(tn_id)){
	op = GetTxOpCodeX(tn_id);
	if(IsOpPseudo(op) || IsOpAux(op))
		continue;
	for(ALLOP(tn_id,operand)){
		an_id = GetTxOperandAd(tn_id,operand);
		switch(m = GetAdMode(an_id)){
		case Absolute:	/* only care about absolutes	*/
		case AbsDef:	/* and absolute deferred  	*/
			endcase;
		default:
			continue;
		}
		type = GetTxOperandType(tn_id,operand);
		offset = GetAdNumber(type,an_id);
		if( offset == 0 ){			/* no const part	*/
			if(m == AbsDef)			/* *X => X		*/
				an_id = GetAdUsedId(an_id,0);
			if(IsAdTranslation(an_id)){
				an_id = GetAdUsedId(an_id,0);	/* X => Y	*/
				if(m == AbsDef)			/* Y => *Y	*/
					an_id = GetAdAddIndInc(Taddress,type,an_id,0);
				PutTxOperandAd(tn_id,operand,an_id);
				
			}
		}
		else{					/* there is a const part*/
			if(m == AbsDef)				/* get X+n	*/
				an_id = GetAdUsedId(an_id,0);
			an0 = GetAdAddToKey(type,an_id,-offset);/* get X	*/
			if(IsAdTranslation(an0)){		/* if X -> Y	*/
				an_id = GetAdUsedId(an0,0);	/* get Y	*/
				if(m == AbsDef)			/* get *Y+n	*/
					an_id = GetAdAddIndInc(Taddress,type,an_id,offset);
				else 				/* get Y+n	*/
					an_id = GetAdAddToKey(type,an_id,offset);
				PutTxOperandAd(tn_id,operand,an_id);
			}
		}
	}
 }
}
	void
AN_INIT()			/* Initialize the address node package. */


{extern AN_Id A;		/* AN_Id of A Condition Code.	*/
 extern AN_Id C;		/* AN_Id of C Condition Code.	*/
 extern AN_Id N;		/* AN_Id of N Condition Code.	*/
 extern AN_Id V;		/* AV_Id of V Condition Code.	*/
 extern AN_Id X;		/* AX_Id of X Condition Code.	*/
 extern AN_Id Z;		/* AZ_Id of Z Condition Code.	*/
 extern m32_target_cpu cpu_chip;	/* Type of cpu chip in use.	*/
 extern AN_Id i0;		/* AN_Id of immediate 0. */
 extern AN_Id i1;		/* AN_Id of immediate 1. */

 A = GetAdStatCont(CCODE_A);
 C = GetAdStatCont(CCODE_C);
 N = GetAdStatCont(CCODE_N);
 V = GetAdStatCont(CCODE_V);
 if(cpu_chip == we32200)
	X = GetAdStatCont(CCODE_X);
 Z = GetAdStatCont(CCODE_Z);

 i0 = GetAdImmediate("0");			/* Set AN_Id of immediate 0. */
 i1 = GetAdImmediate("1");			/* Set AN_Id of immediate 1. */

 return;
}

	void
ParamInit()
{
 extern unsigned int Avg_Expression;	/* Average expression length.	*/
 extern unsigned int Max_Ops;		/* Maximum operands in an instruction.	*/
 extern unsigned int Nom_Instrs;	/* Number of instructions initially. */
 extern void fatal();			/* Handles fatal errors; in common. */
 int size;

 Avg_Expression = 8;	/* Average expression size with NULL.*/
 Max_Ops = MAX_OPS;	/* Maximum operands per instruction. */
 Nom_Instrs = 1000;	/* Instructions allocated at once. */

			/* Set malloc granularity. */
 if(mallopt(M_GRAIN,(int) Avg_Expression) != 0)
	fatal("main: mallopt: M_GRAIN returned non-zero.\n");
			/* Set "bunches" of instrs. */
 if(mallopt(M_NLBLKS,(int) Nom_Instrs) != 0)
	fatal("main: mallopt: M_NLBLKS returned non-zero.\n");
			/* Set size for address nodes.*/
 size = (sizeof(struct node) > (sizeof(struct addrent) + Avg_Expression)) ?
	sizeof(struct node) : sizeof(struct addrent) + Avg_Expression;
 if(mallopt(M_MXFAST,size) != 0)
	fatal("main: mallopt: M_MXFAST returned non-zero.\n");
}
/* @(#)newoptim:m32/local.c	1.33 */
