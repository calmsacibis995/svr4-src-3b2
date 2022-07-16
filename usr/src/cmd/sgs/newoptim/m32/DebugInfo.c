/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/DebugInfo.c	1.11"

/* ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *

   This module gives the optimizer the ability to read elf-style
   debugging information and to write this information out
   to the optimized assembly file, suitably modified.

   During pass 1, the debugging information is scanned and
   and saved in a temporary file, Dfile.  Also a queue is
   built which contains pointers into Dfile ( right now these
   pointers are just line numbers ). The queue contains
   sufficient information to find the start and end in Dfile
   of the debugging information for each function and the
   lines in Dfile which may need to be changed after optimization.

   Also during pass 1 all information in the .line section 
   is discarded: the optimizer keeps track of this information
   using the ..LN labelling convention and a new version of
   .line is created as the text nodes are printed.

   Debugging entries of importance to the optimizer have the
   following tags ( see the header dwarf.h ).
	1) TAG_global_subroutine, TAG_subroutine: these will
	   get changed to TAG_inline_subroutine if the
	   function is inline expanded
	2) TAG_local_variable, TAG_formal_parameter: these may
	   get their location attributes changed, if the optimizer
	   puts them into registers.  In the future, we may
	   also be able to handle  global ( or file static )
	   variables which get put into registers for the
	   duration of a function call.
	3) TAG_label: user defined label. These may get
	   removed by the optimizer, so we must be sure
	   no reference to them remains in the .debug section.

   During pass 2, the Dfile is printed out on a per function
   basis: the entries in the queue for the given function
   are popped and used to direct the ( modified ) printing of
   Dfile.

   At the end of pass 2, pseudo variables in the .debug section
   are given values using .set directives, so that the debugging
   entries for each function properly reflect whether or not that
   function has been inline expanded.

   ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include "defs.h"
#include "ANodeTypes.h"
#include "olddefs.h" 		/* for ldtab and TNodeTypes.h */
#include "OpTabTypes.h"		/* needed for TNodeTypes.h */
#include "ALNodeType.h"		/* for FNodeTypes.h */
#include "TNodeTypes.h"		/* needed for FNodeTypes.h */
#include "FNodeTypes.h"		/* for FN_Id typedef */
#include "FNodeDefs.h"		/* for access to function ids */
#include "optab.h"
#include "CSections.h"
#include "RegId.h"
#include "RegIdDefs.h"
#include "ANodeDefs.h"
#include <dwarf.h>

/* pass 1 routines, called from pass1() in local.c */

void init_debug(); /* initializes DebugInfo module.  Called by pass1() */
void parse_debug(); /* entry point for pass1, called from pass1()
		       in local.c */
void mark_func();  /* enters the FN_Id into the debug info queue */


/* pass 2 routines, called from pass2() in local2.c */

void print_debug(); /* entry point for pass2, called from pass2()
		       in local2.c */
void handle_function_tags(); /* called at the end of pass 2 to
				handle debug info for inline expansions */

/* routines to handle .line section, called from pass2() */
void init_line_section();	/* Must be called before first text
				   node is written out. */
void exit_line_section();	/* Wrapup */
void print_line_info();
void print_FS_line_info(); 	/* Called at the end of print_debug()
				   to output the special start of function
				   source line entry. */

/* global data structures and variables */

extern enum Section prev_section; /* Type of previous control section. */
extern enum Section section;	/* Type of current control section.	*/

typedef struct d_entry {	/* In memory rep of debug info
				   entry that could be of interest. */
	short TAG;	/* TAG_global_subroutine, TAG_subroutine,
			   TAG_inline_subroutine,
			   TAG_local_variable, TAG_formal_parameter,
			   TAG_label, TAG_padding
			   TAG_padding is a hack: if the value of
			       TAG is TAG_padding, loc_head stores the
			       the value of the function's first source
			       line, as read from a ..FS.. label. */
	FN_Id fnid;	/* For subroutine TAGs: type should be FN_Id */
	long loc_head, loc_tail;     /* Offsets in Dfile for lines 
		which may need to be changed. 
		For variables, this is the AT_location attribute.
		For functions, the TAG ( which may get changed to inline ).
		For labels, the AT_low_pc attribute. */
	AN_Mode mode; /* Disp or CPUReg or other */
	RegId regid;  /* e.g., CREG9 */
	int displacement; /* value of displacement off of %ap or
			     %fp, read from location entry */
	struct d_entry *next;	/* Next "interesting" entry */
} D_entry, *D_ptr;

