/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/driver_proc.c	1.19"

#include <sys/types.h>
#include <sys/localtypes.h>
#include <stdio.h>
#include <a.out.h>
#include <varargs.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/boothdr.h>
#include <sys/sys3b.h>
#include <sys/psw.h>
#include <sys/pcb.h>
#include <sys/error.h>
#include <sys/sym.h>
#include <sys/dproc.h>
#include <sys/machdep.h>
#include <sys/gen.h>
#include <sys/off.h>
#include <sys/fcntl.h>
#include <sys/cunix.h>
#include <sys/cred.h>
#include <sys/resource.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/class.h>



struct vfssw	*vfsswtab;
struct vfssw	*vfsswp;
struct class	*classtab;
struct class	*classp;
struct cdevsw    *cdevswp;		/* real address */
struct bdevsw    *bdevswp;		/* real address */
struct fmodsw    *fmodswp;		/* real address */
struct execsw	*execswp;

int nkpcb;

/*
 * Alloc_variables()
 *
 * This routine will allocate any variables which are declared in the
 * optional header built by mkboot(1M).  These variables will be initailized
 * later if necessary.
 */
 void
alloc_variables(dp)
	register struct driver *dp;
	{

	register struct master *mp = dp->opthdr;
	register SYMBOL *sp;
	register long number;
	register char *name;
	struct cdevsw *cdevp;

	cdevp = & cdevswp[ dp->int_major ];

	if (mp->nvar)
		{
		register struct variable *vp = (struct variable *)POINTER(mp->o_variable,mp);
		register struct variable *first = (struct variable *)POINTER(mp->o_variable,mp);
		int dim;


		for (number = 0; number < mp->nvar; ++number, ++vp)
			{
			if (vp->dimension == NULL)
				dim = 1;
			else
				dim = eval((union element *)POINTER(vp->dimension,mp), dp);

			name = (char *)POINTER(vp->name,mp);

			if ((sp = Ksym_name(name)) != NULL)
				{
				/* <name>: already allocated */
				error(ER62, name);
				continue;
				}
			if ((sp = Xsym_name(name))->flag & DEFINED)
				{
				sp->flag |= NOINIT;

				/* <name>: previously allocated */
				error(ER48, name);
				continue;
				}

			if (vp->ninit)
				generate(G_UDATA, name, (long)((dim*vp->size+sizeof(int)-1) & ~(sizeof(int)-1)));
			else
				bss_allocate(name, (long)dim*vp->size);
			}
		if (mp->flag & (CHAR | FUNDRV) && mp->flag & TTYS)
			/*
			 * set cdevsw[].d_ttys
			 */
			Xrelocate(&cdevp->d_ttys, R_DIR32, (char*)POINTER(first->name,mp),&data_rel);
		}
	}


/*
 * Initdata()
 *
 * Initialize the generated initialized variables for the drivers.
 */

#define	ALIGN(locctr,type)	((locctr+sizeof(type)-1)&~(sizeof(type)-1))

#define	SETDATA(locctr,type)	*((type*)locctr)

 void
