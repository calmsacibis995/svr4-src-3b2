/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/CmdLine.c	1.11"

#include	<ctype.h>
#include	<stdio.h>
#include	<string.h>
#include	"defs.h"
#include	"debug.h"
#include	"ANodeTypes.h"
#include	"olddefs.h"
#include	"sgs.h"
	/* private functions */
STATIC char *GetDBmask();	/* Get debugging mask. */
STATIC void mustopen();		/* Opens files.	*/

	void
ParseCmdLine(argc,argv)		/* Parses optimizer's command line.	*/
int argc;			/* Argument count.	*/
char *argv[];			/* Argument pointers.	*/

{extern void DBinit();		/* Initialize debugging.	*/
 extern void Target();		/* Set target configuration.	*/
 extern void SetTargetMath();	/* Set target math chip.	*/
 extern void Xinit();		/* Initilize optimization skipping.	*/
 extern boolean Zflag;		/* Flag indicating nonvolatile globals. */
 extern void addlib();		/* Add a src archive library. 	*/
 register int c;		/* Return value from getopt().	*/
 extern enum CC_Mode ccmode;	/* Mode set by -X? flag.	*/
 extern boolean cflag;		/* TRUE for conservative common tail.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 unsigned int fileseen;		/* Counts arguments without '-': assumed filenames. */
 extern int getopt();		/* Helps parse command lines; in C(3) library.*/
 extern boolean hflag;		/* Peephole disable if TRUE.	*/
 extern boolean identflag;	/* True if output .ident string.	*/
 extern int nrot;		/* Number of rotated loops.	*/
 boolean onopt;			/* For -D, TRUE if turning on optimizations.	*/
 extern char *optarg;		/* Start of option; set by getopt().	*/
 extern int optind;		/* Index of next argument; set by getopt. */
 extern int optmode;		/* Optimization mode wanted.	*/
 extern void pcdecode();
 char *q;
 extern boolean sflag;		/* Want statistics on stderr if TRUE.	*/
 extern long strtol();		/* Converts strings to longs; in C(3) lib. */
 long int xlevel;		/* Requested debugging level.	*/
 unsigned long int xmask;	/* Requested debugging mask.	*/
 unsigned long int xpmask;	/* Requested peephole debugging mask.	*/
 char *xstring;			/* Requested debugging string (func name). */
 extern boolean zflag;		/* TRUE for inline debugging. */

 fileseen = 0;
 xmask = 0;
 xlevel = 0;
 while((c = getopt(argc,argv,"Cd:D:hI:J:K:O:Q:rstT:Vx:X:y:zZ")) != EOF)
	switch(c){			/* Decode option. */
	case 'C':			/* Do aggressive common tail merge.	*/
		cflag = FALSE;
		endcase;
	case 'h':			/*Kill peephole optimizations.*/
		hflag = TRUE;
		endcase;
	case 'I':			/* Want to specify input. */
		mustopen(optarg,"r",stdin);
		fileseen++;
		endcase;
	case 'J':			/* Source archive.	*/
		SkipWhite(optarg);
		addlib(optarg);
		endcase;
	case 'K':
		SkipWhite(optarg);	/* Skip whitespace in case */
					/* someone embedded it in a */
					/* flag.	*/
		switch(*optarg){
		case 's':
			switch(*++optarg){
			case 'd':
				optmode = OSPEED;
				endcase;
			case 'z':
				optmode = OSIZE;
				cflag = FALSE;
				endcase;
			default:
				fatal("ParseCmdLine: bad -K s argument.\n");
					/*NOTREACHED*/
					endcase;
			} /* END OF switch(*optarg++) */
			endcase;
		case 'c':
			SetTargetMath('c');
			endcase;
		case 'f':
			if(strcmp(optarg,"fpe") == 0)
				SetTargetMath('f');
			else
				fatal("ParseCmdLine: bad -K argument (%s)\n", optarg);
			endcase;
		case 'm':
			if(strcmp(optarg,"mau") == 0)
				SetTargetMath('m');
			else
				fatal("ParseCmdLine: bad -K argument (%s)\n", optarg);
			endcase;
		case 'p':
			if(strcmp(optarg,"pic") == 0)
				PIC_flag=1;
			else
				fatal("ParseCmdLine: bad -K argument (%s)\n", optarg);
			endcase;
		case 'P':
			if(strcmp(optarg,"PIC") == 0)
				PIC_flag=1;
			else
				fatal("ParseCmdLine: bad -K argument (%s)\n", optarg);
			endcase;
		case EOS:
			fatal("ParseCmdLine: -K suboption missing.\n");
			endcase;
		default:
			fatal("ParseCmdLine: unrecognized -K suboption '%s'.\n",optarg);
			endcase;
		}
		endcase;
	case 'O':			/* Want to specify output. */
		mustopen(optarg,"w",stdout);
		if(fileseen == 1)
			fileseen++;
		endcase;
	case 'Q':			/* .ident info */
		switch(*optarg){
		case 'y':
			identflag = TRUE;
			endcase;
		case 'n':
			identflag = FALSE;
			endcase;
		default:
			fatal("ParseCmdLine: bad argument ('%c') to -Q\n",*optarg);
		}
		endcase;
	case 'r':
		nrot = -1;
		endcase;
	case 's':
		sflag = TRUE;
		endcase;
	case 't':			/* Turn off all optimizations. */
		Xinit(~0,~0,"");
		endcase;
	case 'T':
		Target(optarg);
		endcase;
	case 'V':			/* Want version ID.	*/
		fprintf(stderr,"optim: %s%s\n",SGU_PKG,SGU_REL);
		endcase;
	case 'd':
	case 'x':	/* set mask for debugging output */
		SkipWhite(optarg);	/* Skip whitespace in case */
					/* someone embedded it in a */
					/* flag.	*/
		xlevel = 0;
		xstring = "";
		optarg = GetDBmask(optarg, &xmask, &xpmask);
		if(*optarg == ','){
			xlevel = strtol(++optarg,&q,0);
			if(optarg == q)
				fatal( "ParseCmdLine: -x level missing.\n" );
			optarg = q;
			if(*optarg == ','){
				xstring = ++optarg;
				}
			}
		DBinit(xmask,xlevel,xstring);
		endcase;
	case 'D':	/* set debugging mask for selecting optimizations */
		SkipWhite(optarg);	/* Skip whitespace in case */
					/* someone embedded it in a */
					/* flag.	*/
		onopt = TRUE;		/* turn on only those specified. */
		if( *optarg == '~' ){
			onopt = FALSE;	/* turn off only those specified. */
			++optarg;
		}
		optarg = GetDBmask(optarg, &xmask, &xpmask);
		if( (xmask & XPEEP) && xpmask != 0 ){
			if(onopt)
				xpmask = ~xpmask;
			else
				xmask &= ~XPEEP;
		}
		if(onopt)
			xmask = ~xmask;
		xstring = "";
		if(*optarg == ','){
			xstring = ++optarg;
			}
		Xinit(xmask,xpmask,xstring);
		endcase;
	case 'X':	/* set ansi flags */
		SkipWhite(optarg);	/* Skip whitespace in case */
					/* someone embedded it in a */
					/* flag.	*/
		switch(*optarg){
		case 't': ccmode = Transition; break;
		case 'a': ccmode = Ansi; break;
		case 'c': ccmode = Conform; break;
		default: 
			fatal("ParseCmdLine: -X requires 't', 'a', or 'c'\n");
			break;
		}
		endcase;
	case 'y': /* specifies limit on expansion of text caused
		     by inline expansion */
		SkipWhite(optarg);	
		pcdecode(optarg);
		endcase;
	case 'Z':
		Zflag = TRUE;
		endcase;
	case 'z':
		zflag = TRUE;
		endcase;
	default:
		fatal("ParseCmdLine: unrecognized option.\n");
		/*NOTREACHED*/
		endcase;
	} /* END OF switch(c) */

 for( ; optind < argc; optind++)		/* Do rest of command line. */
	{switch(fileseen)			/* Alternate files.	*/
		{case 0:			/* None yet: assume input. */
			mustopen(argv[optind],"r",stdin);
			fileseen++;
			endcase;
		 case 1:			/* Input done: assume output. */
			mustopen(argv[optind],"w",stdout);
			fileseen++;
			endcase;
		 default:			/* Both seen: trouble. */
			fatal("main: too many files.\n");
			/*NOTREACHED*/
			endcase;
		} /* END OF switch(fileseen) */
	} /* END OF for( ; optind < argc; optind++) */

 if(ccmode != Transition)			/* ANSI modes guarantees */
	Zflag = TRUE;				/* globals nonvolatile,  */
						/* unless they are explicitly */
						/* volatile.	*/

 if(DBdebug(0,-1))
	setbuf(stdout, (char *)NULL);		/* For easier debugging. */

 return;
}

	STATIC char *