/* externs not in headers, mostly to shut lint up */
extern long strtol();
extern void free();
extern unlink();
extern void fatal();

/* file statics */
static init_debug_flag=0; /* if the compiler does not issue any
			     debugging info,  we will not issue any
			     debugging info ( or line info ) */
static D_ptr D_head=NULL, D_tail; /* Queue of "interesting" entries. */
static FILE *Dfile;	/* Temporary file, created on pass 1, written
			   out in modified form on pass 2. */

/* private declarations for pass1 */
typedef enum tok {tok_label, tok_ps_byte, tok_ps_2byte, tok_ps_4byte, 
		  tok_ps_string, tok_dont_care, tok_end} token;
	/* In the optimizer's simplified view of the world,
	   there are only 6 assembly language tokens:
		tok_label: a label definition, value is pointer to first
			char
		tok_ps_byte, tok_ps_2byte,tok_ps_4byte,tok_ps_string: 
		    pseudo_ops returned by plookup()
		    value is the string operand of the pseudo-op
		tok_end: EOF or a section change directive
		tok_dont_care: what it says
	*/

static char *current_line, *tok_val, *next_char;
    /* pointers to beginning of line, value of token
       current_line is initialized by parse_debug,
       otherwise only get_token() sets them */

static long linenumber;
static token current_token, get_token();

static FILE *infile;
static D_ptr new_entry(); /* Allocates and returns pointer to new entry */
static void put_entry();  /* Push the given entry on the end of the queue */
static void set_offsets();
static void put_Dfile(); /* Write to Dfile */
static void skip_past_label(); /* skip this entry */
static void skip_attribute(); /* skip attribute based on form as
				 defined in dwarf.h */
static int parse_location(); /* parse the location entry, checking to
		       * see if it's a parameter or auto variable.
		       * If it is, update the new entry and return
		       * 1, otherwise return 0 ( we are not interested
		       * in globals, file or block statics */

	void