initdata(dp,fscnt, clcnt)
	struct driver *dp;
	int fscnt, clcnt;
	{

	register struct master *mp;
	register struct variable *vp;
	register struct format *fp;
	register address locctr;
	SYMBOL *sp;
	int n1, n2;
	int clinitflag = 0;
	char *name;

	if ((mp=dp->opthdr)->nvar > 0)
		{

		vp = (struct variable *) POINTER(mp->o_variable,mp);

		for (n1=0; n1<mp->nvar; ++n1,++vp)
			{
			if (vp->ninit == 0)
				continue;

			name = (char *)POINTER(vp->name,mp);

			if ((sp=Ksym_name(name)) != NULL)
				/*
				 * data structure already allocated by kernel
				 */
				continue;

			if ((sp=Xsym_name(name))->flag & NOINIT)
				/*
				 * data structure was not allocated by alloc_variables()
				 */
				continue;

			locctr = REAL(sp->value, data_locctr);

			fp = (struct format *) POINTER(vp->initializer,mp);

			for (n2=0; n2<vp->ninit; ++n2,++fp)
				{

				if (fp->type & FSKIP)
					{
					locctr = ALIGN(locctr,int);
					locctr += fp->value;
					continue;
					}

				/*
				 * if a character string is being initialized, then
				 * it must be handled here;  eval() cannot be called
				 * since any character string initializer would be
				 * allocated in the data section with a value of 
				 * pointer-to-character
				 */
				if ((fp->type & FTYPE) == FSTRING)
					{
					char pname[PARAMNMSZ+1], *svalue;
					register char type;
					int nvalue;
					union element *xp;
					struct param *pp;

					/*
					 * the only initialization allowed for character
					 * strings is either numeric zero (null fill)
					 * or a character string literal; mkboot(1M)
					 * should have caught all errors except those
					 * caused by initialization by a parameter
					 * name located in another driver
					 */
					if (! (fp->type & FEXPR))
						/*
						 * numeric initialization
						 */
						{
						type = 'N';
						nvalue = fp->value;
						}
					else
						/*
						 * initialization by expression; we only
						 * allow expressions of type:
						 *
						 *	N : zero only
						 *	" : string
						 *	I : numeric or string parameter values
						 */
						{
						xp = (union element *) POINTER(fp->value,mp);

						switch (type = xp->operator)
							{
						case 'N':
							nvalue = eval(xp, dp);
							break;
						case '"':
							svalue = &xp->string[1];
							break;
						case 'I':
							if ((pp=searchparam(&xp->identifier[1])) != NULL)
								{
								if ((type=pp->type) == 'N')
									nvalue = pp->value.number;
								else
									svalue = * (char**) POINTER(pp->value.string,pp);
								break;
								}

							/* <dp->name>: data initializer <&xp->identifier[1]> unknown; zero assumed */
							error(ER30, dp->name, strncat(strcpy(pname,""),&xp->identifier[1],PARAMNMSZ));

							type = 'N';
							nvalue = 0;
							break;
						default:
							/* <dp->name>: illegal character string initialization; zero assumed */
							error(ER66, dp->name);

							type = 'N';
							nvalue = 0;
							break;
							}
						}

					if (type == 'N')
						{
						if (nvalue > 0)
							/* <dp->name>: illegal character string initialization; zero assumed */
							error(ER66, dp->name);

						strncpy((char*)locctr, "", (int)fp->strlen);
						}

					if (type == '"')
						{
						if (strlen(svalue) > (int)fp->strlen)
							/* %s: character string initializer truncated */
							error(ER67, dp->name);

						strncpy((char*)locctr, svalue, (int)fp->strlen);
                                               if ((mp->flag & FSTYP) && function(mp->prefix, "name", name))
                                                        Xrelocate((address)&vfsswtab[fscnt].vsw_name, R_DIR32, name, &data_rel);

                                               if ((mp->flag & SCHEDCLASS) && function(mp->prefix, "name", name))
                                 			Xrelocate((address)&classtab[clcnt].cl_name, R_DIR32, name, &data_rel);
                                               		clinitflag++;
						}

					locctr += fp->strlen;
					continue;
					}

				/*
				 * this is a simple numeric variable initialization
				 */
					{
					register long initial_value;

					if (fp->type & FEXPR){
						last_exp = (char *) NULL;
						initial_value = eval((union element *)POINTER(fp->value,mp), dp);
					}
					else
						initial_value = (short)fp->value;

					/*
					 * see eval case '&' to see the need for
					 * rel_spec4.
					 */
					switch (fp->type & FTYPE)
						{
					case FCHAR:
						SETDATA(locctr,char) = initial_value;
						if (last_exp != (char *) NULL)
						 	rel_spec4(locctr);
						locctr += sizeof(char);
						break;
					case FSHORT:
						locctr = ALIGN(locctr,short);
						SETDATA(locctr,short) = initial_value;
						if (last_exp != (char *) NULL)
						 	rel_spec4(locctr);
						locctr += sizeof(short);
						break;
					case FINT:
						locctr = ALIGN(locctr,int);
						SETDATA(locctr,int) = initial_value;
						if (last_exp != (char *) NULL)
						 	rel_spec4(locctr);
						locctr += sizeof(int);
						break;
					case FLONG:
						locctr = ALIGN(locctr,long);
						SETDATA(locctr,long) = initial_value;
						if (last_exp != (char *) NULL)
						 	rel_spec4(locctr);
						locctr += sizeof(long);
						break;
						}
					}
				}
			}
		}
                if ((mp->flag & SCHEDCLASS) && clinitflag == 0)
                        error(ER95, dp->name);
	}


