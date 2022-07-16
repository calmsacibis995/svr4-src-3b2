/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/sym.c	1.6"

/*
 * Symbol table processing routines
 *
 *
 * UNIX KERNEL SYMBOL TABLE
 * ------------------------
 *
 * The UNIX kernel symbol table is arranged to enable two types of access:
 * alphabetically, and by number.  The symbol table is numbered in
 * the object file, and relocation entries refer to symbols by number.
 * For symbol lookup, however, alphabetical searching is required.
 *
 * The Ksymtab[] array is the sorted symbol table containing the symbols
 * from the UNIX object file.  No auxilary entries or file names are
 * included.  Only symbols with storage class of C_STAT or C_EXT are
 * included.  In addition, only defined symbols are included.
 *
 * The K_ascii[] array is indexed by the first letter (ascii character)
 * of the symbol.  This provides the index of the first symbol in the
 * sorted symbol table beginning with that letter.
 *
 * The K_index[] array is indexed by the original symbol number.  This
 * provides a pointer to the symbol.  This pointer will be either a pointer
 * to the entry in Ksymtab[] or to the entry in *Xsymtab.
 *
 *
 * EXTERNAL SYMBOL TABLE
 * ---------------------
 *
 * All external symbols (those symbols which will be assigned values by
 * the boot program) are contained in the *Xsymtab symbol table.  This
 * symbol table is arranged as a sorted binary tree.
 *
 *
 * NOTES
 * -----
 * Symbols having a class of C_EXT or C_STAT are the only symbols included in
 * either symbol table.  Those symbols from the /unix object file having a
 * section number which is NOT zero are assumed to have CORRECT values; they
 * are entered into the Ksymtab[] array as DEFINED, and NO relocation is
 * performed for locations referencing these symbols.  Those symbols from the
 * /unix object file having a section number of zero are not defined; they are
 * entered into the *Xsymtab symbol table as EXTERN.  Relocation IS performed
 * for locations referencing these symbols.  In summary, relocation is performed
 * for symbols flagged EXTERN, and is not performed otherwise.
 *
 * All C_EXT or C_STAT symbols from a driver object file are entered into the
 * *Xsymtab symbol table, unless the symbol is already in the Ksymtab[] array.
 * Relocation is always performed for driver object files.
 */

/*
 * User level tables.c -  Changes have been made to Xsym_resolve, 
 * Xsym_copy, allocate, ucxdefine, Xrelocate and generate. The following
 * routines have been added. bss_allocate, ext_allocate, adj_rel
 * copysym, and Xrel_replace. The main thing to remember know is
 * that we no longer do relocation or define symbols, we merely
 * build symbol tables, relocation tables and text for conf.o . 
 * conf.o is in the COFF format. 
 */

#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/localtypes.h>
#include <a.out.h>
#include <sys/sys3b.h>
#include <sys/off.h>
#include <sys/sym.h>
#include <sys/error.h>
#include <sys/cunix.h>


static ushort		K_ascii[128+1];

static SYMBOL        	**K_index;

static SYMBOL        	*Ksymtab;

int			Ksym_count;



static struct Xsymbol	*Xsymtab = NULL;

int			Xsym_count;		/* running total of symbols in *Xsymtab */
int			Xsym_size;		/* running total of size of symbol names in *Xsymtab */


struct	Xreloc
	{
	struct Xreloc	*next;			/* ==> next relocation or NULL */
	address		r_paddr;		/* real address of reference */
	unsigned short	r_type;			/* relocation type */
	};



/*
 * Ksymread(nsyms)
 *
 * Read and organize the UNIX symbol table from the boot program; a side effect
 * is to allocate the kernel BSS
 */
 void
