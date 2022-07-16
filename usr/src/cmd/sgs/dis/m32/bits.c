/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dis:m32/bits.c	1.4"

#include	"libelf.h"
#include	<stdio.h>
#include	"dis.h"
#include	"sgs.h"
#include	<string.h>

#define		FAILURE 0
#define		MAXERRS	1	/* maximum # of errors allowed before	*/
				/* abandoning this disassembly as a	*/
				/* hopeless case			*/

static short errlev = 0;	/* to keep track of errors encountered 	*/
				/* during the disassembly, probably due	*/ 
				/* to being out of sync.		*/

#define		OPLEN	35	/* maximum length of a single operand	*/
				/* (will be used for printing)		*/

static	char	operand[4][OPLEN];	/* to store operands as they	*/
					/* are encountered		*/
static	char	symarr[4][OPLEN];

static	long	start;			/* start of each instruction	*/
					/* used with jumps		*/

static	int	fpflag;		/* will indicate floating point instruction	*/
				/* (0=NOT FLOATING POINT, 1=SINGLE, 2=DOUBLE) 	*/
				/* so that immediates will be printed in	*/
				/* decimal floating point.			*/
static int	bytesleft = 0;	/* will indicate how many unread bytes are in buffer */

#define	TWO_8	256
#define	TWO_16	65536
#define	MIN(a,b)	((a) < (b) ? (a) : (b))

/*
 *	dis_text ()
 *
 *	disassemble a text section
 */