parse_debug(s)
char *s;
{

    short tag_value;
    short target_attribute, attribute;
    int found_attribute /*, mallocd_label */;
    char *label;

    D_ptr tmp_ptr;
    long starting_line;

    next_char=current_line=s;
    current_token=get_token();
    while(current_token != tok_end) { /* Look for debug entries
					 until end of section. */

	target_attribute=0;
	while(target_attribute==0) { /* loop exits after the tag of an
		      interesting entry has been found */

	    do { /* until the .4byte following a label has
		    been read */

	        while(current_token != tok_label) {
	            current_token=get_token();
		    if (current_token==tok_end) return;
		}
			
		do { /* keep looking */
		    current_token=get_token();
		    if (current_token==tok_end) return;
		} while(current_token == tok_label);
		
	    } while(current_token != tok_ps_4byte);

	    /* .4byte: expect a length in one of the forms
		constant ( skip the entry )
		label ( skip the entry )
	        label-save_label
	    */
	    if(isdigit(*tok_val)){ /* It's a constant, skip entry */
		continue;
	    }
	    else {
		/* if(mallocd_label) free(label); */
		if((label=strdup(tok_val))==(char *) NULL)
		    fatal("parse_debug(): strdup failed\n");
		/* else mallocd_label=1; */
		{ 
		    char *temp;
		    if(temp=strchr(label,'-')) 
			*temp='\0';
		    else {
			skip_past_label(label);
			if(current_token==tok_end) return;
			continue;
		    }
		}
		current_token=get_token();
		if (current_token == tok_end) return;
	    }

	    /* Now look for TAG for this entry */
	    if(current_token != tok_ps_2byte) {
		skip_past_label(label);	
		if(current_token==tok_end) return;
		continue;
	    }
	    tag_value=(short)strtol(tok_val,(char **)NULL,0);

	    switch(tag_value) {
	    case TAG_subroutine:
	    case TAG_global_subroutine:
		target_attribute=AT_name;
		starting_line=linenumber; /* location of tag: may have to 
				  change if fn is inline expanded */
		current_token=get_token();
		tmp_ptr=new_entry(tag_value);
		set_offsets(tmp_ptr,starting_line,linenumber);
		put_entry(tmp_ptr);
		/* Could maybe skip_past_label?  Don't really need name. */
    	    	break;
	    case TAG_formal_parameter:
	    case TAG_local_variable:
		target_attribute=AT_location;
		current_token=get_token();
    	    	break;
	    case TAG_label:
		target_attribute=AT_low_pc;
		current_token=get_token();
    	    	break;
	    default:
    	    	skip_past_label(label);
		if(current_token==tok_end) return;
    	    	continue;
	    } /* switch */
	} /* end of while(target_attribute==0) loop, we've found a tag */

	/* Now skip thru attribute list.  The only
	   attributes of interest are AT_location for
	   the variables, AT_name (?) for subroutines. */

	found_attribute=0;
	while(current_token==tok_ps_2byte) {

	    /* Got an attribute: figure out what it is */	

	    attribute=(short)strtol(tok_val,(char **)NULL,0);
	    if(attribute==target_attribute) {
		found_attribute=1;
		switch(attribute) {
		case AT_location:
		    starting_line=linenumber+1; /* current offset */
		    tmp_ptr=new_entry(tag_value);
		    if(parse_location(tmp_ptr)) {
		        set_offsets(tmp_ptr,starting_line,linenumber);
			put_entry(tmp_ptr);
		    }
		    else
			free(tmp_ptr);
		    break;
		case AT_name:
		    current_token=get_token();
		    if (current_token != tok_ps_string)
			fatal("parse_debug(): expected \".string\"\n");
			/* don't care about name, but could save
			   it here to check it jibes with FN_Id for
			   the function we are about to optimize. */
		    current_token=get_token();
		    break;	
		case AT_low_pc:
		    starting_line=linenumber; /* offset of .2byte AT_low_pc */
		    tmp_ptr=new_entry(tag_value);
		    current_token=get_token();
		    if(current_token != tok_ps_4byte)
			fatal("parse_debug(): expected \".4byte\t<label>\"\n");
		    set_offsets(tmp_ptr,starting_line,linenumber+1);
		    put_entry(tmp_ptr);
		    break;
		default: fatal("parse_debug(): unexpected target_attribute\n");
		}
	    }
	    else { /* skip this attribute and keep looking */
		skip_attribute(attribute&FORM_MASK);
		continue;
	    }
	} /* while(current_token ... */
	if( found_attribute || (target_attribute == AT_location) ) {

		/* If we didn't find the target AT_location, then
		   this formal parm or variable has no location
		   so we don't care about it, e.g. in typedefs. */

	        skip_past_label(label);
		if(current_token==tok_end) return;
	        continue;
	}
	else {
	    fatal("parse_debug(): can't find attribute. \
		   current_line: %s\n",current_line);
        }
    }
} /* parse_debug() */

	static void
skip_past_label(label) /* gets the next token after label */
char *label;
{
    char save_char;
    register char *t;
    int found;
    found=0;

    while(!found) {

	if(current_token==tok_end) 
	    fatal("skip_past_label(): premature EOI\n");
	else if(current_token==tok_label) {
		t=current_line;
		FindWhite(t);
		t--; save_char= *t; /* save ':' */
		*t='\0';
		if(strcmp(current_line,label)==0) found=1;
		*t=save_char; /* restore line */
	}
	current_token=get_token();

    } /* while(!found) */

}

	static int