Ksymread(nsyms)
	long nsyms;
	{

	register i;
	register SYMENT *s;
	register SYMBOL *sp;
	register long *np, *n_shadow;
	char *p;
	int count;
	long size;

	/*
	 * count the number of symbol table entries that will be needed;
	 * only static and extern symbols are counted
	 *
 	 * at the same time, determine how much space is needed for the
	 * symbol names in Ksymtab[]
	 */
	if ((s=coff_symbol(bootprogram.path,(long*)NULL)) == NULL)
		{
		/* <bootprogram.path>: no symbols */
		error(ER60, bootprogram.path);
		goto exit;
		}

	count = size = 0;
	do	{
		if (s->n_scnum != 0  || s->n_sclass == C_TPDEF  ||
		   s->n_sclass == C_MOS  || s->n_sclass == C_MOU) {
			++count;

			if (s->n_zeroes)
				size += strlen(s->n_name) + 1;
			else
				size += strlen(s->n_name+s->n_offset) + 1;
			}
		}
		while ((s=coff_symbol((char*)NULL,(long*)NULL)) != NULL);

	Ksym_count = count;


	/*
	 * get the memory for the UNIX kernel symbol table
	 */
	if ((Ksymtab=sp=(SYMBOL *)malloc((unsigned)(count*sizeof(SYMBOL)+nsyms*sizeof(*K_index)+size))) == NULL)
		goto nomem;

	K_index = (SYMBOL **) (Ksymtab + count);

	p = (char*) (K_index + nsyms);

	/* pad n_shadow[] with one extra slot */
	if ((n_shadow=np=(long *)malloc(((unsigned)count+1)*sizeof(*n_shadow))) == NULL)
		goto nomem;

	/*
	 * build the UNIX symbol table and the n_shadow[] array; the n_shadow[]
	 * array will contain the original symbol table index for each symbol --
	 * that is, symbol[i] occured at location n_shadow[i] in the object file
	 *
	 * the rules used to determine the type of the symbol are:
	 *
	 *	n_scnum != 0	- the symbol is defined and has the value n_value
	 *	n_scnum == 0
	 *		n_value != 0	- the symbol is an undefined .bss symbol
	 *				  having a length of n_value
	 *		n_value == 0	- the symbol is an undefined external reference
	 *
	 * if the symbol is not defined, then add it to the *Xsymtab symbol
	 * table, and set the pointer in K_index now
	 */
	if ((s=coff_symbol(bootprogram.path,np)) == NULL)
		goto exit;

	do {
		if (s->n_scnum != 0  ||  s->n_sclass == C_TPDEF  ||
		   s->n_sclass == C_MOS  ||  s->n_sclass == C_MOU) {
			if (s->n_zeroes)
				strcpy(p, s->n_name);
			else
				strcpy(p, s->n_name+s->n_offset);

			sp->name = p;
			sp->value = s->n_value;
			sp->flag = DEFINED;

			if (s->n_sclass == C_STAT) {
				sp->flag |= STATIC;

				/* make sure Ksym_name() won't find statics */
				sp->name = "";
			} else if (s->n_sclass != C_EXT) {
				sp->flag |= PASSTHRU;
			}

			p += strlen(p) + 1;
			++sp;
			++np;
			}
		else
			{
			register SYMBOL *xsp;

			if (s->n_zeroes)
				xsp = Xsym_name(s->n_name);
			else
				xsp = Xsym_name(s->n_name+s->n_offset);

			K_index[ *np ] = xsp;

			if (s->n_value != 0)
				/*
				 * kernel BSS
				 */
				{
				xsp->flag |= BSS | DEFINED;
				xsp->size = s->n_value;
				}
			}
		}
		while ((s=coff_symbol((char*)NULL,np)) != NULL);


	/*
	 * sort the symbol table alphabetically, moving the corresponding
	 * n_shadow[] element
	 */
		{
		register m;

		for (m=count/2; m>0; m/=2)
			{
			for (i=0; i<count-m; ++i)
				{
				sp = & Ksymtab[i];
				np = & n_shadow[i];

				if (strcmp(sp[0].name,sp[m].name) > 0)
					{
					SYMBOL t;
					long t_shadow;

					t = sp[m];
					t_shadow = np[m];

					do	{
						sp[m] = sp[0];
						np[m] = np[0];
						np -= m;
						}
						while ((sp-=m)>=Ksymtab && strcmp(sp[0].name,t.name)>0);

					sp[m] = t;
					np[m] = t_shadow;
					}
				}
			}
		}


	/*
	 * now finish the K_index[] array; K_index[i] will be a pointer to
	 * the symbol whose original index was i
	 */
	for (i=0; i<count; ++i)
		K_index[ n_shadow[i] ] = Ksymtab + i;

	free((char*)n_shadow);

	/*
	 * now build the K_ascii[] array; K_ascii['c'] will be the subscript
	 * of the first symbol whose name begins with the character 'c'
	 */
	i = 0;
	for (sp=Ksymtab; sp<&Ksymtab[count]; ++sp)
		{
		while (i <= sp->name[0])
			K_ascii[i++] = sp - Ksymtab;
		}

	while (i <= 0x80)
		K_ascii[i++] = count;

#if TEST
	elapsed("UNIX symbol table indexes built");

#endif

	return;


	/*
	 * error exit
	 */
nomem:
	panic("No memory for symbol table\n");

exit:
	exit(1);
	}