void
dis_text(shdr)
Elf32_Shdr	*shdr;
{
	extern void	exit();
	/* the following arrays are contained in tables.c	*/
	extern	struct	instable	opcodetbl[256];

	/* the following entries are from _extn.c	*/
	extern	int	resync();
	extern	char	*sname;
	extern	char	*fname;
	extern	int	Sflag;
	extern	int	Lflag;
	extern	int	sflag;
	extern	int	trace;
	extern	long	 loc;
	extern	char	mneu[];
	extern	char	object[];
	extern	char	symrep[];
	extern	unsigned short	cur1byte;
	extern	int	debug;

	extern	void	printline();
	extern	void	looklabel();
	extern	void	line_nums();
	extern	void	prt_offset();
	extern	int	convert();

	/* the following routines are in this file	*/
	static long		getword();
	static unsigned short	gethalfword();
	static void	get_operand();
	static void	mau_dis();
	static void	get_bjmp_oprnd(),
			get_macro(),
			get_hjmp_oprnd(),
			longprint();
	void		get1byte();

/*	*sech; */

	struct instable	*cp;
	unsigned	key;
	long 		lngtmp;
	char		ctemp[OPLEN];	/* to store EXTOP operand   */

	/* initialization for each beginning of text disassembly	*/

	bytesleft = 0;

	/*
	 * An instruction is disassembled with each iteration of the
	 * following loop.  The loop is terminated upon completion of the
	 * section (loc minus the section's physical address becomes equal
	 * to the section size) or if the number of bad op codes encountered
	 * would indicate this disassembly is hopeless.
	 */

	for (loc = shdr->sh_addr ; ((loc-shdr->sh_addr) < shdr->sh_size) && (errlev < MAXERRS); printline()) {
		start = loc;
		(void) sprintf(operand[0],"");
		(void) sprintf(operand[1],"");
		(void) sprintf(operand[2],"");
		(void) sprintf(operand[3],"");
		(void) sprintf(symarr[0],"");
		(void) sprintf(symarr[1],"");
		(void) sprintf(symarr[2],"");
		(void) sprintf(symarr[3],"");
		fpflag = NOTFLOAT;	/* assume immediates are not floating
					 * point , unless floating point 
					 * operand is disassembled.
					 */

		/* look for C source labels */
		if ( Lflag && debug )
			looklabel(loc);

		line_nums();

		prt_offset();			/* print offset		   */

		/* key is the one byte op code		*/
		/* cp is the op code Class Pointer	*/

		get1byte();
		key = cur1byte;
		cp = &opcodetbl[key];
		(void) sprintf(mneu, "%-8s","	");


			/* print the mnemonic */
			(void) sprintf(mneu,"%-8s",cp -> name);
			(void) sprintf(symrep,"");
			if (trace > 2) (void) printf("\ninst class = %u  ",cp->class);
			/*
			 * Each instruction has been assigned a classby the author.
			 * Individual classes are explained as they are encountered in 
			 * the following switch construct.
			 */
	
			switch(cp -> class){
	
			/* This class handles instructions that wish to ignore	*/
			/* 8 bits of insignificant data				*/
			case	NOOP8:
				get1byte();
				continue;
	
			/* This class handles instructions that wish to ignore	*/
			/* 16 bits of insignificant data			*/
			case	NOOP16:
				(void)gethalfword();
				continue;
	
			/* This case handles the EXTOP instruction for the	*/
			/* m32a simulator i/o routines.				*/
			case	EXT:
				get1byte();
				convert(cur1byte,ctemp,LEAD);
				(void) sprintf(mneu,"%s%s",mneu,ctemp);
				if (sflag)
					(void) sprintf(symrep,"%s%s",symrep,ctemp);
				continue;
	
			/* This class handles instructions with no operands	*/
			case	OPRNDS0:
				continue;
	
			/* This class handles instructions with 1 operand	*/
			/* that is described by a preceeding discriptor		*/
			case	OPRNDS1:
			case	JUMP:
				get_operand(operand[0],symarr[0]);
				(void) sprintf(mneu,"%s%s",mneu,operand[0]);
				if (sflag)
					(void) sprintf(symrep,"%s%s",symrep,symarr[0]);
				continue;
	
			/* This class handles instructions with 2 operands	*/
			/* that are each described by a preceeding discriptor	*/
			case	OPRNDS2:
				get_operand(operand[0],symarr[0]);
				get_operand(operand[1],symarr[1]);
				(void) sprintf(mneu,"%s%s,%s",mneu,operand[0],operand[1]);
				if (sflag)
					(void) sprintf(symrep,"%s%s,%s",symrep,symarr[0],symarr[1]);
				continue;
	
			/* This class handles instructions with 3 operands	*/
			/* that are each described by a preceeding discriptor	*/
			case	OPRNDS3:
				get_operand(operand[0],symarr[0]);
				get_operand(operand[1],symarr[1]);
				get_operand(operand[2],symarr[2]);
				(void) sprintf(mneu,"%s%s,%s,%s",mneu,operand[0],
					operand[1],operand[2]);
				if (sflag)
					(void) sprintf(symrep,"%s%s,%s,%s",symrep,symarr[0],
								symarr[1],symarr[2]);
				continue;
	
			/* This class handles instructions with 4 operands	*/
			/* that are each described by a preceeding discriptor	*/
			case	OPRNDS4:
				get_operand(operand[0],symarr[0]);
				get_operand(operand[1],symarr[1]);
				get_operand(operand[2],symarr[2]);
				get_operand(operand[3],symarr[3]);
				(void) sprintf(mneu,"%s%s,%s,%s,%s",mneu,operand[0],
					operand[1],operand[2],operand[3]);
				if (sflag)
					(void) sprintf(symrep,"%s%s,%s,%s,%s",symrep,symarr[0],
							symarr[1],symarr[2],symarr[3]);
				continue;
	 
			/* This class handles jump instructions that have a one	*/
			/* byte offset (not an ordinary operand)		*/
			case	JUMP1:
				get_bjmp_oprnd(operand[0],symarr[0]);
				(void) sprintf(mneu,"%s%s",mneu,operand[0]);
				if (sflag)
					(void) sprintf(symrep,"%s%s",symrep,symarr[0]);
				continue;
	
			/* This class handles jump instructions that have a two	*/
			/* byte offset (not an ordinary operand)		*/
			case	JUMP2:
				get_hjmp_oprnd(operand[0],symarr[0]);
				(void) sprintf(mneu,"%s%s",mneu,operand[0]);
				if (sflag)
					(void) sprintf(symrep,"%s%s",symrep,symarr[0]);
				continue;
	
			/* This class handles single precision floating point 	*/
			/* instructions with two operands.			*/
			case	SFPOPS2:
				fpflag = FPSINGLE;
				get_operand(operand[0],symarr[0]);
				get_operand(operand[1],symarr[1]);
				(void) sprintf(mneu,"%s%s,%s",mneu,operand[0],operand[1]);
				if (sflag)
					(void) sprintf(symrep,"%s%s,%s",symrep,symarr[0],symarr[1]);
				continue;
	
			/* This class handles single precision floating point	*/
			/* instructions with three operands.			*/
			case	SFPOPS3:
				fpflag = FPSINGLE;
				get_operand(operand[0],symarr[0]);
				get_operand(operand[1],symarr[1]);
				get_operand(operand[2],symarr[2]);
				(void) sprintf(mneu,"%s%s,%s,%s",mneu,operand[0],
					operand[1],operand[2]);
				if (sflag)
					(void) sprintf(symrep,"%s%s,%s,%s",symrep,symarr[0], symarr[1],symarr[2]);
				continue;
	
			/* This class handles double precision floating point 	*/
			/* instructions with two operands.			*/
			case	DFPOPS2:
				fpflag = FPDOUBLE;
				get_operand(operand[0],symarr[0]);
				get_operand(operand[1],symarr[1]);
				(void) sprintf(mneu,"%s%s,%s",mneu,operand[0],operand[1]);
				if (sflag)
					(void) sprintf(symrep,"%s%s,%s",symrep,symarr[0],symarr[1]);
				continue;
			/* This class handles double precision floating point	*/
			/* instructions with three operands.			*/
			case	DFPOPS3:
				fpflag = FPDOUBLE;
				get_operand(operand[0],symarr[0]);
				get_operand(operand[1],symarr[1]);
				get_operand(operand[2],symarr[2]);
				(void) sprintf(mneu,"%s%s,%s,%s",mneu,operand[0],
					operand[1],operand[2]);
				if (sflag)
					(void) sprintf(symrep,"%s%s,%s,%s",symrep,symarr[0], symarr[1],symarr[2]);
				continue;
			/* Support Processor Instruction  "SPOP";  Opcode is 	*/
			/* followed by 32 bit "id" similar to an immediate word */
			/* but has no byte descriptor.				*/
			case SPRTOP0:
				lngtmp = getword();
				if (!Sflag) {
						/* support processor id */
					switch ((lngtmp >> 24) & 0xff) {
					case MAU_ID:
						mau_dis(lngtmp);
						continue;
					default:
						break;
					}
				}
				(void) sprintf(mneu,"%s&",mneu);
				if (sflag)  {
					(void) sprintf(symrep,"%s&",symrep);
					longprint(symrep,symrep,lngtmp);
				}
				longprint(mneu,mneu,lngtmp);
				continue;
			/* Support Processor Instructions "SPOPRS", "SPOPRD",  	*/
			/* "SPOPRT", "SPOPWS", "SPOPWD", and "SPOPWT".  The	*/
			/* opcode is followed by a 32 bit support processor "id"*/
			/* similar to an immediate word by it has no byte 	*/
			/* descriptor. Following the "id" is one normal operand	*/
			/* which has a byte descriptor.				*/
			case SPRTOP1:
				lngtmp = getword();
				if (!Sflag) {
						/* support processor id */
					switch ((lngtmp >> 24) & 0xff) {
					case MAU_ID:
						mau_dis(lngtmp);
						continue;
					default:
						break;
					}
				}
				(void) sprintf(mneu,"%s&",mneu);
				longprint(mneu,mneu,lngtmp);
				get_operand(operand[0],symarr[0]);
				(void) sprintf(mneu,"%s, %s",mneu,operand[0]);
				if (sflag) {
					(void) sprintf(symrep,"%s&",symrep);
					longprint(symrep,symrep,lngtmp);
					(void) sprintf(symrep,"%s, %s",symrep,symarr[0]);
				}
				continue;
			/* Support Processor Instructions "SPOPS2", "SPOPD2",  	*/
			/* "SPOPT2".  These are two operand instructions.  The	*/
			/* opcode is followed by a 32 bit support processor "id"*/
			/* similar to an immediate word by it has no byte 	*/
			/* descriptor. Following the "id" is one normal operand	*/
			/* which has a byte descriptor.				*/
			case SPRTOP2:
				lngtmp = getword();
				if (!Sflag) {
						/* support processor id */
					switch ((lngtmp >> 24) & 0xff) {
					case MAU_ID:
						mau_dis(lngtmp);
						continue;
					default:
						break;
					}
				}
				(void) sprintf(mneu,"%s &",mneu);
				longprint(mneu,mneu,lngtmp);
				get_operand(operand[0],symarr[0]);
				get_operand(operand[1],symarr[1]);
				(void) sprintf(mneu,"%s, %s, %s",mneu,operand[0],operand[1]);
				if (sflag) {
					(void) sprintf(symrep,"%s &",symrep);
					longprint(symrep,symrep,lngtmp);
					(void) sprintf(symrep,"%s, %s, %s",symrep,symarr[0],symarr[1]);
				}
				continue;
			/* an invalid op code (there aren't too many)	*/
			case UNKNOWN:
				(void) sprintf(mneu,"***ERROR--unknown op code***");
				printline();	/* to print the error message	*/
				/* attempt to resynchronize */
				if (resync() == FAILURE)	/* if cannot recover */
					errlev++;		/* stop eventually.  */
				(void) sprintf(object,""); /* to prevent extraneous 	*/
				(void) sprintf(mneu,"");   /* printing when continuing	*/
				if (sflag)
				      (void) sprintf(symrep,""); /* to the for loop iteration*/
				continue;
	
			/* This special case handles the macro rom instructions */
			case MACRO:
				get_macro(operand[0]);
				(void) sprintf(mneu,"%s",operand[0]);
				continue;
	
			default:
				(void) printf("\n%sdis bug: notify implementor:",SGS);
				(void) printf(" case from instruction table not found\n");
				exit(4);
				break;
			} /* end switch */
		}  /* end of for */

	if (errlev >= MAXERRS) {
		(void) printf("%sdis: %s: %s: section probably not text section\n",
			SGS,fname, sname);
		(void) printf("\tdisassembly terminated\n");
		errlev = 0;
		return;
	}
	return;
}