parse_location(ptr)
D_ptr ptr; 		/* put info in here, if you find it */
{
	/* We expect a location of the form
		1) OP_BASEREG OP_CONST OP_ADD ( displ )
		2) OP_REG
		3) OP_ADDR
	   In cases 1 & 2 enter the info and return 1.
	   In case 3 return 0.
	   In all three cases, get the next token after the entry.
	   Anything else is fatal. */

    int entry_length; 	/* Length in bytes of the
				   entry ( after assembly ). */
    short atom; 		/* values defined in dwarf.h */
    current_token=get_token();
    if ((current_token != tok_ps_2byte) || !isdigit(*tok_val))
        fatal("parse_location(): expected .2byte <const>\n");
    entry_length=(int)strtol(tok_val,(char **)NULL,0);

    current_token=get_token(); /* OP_REG, OP_BASEREG or OP_ADDR atom */
    if ((current_token != tok_ps_byte) || !isdigit(*tok_val))
        fatal("parse_location(): expected .byte <atom>\n");
    atom=(short)strtol(tok_val,(char **)NULL,0);
    current_token=get_token();

    switch(atom) {

    case OP_ADDR: /* just make sure it looks right */
	if (current_token != tok_ps_4byte)
	    fatal("parse_location(): OP_ADDR operand\n");
	break;

    case OP_REG:
    case OP_BASEREG:
        if ((current_token != tok_ps_4byte) || !isdigit(*tok_val))
            fatal("parse_location(): expected .4byte <reg#>\n");
    
        /* Convert the number to internal register format and enter it */
    
        ptr->regid=GetRegId( (int)strtol(tok_val,(char **)NULL,0) );
    
        if(atom == OP_REG) {		/* Register parm or auto */
            ptr->mode=CPUReg;
	    if(entry_length != 5)
                fatal("parse_location(): expected 5 byte length\n");
        }
    
        else if (atom == OP_BASEREG) {	/* stack parm or auto */
            ptr->mode=Disp;
	    current_token=get_token();
            atom=(int)strtol(tok_val,(char **)NULL,0);
    	    if ((current_token != tok_ps_byte) || (atom != OP_CONST))
                fatal("parse_location(): expected .byte OP_CONST\n");
	    current_token=get_token();
    	    if ((current_token != tok_ps_4byte) || !isdigit(*tok_val))
                fatal("parse_location(): expected .4byte <displacement>\n");
	    ptr->displacement=(int)strtol(tok_val,(char **)NULL,0);
	    current_token=get_token();
            atom=(int)strtol(tok_val,(char **)NULL,0);
    	    if ((current_token != tok_ps_byte) || (atom != OP_ADD))
                fatal("parse_location(): expected .byte OP_ADD\n");
	    if(entry_length != 11)
                fatal("parse_location(): expected 11 byte length\n");
        }
	break;

    default:
    	fatal("parse_location(): expected OP_REG or OP_BASEREG\n");
    } /* switch */

    current_token=get_token();
    return (atom != OP_ADDR);
}

	static void
skip_attribute(form)
short form; /* 1040A */
			/* Skips an attribute entry, gets
			   the first token after this attribute. */
{
    short bytes_to_skip=0; /* Number of ( object file ) bytes
		    	      we must account for to skip this
		              attribute. */
    short skipped=0; /* counter */

    switch(form) {

    default:
    case FORM_BLOCK4: /* Unused */
    case FORM_NONE: /* error */ 
        fatal("skip_attribute(): bad attribute format code %d\n",form);
	/* FALLTHRU */
    case FORM_DATA2:
	bytes_to_skip=2;
	break;

    case FORM_DATA8:;
	bytes_to_skip=8;
	break;

    case FORM_ADDR:
    case FORM_REF:
    case FORM_DATA4:;
	bytes_to_skip=4;
	break;

    case FORM_BLOCK2: /* Have to look at next two bytes for length */
	current_token=get_token();
	if ((current_token != tok_ps_2byte) || !isdigit(*tok_val))
	    fatal("skip_attribute(): expected .2byte <const>\n");
	bytes_to_skip=(short)strtol(tok_val,(char **)NULL,0);
        break;

    case FORM_STRING:
	current_token=get_token(); 
	if (current_token != tok_ps_string)
	    fatal("skip_attribute(): expected .string\n");
	break;

    } /* switch */

    while(skipped < bytes_to_skip) {
	current_token=get_token();

	switch(current_token) {
	case tok_end:
	    fatal("skip_attribute(): premature end of entry\n");
	    /* FALLTHRU */
	case tok_ps_byte:
	    skipped++;
	    break;
	case tok_ps_2byte:
	    skipped+=2;
	    break;
	case tok_ps_4byte:
	    skipped+=4;
	    break;
	case tok_label:
	    break;
	default:
	    fatal("skip_attribute(): unexpected pseudo_op\n");
	} /* switch */
    } /* while(skipped ... */
    if(skipped != bytes_to_skip)
	fatal("skip_attribute(): weird byte count\n");
    current_token=get_token();
} /* skip_attribute() */

	static token
get_token()