/*
 * Ksym_name(name)
 *
 * Locate a symbol by name in the UNIX kernel symbol table; NULL is returned
 * if not found
 * name must be a NULL terminated string
 */
 SYMBOL *
Ksym_name(name)
	register char *name;
	{

	register SYMBOL *p;
	register SYMBOL *q;
	register result, low, high, middle;

	low = K_ascii[ name[0] ];
	high = K_ascii[ name[0] + 1 ];

	do {
		p = Ksymtab + (middle = (low+high)>>1);

		if ((result=strcmp(name,p->name)) == 0)
			if (p->flag & STATIC  ||  p->flag & PASSTHRU)
				break;
			else
				return(p);

		if (result < 0)
			high = middle-1;
		else
			low = middle+1;
	} while (low <= high);

	if (!(p->flag & PASSTHRU))
		return(NULL);
	
	q = p - 1;
	while (!strcmp(name, q->name)) {
		if (!(q->flag & PASSTHRU))
			return(q);
		q--;
	}
	
	q = p + 1;
	while (!strcmp(name, q->name)) {
		if (!(q->flag & PASSTHRU))
			return(q);
		q++;
	}

	return(NULL);
}

/*
 * Ksym_number(number)
 *
 * Locate a symbol by number in the UNIX kernel symbol table
 */
 SYMBOL *
Ksym_number(number)
	register long number;
	{

	return(K_index[number]);
	}

/*	Ksym_copyall()
**
**	This routine is invoked only if DebugMode is set.
**	It copies all defined entries in Ksymtab to
**	Xsymtab.  This ensures that they get copied into
**	the sys3b symbol table where they can be accessed
**	by demon.
*/

Ksym_copyall()
{
	register SYMBOL	*sp;
	register SYMBOL	*splim;

	sp = Ksymtab;
	splim = &sp[Ksym_count];

	while (sp < splim) {
		if (sp->flag & DEFINED && *(sp->name) != 0 ){
			ucxdefine(sp->name, sp->flag, sp->value);
			}
		sp++;
	}
}

/*
 * Xsym_name(name)
 *
 * Locate a symbol by name in the *Xsymtab binary tree.  If not found, then
 * the symbol is inserted as EXTERN but not DEFINED.  This routine implements
 * a balanced tree search algorithm, reference Knuth (6.2.3) algorithm A.
 *
 * Note that STATIC symbols will never be found.
 */

#define	INVERSE(balance)	(balance ^ ('<'^'>'))

 SYMBOL *