/*
 *	get_operand ()
 *
 *	Determine the type of address mode for this operand and
 *	place the resultant operand text to be printed in the array
 *	whose address was passed to this function (result). If sflag is ON then
 *	put symbolic disassembly information in the second array whose address
 *	was passed to this function (symresult).
 */

static void
get_operand(result,symresult)
char	*result;
char	*symresult;
{
	extern struct formtable adrmodtbl[256];	/* from tables.c	*/
	extern	long	loc;
	extern	int	Rel_data;
	extern	int	sfpconv();		/* in _utls.c	*/
	extern	int	dfpconv();		/* in _utls.c	*/
	extern	int	sflag;			/* in _extn.c	*/
	extern	int	oflag;
	extern	int	trace;			/* in _extn.c	*/
	extern  int	symtab;			/* in extn.c 	*/
	extern	unsigned short	cur1byte;	/* in _extn.c	*/
	extern  long	bext(), hext();
	extern  void	search_rel_data(),
			locsympr(),
			extsympr();

	/* the following routines are in this file	*/
	static long	getword();
	static unsigned short	gethalfword();
	void	get1byte();
	static void	longprint();

	struct	formtable  *tp;
	char	temp[OPLEN], stemp[OPLEN];
	char	*temp2;
	char	*tstr;
	int	regno;			/* used to hold register number */
	long	addr;			/* used to hold operand address or offset */
	long	lngtmp, ltemp;		/* temporary variables */
	unsigned short 	wdtmp;		/* a temporary variable */
	double		fpnum;		/* used to hold decimal representation	*/
					/* of floating point immediate operand.	*/
	int		fptype;
	short		fpbigex;	/* used to hold exponent of floating	*/
					/* point number that is too large for	*/
					/* pdp11 or vax.			*/

	unsigned	long		rel_address;

	/* "tp" is the Type Pointer, pointing to the entry	*/
	/* in the address modes table.				*/
	/* "temp2" holds the pointer to the string from the	*/
	/* address modes table that will be printed		*/
	/* as part or all of the current operand.		*/

	get1byte();
	tp = &adrmodtbl[cur1byte];
	temp2 = tp -> name;

	if (trace > 0)
		(void) printf("\nadr mod typ = %x\n",tp->typ);

	tstr= symresult;
	switch (tp -> typ){

	/* type = immediate numbers 0 to 63 (IM6)		*/
	/* type = immediate numbers -16 to -1 (IM4)		*/
	/* For this type the address modes table holds		*/
	/* the entire text for this operand.			*/
	case	IM4:
	case	IM6:
		(void) sprintf(result,"%s",temp2);
		if (sflag)
			(void) sprintf(symresult,"%s",temp2);
		return;

	/* type = register (R) 					*/
	/* For this type the address modes table holds		*/
	/* the entire text for this operand.			*/
	case	R:
		(void) sprintf(result,"%s",temp2);
		if (sflag) {
			regno = cur1byte & 0xf;
			if (cur1byte >> 4 == 5) {
				addr= 0;
				locsympr(addr,regno,&tstr);
			} else {
				locsympr((long)regno,-1,&tstr);

			}
		}
		return;

	/* type = register and displacement of 0 to 14 (R4)	*/
	/* For this type the address modes table holds		*/
	/* the entire text for this operand.			*/
	case	R4:
		(void) sprintf(result,"%s",temp2);
		if (cur1byte >> 4 == 6)
			regno= 9;
		else
			regno= 10;
		addr= cur1byte & 0xf;
		if (sflag){
			locsympr(bext(addr),regno,&tstr);
		}
		return;

	/* type = immediate operand in following byte (IMB)	*/
	/* For this type the next byte is read, converted for	*/
	/* printing and preceded by an "&".			*/
	case	IMB:
		get1byte();
		convert(cur1byte,temp,LEAD);
		(void) sprintf(result,"&%s",temp);
		if (sflag)
			(void) sprintf(symresult,"&%s",temp);
		return;

	/* type = register + byte displacement (BDB)		*/
	/* For this type the next byte is read and converted	*/
	/* and prepended to the register name taken from the	*/
	/* address modes teable.				*/
	case	BDB:
		regno= cur1byte & 0xf;
		get1byte();
		convert(cur1byte,temp,LEAD);
		(void) sprintf(result,"%s%s",temp,temp2);
		if (sflag) {
			if (regno == PCNO) {
				ltemp= start + bext(cur1byte);
				extsympr(ltemp,&tstr);
			} else{
				locsympr(bext(cur1byte),regno,&tstr);
			}
		}
		return;

	/* type = register + byte displacement deferred (BDBDF)	*/
	/* Same as BDB except an additional "*" is prepended.	*/
	case	BDBDF:
		regno= cur1byte & 0xf;
		get1byte();
		convert(cur1byte,temp,LEAD);
		(void) sprintf(result,"*%s%s",temp,temp2);
		if (sflag) {
			(void) sprintf(tstr++,"*");
			if (regno == PCNO) {
				ltemp= start + bext(cur1byte);
				extsympr(ltemp,&tstr);
			} else {
				locsympr(bext(cur1byte),regno,&tstr);
			}
		}
		return;

	/* type = immediate operand in following half word (IMH)	*/
	/* For this type the next half word is read, converted for	*/
	/* printing and preceded by an "&".				*/
	case	IMH:
		wdtmp = gethalfword();
		convert(wdtmp,temp,LEAD);
		(void) sprintf(result,"&%s",temp);
		if (sflag)
			(void) sprintf(symresult,"&%s",temp);
		return;

	/* type = register + half word displacement (BDH)		*/
	/* For this type the next half word is read and converted	*/
	/* and prepended to the register name taken from the		*/
	/* address modes table.						*/
	case	BDH:
		regno= cur1byte & 0xf;
		wdtmp = gethalfword();
		convert(wdtmp,temp,LEAD);
		(void) sprintf(result,"%s%s",temp,temp2);
		if (sflag) {
			if (regno == PCNO) {
				ltemp= start + hext(wdtmp);
				extsympr(ltemp,&tstr);
			} else {
				locsympr(hext(wdtmp),regno,&tstr);					}
		}
		return;

	/* type = register + half word displacement deferred (BDHDF)	*/
	/* Same as BDH except an additional "*" is prepended.		*/
	case	BDHDF:
		regno= cur1byte & 0xf;
		wdtmp = gethalfword();
		convert(wdtmp,temp,LEAD);
		(void) sprintf(result,"*%s%s",temp,temp2);
		if (sflag) {
			(void) sprintf(tstr++,"*");
			if (regno == PCNO) {
				ltemp= start + hext(wdtmp);
				extsympr(ltemp,&tstr);
			} else {
				locsympr(hext(wdtmp),regno,&tstr);
			}
		}
		return;

	/* type = immediate operand in following word (IMW)		*/
	/* NOTE: immediate words may be floating point or nonfloating	*/
	/*       point. Doubleword immediates are floating point.	*/
	/* For this type the next word is read, converted for		*/
	/* printing and preceded by an "&".				*/
	case	IMW:
		rel_address = (unsigned)loc;
		lngtmp = getword();
		switch (fpflag) {

		case	NOTFLOAT:
			longprint(result,"&",lngtmp);
			if (sflag && symtab) 
			{
				(void) sprintf(tstr++,"&");
				if (Rel_data && symtab) 
                                	search_rel_data(rel_address,lngtmp,&tstr);
				else
					tstr+=sprintf(tstr,oflag?"0%o":"0x%x",lngtmp);
			}
			return;

		case	FPSINGLE:
			fptype = sfpconv(lngtmp,  &fpnum);
			break;
		case	FPDOUBLE:
			ltemp = getword();
			fptype = dfpconv(lngtmp, ltemp, &fpnum, &fpbigex);
			break;

		} /* switch (fpflag)	*/
		/* fptype is the return value of the floating point 
		 * conversion routines: sfpconv() and dfpconv().
		 */
		switch (fptype) {

		case	NOTANUM:
			(void) sprintf (result, "INVALID FP NUMBER");
			if (sflag)
				(void) sprintf(symresult,"-");
			break;
		case	NEGINF:
			(void) sprintf (result, "NEGATIVE INFINITY");
			if (sflag)
				(void) sprintf(symresult,"-");
			break;
		case	INFINITY:
			(void) sprintf (result, "INFINITY");
			if (sflag)
				(void) sprintf(symresult,"-");
			break;
		case	ZERO:
			(void) sprintf (result, "&0.0");
			if (sflag)
				(void) sprintf(symresult,"&0.0");
			break;
		case	NEGZERO:
			(void) sprintf (result, "&-0.0");
			if (sflag)
				(void) sprintf(symresult,"&-0.0");
			break;
		case	FPNUM:
			(void) sprintf(result,"&%.15e", fpnum);
			if (sflag)
				(void) sprintf(symresult,"&%.15e", fpnum);
			break;
		case	FPBIGNUM:
			(void) sprintf(result, "&%.13fE%d{RADIX 2}", 
				fpnum, fpbigex);
			if (sflag)
				(void) sprintf(symresult, "&%.13fE%d{RADIX 2}", 
							fpnum, fpbigex);
			break;

		} /* switch (fptype)	*/
		return;

	/* type = register + word displacement (BDW)		*/
	/* For this type the next word is read and converted	*/
	/* and prepended to the register name (for BDW) or	*/
	/* the null string (for ABAD) from the address modes	*/
	/* table.						*/
	case	BDW:
		regno= cur1byte & 0xf;
		lngtmp = getword();
		longprint(result,"",lngtmp);
		(void) sprintf(result,"%s%s",result,temp2);
		if (sflag) {
			if (regno == PCNO) {
				ltemp= start + lngtmp;
				extsympr(ltemp,&tstr);
			} else {
				locsympr(lngtmp,regno,&tstr);
			}
		}
		return;

	/* type = absolute address (ABAD)			*/
	case	ABAD:
		rel_address = (unsigned)loc;
		lngtmp = getword();
		longprint(result,"$",lngtmp);
		(void) sprintf(result,"%s%s",result,temp2);
		if (sflag)
		{
			if (Rel_data && symtab)
				search_rel_data(rel_address,lngtmp,&tstr);
			else if (symtab)
				extsympr(lngtmp, &tstr);
		}
		return;

	/* type = register + word displacement deferred (BDWDF)	*/
	/* Same as BDW and ABAD except an additional "*" is 	*/
	/* prepended.						*/
	case	BDWDF:
		regno= cur1byte & 0xf;
		lngtmp = getword();
		longprint(result,"*",lngtmp);
		(void) sprintf(result,"%s%s",result,temp2);
		if (sflag) {
			(void) sprintf(tstr++,"*");
			if (regno == PCNO) {
				ltemp= start + lngtmp;
				extsympr(ltemp,&tstr);
			} else {
				locsympr(lngtmp,regno,&tstr);
			}
		}
		return;

	/* type = absolute address deferred (ABADDF)		*/
	case	ABADDF:
		lngtmp = getword();
		longprint(result,"*$",lngtmp);
		(void) sprintf(result,"%s%s",result,temp2);
		if (sflag) {
			tstr+=sprintf(tstr,"*");
			extsympr(lngtmp, &tstr);
		}
		return;

	/* type = expanded operand			*/
	/* Data from the address modes table indicates	*/
	/* {Signed/Unsigned BYTE/HALFword/WORD}		*/
	/* followed by the operand.			*/
	case	EXPSB:
	case	EXPUB:
	case	EXPSH:
	case	EXPUH:
	case	EXPSW:
	case	EXPUW:
		get_operand(temp,stemp);
		(void) sprintf(result,"%s%s",temp2,temp);
		if (sflag)
			(void) sprintf(symresult,"%s%s",temp2,stemp);
		return;

	default:
		(void) printf("\n%sdis: invalid general addressing mode (%x)\n",
			SGS, cur1byte);
		(void) printf("\tnotify disassembler implementor\n");
	}	/* end switch */
	return;
}