/* Side effects:
	1) Handles update of next_char
	2) Calls getline at end of line and sets current_line
	3) Handles EOF and section change nasties
	4) Calls plookup() to figure out a pseudo-op
	5) Outputs lines to the temporary Dfile
	6) Prints the section change on stdout

	This is the only function in DebugInfo.c which 
		a) does any I/O
		b) knows about assembly directives
		c) knows anything about input characters
		d) assigns to next_char and current_line
		   and tok_val 
*/

{
    char savechar; 
    register char *t;
    unsigned int pop; /* returned by plookup() */
    extern char *getline(); /* defined in m32/local.c */
    extern unsigned plookup(); /* in lookup.c */
    static enum Section tmp_section; 	/* for .previous swap */

    /* find a label or regular instruction */

    t=next_char;
    SkipWhite(t);
    while((*t==EOS) ||(*t==ComChar)) {
	/* No more on this line, save it. */
	put_Dfile(current_line);
	current_line=getline(infile);
	if(current_line==NULL) {
	    return tok_end;
	}
	t=current_line;
	SkipWhite(t);
    } /* while */

    /* t now points at the next non white character
       in the input stream */

    if (t==current_line) { /* It's a label */
	tok_val=current_line;
	FindWhite(t); /* Get next_char ready for next call */
	next_char=t;
	return tok_label;
    }
    next_char=t;
    FindWhite(t);
    savechar= *t;			/* Mark end of pseudo-op */
    *t = EOS;
    pop=plookup(next_char);			/* Identify pseudo-op. */
    *t = savechar;			/* Restore saved character. */
    SkipWhite(t);

    /* t now points at the operand, if there is one */

    switch(pop){	/* dispatch on pseudo-op */
    case PLOWER:	/* pseudo-op not found */
	fatal("get_token(): illegal directive \"%s\".\n",next_char);
	break;
	
/* Next three cases are all section changes.  */

    case PS_DATA:
	prev_section = section;
	section = CSdata;
	printf("%s\n",current_line); /* Don't print section changes to Dfile */
	return tok_end;

    case PS_PREVIOUS:
	tmp_section = section;
	section = prev_section;
	prev_section = tmp_section;
	printf("%s\n",current_line); /* Don't print section changes to Dfile */
	return tok_end;

    case PS_SECTION:
    	next_char=t;	
	while((*t!= EOS) && (*t != COMMA ) && ~isspace(*t))
	    t++;
	savechar = *t;
	*t=EOS;
	prev_section = section;
	if(strcmp(next_char, ".rodata") == 0)
	    section = CSrodata;
	else if(strcmp(next_char, ".data1") == 0)
	    section = CSdata1;
	else if(strcmp(next_char, ".data") == 0)
	     section = CSdata;
	else if(strcmp(next_char, ".text") == 0)
	    section = CStext;
	else if(strcmp(next_char, ".debug") == 0)
	    section=CSdebug; /* does this work ?? */
	else if(strcmp(next_char, ".line") == 0)
	    section=CSline;
	else section = CSother;
	*t = savechar;
	printf("%s\n",current_line); /* Don't print section changes to Dfile */
	return tok_end; /* Don't care about next_char */
    case PS_TEXT:
	prev_section = section;
	section = CStext;
	printf("%s\n",current_line); /* Don't print section changes to Dfile */
	return tok_end;

    /* Data for .debug section */

    default: /* pseudo-op with operand */
	tok_val=t;
	FindWhite(t);
	next_char=t; /* ready for next call */

	switch(pop) {
        case PS_BYTE:
	    return tok_ps_byte;
        case PS_2BYTE:
	    return tok_ps_2byte;
        case PS_4BYTE:
	    return tok_ps_4byte;
	case PS_STRING:	
	    return tok_ps_string;
        default:
	    return tok_dont_care;
	}
    }
/* NOTREACHED */
}

	void
init_debug(inputfile) 
FILE *inputfile;
{ /* Open intermediate file Dfile, set infile */

 char *filename;
 extern int errno; /* UNIX system errno */

 init_debug_flag=1;
 infile=inputfile;
 filename = tempnam("/usr/tmp","optmD");
 if(filename == NULL)
	fatal("do_optim: couldn't make tempname.\n");

 Dfile = fopen(filename,"w+");
 if(Dfile == (FILE *)NULL)
	fatal("do_optim: couldn't open file: %s (%d).\n",filename,errno);

 if(unlink(filename) == -1)		/* Unlink: file will be removed
					   when program ends.	*/
	fatal("do_optim: couldn't unlink file: %s (%d).\n",filename,errno);
 free(filename);
}

	static void