/*
 * Loadriver(dp)
 *
 * Generate all driver variables, do relocation, and finish the 
 * cdevsw[] and bdevsw[] arrays.
 * The driver is represented by the dp->driver structure.
 *
 * The io_init[], io_start[], pwr_clr[], io_poll[], and io_halt[]
 * arrays are filled here.
 */
 void
loadriver(dp)
	register struct driver *dp;
	{

	register struct master *mp;
	register SYMENT *s;
	register SYMBOL *sp;
	long number;
	struct bdevsw *bdevp;
	struct cdevsw *cdevp;
	struct fmodsw *fmodp;

	mp = dp->opthdr;

	/*
	 * initialize:	bdevp -> correct entry in bdevsw[]
	 *		cdevp -> correct entry in cdevsw[]
	 *		fmodp -> next entry in fmodsw[]
	 *		vfsswp-> next entry in vfssw[]
	 */
	bdevp = & bdevswp[ dp->int_major ];
	cdevp = & cdevswp[ dp->int_major ];

	for (fmodp = fmodswp ; fmodp->f_name[0] != '\0' ; ++fmodp)
		continue;
		  
	if (mp->flag & FSTYP) {
		vfsswp++;
		Xrelocate((address)&vfsswp->vsw_init, R_DIR32, rtname[RNULL].symbol->name, &data_rel);
	}

	if (mp->flag & EXECTYP){
		Xrelocate((address)&(execswp->exec_magic),R_DIR32,rtname[RNULLMAG].symbol->name, &data_rel);
		Xrelocate((address)&(execswp->exec_func),R_DIR32,rtname[RNOEXEC].symbol->name, &data_rel);
		Xrelocate((address)&(execswp->exec_core),R_DIR32,rtname[RNOCORE].symbol->name, &data_rel);
	}

	/*
	 * set cdevsw[] and bdevsw[] elements to nulldev()/nodev()
	 */
	if (mp->flag & BLOCK)
		{
		register struct bdevsw *p = bdevp;

		Xrel_replace((address)&(p->d_open),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_close),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_strategy),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_print),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_size),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_xpoll),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_xhalt),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		}

	if (mp->flag & (CHAR | FUNDRV)) {
		register struct cdevsw *p = cdevp;

		Xrel_replace((address)&(p->d_open),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_close),R_DIR32,rtname[RNULL].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_read),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_write),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_ioctl),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_mmap),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_segmap),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_poll),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_xpoll),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
		Xrel_replace((address)&(p->d_xhalt),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
		if (mp->flag & FUNDRV) {
			char	name[sizeof(mp->prefix)+4];

			strcat(strcpy(name, mp->prefix), "info");
			ext_allocate(name);
			Xrelocate((address)&p->d_str, R_DIR32, name,&data_rel);
		}
	}

	if (mp->flag & FUNMOD) {
		char	name[sizeof(mp->prefix) + 4];

		lcase(strncpy(fmodp->f_name, dp->name,
			sizeof(fmodp->f_name) - 1));
		strcat(strcpy(name, mp->prefix), "info");
		ext_allocate(name);
		Xrelocate(&fmodp->f_str, R_DIR32, name,&data_rel);
	}

	/*
	 * allocate all of the driver specific variables
	 */
	alloc_variables(dp);


	/*
	 * read the symbol table and build the N_index[] and N_value[] arrays
	 */

	if (chdir(slash_boot) == -1)
		{
		error(ER77, slash_boot);
		goto exit;
		}

	if ((s=coff_symbol(dp->name,&number)) != NULL)
		/*
		 * there are some symbols
		 */
		do	{
			char *np;

			if (s->n_sclass == C_TPDEF  ||
			   s->n_sclass == C_MOS    ||
			   s->n_sclass == C_MOU) {
				continue;
			}

			if (s->n_zeroes)
				np = s->n_name;
			else
				np = s->n_name + s->n_offset;

			if (s->n_scnum !=0){
				if (s->n_sclass != C_STAT)
					sp = Xsym_name(np);
				else
					{
					if ((sp=Xsym_name(np))->flag & DEFINED || sp->value !=0){
						SYMBOL save, *np;
						save = *sp;
						sp->flag |= STATIC;
						np = Xsym_name(sp->name);
						*sp = save;
						sp = np;
						}
					sp->flag |= STATIC ;
					}
				sp->flag |= DRIVER | DEFINED;
		
			if (dp->s_index[s->n_scnum] == 0){
				register struct bdevsw *b = bdevp;
				register struct cdevsw *c = cdevp;

                                if ((mp->flag & FSTYP) == 0 && (mp->flag & SCHEDCLASS) == 0 && function(mp->prefix, "init", np)) {
					ext_allocate(np);
					Xrelocate(((address)next_init++ - (address)io_init),R_SPEC1,np,&data_rel);
					}

				if (function(mp->prefix,"start",np)){
					ext_allocate(np);
					Xrelocate(((address)next_start++ - (address)io_start),R_SPEC2,np,&data_rel);
					}
				if (function(mp->prefix,"clr",np)){
					ext_allocate(np);
					Xrelocate(((address)next_pwrclr++ - (address)pwr_clr),R_SPEC3,np,&data_rel);
					}
				if ((mp->flag & (BLOCK | CHAR)) && function(mp->prefix,"poll",np)){
					ext_allocate(np);
					Xrelocate(((address)next_poll++ - (address)io_poll),R_SPEC5,np,&data_rel);
					}
				if ((mp->flag & (BLOCK | CHAR)) && function(mp->prefix,"halt",np)){
					ext_allocate(np);
					Xrelocate(((address)next_halt++ - (address)io_halt),R_SPEC6,np,&data_rel);
					}

                                if ((mp->flag & EXECTYP) && function(mp->prefix, "exec", np)) {
                                        ext_allocate(np);
                                        Xrel_replace((address)&(execswp->exec_func), R_DIR32, np, &data_rel);
                         	}

                                if ((mp->flag & EXECTYP) && function(mp->prefix, "core", np)) {
                                        ext_allocate(np);
                                        Xrel_replace((address)&(execswp->exec_core), R_DIR32, np, &data_rel);
                         	}

                                if ((mp->flag & FSTYP) && function(mp->prefix, "init", np)) {
                                        ext_allocate(np);
                                        Xrel_replace((address)&vfsswp->vsw_init, R_DIR32, np, &data_rel);
                         	}

                                if ((mp->flag & SCHEDCLASS) && function(mp->prefix, "init", np)) {
                  			ext_allocate(np);
                                        Xrelocate((address)&classp->cl_init, R_DIR32, np, &data_rel);
                                        classp++;
                                        classcnt++;
                                }


					if ((mp->flag & NOTADRV) || ((mp->flag & (CHAR | BLOCK | FUNMOD | FUNDRV)) == FUNMOD))
						/*
						 * no more special names if not a driver
						 */
						continue;

					if (function(mp->prefix,"open",np)){
						if (mp->flag & BLOCK){
							ext_allocate(np);
							Xrel_replace((address)&(b->d_open),R_DIR32,np,&data_rel);
							}
						if (mp->flag & CHAR){
							if (!(mp->flag & BLOCK))
								ext_allocate(np);
							Xrel_replace((address)&(c->d_open),R_DIR32,np,&data_rel);
							}
						}
					if (function(mp->prefix,"close",np)){
						if (mp->flag & BLOCK){
							ext_allocate(np);
							Xrel_replace((address)&(b->d_close),R_DIR32,np,&data_rel);
							}
						if (mp->flag & CHAR){
							if (!(mp->flag & BLOCK))
								ext_allocate(np);
							Xrel_replace((address)&(c->d_close),R_DIR32,np,&data_rel);
							}
						}
					if (function(mp->prefix,"poll",np)){
						if (mp->flag & BLOCK){
							ext_allocate(np);
							Xrel_replace((address)&(b->d_xpoll),R_DIR32,np,&data_rel);
							}
						if (mp->flag & CHAR){
							if (!(mp->flag & BLOCK))
								ext_allocate(np);
							Xrel_replace((address)&(c->d_xpoll),R_DIR32,np,&data_rel);
							}
						}
					if (function(mp->prefix,"halt",np)){
						if (mp->flag & BLOCK){
							ext_allocate(np);
							Xrel_replace((address)&(b->d_xhalt),R_DIR32,np,&data_rel);
							}
						if (mp->flag & CHAR){
							if (!(mp->flag & BLOCK))
								ext_allocate(np);
							Xrel_replace((address)&(c->d_xhalt),R_DIR32,np,&data_rel);
							}
						}
					if (mp->flag & BLOCK)
						{
						if (function(mp->prefix,"strategy",np)){
							ext_allocate(np);
							Xrel_replace((address)&(b->d_strategy),R_DIR32,np,&data_rel);
							}
						if (function(mp->prefix,"print",np)){
							ext_allocate(np);
							Xrel_replace((address)&(b->d_print),R_DIR32,np,&data_rel);
							}
						if (function(mp->prefix,"size",np)){
							ext_allocate(np);
							Xrel_replace((address)&(b->d_size),R_DIR32,np,&data_rel);
							}
						}
					if (mp->flag & CHAR)
						{
						if (function(mp->prefix,"read",np)){
							ext_allocate(np);
							Xrel_replace((address)&(c->d_read),R_DIR32,np,&data_rel);
							}
						if (function(mp->prefix,"write",np)){
							ext_allocate(np);
							Xrel_replace((address)&(c->d_write),R_DIR32,np,&data_rel);
							}
						if (function(mp->prefix,"ioctl",np)){
							ext_allocate(np);
							Xrel_replace((address)&(c->d_ioctl),R_DIR32,np,&data_rel);
							}
						if (function(mp->prefix,"mmap",np)){
							ext_allocate(np);
							Xrel_replace((address)&(c->d_mmap),R_DIR32,np,&data_rel);
							}
						if (function(mp->prefix,"segmap",np)){
							ext_allocate(np);
							Xrel_replace((address)&(c->d_segmap),R_DIR32,np,&data_rel);
							}
						if (function(mp->prefix,"chpoll",np)){
							ext_allocate(np);
							Xrel_replace((address)&(c->d_poll),R_DIR32,np,&data_rel);
							}
						}
					}
					if ((dp->s_index[s->n_scnum] == 1)){ 
						if (function(mp->prefix,"devflag",np)){
							if (mp->flag & BLOCK){
                                        			ext_allocate(np);
                                        			Xrel_replace((address)&(bdevp->d_flag), R_DIR32, np, &data_rel);
							}
							if (mp->flag & (CHAR | FUNDRV)){
                                        			ext_allocate(np);
                                        			Xrel_replace((address)&(cdevp->d_flag), R_DIR32, np, &data_rel);
							}
							if (mp->flag & FUNMOD){
                                        			ext_allocate(np);
                                        			Xrel_replace((address)&(fmodp->f_flag), R_DIR32, np, &data_rel);
							}
						}
                                		if ((mp->flag & EXECTYP) && function(mp->prefix, "magic", np)) {
                                        		ext_allocate(np);
                                        		Xrel_replace((address)&(execswp->exec_magic), R_DIR32, np, &data_rel);
                         			}
							
					}
				}
				else 
					{
					if (s->n_value !=0){
						sp = Xsym_name(np);
						sp->flag |= BSS | DRIVER;
						sp->flag |= DEFINED;
						}
					else
						if ((sp=Ksym_name(np)) == NULL)
							sp = Xsym_name(np);
					}
			} while((s=coff_symbol((char *)NULL,&number)) != NULL);

	if (mp->flag & EXECTYP)  /* next execsw entry */
		execswp++;
	
	(void) chdir(cwd);

	return;


	/*
	 * Error exit
	 */