/*
 *	mau_dis(cmd)
 *
 *	Disassemble mau instruction symbolically
 */

static void
mau_dis(cmd)
long cmd;
{
	extern void	printline();
	/* the following entries are from _extn.c */

	extern char mneu[];
	extern int sflag;
	extern char symrep[];
	extern char object[];
	/*
	 * This string array contains "formats" (like printf) for each of the
	 * mau opcodes.  The meaning of each of the format specifications is:
	 *	-	illegal opcode
	 *	S	source operand size: s, d, or x
	 *	D	destination operand size: s, d, or x
	 *	O	operand count (after optimization): 1, 2 or 3
	 *	R	rounding mode (WE 32206 only)
	 *	I	instruction size (for operations)
	 */
	static char *mau_mnem[] = {
		/* 0x00 */ "-",        "-",        "mfaddIOR", "mfsubIOR",
		/* 0x04 */ "mfdivIOR", "mfremIOR", "mfmulIOR", "mmovSDR", 
		/* 0x08 */ "mmovfa",   "mmovta",   "mfcmpS",   "mfcmptS", 
		/* 0x0c */ "mfabsIOR", "mfsqrIOR", "mfrndIOR", "mmovSwR", 
		/* 0x10 */ "mmovwDR",  "mmov10DR", "mmovS10R", "mnop", 
		/* 0x14 */ "mmovfd",   "-",        "-",        "mfnegIOR", 
		/* 0x18 */ "mmovtd",   "-",        "mfcmpsS",  "mfcmpstS", 
		/* 0x1c */ "mfsinIOR",  "mfcosIOR",  "mfatnIOR",  "mfpiIR" 
	};
	static char szarr[] = "sdx";
	static char *mau_round[] = { "", "", "", "", ".rn", ".rp", ".rm", ".rz" };
	static char *mau_regs[][8] = {
		{ "%s0","%s1","%s2","%s3","%s4","%s5","%s6","%s7" },
		{ "%d0","%d1","%d2","%d3","%d4","%d5","%d6","%d7" },
		{ "%x0","%x1","%x2","%x3","%x4","%x5","%x6","%x7" }
	};

	/*
	 * if the operation is	then register source "size" is
	 * -------------------	------------------------------
	 *   comparison		  if both ops in regs, size == "x"
	 *   conversion		  size == "x"
	 *   other ops.		  size == destination size
	 */
	int opcode	= ((cmd >> 10) & 0x1f);
	int numop	= 0;
	int op1		= ((cmd >> 7) & 0x7);
	int op2		= ((cmd >> 4) & 0x7);
	int op3		= cmd & 0xf;
	int op1_size;
	int op2_size;
	int op3_size	= (op3 < 0xc) ? (op3>>2) : (op3 & 0x3);
	int op1_reg	= (op1 < 0x4) ? (op1 + ((cmd >> 18) & 0x4))         : -1;
	int op2_reg	= (op2 < 0x4) ? (op2 + ((cmd >> 17) & 0x4))         : -1;
	int op3_reg	= (op3 < 0xc) ? ((op3 & 0x3) + ((cmd >> 16) & 0x4)) : -1;
	int i;
	char *mnem	= mneu;
	char *p;
	char *rnd;

	if (op1 < 0x4) {		/* register operand */
		switch (opcode) {
		case 0x0a: case 0x0b: case 0x1a: case 0x1b:	/* comparisons */
		case 0x07: case 0x0f: case 0x12:		/* conversions */
			op1_size = 2;	/* "x" */
			break;
		default:					/* other ops */
			op1_size = op3_size;
			break;
		}
		(void)strcpy(operand[numop++],mau_regs[op1_size][op1_reg]);
	}
	else if (op1 < 0x7) {		/* memory operand */
		op1_size = op1 & 0x3;
		get_operand(operand[numop],symarr[numop]);
		++numop;
	}
	else
		op1_size = 2;
	
	if (op2 < 0x4) {		/* register operand */
		switch (opcode) {
		case 0x0a: case 0x0b: case 0x1a: case 0x1b:	/* comparisons */
		case 0x07: case 0x0f: case 0x12:		/* conversions */
			op2_size = 2;	/* "x" */
			break;
		default:					/* other ops */
			op2_size = op3_size;
			break;
		}
		(void)strcpy(operand[numop++],mau_regs[op2_size][op2_reg]);
	}
	else if (op2 < 0x7) {		/* memory operand */
		op2_size = op2 & 0x3;
		get_operand(operand[numop],symarr[numop]);
		++numop;
	}
	else
		op2_size = 2;
	
	if (op3 < 0xc) {		/* register operand */
		(void)strcpy(operand[numop++],mau_regs[op3_size][op3_reg]);
	}
	else if (op3 < 0xf) {		/* memory operand */
		get_operand(operand[numop],symarr[numop]);
		++numop;
	}
	
	/*
	 * Operand optimizations:
	 * Try to identify when operands are identical, to allow
	 * for example, mfdivs3 %s0,%s1,%s1 to be written as mfdivs2 %s0,%s1
	 */
	switch (opcode) {
	case 0x02:	/* mfadd[sdx][23] */	/* binary functions */
	case 0x03:	/* mfsub[sdx][23] */
	case 0x04:	/* mfdiv[sdx][23] */
	case 0x05:	/* mfrem[sdx][23] */
	case 0x06:	/* mfmul[sdx][23] */
		if (strcmp(operand[1],operand[2]) == 0)
			--numop;
		else if (op3_reg != -1 && op2_reg == op3_reg) {
			--numop;
			(void)strcpy(operand[1], operand[2]);
		}
		break;
	case 0x0c:	/* mfabs[sdx][12] */	/* unary functions */
	case 0x0d:	/* mfsqr[sdx][12] */
	case 0x0e:	/* mfrnd[sdx][12] */
	case 0x17:	/* mfneg[sdx][12] */
	case 0x1c:	/* mfsin[sdx][12] */
	case 0x1d:	/* mfcos[sdx][12] */
	case 0x1e:	/* mfatn[sdx][12] */
		if (strcmp(operand[0],operand[1]) == 0)
			--numop;
		else if (op3_reg != -1 && op1_reg == op3_reg) {
			--numop;
			(void)strcpy(operand[0], operand[1]);
		}
		break;
	}

	/* Machine which processes opcode formats */
	for (p = mau_mnem[opcode]; *p != '\0'; p++)
		switch (*p) {
		case '-':	/* it's an illegal opcode */
			goto bad_spop;
		case 'S':	/* source data size: s,d,x */
			*mnem++ = szarr[MIN(op1_size, op2_size)];
			break;
		case 'D':	/* dest data size: s,d,x */
		case 'I':	/* instruction size: s,d,x */
			*mnem++ = szarr[op3_size];
			break;
		case 'O':	/* operand count */
			(void) sprintf(mnem++, "%d", numop); /* always 1 digit */
			break;
		case 'R':	/* rounding mode: "", .rn, .rp, .rm, .rz */
			rnd = mau_round[(cmd >> 15) & 0x7];
			(void)strcpy(mnem, rnd);
			mnem += strlen(rnd);
			break;
		default:
			*mnem++ = *p;	/* append char to output stream */
			break;
		}
	*mnem = '\0';			/* terminate opcode string */

	for (i = 0; i<numop; i++)
		if (symarr[i][0] == '\0')
			(void) strcpy(symarr[i],operand[i]);

	switch (numop) {
	case 1:
		(void) sprintf(mneu,"%-7s %s",mneu,operand[0]);
		if (sflag)
			(void) sprintf(symrep,"%s",symarr[0]);
		break;
	case 2:
		(void) sprintf(mneu,"%-7s %s,%s",mneu,operand[0],operand[1]);
		if (sflag)
			(void) sprintf(symrep,"%s,%s",symarr[0],symarr[1]);
		break;
	case 3:
		(void) sprintf(mneu,"%-7s %s,%s,%s",mneu,operand[0],operand[1],operand[2]);
		if (sflag)
			(void) sprintf(symrep,"%s,%s,%s",symarr[0],symarr[1],symarr[2]);
		break;
	}
	return;

bad_spop:
	(void) sprintf(mneu,"***ERROR--illegal mau operation***");
	printline();
	if (resync() == FAILURE)
		++errlev;
	(void) sprintf(object,"");
	(void) sprintf(mneu,"");
	if (sflag)
		(void) sprintf(symrep,"");
	return;
}