put_Dfile(s)
register char *s;
{
	if (isspace(*s)) {
		SkipWhite(s);
		*--s='\t';
	}
	if(fprintf(Dfile,"%s\n",s)<0)
		fatal("put_Dfile: fprintf failed\n"); /* for now */
	linenumber++;
}



/* queue handling routines */

	static D_ptr
new_entry(tag_val) /* malloc space for a new entry, enter the tag */
short tag_val;
{
    D_ptr tmp_ptr;
    tmp_ptr=(D_ptr)malloc(sizeof(D_entry));
    if(tmp_ptr==NULL)
        fatal("new_queue_entry(): malloc failed\n");
    tmp_ptr->TAG=tag_val;
    return tmp_ptr;
}

	static void
put_entry(ptr)
D_ptr ptr;
{
    ptr->next=NULL;
    if(D_head==NULL) {
        D_head=ptr;
        D_tail=ptr;
    }
    else {
        D_tail->next=ptr;
        D_tail=ptr;
    }
}

	static void
set_offsets(ptr,start,end)
D_ptr ptr;
long start, end;
{
    if(ptr==NULL)
	fatal("set_offsets(): nobody home\n");
    ptr->loc_head=start;
    ptr->loc_tail=end;
}

	void
mark_func(fn_id)	/* Fix up the debug entry for a function
			   to contain the function node info.
			   This gets called from parse_instr()
			   in local.c when a new function is
			   entered, which occurs when a  SAVE or
			   ISAVE instruction is seen. */
FN_Id fn_id;
{
    if (!init_debug_flag) return;
    if (D_tail != NULL) 
	switch(D_tail->TAG) {
	case TAG_global_subroutine:
	case TAG_subroutine:
	case TAG_inline_subroutine:
	    D_tail->fnid=fn_id;
	    break;
	default: fatal("mark_func(): end of queue not function\n");
        }
    else
	fatal("mark_func(): empty queue\n");
}


/* Output routines */
static void print_lines();
static void skip_lines();
static void print_location();

static D_ptr remember_head;

	void
print_debug(ldtab, FuncId)  /* print all debugging entries for a function */
struct ldent ldtab[];   /* live dead table for current function */
FN_Id FuncId;		/* current function */
{
    static long line_offset; /* offset in Dfile for next line */
    static FN_Id lastfn=(FN_Id)NULL; /* last function processed by print_debug(); */
    long first_src_line = 0; /* If there is a start of function source line
			   entry, we remember it here. A value of 0 means
			   no entry was seen. */

    if(!init_debug_flag) return;
    if(lastfn==(FN_Id)NULL) { /* first call to parse_debug */
	remember_head=D_head;	/* handle_function_tags will make
				   a second pass over the queue. */
	rewind(Dfile);
    }

    printf("\t.section\t.debug\n");

    /* Next, we look for header record for current fn */

    while(lastfn != FuncId) { 
        if(D_head == (D_ptr)NULL)
	    fatal("print_debug(): no debug info for function\n");
	else switch(D_head->TAG){
	     case TAG_global_subroutine:
	     case TAG_subroutine:
	     case TAG_inline_subroutine:
	         lastfn=D_head->fnid;
	         break;
	     default:
	         D_head=D_head->next;
	     }
    }


    /* print everything before subroutine tag entry */
    print_lines(D_head->loc_head,&line_offset);

    /* The function may get inline expanded someplace.
       Since we don't know yet, we have to use a pseudo
       variable for the TAG. The .set for this variable
       will be generated AFTER all the text nodes have
       been printed out, by a call to handle_function_tags()
       from pass2() of local2.c */

    printf("\t.2byte\t.TAG_%s\n",GetFnName(FuncId)->key.K.string+1);
    skip_lines(D_head->loc_tail,&line_offset);

    while(lastfn == FuncId) {
	D_head=D_head->next;
        if(D_head == (D_ptr)NULL) break;
	switch(D_head->TAG) {
	case TAG_local_variable:
	case TAG_formal_parameter:
	    print_lines(D_head->loc_head,&line_offset);
	    print_location(ldtab);	/* Go print the ( possibly
					   modified ) location entry. */
	    skip_lines(D_head->loc_tail,&line_offset);
	    break;
	case TAG_label:
	    print_lines(D_head->loc_head,&line_offset);
	    skip_lines(D_head->loc_tail,&line_offset);
	    break;
	case TAG_global_subroutine:
	case TAG_subroutine:
	case TAG_inline_subroutine:
	    lastfn=D_head->fnid;
	    break;
	case TAG_padding:
	    first_src_line = D_head->loc_head;
	    break;
	}
    } /* while */		
    if(D_head==(D_ptr)NULL) { /* won't be back so print everything else */
	print_lines(0,&line_offset);
    }
    printf("\t.previous\n");
    if(first_src_line != 0) {
	print_FS_line_info(first_src_line,GetFnName(FuncId)->key.K.string+1);
    }
}

	void