GetDBmask(s, mp, pmp)
register char *s;
unsigned long *mp, *pmp;
{
 static struct mtent {
	char *mname;
	unsigned long mbit;
	} mtab[] = {
	"CSE",		XCSE,
	"GRA",		XGRA,
	"IL_FP",	XIL_FP,
	"IL_INSERT",	XIL_INSERT,
	"IL_PUSH",	XIL_PUSH,
	"IL",		XIL,
	"INCONV",	XINCONV,
	"LICM_1",	XLICM_1,
	"LICM_2",	XLICM_2,
	"LICM_3",	XLICM_3,
	"LICM_4",	XLICM_4,
	"LICM",		XLICM,
	"OUTCONV",	XOUTCONV,
	"PCI_BGR",	XPCI_BGR,
	"PCI_COMT",	XPCI_COMT,
	"PCI_LDA",	XPCI_LDA,
	"PCI_MBR",	XPCI_MBR,
	"PCI_REORD",	XPCI_REORD,
	"PCI_RMBR",	XPCI_RMBR,
	"PCI_RL",	XPCI_RL,
	"PCI_RUNR",	XPCI_RUNR,
	"PCI",		XPCI,
	"PEEP_1",	XPEEP_1,
	"PEEP_2",	XPEEP_2,
	"PEEP_3",	XPEEP_3,
	"PEEP",		XPEEP,
	"TB_AD",	XTB_AD,
	"TB_LD",	XTB_LD,
	"V_USED",	XV_USED,
	"V_SET",	XV_SET,
	"VTRACE",	XVTRACE,
	NULL,		0,
	};
 register struct mtent *tp;
 int len;
 unsigned long mask;
 unsigned long pmask;
 int pno;
 char *t;
 unsigned long GetPmask();
 extern void fatal();
 extern long strtol();		/* Converts strings to longs; in C(3) lib. */

	/* syntax recognized: 
	 *	IL+...+PEEP#201+...+PEEP_3#101+...
	 * generates a mask that has a 1 bit for each corresponding opt name.
	 * for PEEP, the #n represents a specific  peephole, and a corresponding
	 * bit is set in a separate mask.
	 */

	mask = pmask = 0;
	for(;;){
		for(tp = &mtab[0]; tp->mname != NULL; ++tp){
			len = strlen(tp->mname);
			if(strncmp(s, tp->mname, len) == 0){
				mask |= tp->mbit;
				s += len;
				break;
			}
		}
		if(tp->mname == NULL)
			fatal("GetDBmask: unknown mask name '%s'\n", s);
		else if( (tp->mbit & XPEEP) &&  *s == '#' ){
			pno = strtol(++s, &t, 0);
			pmask |= GetPmask(pno);
			s = t;
		}
		switch(*s){
		case ',':	
		case EOS:	break;
		case '+':	++s; continue;
		default:
			fatal("GetDBmask: ill-formed mask '%s'\n", s);
		}
		break;
	}
	*mp = mask;
	*pmp = pmask;
	return(s);
}


	STATIC void
mustopen(name,dir,file)
char *name,*dir;
FILE *file;

{extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 /*extern FILE *freopen();	** Reopens a stream; in C(3) library.	*/

 if(freopen(name,dir,file) == NULL)
	fatal("mustopen: can't open: %s (%d).\n",name,errno);
 return;
}