/*
 *	get_bjmp_oprnd (result, symresult)
 *
 *	Get the next byte as a jump displacement operand.
 *	Call pr_bjmp_oprnd() to convert the characters, calculate the 
 *	jump address and prepare for printing.
 *
 */

static void
get_bjmp_oprnd(result, symresult)
char	*result;
char	*symresult;
{
	extern	unsigned  short  cur1byte;
	long	tlng;
	static void pr_bjmp_oprnd();
	void get1byte();

	get1byte();
	tlng = ((long)cur1byte) & 0x000000ffL;
	pr_bjmp_oprnd(result, symresult, tlng);
	return;
}

/*
 *	get_macro (result)
 *
 *	Get the next byte and use that as the index of the
 *	macro rom instruction.
 *
 */

static void
get_macro(result)
char	*result;
{
	extern	unsigned  short  cur1byte;
	extern	int	oflag;
	static struct romlist {
		short	romcode;
		char	*romstring;
	} romlist[] = {
		0x9,"MVERNO",
		0xd,"ENBVJMP",
		0x13,"DISVJMP",
		0x19,"MOVBLW",
		0x1f,"STREND",
		0x2f,"INTACK",
		0x35,"STRCPY",
		0x3c,"SLFTST",
		0x45,"RETG",
		0x61,"GATE",
		0xac,"CALLPS",
		0xc8,"RETPS",
		0,""
	};
	register struct romlist *rpt;
	void	get1byte();

	get1byte();
	cur1byte = cur1byte & 0x00ff;
	for( rpt = romlist; rpt->romcode != 0; rpt++) {
		if( cur1byte == rpt->romcode) {
			(void)strcpy(result, rpt->romstring);
			return;
		}
	}
	(oflag > 0)?	(void) sprintf(result,"MACRO %o",cur1byte):
			(void) sprintf(result,"MACRO %X",cur1byte);
	return;
}