handle_function_tags()
{
    D_ptr dp=remember_head;
    char *fname;
    if(!init_debug_flag) return;
    printf("\t.section\t.debug\n");
    while(dp != NULL) {
	switch(dp->TAG) {
	case TAG_global_subroutine:
	case TAG_subroutine:
	case TAG_inline_subroutine:
	    fname=GetFnName(dp->fnid)->key.K.string+1;
	    if(GetFnExpansions(dp->fnid) > 0) {
#ifdef DEBUGDEBUG
	        printf("\t.set\t.TAG_%s,%#x #TAG_inline_subroutine\n",fname,TAG_inline_subroutine);
#else
	        printf("\t.set\t.TAG_%s,%#x\n",fname,TAG_inline_subroutine);
#endif
	    }
	    else {
	        printf("\t.set\t.TAG_%s,%#x\n",fname,dp->TAG);
	    }
	    break;
	case TAG_local_variable:
	case TAG_formal_parameter:
	case TAG_label:
	case TAG_padding:
	    break;
	default:
	    fatal("handle_function_tags(): unknown TAG\n");
	    break;
	}
	dp=dp->next;
    }
    printf("\t.previous\n");
}

	static void
print_location(ldtab) 	/* Reconstitute a location entry. 
			   This code mimics the old prdefs.c. */
struct ldent ldtab[];   /* live dead table for current function */
{
    unsigned ld;
    AN_Id ad;	/* address id of candidate location */
    int reg;

    /* Check the live-dead table to see if this location has been
       reassigned during global register allocation.  If it has, update
       the location entry:
		1) set regid to the newly allocated register
		2) set mode to CPUReg */

    for(ld=0; ld < LIVEDEAD; ld++) {
/* Just for the next declaration! */
#include "OperndType.h"
	extern OperandType NAQ_type();
	if( ldtab[ld].passign != NULL )	{/* Reassigned? */
	    ad=ldtab[ld].laddr;

	    /* We have a match iff:
		1) the modes are the same ( Disp or CPUReg )
		2) the registers are the same
		3) the offsets match ( Disp mode only ) */

	    if( (D_head->mode != GetAdMode(ad))) /*  && */
		continue;
	    if (D_head->regid != GetAdRegA(ad)) /* && */
		continue;
	    if ( (D_head->mode != Disp) ||
		  (D_head->displacement == GetAdNumber(NAQ_type(ad),ad))  ) {
			
			D_head->regid=GetAdRegA(ldtab[ld].passign->laddr);
			D_head->mode=CPUReg;
			break;
	    }
	}
    }
    
    /* Now print the entry */
#ifdef DEBUGDEBUG
	printf("# optimized location:\n");
#endif

    reg=GetRegNo(D_head->regid);
    if(D_head->mode == CPUReg) {
#ifdef DEBUGDEBUG
	printf("\t.2byte\t5; .byte %#x; .4byte %d\t# OP_REG\n", OP_REG, reg);
#else
	printf("\t.2byte\t5; .byte %#x; .4byte %d\n", OP_REG, reg);
#endif

    }
    else {
	printf("\t.2byte\t11; .byte %#x; .4byte %d; .byte %#x; .4byte %#x; \
.byte %#x", OP_BASEREG, reg,OP_CONST,D_head->displacement,OP_ADD);
#ifdef DEBUGDEBUG
	printf("\t# OP_BASEREG OP_CONST OP_ADD\n");
#else
	putchar('\n');
#endif
    }
}

	static void