exit:	

	exit(1);
	}


/*
 * Generate(what, args...)
 *
 * Allocate the memory and define the symbol for a generated data structure
 *
 *
 *		      WHAT        #ARGS
 *		----------------  -----  ------------ARGUMENTS-------------
 *	G_TEXT	            text     3   char* name, long size, char* init
 *	G_DATA	initialized data     3   char* name, long size, char* init
 *	G_UDATA	    uninit. data     2   char* name, long size
 *	G_BSS	             bss     2   char* name, long size
 *	G_PCB	             pcb     4   char* name, char* entry, ipl, vector#
 *	G_IRTN	interupt routine     3   char* name, char* entry, minor
 *	G_IOSYS	   I/O subsystem     0
 */

/*
 * Template for interrupt routine interface
 */
static	char	template[] ={   
				0xa0, 0x5f, 0, 0,			/* PUSHW &0 */
				0x2c, 0xcc, 0xfc, 0x7f, 0, 0, 0, 0,	/* CALL  -4(%sp),0 */
				0x24, 0x7f, 0, 0, 0, 0 };		/* JMP   int_ret */

int		IRTNSIZE ={ (sizeof(template) + sizeof(int) -1) & ~(sizeof(int)-1) };

#define	PUSHW	0		/* offset in template[] to PUSHW instruction */
#define	CALL	4		/* offset in template[] to CALL instruction */
#define	JMP	12		/* offset in template[] to JMP instruction */