/*
 *	get_hjmp_oprnd (result, symresult)
 *
 *	Get the next half word as a jump displacement operand.
 *	Call pr_hjmp_oprnd() to convert the characters, calculate the
 *	jump address and prepare for printing.
 *
 */

static void
get_hjmp_oprnd(result, symresult)
char	*result;
char	*symresult;
{
	static unsigned short	gethalfword();
	long	tlng;
	static void	pr_hjmp_oprnd();

	tlng = ((long)gethalfword()) & 0x0000ffffL;
	pr_hjmp_oprnd(result, symresult, tlng);
	return;
}

/*
 *	pr_hjmp_oprnd()
 *
 *	Calculates and prints jump address of branch instructions.
 *
 */

static void
pr_hjmp_oprnd(result, symresult, tmplng)
char	*result;
char	*symresult;
long	tmplng;
{
	long	neglng;
	extern	int	oflag;
	extern	int	sflag;

	if (tmplng & 0x00008000L) {
		neglng = TWO_16 - tmplng;
		tmplng = start - neglng;
		(void) sprintf(result++,"-");
	} else {
		neglng = tmplng;
		tmplng = start + tmplng;
	}
	if (oflag > 0) {
		if (sflag) {
			(void) sprintf(result,"0%lo",neglng);
			(void) sprintf(symresult,"<0%lo>",tmplng);			
		} else
			(void) sprintf(result,"0%lo <0%lo>",neglng, tmplng);
	} else {
		if (sflag) {
			(void) sprintf(result,"0x%lx",neglng);
			(void) sprintf(symresult,"<0x%lx>",tmplng);
		} else
			(void) sprintf(result,"0x%lx <0x%lx>",neglng, tmplng);
	}
	return;
}