print_lines(limit,line_offset)

/* print all lines of Dfile in the range *line_offset to limit-1,
   reset *line_offset to limit, unless limit is 0, in which case
   just print all the remaining lines in the file;
*/

long limit;
long *line_offset;
{
    char s[132]; /* for now */
    int i;
    if(limit == 0)
	while(fgets(s,132,Dfile)) printf("%s",s);
    else {
        for(i= *line_offset; i<limit; i++)
	    if(fgets(s, 132, Dfile)) {
	        printf("%s",s);
	    }
	    else fatal("print_lines(): fgets() failed\n"); 
        *line_offset=limit;
    }
}

	static void
skip_lines(limit,line_offset)	/* same as print_lines, but no output,
				   except in debugging mode */

/* skip all lines of Dfile in the range *line_offset to limit-1,
   reset *line_offset to limit, unless limit is 0, in which case
   just print all the remaining lines in the file;
*/

long limit;
long *line_offset;
{
    char s[132]; /* for now */
    int i;

#ifdef DEBUGDEBUG
    printf("# old debugging info:\n");
#endif

    if(limit == 0)
	while(fgets(s,132,Dfile)) printf("%s",s);
    else {
        for(i= *line_offset; i<limit; i++)
	    if(fgets(s, 132, Dfile)) {
#ifdef DEBUGDEBUG
    printf("#%s",s);
#else
	/* EMPTY */
    ;
#endif
	    }
	    else fatal("skip_lines(): fgets() failed\n"); 
        *line_offset=limit;
    }
}


/* .line section routines */
static char text_begin[EBUFSIZE];
static init_line_flag=0;

/* Remember ..FS<first_src_line> label */
	void
put_FS_entry(first_src_line)
long first_src_line;
{
	D_ptr ptr;
	ptr = new_entry(TAG_padding);
	ptr->loc_head = first_src_line;
	put_entry(ptr);
}

	void
init_line_section()
/* Called from local2.c when we are about to start printing
   otimized output file.
   Defines the line number table in .section .line.
   Must be called before any code in .text has been output.
*/

{
    extern void newlab();
    if(!init_debug_flag) return;
    init_line_flag=1;
    newlab(text_begin,"..Otext.b.",sizeof(text_begin)); 
    printf("\t.text\n");
    printf("%s:\n",text_begin);
    printf("\t.previous\n");
    printf("\t.section\t.line\n");
    printf("..line.b:\n");
    printf("\t.4byte ..line.e-..line.b; .4byte %s\n",text_begin);
    printf("\t.previous\n");
}

	void
exit_line_section()
{
    if(!init_line_flag) return;
    printf("\t.section\t.line\n");
    printf("\t.4byte  0; .2byte 0xffff; .4byte ..text.e-%s\n",text_begin);
    printf("..line.e:\n");
    printf("\t.previous\n");
}

	void
print_line_info(prefix_str,UniqueId)
char * prefix_str; /* either "#" or "" */
unsigned long UniqueId;
{
    if(!init_line_flag) return;
    /* ouput a ..LN label in .text */
    printf("%s..LN%ld:\n",prefix_str,UniqueId); 

    /* now output the corresponding entry in the .line section */

    printf("%s\t.section\t.line\n",prefix_str); 
    printf("%s\t.4byte %ld;", prefix_str,UniqueId); /* source line */
    printf(" .2byte 0xffff;"); /* position in line */
    printf(" .4byte ..LN%ld-%s\n",UniqueId,text_begin); /* address delta */
    printf("%s\t.previous\n",prefix_str);
}

	void
print_FS_line_info(first_src_line,fname)
long first_src_line;
char *fname;
{
    printf("\t.section\t.line\n"); 
    printf("\t.4byte %ld; .2byte 0xffff; .4byte %s-%s\n",
		first_src_line,fname,text_begin);
    printf("\t.previous\n");

/* Typical entry:
	.section	.line
	.4byte 5; .2byte 0xffff; .4byte foo-..Otext.b.0
	.previous
*/

}

#ifdef DEBUGDEBUG
/* Debugging routines -- call from sdb */
	void
print_queue()
{
    D_ptr dp=D_head;
    char *fname;
    while(dp != NULL) {
	dp=dp->next;
    }
}
#endif