Xsym_name(name)
	char *name;
	{

	register struct Xsymbol	*p;		/* Pointer moving down the tree */
	register struct Xsymbol	*q;		/* New node if key not found */
	register struct Xsymbol	*r;		/* Re-balance pointer */
	register struct Xsymbol	*s;		/* Re-balance point */
	register struct Xsymbol	*t = NULL;	/* Father of S */
	register int result;			/* Result of compare/balance factor */

	if ((p = Xsymtab) == NULL)				/* A1 */
		/*
		 * tree is empty
		 */
		{
		if ((p = Xsymtab = (struct Xsymbol*) xmalloc(sizeof(*p)+strlen(name)+1)) == NULL)
			panic("No memory for Xsymbol");

		Xsym_count = 1;
		Xsym_size = (strlen(name)+1+sizeof(long) + sizeof(long)-1) & ~(sizeof(long)-1);

		p->name = strcpy((char*)(p+1), name);
		p->size = 0;
		p->x.reloc = NULL;
		p->flag = EXTERN;
		p->balance = 0;
		p->lnext = p->rnext = NULL;

		return((SYMBOL *) p);
		}

	for (s=p; ; p=q)					/* A1; ; A3,A4 */
		{

		if ((result=strcmp(name,p->name)) == 0) /* A2: compare */
			if (p->flag & STATIC)
				result = 1;
			else
				return((SYMBOL *) p);

		if (result < 0)				/* A3: move left */
			{
			if ((q=p->lnext) == NULL)
				/*
				 * create new node
				 */
				{
				if ((q=p->lnext=(struct Xsymbol *)xmalloc(sizeof(*q)+strlen(name)+1)) == NULL)
					panic("No memory for Xsymbol");
				break;
				}
			}
		else						/* A4: move right */
			{
			if ((q=p->rnext) == NULL)
				/*
				 * create new node
				 */
				{
				if ((q=p->rnext=(struct Xsymbol *)xmalloc(sizeof(*q)+strlen(name)+1)) == NULL)
					panic("No memory for Xsymbol");
				break;
				}
			}

		if (q->balance != 0)
			{
			t = p;
			s = q;
			}
		}

	++Xsym_count;
	Xsym_size += (strlen(name)+1+sizeof(long) + sizeof(long)-1) & ~(sizeof(long)-1);

	q->name = strcpy((char*)(q+1), name);			/* A5: initialize new node */
	q->size = 0;
	q->x.reloc = NULL;
	q->flag = EXTERN;
	q->balance = 0;
	q->lnext = q->rnext = NULL;

	if ((result=strcmp(name,s->name)) < 0)		/* A6: adjust balance factors */
		r = p = s->lnext;
	else
		r = p = s->rnext;

	while (p != q)
		{
		if (strcmp(name,p->name) < 0)
			{
			p->balance = '<';
			p = p->lnext;
			}
		else
			{
			p->balance = '>';
			p = p->rnext;
			}
		}

	result = (result<0)? '<' : '>';				/* A7: balancing act */

	if (s->balance == 0)
		/*
		 * the tree has grown higher
		 */
		{
		s->balance = result;
		return((SYMBOL *) q);
		}

	if (s->balance == INVERSE(result))
		/*
		 * the tree has gotten more balanced
		 */
		{
		s->balance = 0;
		return((SYMBOL *) q);
		}

	/* s->balance == result
	 *
	 * the tree has gotten out of balance
	 */
	if (r->balance == result)
		/*
		 * single rotation				   A8
		 */
		{
		p = r;
		if (result == '<')
			{
			s->lnext = r->rnext;
			r->rnext = s;
			}
		else
			{
			s->rnext = r->lnext;
			r->lnext = s;
			}
		s->balance = r->balance = 0;
		}
	else
		/*
		 * double rotation				   A9
		 */
		{
		if (result == '<')
			{
			p = r->rnext;
			r->rnext = p->lnext;
			p->lnext = r;
			s->lnext = p->rnext;
			p->rnext = s;
			}
		else
			{
			p = r->lnext;
			r->lnext = p->rnext;
			p->rnext = r;
			s->rnext = p->lnext;
			p->lnext = s;
			}

		if (p->balance == 0)
			s->balance = r->balance = 0;
		else
			{
			if (p->balance == result)
				{
				s->balance = INVERSE(result);
				r->balance = 0;
				}
			else
			  /* p->balance == INVERSE(result) */
				{
				s->balance = 0;
				r->balance = result;
				}
			p->balance = 0;
			}
		}

	/*
	 * re-balancing transformation is now complete, with p pointing to the
	 * new root and t pointing to the father of the old root
	 */
	if (t == NULL)					/* A10: finishing touch */
		Xsymtab = p;
	else
		if (s == t->rnext)
			t->rnext = p;
		else
			t->lnext = p;

	return((SYMBOL *) q);
	}

/*
 * Xsym_walk(root, function, argument)
 *
 * Perform an in-order walk of the *Xsymtab symbol table.  At each node
 * call (*function) with arguments (sp, level) where "sp" is a pointer
 * to the node, and "level" is the current depth of the tree.
 */
 /*VARARGS2*/
 void
Xsym_walk(root, function, argument)
	register struct Xsymbol *root;
	register void (*function)();
	int argument;
	{

	register int level;
	register struct Xsymbol *r;

	if (root == NULL)
		/*
		 * this is the first time; initiate an inorder traversal
		 */
		{
		if ((root = Xsymtab) == NULL)
			return;

		level = 1;
		}
	else
		level = argument;

	if (root->lnext)
		Xsym_walk(root->lnext, function, level+1);

	r = root->rnext;	/* in case *root is freed by (*function)() */

	(*function)(root, level);

	if (r)
		Xsym_walk(r, function, level+1);
	}

/*
 * Xsym_free(root)
 *
 * Free the Xsymtab entry *root and any chained relocation entries
 */
 static
 void