/*
 *	pr_bjmp_oprnd()
 *
 *	Calculates and prints jump address of branch instructions.
 *
 */

static void
pr_bjmp_oprnd(result, symresult, tmplng)
char	*result;
char	*symresult;
long	tmplng;
{
	long	neglng;
	extern	int	oflag;
	extern	int	sflag;

	if (tmplng & 0x00000080L) {
		neglng = TWO_8 - tmplng;
		tmplng = start - neglng;
		(void) sprintf(result++,"-");
	} else {
		neglng = tmplng;
		tmplng = start + tmplng;
	}
	if (oflag > 0) {
		if (sflag) {
			(void) sprintf(result,"0%lo",neglng);
			(void) sprintf(symresult,"<0%lo>",tmplng);
		} else
			(void) sprintf(result,"0%lo <0%lo>",neglng, tmplng);
	} else {
		if (sflag) {
			(void) sprintf(result,"0x%lx",neglng);
			(void) sprintf(symresult,"<0x%lx>",tmplng);
		} else
			(void) sprintf(result,"0x%lx <0x%lx>",neglng, tmplng);
	}
	return;
}


/*
 *	get1byte()
 *
 *	This routine will read the next byte in the object file from
 *	the buffer (filling the 4 byte buffer if necessary). 
 *
 */

void
get1byte()
{
	extern	void	exit();
	extern	long	loc;
	extern	int	oflag;
	extern	char	object[];
	extern	int	trace;
	extern	char	bytebuf[];
	extern	unsigned short	cur1byte;
	static void	fillbuff();

	if (bytesleft == 0) {
		fillbuff();
		if (bytesleft == 0) {
			(void) fprintf(stderr,"\n%sdis:  premature EOF\n",SGS);
			exit(4);
		}
	}
	cur1byte = ((unsigned short) bytebuf[4-bytesleft]) & 0x00ff;
	bytesleft--;
	(oflag > 0)?	(void)sprintf(object, "%s%.3o ",object,cur1byte):
			(void)sprintf(object, "%s%.2x ",object,cur1byte);
	loc++;
	if (trace > 1)
		(void) printf("\nin get1byte object<%s> cur1byte<%.2x>\n",object,cur1byte);
	return;
}

/*
 *	gethalfword()
 *
 *	This routine will read the next 2 bytes in the object file from
 *	the buffer (filling the 4 byte buffer if necessary).
 */

static unsigned short
gethalfword()
{
	extern	unsigned short	cur1byte;
	extern	char	object[];
	extern	int	trace;
	union {
		unsigned short 	half;
		char		bytes[2];
	} curhalf;

	curhalf.half = 0;
#ifdef AR32W
	get1byte();
	curhalf.bytes[1] = (char)cur1byte;
	get1byte();
	curhalf.bytes[0] = (char)cur1byte;
#else
	get1byte();
	curhalf.bytes[0] = cur1byte;
	get1byte();
	curhalf.bytes[1] = cur1byte;
#endif
	if (trace > 1)
		(void) printf("\nin gethalfword object<%s> halfword<%.4x>\n",object,curhalf.half);
	return(curhalf.half);
}

