/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:sys/sym.h	1.2"

/*
 * Struct Xsymbol must match SYMBOL
 */
struct	Xsymbol
	{
        char			*name;			/* symbol name */
	union	{
                address value;				/* if DEFINED: value of
symbol */
         	struct Xreloc *reloc;			/*	 else: ==> relocation entry chain */
		}
                 		x;
	unsigned long		size;			/* sizeof symbol */
	unsigned short		flag;			/* always EXTERN; DEFINED? */
 	int nsymindex;					/* symbol index in nsym
*/
	char			balance;		/* balance factor; '<',
0 or '>' */
	struct Xsymbol		*lnext;			/* ==> left child */
	struct Xsymbol		*rnext;			/* ==> right child */
	};




/*
 * Symbol table entries
 */
typedef struct	{
		char			*name;			/* symbol name */
                address                 value;                  /* value of symb
ol */
		unsigned long		size;			/* sizeof symbol */
		unsigned short		flag;

		int			nsymindex;		/* only defined
for symbols with CONFLOCAL set in flag */
		}
		SYMBOL;

#define CONFLOCAL	0x100	/* symbol defined in conf.o */
#define EXTERN		0x80	/* symbol must be DEFINED by lboot */
#define STATIC		0x40	/* symbol is declared STATIC;
                                 * it will not be found by Ksym_name()
                                 * or Xsym_name() */
#define BSS		0x20	/* symbol is defined in BSS section */
#define PASSTHRU	0x10	/* This symbol is only here to pass
                                 * thru to the sys3b symbol table.
                                 * It should not be used for and
                                 * relocation.			  */
#define DEFER		0x08	/* do not attempt to find a routine
                                 * name to resolve this symbol
                                 */
#define NOINIT		0x04	/* driver data structure is not to be
                                 * initialized */
#define DRIVER		0x02	/* symbol is defined within a driver */
#define DEFINED		0x01	/* symbol is defined */

extern struct	rtname				/* functions defined in the UNIX kernel */
          	{
                char   *name;
		SYMBOL *symbol;
		}
                 	rtname[];


extern SYMBOL	      *Xsym_name();		/* search external symbol table
by name */
extern void	       Xsym_resolve();		/* resolve all undefined symbols in external symbol table */
extern void	       Xsym_walk();		/* do an in-order traversal of the external symbol table */
extern SYMBOL	      *Ksym_name();		/* lookup bootprogram symbol by
name */

extern int	Xsym_count;		/* total symbols in Xsymtab */
extern int	Xsym_size;		/* total size of symbol names in Xsymtab */