Xsym_free(root)
	register struct Xsymbol *root;
	{

	register struct Xreloc *rp, *tp;


	if (!(root->flag & DEFINED))
		{
		rp = root->x.reloc;

		while ((tp=rp) != NULL)
			{
			rp = rp->next;
			free((char*)tp);
			}
		}

	free((char*)root);
	}

#if DEBUG3
/*
 * Xdepth(root, level)
 *
 * Compute the depth of the Xsymtab binary tree
 */
 /*VARARGS1*/
 int
Xdepth(root, level)
	register SYMBOL *root;
	register level;
	{

	static long maxlevel;

	if (root == NULL)
		/*
		 * first time called; walk the tree
		 */
		{
		maxlevel = 0;

		Xsym_walk(Xsymtab, Xdepth, 1);

		return(maxlevel);
		}

	maxlevel = max(maxlevel, (long)level);
	}

/*
 * Xprint(root, level)
 *
 * Print the contents of the *Xsymtab in alphabetical order.
 */
 /*VARARGS1*/
 void
Xprint(root, level)
	register SYMBOL *root;
	register level;
	{

	if (root == NULL)
		/*
		 * first time called; walk the tree
		 */
		Xsym_walk(Xsymtab, Xprint, 1);
	else
		{

		printf("   [%2d]  size=%5D  flag=%2X  0x%8lX  %s\n", level, root->size, root->flag, root->value, root->name);
		}
	}
#endif


/*
 * Xsym_resolve(sp)
 *
 * Make sure that every symbol in *Xsymtab is resolved.  If a symbol is
 * not yet DEFINED (and it is not marked DEFER) then it may be a reference
 * to a routine name in an unloaded driver.  If so, call routine() to define
 * the symbol.  Otherwise, the symbol is an unresolved external reference.
 * User level comment: see routine in loadunix.c, if a symbol is unresolved
 * let the loader handle it later.
 */
 void
Xsym_resolve(sp)
	register struct Xsymbol *sp;
	{

	if (!(sp->flag & DEFINED))
		/*
		 * symbol is still undefined
		 */
		{


		if (sp->flag & DEFER)
			return;
		(void) routine(sp->name,0);
			
		}
	}
/*
 * Xsym_copy(root, level, origin)
 *
 * Copy the *Xsymtab symbol table alphabetically to the s3bsym structure for
 * the sys3b(2) system call.
 */

 void
Xsym_copy(root, level, origin)
	register SYMBOL *root;
	int level;
	struct s3bsym *origin;
{

	register length;
	static char *s3bname;

	if (root == NULL)
		/*
		 * first time called; initiate the traversal of *Xsymtab
		 */
		{
		s3bname = origin->symbol;

		Xsym_walk(Xsymtab, Xsym_copy, 0);

		return;
		}

	/* if (!(root->flag & DEFINED))
		panic(root->name); */

	if (!(root->flag & STATIC)){  /* can't get access to statics */
		length = (strlen(root->name)+1 + sizeof(long)-1) & ~(sizeof(long)-1);

		strncpy(s3bname, root->name, length);
		if (!(root->flag & PASSTHRU)){
			/* if symbol defined within conf.o no need to ext_allocate it */
			if (!(root->flag & CONFLOCAL)) 
				ext_allocate(s3bname);
			Xrelocate((address)(s3bname + length),R_DIR32,s3bname,&data_rel);
		}
		else 	{ /* these are MOS and MOU values */
			*(long *) (s3bname + length) = root->value;
			}

	/*
	 * bump the floating pointer
	 */
		s3bname += length + sizeof(long);
	}
}
/*	ucxdefine(name, value)
**
**	Unconditionally define a symbol.  This rouine is basically
**	the same as "define(name, value)" above except that here
**	it is not an error for the symbol to already be defined
**	and no relocation is done.  This routine is used by
**	Ksym_copyall() to copy all defined symbols from Ksymtab
**	to Xsymtab just before the sys3b symbol table is written.
**	This ensures that all defined symbols are known to demon.
*/

ucxdefine(name, flag, value)
register char		*name;
unsigned char flag;
register address	value;
{
	register struct Xsymbol	*sp;

	sp = (struct Xsymbol *)Xsym_name(name);
	if (sp->flag &~ DEFINED){
		sp->flag |= DEFINED;
		if (flag & PASSTHRU)
			sp->flag |= PASSTHRU;
		sp->x.value = value;
	}
}

