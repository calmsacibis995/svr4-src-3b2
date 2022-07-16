/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mkboot-3b2:mkboot.h	1.8.5.1"

# include	<sys/types.h>
# include	<sys/param.h>
# include	<sys/dir.h>
# include	<sys/boothdr.h>
# include	<unistd.h>
# include	<stdlib.h>
# include	<string.h>
# include	<setjmp.h>



/*
 * CONSTANTS
 */

# define	TRUE		(2>1)
# define	FALSE		(1>2)

/*
 * We have to define NBBY here since it's not defined in the Amdahl header files.
 *	See SPECIAL ITEMS comment below.
 */
# ifdef CROSS
#     define NBBY 8
# endif

/* maximum (signed) literal that will fit in a struct format */
# define	MAXLITERAL	((int)((1 << ((sizeof(offset) * NBBY) - 1)) - 1))

/*
 * ARRAY SIZES
 */

# define	MAXNODE		250	/* maximum expression tree nodes per expression */

# define	MAXDEP		25	/* maximum dependencies per module */
# define	MAXRTN		200	/* maximum routine definitions per module */
# define	MAXVAR		200	/* maximum variable definitions per module */
# define	MAXINIT		1000	/* maximum initializer format items per module */
# define	MAXEXPR		2000	/* maximum size of all expressions per module */
# define	MAXSTRING	3000	/* maximum size of all strings/names per module */

# define	MAXPARAM	100	/* maximum number of parameters per module */


/*
 * MACROS
 */

# define	min(x,y)	(((x)<=(y))? (x) : (y))
# define	max(x,y)	(((x)>=(y))? (x) : (y))


/*
 * SPECIAL ITEMS - Since mkboot needs to be compile for both
 *    the native (3b2) and cross (Amdahl) environments, we
 *    have to make special concessions here.  New POSIX typedefs
 *    are not defines for the Amdahl so we have to use trickery
 *    to determine what it should be defined as.
 */

# ifdef CROSS
#     define MODE_T ushort
# else
#     define MODE_T mode_t
# endif


/*
 * STRUCTURES
 */

extern struct master		master;
extern struct depend		depend[], *ndepend;
extern struct param		param[], *nparam;
extern struct routine		routine[], *nroutine;
extern struct variable		variable[], *nvariable;
extern struct format		format[], *nformat;
extern char			element[], *nelement;
extern char			string[], *nstring;

extern jmp_buf			*jmpbuf;
extern char			any_error;

/*
 * Expression tree nodes
 */
struct	tnode
	{
	struct tnode	*left;	/* pointer to left tnode	*/
	struct tnode	*right;	/* pointer to right tnode	*/
	int		value;	/* numerical value	*/
	char		*name;	/* name or string */
	char		type;	/* type	*/
	};

extern struct tnode	tree[], *ntree;

/*
 * Function declarations
 */
typedef unsigned char	boolean;

extern char		*basename();
extern int		build_header();
extern boolean		check_master();
extern char		copy_driver();
extern char		*copystring();
extern void		fatal();
extern struct param	*findparam();
extern void		getparam();
extern int		getsize();
extern char		*lcase();
extern int		mylseek();
extern void		myperror();
extern int		myread();
extern int		mywrite();
extern struct tnode	*node();
extern void		polish();
extern void		print_expression();
extern char		*print_flag();
extern void		print_master();
extern char		*ucase();
extern void		warn();
extern void		yyerror();
extern void		yyfatal();
extern int		yyparse();