/*
 *	getword()
 *	This routine will read the next 4 bytes in the object file from
 *	the buffer (filling the 4 byte buffer if necessary).
 *
 */

static long
getword()
{
	extern	void	exit();
	extern	long	loc;
	extern	int	oflag;
	extern	char	object[];
	extern	char	bytebuf[];
	extern	int	trace;
	char	temp1;
	short	byte0, byte1, byte2, byte3;
	int	i, j, bytesread;
	union {
		char	bytes[4];
		long	word;
	} curword;
	static void fillbuff();

	curword.word = 0;
	for(i = 0, j= 4 - bytesleft; i < bytesleft; i++, j++)
		curword.bytes[i] = bytebuf[j];
	if (bytesleft < 4) {
		bytesread = bytesleft;
		fillbuff();
		if ((bytesread + bytesleft) < 4) {
			(void) fprintf(stderr,"\n%sdis:  premature EOF\n",SGS);
			exit(4);
		}
		for (i = bytesread, j= 0; i < 4; i++, j++) {
			bytesleft--;
			curword.bytes[i] = bytebuf[j];
		}
	}
	byte0 = ((short)curword.bytes[0]) & 0x00ff;
	byte1 = ((short)curword.bytes[1]) & 0x00ff;
	byte2 = ((short)curword.bytes[2]) & 0x00ff;
	byte3 = ((short)curword.bytes[3]) & 0x00ff;
	(oflag > 0)?	(void)sprintf(object,"%s%.3o %.3o %.3o %.3o ",object,
					byte0, byte1, byte2, byte3):
			(void)sprintf(object,"%s%.2x %.2x %.2x %.2x ",object,
					byte0, byte1, byte2, byte3);
#ifdef AR16WR
	temp1 = curword.bytes[0];
	curword.bytes[0] = curword.bytes[2];
	curword.bytes[2] = temp1;
	temp1 = curword.bytes[1];
	curword.bytes[1] = curword.bytes[3];
	curword.bytes[3] = temp1;
#endif
#ifdef AR32W
	temp1 = curword.bytes[0];
	curword.bytes[0] = curword.bytes[3];
	curword.bytes[3] = temp1;
	temp1 = curword.bytes[1];
	curword.bytes[1] = curword.bytes[2];
	curword.bytes[2] = temp1;
#endif
	loc += 4;
	if (trace > 1)
		(void)printf("\nin getword object<%s>> word<%.8lx>\n",object,curword.word);
	return(curword.word);
}

/*
 * 	longprint
 *	simply a routine to print a long constant with an optional
 *	prefix string such as "*" or "$" for operand descriptors
 */

static void
longprint(result,prefix,value)
char	*result;
char	*prefix;
long	value;
{
	extern	int	oflag;

	if(oflag){
		(void) sprintf(result,"%s0%lo",prefix,value);
		}
	else{
		(void) sprintf(result,"%s0x%lx",prefix,value);
		}
	return;
}

/*
 *	fillbuff()
 *
 *	This routine will read 4 bytes from the object file into the 
 *	4 byte buffer.
 *	The bytes will be stored in the buffer in the correct order
 *	for the disassembler to process them. This requires a knowledge
 *	of the type of host machine on which the disassembler is being
 *	executed (AR32WR = vax, AR32W = maxi or 3B, AR16WR = 11/70), as
 *	well as a knowledge of the target machine (FBO = forward byte
 *	ordered, RBO = reverse byte ordered).
 *
 */

static void
fillbuff()
{
	extern	char	bytebuf[];
	extern  unsigned char *p_data;
	int i = 0;

	while( p_data != NULL && i<4){
                bytebuf[i] = *p_data;
                bytesleft = i+1;
                i++;
                p_data++;
        }

	switch (bytesleft) {
	case 0:
	case 4:
		break;
	case 1:
		bytebuf[1] = bytebuf[2] = bytebuf[3] = 0;
		break;
	case 2:
		bytebuf[2] = bytebuf[3] = 0;
		break;
	case 3:
		bytebuf[3] = 0;
		break;
	}
	/* NOTE		The bytes have been read in the correct order
	 *		if one of the following is true:
	 *
	 *		host = AR32WR  and  target = FBO
	 *			or
	 *		host = AR32W   and  target = RBO
	 *
	 */
#if !M32
#if (RBO && AR32WR) || (FBO && AR32W)
	bytebuf[0] = (char)((tlong >> 24) & 0x000000ffL);
	bytebuf[1] = (char)((tlong >> 16) & 0x000000ffL);
	bytebuf[2] = (char)((tlong >>  8) & 0x000000ffL);
	bytebuf[3] = (char)( tlong        & 0x000000ffL);
#endif

#if (FBO && AR32WR) || (RBO && AR32W)
	bytebuf[0] = (char)( tlong        & 0x000000ffL);
	bytebuf[1] = (char)((tlong >>  8) & 0x000000ffL);
	bytebuf[2] = (char)((tlong >> 16) & 0x000000ffL);
	bytebuf[3] = (char)((tlong >> 24) & 0x000000ffL);
#endif

#if RBO && AR16WR
	bytebuf[0] = (char)((tlong >>  8) & 0x000000ffL);
	bytebuf[1] = (char)( tlong        & 0x000000ffL);
	bytebuf[2] = (char)((tlong >> 24) & 0x000000ffL);
	bytebuf[3] = (char)((tlong >> 16) & 0x000000ffL);
#endif
#if FBO && AR16WR
	bytebuf[0] = (char)((tlong >> 16) & 0x000000ffL);
	bytebuf[1] = (char)((tlong >> 24) & 0x000000ffL);
	bytebuf[2] = (char)( tlong        & 0x000000ffL);
	bytebuf[3] = (char)((tlong >>  8) & 0x000000ffL);
#endif
#endif
	return;
}
