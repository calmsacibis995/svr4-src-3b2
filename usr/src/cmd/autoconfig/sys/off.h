/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:sys/off.h	1.4"


/*
 * Location counter
 */
typedef struct	{
	address v_locctr;	/* virtual location counter */
	address v_origin;	/* virtual origin */
	address end;		/* end of allocated area */
	} LOCCTR;

typedef struct {
	int count;
	address origin;
	address end;		/* end of allocated area */
	RELOC *rp;
	} REL_TAB;

typedef struct {
	int count;
	address origin;
	address end;		/* end of allocated area */
	SYMENT *nsyment;
	} NSYMENT;

typedef struct {
	address origin;
	address locctr;
	address end;		/* end of allocated area */
	} STRING_TAB;

#define REAL(vaddr,x)	(vaddr)
#define SIZE(locctr)	(locctr.v_locctr - locctr.v_origin)


extern SCNHDR	      *coff_section();		/* read common object file section headers */
extern SYMENT	      *coff_symbol();		/* read common object file symbol table entries */


extern REL_TAB		data_rel;		/* relocation table data */
extern REL_TAB		text_rel;		/* relocation table text */
extern NSYMENT nsym;				/* number of symbols	 */
extern LOCCTR		 text_locctr;		/* .text */
extern LOCCTR		 data_locctr;		/* .data */
extern STRING_TAB	 string_tab;		/* string table */


extern short donotload;

extern int data_symndx;
extern int text_symndx;

extern void fake_sect();
extern void init_sym_size();

#define MAXTSIZE 2048
#define MAXTRELOC 2048
#define MAXDSIZE 204800 
#define MAXDRELOC 81920
#define MAXSYM 204800
#define MAXSTRING 81920