/*VARARGS1*/
 void
generate(va_alist)
	va_dcl
	{
	va_list ap;
	int what;
	union	what
			{
			struct	{		/* G_TEXT, G_DATA, G_UDATA, G_BSS */
				char	*name;
				long	size;
				char	*init;
				}
				db;

			struct	{		/* G_PCB, G_IRTN */
				char	*name;
				char	*entry;
				int	value;
				int	vec;
				}
				pi;
			} args;
	register union what *arg = &args;

	char name[9];

	va_start(ap);
	what = va_arg(ap, int);
	switch (what)
		{
		case G_TEXT:
		case G_DATA:
			arg->db.name = va_arg(ap, char *);
			arg->db.size = va_arg(ap, long);
			arg->db.init = va_arg(ap, char *);
			break;
		case G_UDATA:
			arg->db.name = va_arg(ap, char *);
			arg->db.size = va_arg(ap, long);
			break;
		case G_BSS:
		case G_IOSYS:
			break;
		case G_IRTN:
			arg->pi.name = va_arg(ap, char *);
			arg->pi.entry = va_arg(ap, char *);
			arg->pi.value = va_arg(ap, int);
			break;
		case G_PCB:
			arg->pi.name = va_arg(ap, char *);
			arg->pi.entry = va_arg(ap, char *);
			arg->pi.value = va_arg(ap, int);
			arg->pi.vec = va_arg(ap, int);
			break;
		}
	va_end(ap);
	switch (what)
		{
	/*
	 * TEXT
	 */
	case G_TEXT:
			{
			register char *to;

			to = (char*) REAL(allocate(&text_locctr.v_locctr,arg->db.name,arg->db.size,1), text_locctr);
			if (to > (char *)0)
				bcopy(arg->db.init, to, (unsigned)arg->db.size);
			}
		break;
	/*
	 * Unitialized DATA
	 */
	case G_UDATA:
			{

			/* 
			 * Note that io_.. and pwr_ are now generated as
			 * unitialized data, adj_rel adjust all the
		 	 * relocation entries of type RSPEC? now that an
			 * actual address location is known.
			 */
			if (strcmp(arg->db.name,"io_init") == 0)
				adj_rel(data_locctr.v_locctr,R_SPEC1);
			if (strcmp(arg->db.name,"io_start") == 0)
				adj_rel(data_locctr.v_locctr,R_SPEC2);
			if (strcmp(arg->db.name,"pwr_clr") == 0)
				adj_rel(data_locctr.v_locctr,R_SPEC3);
			if (strcmp(arg->db.name,"io_poll") == 0)
				adj_rel(data_locctr.v_locctr,R_SPEC5);
			if (strcmp(arg->db.name,"io_halt") == 0)
				adj_rel(data_locctr.v_locctr,R_SPEC6);
			(void)allocate(&data_locctr.v_locctr,arg->db.name,arg->db.size,2);

			}
		break;
	/*
	 * DATA
	 */
	case G_DATA:
			{
			register char *to;

			to = (char*) REAL(allocate(&data_locctr.v_locctr,arg->db.name,arg->db.size,2), data_locctr);

			if (to > (char *)0)
				bcopy(arg->db.init, to, (unsigned)arg->db.size);

			}
		break;
	/*
	 * BSS
	 */
	case G_BSS:
		break;
	/*
	 * I/O subSYStem: [cb]devcnt, [cb]devsw, MAJOR, MINOR
	 */
	case G_IOSYS:
			{
			register i, x;

			nkpcb = 0;
			generate(G_DATA, "cdevcnt", sizeof(int), &cdevcnt);
			generate(G_UDATA, "cdevsw", cdevcnt*sizeof(struct cdevsw));
			/* For backward compatibility */
			generate(G_UDATA, "shadowcsw", cdevcnt*sizeof(struct cdevsw));
			cdevswp = (struct cdevsw *) REAL(Xsym_name("cdevsw")->value, data_locctr);
			for (i=0; i<cdevcnt; ++i)
				{
				register struct cdevsw *p = &cdevswp[i];

				Xrelocate((address)&(p->d_open),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_close),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_read),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_write),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_ioctl),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_mmap),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_segmap),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_poll),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_xpoll),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_xhalt),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				/*p->d_ttys = NULL; */
				Xrelocate((address)&(p->d_flag),R_DIR32,rtname[RNODEVFLAG].symbol->name,&data_rel); /* Assume old-style driver.*/

				}

			generate(G_DATA, "bdevcnt", sizeof(int), &bdevcnt);
			generate(G_UDATA, "bdevsw", bdevcnt*sizeof(struct bdevsw));
			/* For backward compatibility */
			generate(G_UDATA, "shadowbsw", bdevcnt*sizeof(struct bdevsw));
			bdevswp = (struct bdevsw *) REAL(Xsym_name("bdevsw")->value, data_locctr);
			for (i=0; i<bdevcnt; ++i)
				{
				register struct bdevsw *p = &bdevswp[i];

				Xrelocate((address)&(p->d_open),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_close),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_strategy),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_print),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_size),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_xpoll),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_xhalt),R_DIR32,rtname[RNODEV].symbol->name,&data_rel);
				Xrelocate((address)&(p->d_flag),R_DIR32,rtname[RNODEVFLAG].symbol->name,&data_rel); /* Assume old-style driver.*/
				}

			generate(G_UDATA, "MAJOR", 256);
			MAJOR = (char*) REAL(Xsym_name("MAJOR")->value, data_locctr);
			x = max((long)bdevcnt, (long)cdevcnt);
			for (i=0; i<256; ++i)
				MAJOR[i] = (char)x;

			generate(G_UDATA, "MINOR", 256);
			MINOR = (char*) REAL(Xsym_name("MINOR")->value, data_locctr);

			generate(G_DATA, "fmodcnt", sizeof(int), &fmodcnt);
			generate(G_UDATA, "fmodsw",
				 fmodcnt * sizeof(struct fmodsw));
			fmodswp = (struct fmodsw *)REAL(Xsym_name("fmodsw")->value,
					data_locctr);

			for (i=0; i<fmodcnt; ++i)

				Xrelocate((address)&(fmodswp[i].f_flag),R_DIR32,rtname[RNODEVFLAG].symbol->name,&data_rel); /* Assume old-style driver.*/
			


                        generate(G_DATA,"nfstype",sizeof(int),&nfstype);
                        generate(G_UDATA,"vfssw",nfstype*sizeof(struct vfssw));
                        vfsswtab = (struct vfssw *)
                          	REAL(Xsym_name("vfssw")->value, data_locctr);
                        vfsswp = vfsswtab;

                        generate(G_DATA,"nexectype",sizeof(int),&nexectype);
                        generate(G_UDATA,"execsw",nexectype*sizeof(struct execsw));
                        execswp = (struct execsw *)
                          	REAL(Xsym_name("execsw")->value, data_locctr);
                        generate(G_UDATA, "class", classcnt * sizeof(struct class));
    			classtab = (struct class *)REAL(Xsym_name("class")->value,
                            data_locctr);
                        classp = classtab;
                        ext_allocate("sys_name");
                        Xrelocate((address)&(classp[0].cl_name), R_DIR32, "sys_name",
                            &data_rel);
                        ext_allocate("sys_init");
                        Xrelocate((address)&(classp[0].cl_init), R_DIR32, "sys_init",
                            &data_rel);
			classp++;


		}
		break;
	/*
	 * Interrupt RouTiNe
	 */
	case G_IRTN:
			{
			register char *text;

			generate(G_TEXT, arg->pi.name, IRTNSIZE, template);
			text = (char*) REAL(Xsym_name(arg->pi.name)->value, text_locctr);

			text[PUSHW+2] = arg->pi.value;
			
			ext_allocate(arg->pi.entry);
			Xrelocate((address)&text[CALL+4], R_DIR32S, arg->pi.entry,&text_rel);
			if (!(Xsym_name("int_ret")->flag & CONFLOCAL))
				ext_allocate("int_ret");
			Xrelocate((address)&text[JMP+2], R_DIR32S, "int_ret",&text_rel);
			}
		break;
	/*
	 * Process Control Block
	 */
	case G_PCB:
			{
			static psw_t psw = KPSW0;
			register struct kpcb *pcbp;
			register SYMBOL *xp;

			generate(G_UDATA, arg->pi.name, sizeof(struct kpcb));
			pcbp = (struct kpcb *) REAL(Xsym_name(arg->pi.name)->value, data_locctr);

			pcbp->ipcb.psw = psw;
			pcbp->ipcb.psw.IPL = arg->pi.value;
			Xrelocate((address) &(pcbp->ipcb.pc), R_DIR32, arg->pi.entry,&data_rel);

			strcpy(name, "kstk?");
			name[4] = "0123456789ABCDEF"[arg->pi.value];
			if ((xp=Ksym_name(name)) == NULL)
				{
				xp = Xsym_name(name);
				if (xp->flag & DEFINED){
					ext_allocate(name);
					Xrelocate(&pcbp->ipcb.sp,R_DIR32,name,&data_rel);
				}	 
				else
					{
					bss_allocate(name,(long)(ISTKSZ * sizeof(int)));
					Xrelocate((address)&pcbp->ipcb.sp,R_DIR32,name,&data_rel);
					}
				}
			else	{
				ext_allocate(name);
				Xrelocate(&pcbp->ipcb.sp,R_DIR32,name,&data_rel);
				}

			Xrelocate(&pcbp->slb,R_DIR32,name,&data_rel);
			Xrelocate(&pcbp->sub,R_DIR32,name,&data_rel);
			pcbp->sub += ISTKSZ;
			pcbp->movesize = 0;

			pcb_patch[nkpcb].suffix[0] = arg->pi.name[4];
			pcb_patch[nkpcb].suffix[1] = arg->pi.name[5];
			pcb_patch[nkpcb].vector = arg->pi.vec;
			nkpcb++;
			}
		break;
		}
	}
