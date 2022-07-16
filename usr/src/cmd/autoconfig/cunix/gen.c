/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/gen.c	1.27"
#include <sys/types.h>
#include <sys/localtypes.h>
#include <stdio.h>
#include <a.out.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/dir.h>
#include <dirent.h>
#include <sys/sys3b.h>
#include <sys/sbd.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/boothdr.h>
#include <ctype.h>
#include <sys/error.h>
#include <sys/ledt.h>
#include <sys/sym.h>
#include <sys/dproc.h>
#include <sys/machdep.h>
#include <sys/gen.h>
#include <sys/off.h>
#include <sys/fcntl.h>
#include <sys/cunix.h>
#include <vm/bootconf.h>
#include <libelf.h>


#define CONFIG_NAME	".config_header"

struct config_note {
	int namesz;
	int descsz;
	int type;
	char name[20];
};

int		cdevcnt;
int		bdevcnt;
int		fmodcnt;
int		nfstype;
int		nexectype;
int		classcnt;

address maxmem;
static void check_ifile();
static void ckconferror();
void elf_readboot();

struct rtname	 rtname[]		/* functions defined in the UNIX kernel
*/
                                ={	/*	these must be in one-to-one correspondence with RNULL,RNOSYS,... */
                     /* [RNULL] */	{ "nulldev", NULL },
                     /* [RNOSYS] */	{ "nosys", NULL },
                     /* [RNODEV] */	{ "nodev", NULL },
                     /* [RTRUE] */	{ "rtrue", NULL },
                     /* [RFALSE] */	{ "rfalse", NULL },
                     /* [RFSNULL]*/	{ "fsnull",NULL},
                     /* [RFSTRAY]*/	{ "fsstray",NULL},
                     /* [NOPKG]*/	{ "nopkg",NULL},
                     /* [NOREACH]*/	{ "noreach",NULL},
                     /* [RNULLMAG]*/	{ "nullmagic",NULL},
                     /* [RNOEXEC]*/	{ "noexec",NULL},
                     /* [RNOCORE]*/	{ "nocore",NULL},
		     /* [RNODEVFLAG]*/  { "nodevflag",NULL},
                                        0
                                 };

char *lsp,*last_exp;		/* lsp used to build load_string */
                                /* last_exp is used to patch up comunication
                                   problems between eval and initdata */
char *load_string;

extern int errno;

boolean routine();
void    print_configuration();


struct	check
	{
	struct driver	*dp;
	struct param	*pp;
	};

/*
 * checkcmp(cp1, cp2)
 * 
 * Compare the parameter values of two check structures;  return TRUE if the
 * values are equal and return FALSE if the values are not equal
 */
 static
 boolean
checkcmp(cp1, cp2)
	register struct check *cp1, *cp2;
	{

	register struct param *pp1, *pp2;

	if ((pp1=cp1->pp)->type == (pp2=cp2->pp)->type)
		/*
		 * types are the same, the actual values must be compared
		 */
		switch (pp1->type)
			{
		case 'N':
			return(pp1->value.number == pp2->value.number);
		case '"':
			return(strcmp(POINTER(pp1->value.string,cp1->dp->opthdr),
					POINTER(pp2->value.string,cp2->dp->opthdr)) == 0);
			}

	/*
	 * types are not the same (values cannot be the same), or the
	 * values did not compare equal
	 */
	return(FALSE);
	}

/*
 * Check_param()
 *
 * Check all of the tunable parameter names for possibly multiply defined
 * values.  If any multiply defined values are found, issue an error message
 * and take steps to recover.
 */


 static
 void
check_param()
	{

	register struct check *check, *cp, *Scp;
	register int nparam;
	boolean setzero;

	/*
	 * count the total number of parameters
	 */
	nparam = 0;

		{
		register struct driver *dp = (struct driver*) kernel;

		do	{
			nparam += dp->opthdr->nparam;
			}
			while (dp = dp->next);
		}

	/*
	 * allocate the check[] array
	 */
	if ((check=(struct check*)malloc(nparam*sizeof(struct check))) == NULL)
		panic("No memory for parameter checking\n");

	/*
	 * initialize the check[] array
	 */
		{
		register struct driver *dp = (struct driver*) kernel;
		register i;

		cp = check;

		do	{
			register struct master *mp = dp->opthdr;

			for (i=0; i<mp->nparam; ++i,++cp)
				{
				cp->dp = dp;
				cp->pp = &((struct param*) POINTER(mp->o_param,mp))[i];
				}
			}
			while (dp = dp->next);
		}

	/*
	 * sort the check[] array into alphabetical order by parameter name
	 */
		{
		register i, m;

		for (m=nparam/2; m>0; m/=2)
			{
			for (i=0; i<nparam-m; ++i)
				{
				cp = &check[i];

				if (strncmp(cp[0].pp->name,cp[m].pp->name,PARAMNMSZ) > 0)
					{
					struct check temp;

					temp = cp[m];

					do	{
						cp[m] = cp[0];
						}
						while ((cp-=m)>=check && strncmp(cp[0].pp->name,temp.pp->name,PARAMNMSZ)>0);

					cp[m] = temp;
					}
				}
			}
		}

	/*
	 * now search the check[] array for duplicate parameter names which
	 * have different values
	 */
	Scp = cp = check;

	while (cp < &check[nparam-1])
		{
		char name[PARAMNMSZ+1];
		register struct param *pp1;
		register struct check *last, *Icp;
		register struct driver *dp;

		if (strncmp((pp1=cp[0].pp)->name,cp[1].pp->name,PARAMNMSZ) != 0)
			{
			Scp = ++cp;
			continue;
			}

		/*
		 * a duplicate parameter name; compare the values
		 */
		if (checkcmp(&cp[0],&cp[1]))
			{
			++cp;
			continue;
			}

		/*
		 * values are not equal; Parameter <pp1->name> multiply defined
		 */
		error(ER70, strncat(strcpy(name,""),pp1->name,PARAMNMSZ));

		/*
		 * we have a situation where the parameter has been
		 * assigned different values, therefore we must attempt
		 * to resolve the conflict
		 *
		 * let the set of all duplicated parameters be {S}; then
		 *
		 * 	{S} = {I} + {X}
		 *
		 * where {I} is the subset of {S} of all parameters defined
		 * by drivers that are not excluded, and {X} is the subset
		 * of {S} of all parameters defined by drivers that are
		 * excluded
		 *
		 * step 1)
		 *	all parameters in {X} will be totally ignored
		 *
		 * step 2)
		 *	if the remaining parameters in {I} all have the
		 *	same value, then the conflict is resolved -- the
		 *	boot will proceed using this parameter value
		 *
		 * step 3)
		 *	since the remaining parameters in {I} still contain
		 *	the conflict, it is impossible to choose which
		 *	value is to be used; therefore, all of the values
		 *	are set to numeric zero
		 *
		 * unfortunately, the result of step 3 will probably be
		 * to cause a kernel panic or malfunction which would
		 * preclude any attempt to correct the problem in the
		 * /boot directory
		 *
		 * steps 1 and 2 provide the means to recover; diagnostic
		 * messages are printed which identify the offending
		 * drivers and the parameter values, therefore the
		 * operator need only re-boot and manually exclude the
		 * offending driver(s)
		 */

		Icp = NULL;

		/*
		 * we will only set parameter values to zero if there is
		 * a multiply defined parameter in {I}
		 */
		setzero = FALSE;

		for (last=Scp; last<&check[nparam] && strncmp(name,last[0].pp->name,PARAMNMSZ)==0; ++last)
			{
			if (! (last->dp->flag & EXCLUDE))
				/*
				 * we have a member of set {I}
				 */
				{
				if (Icp == NULL)
					/*
					 * remember the first member of {I}
					 */
					Icp = last;
				else
					/*
					 * compare the parameter value of all members of {I}
					 */
					if (! checkcmp(Icp,last))
						/*
						 * a multiply defined parameter in {I}
						 */
						setzero = TRUE;
				}
			}

		/*
		 * go through {S} and
		 *
		 *	a) print a diagnostic
		 *	b) ignore parameters from excluded drivers
		 *	c) set parameter values to zero if setzero==TRUE
		 */
		for (cp=Scp; cp < last; ++cp)
			{
			/*
			 * there are six unique messages that may be issued
			 * depending on:
			 *
			 *	parameter value - numeric or string
			 *	driver status - EXCLUDED or not
			 *	conflict resolved - yes or no (set zero)
			 *
			 * "driver status" is mutually exclusive with "conflict
			 * resolved", therefore there are only six permutations
			 * rather than eight
			 *
			 * the message is selected by using the low order three
			 * bits of `m' to encode a condition:
			 *
			 *	+----------------+
			 *	|       N  X  Z  |
			 *	+------ |  |  | -+
			 *	        |  |  +---- set zero
			 *	        |  +------- EXCLUDED
			 *	        +---------- value numeric
			 */
			static short message[8] ={ ER74, ER76, ER75, 0, ER71, ER73, ER72, 0 };
			int m = 0;
			int number;
			char *string;

			dp = cp[0].dp;

			if ((pp1=cp[0].pp)->type == 'N')
				{
				number = pp1->value.number;
				m |= 04;
				}
			else
				string = POINTER(pp1->value.string,dp->opthdr);

			if (dp->flag & EXCLUDE)
				{
				m |= 02;
				pp1->name[0] = '\0';			/* it cannot be found again */
				}
			else
				if (setzero)
					{
					m |= 01;
					pp1->type = 'N';
					pp1->value.number = 0;
					}

			switch (m = message[m])
				{
			case ER71:
				/* <driver>: <parameter> = <n> */
			case ER72:
				/* <driver>: <parameter> = <n> (<driver EXCLUDED, parameter ignored) */
			case ER73:
				/* <driver>: <parameter> = <n> (set to zero) */
				error(m, dp->name, name, number, dp->name);
				break;
			case ER74:
				/* <driver>: <parameter> = "<s>" */
			case ER75:
				/* <driver>: <parameter> = "<s>" (<driver EXCLUDED, parameter ignored) */
			case ER76:
				/* <driver>: <parameter> = "<s>" (set to zero) */
				error(m, dp->name, name, string, dp->name);
				break;
				}
			}

		Scp = last;
		}

	free(check);
	}

/*
 * searchparam(name)
 *
 * Search the kernel and driver linked-list to locate a parameter `name'; when
 * found, return the pointer to a static struct param: if the value is a string,
 * then the string offset is modified to an offset from the struct param which
 * points to a character pointer which points to the actual string:
 *
 *		if (p->type == '"')
 *			*(char**)POINTER(p->value.string,p) -> the string
 *
#ifdef u3b15
 * If the parameter name is not found return NULL.
#endif
#ifdef u3b2
 * If the parameter name is not found, return NULL unless the parameter name
 * is "NBUF".  In this case, return a value which is derived from the size of
 * memory.
#endif
 */
 struct param *
searchparam(name)
	register char *name;
	{

	register struct driver *dp;
	register struct master *mp;
	register struct param *pp;
	register i;
	static struct
		{
		struct param	param;
		char		*string;
		}
		pentry;

#ifdef FALCON

#define	K		1024
#define	M		K*K

#define	DIM(array)	(sizeof(array)/sizeof(array[0]))

#define	EPORT_ID       0x102
#define	PORT_ID        0x003

	struct tune_vs_memory 
	{   
		int	nbuf;	/* number of buffers */
		int	ninode;
		int	ns5inode;
		int	nfile;
		int	nproc;
		int	nregion;
		int	nhbuf;
		int	nautoup;
		address	memory;	/* upper bound on memory */
	};

	static struct tune_vs_memory t_vs_m[] ={
	{   50,  300,  300,  300, 120,  210,  256, 10, MAINSTORE + M/2  },
	{  100,  300,  300,  300, 120,  210,  256, 10, MAINSTORE + M    },
	{  250,  300,  300,  300, 120,  210,  256, 10, MAINSTORE + 2*M  },
	{  400,  300,  300,  300, 120,  210,  256, 10, MAINSTORE + 3*M  },
	{  600,  400,  400,  400, 120,  360,  256, 15, MAINSTORE + 4*M  },
	{  800,  500,  500,  500, 150,  450,  256, 15, MAINSTORE + 6*M  },
	{ 1000,  600,  600,  600, 200,  600,  512, 30, MAINSTORE + 8*M  },
	{ 1200,  800,  800,  800, 240,  720,  512, 30, MAINSTORE + 10*M },
	{ 1400, 1000, 1000, 1000, 300,  900, 1024, 45, MAINSTORE + 12*M },
	{ 1600, 1100, 1100, 1100, 350, 1050, 1024, 45, MAINSTORE + 14*M },
	{ 2200, 1300, 1300, 1300, 400, 1200, 1024, 45, MAINSTORE + 16*M },
       };

	/*  Needed to look through the EDT and determine how many 
	 *  PORTS and EPORTS boards on the system.
	 *  From that we auto tune NCLIST if not defined in the
	 *  kernel master file.
         */
	int nep, npo;

#endif
	dp = (struct driver *) kernel;

	do	{
		mp = dp->opthdr;

		pp = (struct param *) POINTER(mp->o_param,mp);

		for (i=0; i<mp->nparam; ++i,++pp)
			if (0 == strncmp(pp->name,name,PARAMNMSZ))
				{
				pentry.param = *pp;

				if (pp->type == '"')
					{
					pentry.string = (char*) POINTER(pp->value.string,mp);
					pentry.param.value.string = OFFSET(&pentry.string, &pentry.param);
					}

				return(&pentry.param);
				}
		}
		while (dp = dp->next);

#ifdef u3b2
#ifndef FALCON

#define	K		1024
#define	M		K*K

#define	DIM(array)	(sizeof(array)/sizeof(array[0]))

	if (0 == strcmp(name,"NBUF"))
		/*
		 * the number of buffers depends upon the size of memory for
		 * the 3B2
		 */
		{

		/*
		 * use "nbuf" buffers if the highest address of system memory
		 * is at least "memory"
		 */
		struct nbuf_vs_memory
			{
			int	nbuf;	/* number of buffers */
			address	memory;	/* upper bound on memory */
			};

		static struct nbuf_vs_memory n_vs_m[] ={
							{  50, MAINSTORE+  M/2 },
							{ 100, MAINSTORE+  M   },
							{ 250, MAINSTORE+2*M   },
							{ 400, MAINSTORE+3*M   },
							{ 600, MAINSTORE+4*M   },
						       };


		for (i=0; ; ++i)
			switch (i)
				{
			default:
				if (maxmem <= n_vs_m[i].memory)
					{
			case DIM(n_vs_m)-1:
					pentry.param.type = 'N';
					pentry.param.value.number = n_vs_m[i].nbuf;
					return(&pentry.param);
					}
				}
		}


#else  /* FALCON */

	if (0 == strncmp(name,"NBUF",PARAMNMSZ))
	{
		for (i=0; ; ++i)
			switch (i)
			{
			default:
				if (maxmem <= t_vs_m[i].memory)
					{
			case DIM(t_vs_m)-1:
					pentry.param.type = 'N';
					pentry.param.value.number = t_vs_m[i].nbuf;
					return(&pentry.param);
					}
			}
	}


	if (0 == strncmp(name,"NINODE",PARAMNMSZ))
	{
		for (i=0; ; ++i)
			switch (i)
			{
			default:
				if (maxmem <= t_vs_m[i].memory)
					{
			case DIM(t_vs_m)-1:
					pentry.param.type = 'N';
					pentry.param.value.number = t_vs_m[i].ninode;
					return(&pentry.param);
					}
			}
	}
	if (0 == strncmp(name,"NS5INODE",PARAMNMSZ))
	{
		for (i=0; ; ++i)
			switch (i)
			{
			default:
				if (maxmem <= t_vs_m[i].memory)
					{
			case DIM(t_vs_m)-1:
					pentry.param.type = 'N';
					pentry.param.value.number = t_vs_m[i].ns5inode;
					return(&pentry.param);
					}
			}
	}
	if (0 == strncmp(name,"NFILE",PARAMNMSZ))
	{
		for (i=0; ; ++i)
			switch (i)
			{
			default:
				if (maxmem <= t_vs_m[i].memory)
					{
			case DIM(t_vs_m)-1:
					pentry.param.type = 'N';
					pentry.param.value.number = t_vs_m[i].nfile;
					return(&pentry.param);
					}
			}
	}
	if (0 == strncmp(name,"NPROC",PARAMNMSZ))
	{
		for (i=0; ; ++i)
			switch (i)
			{
			default:
				if (maxmem <= t_vs_m[i].memory)
					{
			case DIM(t_vs_m)-1:
					pentry.param.type = 'N';
					pentry.param.value.number = t_vs_m[i].nproc;
					return(&pentry.param);
					}
			}
	}
	if (0 == strncmp(name,"NREGION",PARAMNMSZ))
	{
		for (i=0; ; ++i)
			switch (i)
			{
			default:
				if (maxmem <= t_vs_m[i].memory)
					{
			case DIM(t_vs_m)-1:
					pentry.param.type = 'N';
					pentry.param.value.number = t_vs_m[i].nregion;
					return(&pentry.param);
					}
			}
	}
	if (0 == strncmp(name,"NHBUF",PARAMNMSZ))
	{
		for (i=0; ; ++i)
			switch (i)
			{
			default:
				if (maxmem <= t_vs_m[i].memory)
					{
			case DIM(t_vs_m)-1:
					pentry.param.type = 'N';
					pentry.param.value.number = t_vs_m[i].nhbuf;
					return(&pentry.param);
					}
			}
	}
	if (0 == strncmp(name,"NAUTOUP",PARAMNMSZ))
	{
		for (i=0; ; ++i)
			switch (i)
			{
			default:
				if (maxmem <= t_vs_m[i].memory)
					{
			case DIM(t_vs_m)-1:
					pentry.param.type = 'N';
					    pentry.param.value.number = t_vs_m[i].nautoup;
					return(&pentry.param);
					}
			}
	}
	if (0 == strncmp(name,"NCLIST",PARAMNMSZ))
	{
	    nep = escan(EPORT_ID);
	    npo = escan(PORT_ID);
	    /* for (i=0, nep=0, npo=0, edtp=EDT_START; i < NUM_EDT; ++i)
	    {   
		if (edtp->opt_code == EPORT_ID)
		    nep++;
		if (edtp->opt_code == PORT_ID)
		    npo++;
		edtp++;
	    }*/
	    /*  NCLIST is determined by 144 * the number of EPORTS 
	     *  80 * the number of PORTS and 28 which is needed for
	     *  the console and contty.
	     */
	    pentry.param.type = 'N';
	    pentry.param.value.number = (28 + (144 * nep) + (80 * npo));
	    return(&pentry.param);
	}

#endif  /* FALCON */
#endif  /* u3b2 */

	return(NULL);
	}


/*
 * searchdriver(name)
 *
 * Search the driver linked-list to locate driver `name'; when found, return
 * the pointer to the struct driver entry; if not found return NULL.
 */
 struct driver *
searchdriver(name)
	char *name;
	{

	register struct driver *dp;

	if ((dp=driver) != NULL)
		do	{
			if (0 == strcmp(dp->name,name))
				return(dp);
			}
			while (dp = dp->next);

	return(NULL);
	}

/*
 * Include(dname, number)
 *
 * Mark driver `dname' to be included for `number' more controllers.
 *
 * If the driver was found in the EDT, then `number' is ignored since the
 * number of controllers is already determined.  Likewise, if the driver
 * is a required driver, then the number of controllers must be set to one,
 * so `number' is ignored.
 *
 * If the driver was not found in the EDT, then it cannot be included
 * unless it is a software driver, or it is just an independent module.
 */
 void
include(dname, number)
	register char *dname;
	int number;
	{

	register struct driver *dp;
	register struct master *mp;

	if ((dp=searchdriver(dname)) == NULL)
		/* INCLUDE: <dname>; driver not found */
		error(ER18, dname);
	else
		{
		if (dp->flag & EXCLUDE)
			{
			/* INCLUDE: <dname>; driver is EXCLUDED */
			error(ER19, dname);
			return;
			}

		if (dp->flag & INEDT)
			/*
			 * it will be included based on number of times in EDT
			 */
			{
			dp->flag |= INCLUDE;
			return;
			}

		mp = dp->opthdr;
		if (mp->flag & (NOTADRV | FUNMOD))
			{
			dp->flag |= INCLUDE;
			dp->nctl += number;
			return;
			}

		if (! (mp->flag & SOFT))
			/* INCLUDE: <dname>; device not equipped */
			error(ER20, dname);
		else
			{
			dp->flag |= INCLUDE;

			if (! (mp->flag & REQ))
				dp->nctl += number;
			}
		}
	}


/*
 * Ignore(name)
 *
 * Remember this EDT device name so that ignoredt() can respond with the correct
 * answer.  This routine is called when the /etc/system EXCLUDE lines are being
 * processed and there is no driver in the driver linked-list for this name.
 *
 * The EXCLUDE lines were not available at the time the driver linked-list was
 * being built, so EDT entries were just bypassed if there was no corresponding
 * driver found in /boot -- it could not be determined whether a driver was
 * missing or the EDT entry was to be ignored.
 *
 * Later, the EDT will be scanned again, and ignoredt() called in order to catch
 * the EDT entries to be ignored.  Diagnostics will then be issued for missing
 * drivers.
 */

struct	ignore
	{
	struct ignore	*next;
	char		name[DIRSIZ+1];
	};

static struct ignore *edtlist;

 void
ignore(name)
	char *name;
	{

	register struct ignore *ip;

	if ((ip=(struct ignore*)malloc(sizeof(*ip))) == NULL)
		panic("No memory for EXCLUDE list");

	ip->next = edtlist;
	strncat(strcpy(ip->name,""), name, sizeof(ip->name));

	edtlist = ip;
	}

/*
 * Exclude(dname)
 *
 * Mark driver `dname' to be excluded or ignored
 */
 void
exclude(dname)
	register char *dname;
	{

	register struct driver *dp;

	if ((dp=searchdriver(dname)) == NULL)
		/*
		 * this may be an EDT entry that is to be ignored
		 */
		ignore(dname);
	else
		{
		if (dp->flag & INCLUDE)
			/* EXCLUDE: <dname>; driver is INCLUDED */
			error(ER21, dname);
		else
			dp->flag |= EXCLUDE;
		}
	}


/*
 * ignoredt(name)
 *
 * Answer the question: should this entry from the EDT be ignored?
 * Return TRUE if so, FALSE if it is not to be ignored.
 */
 
 boolean
ignoredt(name)
	char *name;
	{

	register struct ignore *ip;

	for (ip=edtlist; ip!=NULL; ip=ip->next)
		if (0 == strcmp(ip->name,name))
			return(TRUE);

	return(FALSE);
	}

/*
 * Dependency(pdriver)
 *
 * Driver *pdriver has dependencies.  Find them in the driver linked-list, mark
 * them to be loaded, and follow their dependencies.
 */
 static
 void
dependency(pdriver)
	register struct driver *pdriver;
	{

	register struct driver *dp;
	register struct master *m, *mp = pdriver->opthdr;
	register struct depend *dep;
	register count;
	char *name;

	dep = (struct depend*) POINTER(mp->o_depend, mp);

	for (count=0; count<mp->ndep; ++count, ++dep)
		{

		if ((dp=searchdriver(name=(char*)POINTER(dep->name,mp))) == NULL)
			{
			/* <pdriver->name>: dependent driver <name> not available */
			error(ER22, pdriver->name, name);
			continue;
			}

		if (dp->flag & LOAD)
			/*
			 * already marked to be loaded
			 */
			continue;

		if (dp->flag & EXCLUDE)
			/*
			 * hey! this driver was excluded
			 */
			{
			/* <pdriver->name>: dependent driver <name> is EXCLUDED */
			error(ER23, pdriver->name, name);
			continue;
			}

		m = dp->opthdr;
		if (!((m->flag & (SOFT|NOTADRV)) ||
		    ((m->flag & (CHAR | BLOCK | FUNMOD | FUNDRV)) == FUNMOD)) &&
		    !(dp->flag & (INEDT|ISEBUS)))

			/*
			 * driver is not a software driver (ie. it is a
			 * hardware driver) but the hardware does not exist
			 */
			{
			/* <pdriver->name>: device not equipped for dependent driver <name> */
			error(ER24, pdriver->name, name);
			continue;
			}

		dp->flag |= LOAD;

		if (m->ndep > 0)
			/*
			 * follow the dependency chain
			 */
			dependency(dp);

		if ((m->flag & (SOFT|NOTADRV) ||
		     ((m->flag & (CHAR | BLOCK | FUNMOD | FUNDRV)) == FUNMOD)) && 
		    ! (dp->flag & (INEDT|ISEBUS)) && dp->nctl == 0)
			/*
			 * make sure that #C is set if not done already
			 */
			dp->nctl = 1;
		}
	}

/*
 * eval(expression, dp)
 *
 * Evaluate a prefix polish expression string.  The `expression' must be a
 * pointer to the beginning of the string.  The current driver structure
 * element is pointed to by `dp'.
 * see case '&' for user level changes.
 */
 long
eval(expression, dp)
	register union element *expression;
	register struct driver *dp;
	{

	register SYMBOL *sp;
	register long temp;
	int i,j;

	static union element *xp;

	if (expression == NULL)
		expression = xp;

	switch (expression->operator)
		{
	/*
	 * binary arithmetic operators
	 */
	case '+':
		xp = XBUMP(expression,operator);
		temp = eval((union element *)NULL, dp);
		return(temp + eval((union element *)NULL,dp));
	case '-':
		xp = XBUMP(expression,operator);
		temp = eval((union element *)NULL, dp);
		return(temp - eval((union element *)NULL,dp));
	case '*':
		xp = XBUMP(expression,operator);
		temp = eval((union element *)NULL, dp);
		return(temp * eval((union element *)NULL,dp));
	case '/':
		/*
		 * note: division by zero is allowed, and result is zero
		 */
		xp = XBUMP(expression,operator);
		temp = eval((union element *)NULL, dp);

			{
			register long divisor = eval((union element *)NULL, dp);

			if (divisor == 0)
				return(0);
			else
				return(temp / divisor);
			}
	/*
	 * leaf - builtin fuctions: min, max
	 */
	case '<':
		xp = XBUMP(expression,function);
		temp = eval((union element *)NULL, dp);
		return(min(temp,eval((union element *)NULL,dp)));
	case '>':
		xp = XBUMP(expression,function);
		temp = eval((union element *)NULL,dp);
		return(max(temp,eval((union element *)NULL,dp)));
	/*
	 * leaf - builtin variables
	 */
	case 'C':
		xp = XBUMP(expression,nC);
		if (expression->nC[1] == '\0')
			return(dp->nctl);
		else
			{
			register struct driver *d;
			if ((d=searchdriver(&expression->nC[1])) != NULL)
				return(d->nctl);

			/* <dp->name>: data initializer #C(<&expression->nC[1]>) unknown; zero assumed */
			error(ER25, dp->name, &expression->nC[1]);
			return(0);
			}
	case 'S':
                xp = XBUMP(expression,nS);
		if (expression->nS[1] == '\0')
                        return(dp->ntc_lu);
		else
                    	{
                        register struct driver *d;
                        if ((d=searchdriver(&expression->nS[1])) != NULL)
                                return(d->ntc_lu);

                        /* <dp->name>: data initializer #S(<&expression->nS[1]>) unknown; zero assumed */
                        error(ER90, dp->name, &expression->nS[1]);
                        return(0);
                        }

	case 'D':
		xp = XBUMP(expression,nD);
		if (expression->nD[1] == '\0')
			return(dp->opthdr->ndev);
		else
			{
			register struct driver *d;
			if ((d=searchdriver(&expression->nD[1])) != NULL)
				return(d->opthdr->ndev);

			/* <dp->name>: data initializer #D(<&expression->nD[1>) unknown; zero assumed */
			error(ER26, dp->name, &expression->nD[1]);
			return(0);
			}
	case 'M':
		xp = XBUMP(expression,nM);
		if (expression->nM[1] == '\0')
			return(dp->int_major);
		else
			{
			register struct driver *d;
			if ((d=searchdriver(&expression->nM[1])) != NULL)
				return(d->int_major);

			/* <dp->name>: data initializer #M(<&expression->nM[1>) unknown; zero assumed */
			error(ER27, dp->name, &expression->nM[1]);
			return(0);
			}
	/*
	 * leaf - address of
	 * User level comment - basically all we do here is que a relocation
	 * entry. See Xrelocate for tricky conditions. Also have to handle
	 * stubbed out entries. last_exp used so that later in initdata
	 * when we finally have an address in conf.o to bind the symbol
	 * we can run through the relocation table for RSPEC4 symbols and
	 * adjust the vaddr field. See init_data and rel_spec4 .
	 */
	case '&':
			if ((sp=Ksym_name(&expression->address_of[1])) == NULL)
				sp = Xsym_name(&expression->address_of[1]);

			if ((sp->flag & DEFINED)){
				if (sp->flag & CONFLOCAL){
					i = sp->nsymindex;
					if ((j = nsym.nsyment[i].n_scnum) != 0)
						if (j == 1)
	
							data_rel.rp[data_rel.count].r_symndx = text_symndx;
						else
							data_rel.rp[data_rel.count].r_symndx = data_symndx;
					else
						data_rel.rp[data_rel.count].r_symndx = i;
				}
				else {
					ext_allocate(&expression->address_of[1]);
					data_rel.rp[data_rel.count].r_symndx = searchnsym(&expression->address_of[1]);
			     	}
				data_rel.rp[data_rel.count++].r_type = R_SPEC4;
			}
			else
				if (! routine(&expression->address_of[1],1))
					{
					/* <dp->name>: data initializer &<&expression->address_of[1])> cannot be resolved*/
					error(ER28, dp->name, &expression->address_of[1]);
	
					exit(1);
					}
			last_exp = &expression->address_of[1];
			xp = XBUMP(expression,address_of);
			return(0);
	/*
	 * leaf - size of
	 */
	case '#':
			{
			struct driver *d;

			d = (struct driver*)kernel;
			do	{
				register struct master *mp = d->opthdr;
				register struct variable *vp;

				vp = (struct variable *) POINTER(mp->o_variable,mp);
				for (temp=0; temp<mp->nvar; ++temp, ++vp)
					{
					if (0 == strcmp(&expression->size_of[1],(char *)POINTER(vp->name,mp)))
						{
						xp = XBUMP(expression,size_of);
						return(vp->size);
						}
					}
				}
				while (d = d->next);

			/* <dp->name>: data initializer #<&expression->size_of[1> unknown; zero assumed */
			error(ER29, dp->name, &expression->size_of[1]);

			xp = XBUMP(expression,size_of);
			return(0);
			}
	/*
	 * leaf - string
	 */
	 case '"':
		xp = XBUMP(expression,string);
		return(alloc_string(&expression->string[1]));
	/*
	 * leaf - identifier
	 */
	case 'I':
		{
		register struct param *pp;
		char name[PARAMNMSZ+1];

		xp = XBUMP(expression,identifier);

		if ((pp=searchparam(&expression->identifier[1])) != NULL)
			{
			if (pp->type == '"')
				return(alloc_string(*(char**)POINTER(pp->value.string,pp)));
			else
				return(pp->value.number);
			}

		/* <dp->name>: data initializer <&expression->identifier[1]> unknown; zero assumed */
		error(ER30, dp->name, strncat(strcpy(name,""),&expression->identifier[1],PARAMNMSZ));
		return(0);
		}
	/*
	 * leaf - literal value
	 */
	case 'N':
		temp = ((expression->literal[1]<<8 | expression->literal[2])<<8 | expression->literal[3])<<8 | expression->literal[4];
		xp = XBUMP(expression,literal);
		return(temp);
	default:
		/* Undefined expression element */
		error(ER3);
		}
	/*NOTREACHED*/
	}

/*
 * findrivers()
 *
 * Search the /boot directory, and the EDT and build the driver linked-list; if
 * the driver linked-list cannot be built, set-up a dummy linked-list with:
 *
 *		driver == &driver
 *
 * which can be tested for by:
 *
 *		driver->next == driver
 */
 static
 void
findrivers()
{
	register DIR *dirp;
	register struct dirent *dentp;
	register i;
	struct driver *dp;
	int fd, count;
	struct stat statbuf;
	FILHDR fhdr;
	SCNHDR shdr;

	char sections;

#define	TFOUND	0x08		/* .text section found */
#define	DFOUND	0x04		/* .data section found */
#define	BFOUND	0x02		/* .bss section found */




	/*
	 * build the driver linked-list and read the optional headers for each driver
	 */
	loader_type = O_COFF;		/* assume COFF */


	if ((dirp = opendir(slash_boot)) == NULL){
		error(ER80,slash_boot);
		goto fail2;
		}

	if (chdir(slash_boot) == -1){
		error(ER77,slash_boot);
		goto fail2;
		} 

	count = 0;


	while ((dentp = readdir(dirp)) != NULL){
		register struct master *mp;


		if (stat(dentp->d_name,&statbuf) == -1)
			{
			error(ER80, dentp->d_name);
			continue;
			}

		if (statbuf.st_mode & (S_IFDIR|S_IFCHR|S_IFBLK|S_IFIFO))
			continue;

		if ((fd=open(dentp->d_name,O_RDONLY)) == -1)
			{
			error(ER80, dentp->d_name);
			continue;
			}

		dp = NULL;

		switch (object_file_type(fd)){
		case O_ELF:
			switch (elf_findrivers (fd, &count, dentp->d_name)) {
			case -3:
				error(ER15, dentp->d_name);
				goto badriver;
			case -2:	/* No memory */
				close (fd);
				goto fail1;
			case -1:	/* Error */
					/* But not Bad Driver */
				goto badriver;
			default:
				break;
			}
			break;

		case O_COFF:
			lseek (fd, (long) 0, 0);
			read_and_check(fd, (char*)&fhdr, FILHSZ);

			if (fhdr.f_magic == FBOMAGIC)
				{

				if (fhdr.f_opthdr != 0)
					{
					if ((dp=(struct driver*)malloc(sizeof(struct driver) +
									fhdr.f_opthdr +
									(fhdr.f_nscns+1)*sizeof(short) +
									strlen(dentp->d_name)+1)) == NULL)
						{
						close(fd);
						goto fail1;
						}

                                	dp->ntc_lu = 0;			/* number of TC
logical
	                                                                 * units across
HA
	                                                                 */

					dp->flag = 0;
					dp->otype = O_COFF;

					if (0 == strcmp("LBE",dentp->d_name))
						dp->flag |= ISLBE;

					dp->nctl = 0;
					dp->maj[0] = 0;
					dp->opthdr = mp = (struct master *) (dp + 1);
					dp->s_index = (short *) ((char*)mp + fhdr.f_opthdr);

					for (i=0; i<=(int)fhdr.f_nscns; ++i)
						dp->s_index[i] = -1;

					strcpy(dp->name = (char*)(dp->s_index+fhdr.f_nscns+1), dentp->d_name);

					read_and_check(fd, (char*)mp, fhdr.f_opthdr);
					}

				if (fhdr.f_opthdr == 0 || mp->magic != MMAGIC)
					{
					/* Driver <name>: not processed by mkboot(1M) */
					error(ER12, dentp->d_name);
					goto badriver;
					}

				if (mp->flag & KERNEL)
					/*
					 * ignore kernel object files stored in /boot
					 */
					goto badriver;

        			if ( (mp->soft != NODEV) &&
                        	   (((!(mp->flag & NEWDRV)) && mp->soft > 127)||
                                    ((mp->flag & NEWDRV) && mp->soft > 255)))
                                	{
	
                                	/* Driver <name>: major number greater than 127
                                	** for old driver type or major number greater than
                                	** 255 for new driver type.
                                	*/

                                        	error(ER13, dentp->d_name);
                                        	goto badriver;
                                	}

				dp->timestamp = fhdr.f_timdat;
				dp->nsyms = fhdr.f_nsyms;
				dp->symptr = fhdr.f_symptr;


				sections = 0;

				for (i=1; fhdr.f_nscns-- != 0; ++i)
					{
					read_and_check(fd, (char*)&shdr, SCNHSZ);

					if (0 == strcmp(".text",shdr.s_name))
						{
						sections |= TFOUND;
						dp->shdr[ dp->s_index[i] = 0 ] = shdr;
						}
					if (0 == strcmp(".data",shdr.s_name))
						{
						sections |= DFOUND;
						dp->shdr[ dp->s_index[i] = 1 ] = shdr;
						}
					if (0 == strcmp(".bss",shdr.s_name))
						{
						sections |= BFOUND;
						dp->shdr[ dp->s_index[i] = 2 ] = shdr;
						}
					}

				if (!(sections & TFOUND))
					{
					/* Driver <dp->name>: missing section .text */
					error(ER14, dp->name);
					goto badriver;
					}
				if (!(sections & DFOUND))
					fake_sect(&dp->shdr[1], ".data", STYP_DATA);
				if (!(sections & BFOUND))
					fake_sect(&dp->shdr[2], ".bss", STYP_BSS);


				/*
				 * driver is OK
				 */

				++count;

				dp->next = driver;
				driver = dp;
				}
			else
				{
				/* Driver <name>: not a valid object file */
				error(ER15, dentp->d_name);
			badriver:
				if (dp)
					free((char*)dp);
				}

				break;
			default:
				error(ER15, dentp->d_name);
				break;
			}
		close(fd);
	} /* while dir */

	if (count <= 0)
		{
		/* <slash_boot>: No drivers */
		error(ER16, slash_boot);
		goto fail2;
		}

	/*
	 * search the EDT and mark those drivers that are matched
	 */
	edtscan(mark);


	(void) chdir(cwd); 

	return;


	/*
	 * Error return; cleanup and set driver linked-list empty
	 */
fail1:
	panic(stderr,"No memory for driver linked-list\n");

fail2:
	exit(1);

	}

struct config_note *
elf_getconfig_note (elfdin)
Elf *elfdin;
{
	register char *sh_name;
	register Elf_Scn *eseci;
	register Elf32_Shdr *eshdri;
	Elf_Data *esdatai;
	Elf32_Ehdr *ehdrin;
	char *strtab;

	if ((ehdrin = elf32_getehdr (elfdin)) == NULL)
		return (NULL);

	/* Get string table for section headers */

	if ((eseci = elf_getscn(elfdin, ehdrin->e_shstrndx)) == NULL)
		return (NULL);

	if ((esdatai = elf_getdata (eseci, NULL)) == NULL || esdatai->d_buf == NULL)
		return (NULL);

	strtab = (char *)esdatai->d_buf;

	for (eseci = (Elf_Scn *) 0; eseci = elf_nextscn (elfdin, eseci); ) {
		if ((eshdri = elf32_getshdr(eseci)) == NULL ||
		    (sh_name = (strtab + eshdri->sh_name)) == NULL)
			return (NULL);
		if ((strcmp (sh_name, CONFIG_NAME)) == 0)
			break;
	}
	if (eseci == NULL)
		return (NULL);
	if ((esdatai = elf_getdata (eseci, NULL)) == NULL || esdatai->d_buf == NULL)
		return (NULL);
	return ((struct config_note *) esdatai->d_buf);
}

/*
**	This function can process an object file with only 1 symbol table
*/
long
elf_nsyms (elfdin)
Elf *elfdin;
{
	register Elf_Scn *eseci;
	register Elf32_Shdr *eshdri;

	for (eseci = (Elf_Scn *) 0; eseci = elf_nextscn (elfdin, eseci); ) {

		if ((eshdri = elf32_getshdr (eseci)) == NULL)
			return ((long) -1);

		if (eshdri->sh_type == SHT_SYMTAB)
			return ((long)(eshdri->sh_size / eshdri->sh_entsize - 1));
	}

	return ((long)-1);
}

elf_findrivers (fd, count, name)
int fd, *count;
char *name;
{
	register char *sectname;
	register Elf32_Shdr *eshdr;
	register Elf_Scn *esec;
	char *strtab;
	Elf *elfdin;
	Elf32_Ehdr *ehdrin;
	Elf_Data *edata;
	struct driver *dp;
	struct master *mp;
	struct config_note *config_note;
	int i;
	struct stat st;

	if ((elfdin = elf_begin (fd, ELF_C_READ, NULL)) == NULL) {
		/* Can't begin elf input file: %s */
		error (ER106, name, elf_errmsg(-1));
		return (-1);
	}

	if (!is_elf_reloc(elfdin)) {
		elf_end(elfdin);
		return (-3);	/* findriver() will error(ER15) for us */
	}

	if ((ehdrin = elf32_getehdr (elfdin)) == NULL) {
		elf_end (elfdin);
		return (-3);
	}

	/* Get string table for section headers */

	if ((esec = elf_getscn(elfdin, ehdrin->e_shstrndx)) == NULL) {
		elf_end (elfdin);
		return (-3);
	}

	if ((edata = elf_getdata (esec, NULL)) == NULL || edata->d_buf == NULL) {
		elf_end (elfdin);
		return (-3);
	}

	strtab = (char *)edata->d_buf;

	/* Get config_note */

	config_note = NULL;
	for (esec = (Elf_Scn *) 0; esec = elf_nextscn (elfdin, esec); ) {

		if ((eshdr = elf32_getshdr(esec)) == NULL ||
		    (sectname = (strtab + eshdr->sh_name)) == NULL) {
			elf_end (elfdin);
			return (-3);
		}

		if ((strcmp (sectname, CONFIG_NAME)) == 0) {
			if (((edata = elf_getdata(esec, NULL)) == NULL)
				|| (edata->d_buf == NULL)) {
					elf_end (elfdin);
					return (-3);
			}
			config_note = (struct config_note *)edata->d_buf;
			break;
		}
	}

	if (config_note == NULL) {
		elf_end (elfdin);
		error (ER109, name, "");
		return (-1);
	}

	if ((dp = (struct driver *) malloc (sizeof(struct driver) + config_note->descsz +
	    (ehdrin->e_shnum + 1) * sizeof(short) + strlen (name) + 1)) == NULL)
		return (-2);
	loader_type = O_ELF;
	dp->ntc_lu = 0;
	dp->flag = (strcmp ("LBE", name) ? 0 : ISLBE);
	dp->otype = O_ELF;
	dp->nctl = 0;
	dp->maj[0] = 0;
	dp->opthdr = mp = (struct master *) (dp + 1);
	dp->s_index = (short *) ((char *) mp + config_note->descsz);
	for (i = 0; i <= (int) ehdrin->e_shnum; i++)
		dp->s_index[i] = -1;
	strcpy (dp->name = (char *) (dp->s_index + ehdrin->e_shnum + 1), name);
	memcpy ((char *) mp, (char *) (config_note + 1), config_note->descsz);
	if (mp->magic != MMAGIC) {
		error (ER12, name);
		elf_end (elfdin);
		return (-1);
	}
	if (mp->flag & KERNEL) {
		elf_end (elfdin);
		return (-1);
	}
        if ( (mp->soft != NODEV) &&
		(((!(mp->flag & NEWDRV)) && mp->soft > 127) ||
                  ((mp->flag & NEWDRV) && mp->soft > 255))){
		error (ER13, name);
		elf_end (elfdin);
		return (-1);
	}

	if ((fstat(fd, &st)) == -1) {
		elf_end (elfdin);
		error(ER80, name);
		return (-1);
	}

	dp->timestamp = st.st_mtime;

	if ((dp->nsyms = elf_nsyms (elfdin)) == -1) {
		/* No symbol table */
		error (ER111, name);
		elf_end (elfdin);
		return (-1);
	}

	dp->symptr = 0;

	for (esec = (Elf_Scn *) 0; esec = elf_nextscn(elfdin, esec);){

		if ( (eshdr = elf32_getshdr(esec)) == NULL) {
			error(ER114, elf_errmsg(elf_errno()));
			elf_end(elfdin);
			return(-1);
		}

		sectname = strtab + eshdr->sh_name;

		if (strcmp(sectname, ".data") == 0)
			break;
	}

	if (esec != NULL){
		dp->shdr[1].s_scnptr = eshdr->sh_offset;
		dp->shdr[1].s_vaddr = 0;
	}
	dp->s_index[1] = 0;
	dp->s_index[2] = 1;
	(*count)++;
	dp->next = driver;
	driver = dp;
	elf_end (elfdin);
	return (1);
}

/*
 * Function(prefix, name, symbol)
 *
 * Test whether the symbol is prefix-name
 */
 boolean
function(prefix, name, symbol)
	register char *prefix;
	char *name;
	char *symbol;
	{

	register len = strlen(prefix);

	return(0 == strncmp(prefix,symbol,len) && 0 == strcmp(name,&symbol[len]));
	}

/*
 * Routine(name)
 *
 * Symbol "name" is undefined.  Attempt to find a routine definition in
 * an unloaded driver which will resolve the symbol.  If successful, return
 * TRUE, otherwise return FALSE.
 * ULC - case 1 is coming from eval case '&'. case 0 from Xsym_resolve
 * note the way stubbed entries are handled. code is generated that
 * jumps to the correct place. If an aliasing function was available
 * on the loader such kludgy stuff would not be necessarry. Note
 * also that stubbed entries from eval are handled cleanly.
 */
 boolean
routine(name,code)
	register char *name;
	{

	register struct driver *dp;
	register struct master *mp;
	register struct routine *rp;
	register count;

	dp = (struct driver *)kernel;

	do	{
		if (dp->flag & LOAD)
			/*
			 * driver is already loaded
			 */
			continue;

		if ((mp=dp->opthdr)->nrtn)
			/*
			 * search for a routine of the same name
			 */
			{
			rp = (struct routine *) POINTER(mp->o_routine,mp);

			for (count=0; count<mp->nrtn; ++count, ++rp)
				{
				if (0 == strcmp(name,(char *)POINTER(rp->name,mp)))
					/*
					 * symbol is defined by routine reference
					 */
					{
				    switch (code) {
					case 1:
						switch (rp->id){
						default:
							/* <dp->name>: routine <name>: unknown id; RNULL assumed */
		
							error(ER50, dp->name, name);
							rp->id = RNULL;
						case RNULL:
						case RNOSYS:
						case RNODEV:
						case RTRUE:
						case RFALSE:
						case NOPKG:
						case NOREACH:
							data_rel.rp[data_rel.count].r_symndx = ((SYMBOL *)Xsym_name(rtname[rp->id].symbol->name))->nsymindex;
							data_rel.rp[data_rel.count].r_type = R_SPEC4;
							data_rel.count++;	
							return(TRUE);
							}
				     		break;
#define STUB_JMP 0
				case 0:
					{
						static char stub_template[]= {
							0x24,0x7f,0,0,0,0};
						static int STUBSIZE = (sizeof(stub_template) + sizeof(int) -1) & ~(sizeof(int) - 1);
						register char *text;

						generate(G_TEXT,name,STUBSIZE,stub_template);
						text = (char *) REAL(Xsym_name(name)->value,text_locctr);
						switch (rp->id){
						default:
							/*  <dp->name>: routine <name>: unknown id; RNULL assumed */
		
							error(ER50, dp->name, name);
							rp->id = RNULL;
						case RNULL:
						case RNOSYS:
						case RNODEV:
						case RTRUE:
						case RFALSE:
						case NOPKG:
						case NOREACH:
							/* printf("Routine %s resolved from driver %s\n", name, dp->name); */
							Xrelocate((address)&text[STUB_JMP + 2],R_DIR32S,rtname[rp->id].symbol->name,&text_rel);
						return(TRUE);
						}
					}
						
				}
				}
			}
			}
		}
		while (dp = dp->next);

	return(FALSE);
	}


/*
 * loadunix()
 *
 *   Process all drivers and bootprogram.path (KERNEL). Call routines to
 * build conf.o and unix_abs
 */


 void
loadunix()
{
	register struct driver *dp;
	register struct master *mp;
	register i;
	int fd = -1;
	int number_drivers;
	int b_major, c_major, bc_major;
	FILHDR fhdr;
	struct stat statbuf;
	char *dname;
	FILE *stream;
	dev_t soft_major[256];
	extern char *do_swapfile();
	int nsyms;

	/*
         * maximum memory in current system
    	 */

	maxmem = sizemem();
	/*
	 * Initialize local edt structure. escan and edtscan use this info.
  	 *
 	 */
	initedt();

	/*
	* build driver linked-list
	*/
	findrivers();

	if ((fd=open(etcsystem,O_RDONLY)) == -1){
		error(ER80, etcsystem);
		exit(1);
	}

	fsystem(stream=fdopen(fd,"r"));
	fclose(stream);

	/*
	 * if the boot program name was not specified in /etc/system, it has
	 * to be gotten now
	 */
	if (! bootprogram.path[0]){
		error(ER10,etcsystem);
		exit(1);
	}

	/*
	 * open the boot program file, and figure out what kind of object
	 * file it is
	 */
	if (stat(bootprogram.path,&statbuf) == -1 || (fd=open(bootprogram.path,O_RDONLY)) == -1){
		error(ER80, bootprogram.path);
		exit(1);
	}

	if (! (statbuf.st_mode & (S_IEXEC|S_IEXEC>>3|S_IEXEC>>6)))
		/* <bootprogram.path>: warning: not executable */
		error(ER31, bootprogram.path);

	switch(object_file_type(fd)){
	case O_ELF:
		elf_readboot (fd, &nsyms);
		break;
	case O_COFF:
		/*
		 * Read file header and verify it.
		 */
		lseek (fd, (long) 0, 0);
		read_and_check(fd, (char*)&fhdr, FILHSZ);
		nsyms = fhdr.f_nsyms;

		if (fhdr.f_magic != FBOMAGIC){
			/* <bootprogram.path>: not MAC32 magic */
			error(ER32, bootprogram.path);
			exit(1);
		}

		bootprogram.timestamp = fhdr.f_timdat;

		if (fhdr.f_nscns == 0){
			/* <bootprogram.path>: no section headers */
			error(ER33, bootprogram.path);
			exit(1);
		}

		/*
		 * check that supplied ifiles can be used with bootprogram.
		 */

		(void) check_ifile();

		/*
		 * Allocate kernel structure and read optional header if present;
		 * note that the driver linked list may not yet be allocated so it
		 * cannot be linked to the kernel list yet.
		 */
		if ((kernel=(struct kernel*)malloc((unsigned int)(sizeof(struct kernel)+max((long)fhdr.f_opthdr,(long)sizeof(struct master))))) == NULL){
			panic(stderr,"No memory for kernel optional header\n");
			exit(1);
		}

		kernel->opthdr = mp = (struct master*) (kernel + 1);
		kernel->otype = O_COFF;
		kernel->flag = 0;
		kernel->nctl = 0;
		kernel->int_major = 0;
		kernel->name = bootprogram.path;

		read_and_check(fd, (char*)mp, fhdr.f_opthdr);

		if (fhdr.f_opthdr == 0 || mp->magic != MMAGIC){
			mp->flag = KERNEL;
			mp->vec = 0;
			mp->ndev = 0;
			mp->ndep = 0;
			mp->nparam = 0;
			mp->nrtn = 0;
			mp->nvar = 0;
		}
		else if (! (mp->flag & KERNEL)){
			/* <bootprogram.path>: not flagged as KERNEL by mkboot(1M) */
			error(ER47, bootprogram.path);
			exit(1);
		}

		if (driver->next == driver){
			/*
			 * no driver linked-list could be built; self-config cannot be done:
			 * No drivers available, absolute BOOT program must be used
			 */
			error(ER34);
			exit(1);
		}

		/*
		 * link the driver linked-list to the kernel data structure
		 */
		kernel->next = driver;
		break;
	default:
		error(ER51, bootprogram.path);
		exit(1);
		break;
	}

	close(fd);

	/*
	 * go through the EDT again now that the /etc/system file has
	 * been processed; the EXCLUDE entries will be caught here for
	 * those EDT entries which did not have a corresponding driver
	 * in /boot
	 * ULC - Note that the edtscan routine has been re-written.
	 */

	edtscan(catch);

	edtscan(fndxbus);

	/*
	 * Assign rootdev. 
	 * User Level comment (ULC)- no longer do any frimware
	 * stuff here. see proc_vtoc.
	 */

	if (rootdev == NODEV || swapdone == -1){
		char *arg, *err;

		proc_vtoc();
		if (VTOC_root != -1 && rootdev == NODEV)
                        rootdev = makedevice(VTOC_major,VTOC_minor + VTOC_root);
		if (swapdone == -1){
                        arg = "/dev/swap";
                        err = do_swapfile(1, &arg, &swapdev);
                        if (err == NULL) {
				swapfile.bo_offset = 0;
				swapfile.bo_size = VTOC_nswap;
			} else {
                                printf("do_swapfile: %s\n",err);
				swapdone = -1;
			}
                }
	}

	if (rootdev == NODEV || swapdone == -1){
		error(ER44);
		exit(1);
	}
	/*
	 * check the parameters for multiply defined values
	 */
	check_param();


	/*
	 * if the LBE driver does not exist, a LBE is not installed, or
	 * the LBE driver has been EXCLUDED, then devices on the LBE
	 * cannot be supported
	 */
	if ((dp=searchdriver("LBE")) == NULL || ! (dp->flag & INEDT) || dp->flag & EXCLUDE)
		/*
		 * remove any devices on a LBE
		 */
		{
		register j;

		dp = driver;
		do	{
			if (dp->flag & EXCLUDE)
				continue;

			if (! (dp->flag & INEDT))
				continue;

			for (i=j=0; i<(int)dp->nctl; ++i)
				{
				if (ONLBE(dp->maj[i]))
					{
					/* <dp->name>: device not configured (LBE <n>, board code <n>) */
					error(ER35, dp->name, LBELBA(dp->maj[i]), LBEELB(dp->maj[i]));
					}
				else
					dp->maj[j++] = dp->maj[i];
				}

			if ((dp->nctl = j) == 0)
				dp->flag |= EXCLUDE;
			}
			while (dp = dp->next);
		}

	/*
	 * determine all drivers to be loaded
	 */
	dp = driver;
	do	{
		if ((mp=dp->opthdr)->flag & REQ)
			{
			if (dp->flag & EXCLUDE)
				{
				/* <dp->name>: required driver is EXCLUDED */
				error(ER36, dp->name);
				continue;
				}

			dp->flag |= LOAD;
			dp->nctl = 1;
			}

		if (dp->flag & EXCLUDE)
			continue;


                if(dp->flag & (INEDT|INCLUDE|ISEBUS))
                        dp->flag |= LOAD;

		if (dp->flag & LOAD && mp->ndep > 0)
			dependency(dp);
	} while (dp = dp->next);

	if (!searchbymaj(getemajor(rootdev),&dname)){
		error(ER45,"root",getemajor(rootdev),geteminor(rootdev));
		exit(1);
	}

	if (swapdev != NODEV) {
		if (!searchbymaj(getemajor(swapdev),&dname)){
			error(ER45,"swap",getemajor(swapdev),geteminor(swapdev));
			exit(1);
		}
	}

	if (mirrordev[0] != NODEV && mirrordev[1] != NODEV){
		if (VTOC_root != -1 && 
		   geteminor(mirrordev[0]) != geteminor(rootdev) && 
		   geteminor(mirrordev[1]) != geteminor(rootdev))
			error(ER101,getemajor(rootdev),geteminor(rootdev));

		if (swapdev != NODEV && 
		   geteminor(mirrordev[0]) != geteminor(swapdev) && 
	 	   geteminor(mirrordev[1]) != geteminor(swapdev))
			error(ER101,getemajor(swapdev),geteminor(swapdev));
	}


	/*
	 * Assign the internal major numbers.  This is a two pass approach,
	 * first the drivers which are both BLOCK and CHAR are assigned numbers
	 * then the remaining drivers are assigned.  This will minimize the size
	 * of the character and block device switch tables.
	 */

	for (i=0; i<256; i++)
		soft_major[i] = (dev_t) (NODEV);

	for (bc_major=0, i=0; i<2; b_major=c_major=bc_major, ++i)
		{
		dp = driver;

		do	{
			if (! (dp->flag & LOAD))
				continue;

			mp = dp->opthdr;

			if ((mp->flag & SOFT) && (mp->soft == NODEV)){
				error(ER82, dp->name);
				dp->flag &= ~LOAD;
				continue;
			}

			if (mp->flag & SOFT && i == 0)
				if (soft_major[mp->soft] != (dev_t)(NODEV)){
					(void)searchbymaj(mp->soft, &dname);
					error(ER81,dp->name, dname, mp->soft, dp->name);
					dp->flag &= ~LOAD;
					continue;
				} else
					soft_major[mp->soft] = mp->soft;

			if (i == 0)
				{
				if ((mp->flag & (BLOCK|CHAR)) == (BLOCK|CHAR))
					dp->int_major = bc_major++;
				}
			else
				{
				if ((mp->flag & (BLOCK|CHAR)) == BLOCK)
					dp->int_major = b_major++;
				else
					if ((mp->flag & (BLOCK|CHAR)) == CHAR || mp->flag & FUNDRV)
						dp->int_major = c_major++;
				}
			}
			while (dp = dp->next);
		}

	/*
	 * Compute cdevcnt, bdevcnt, and fmodcnt; also count the total 
	 * number of drivers to be loaded, and interrupt routines that 
	 * will be needed
	 */

	number_drivers = 0;

	dp = driver;
	do	{

		if (dp->flag & LOAD)
			{
			++number_drivers;


			if ((mp=dp->opthdr)->flag & ONCE && dp->nctl != 1)
				{
				/* <dp->name>: flagged as ONCE only; #C set to 1 */
				error(ER37, dp->name);
				dp->nctl = 1;
				}

			if (mp->flag & EXECTYP) {
				++nexectype;
				continue;
			}

			if (mp->flag & FSTYP) {
				++nfstype;
				continue;
			}

                        if (mp->flag & SCHEDCLASS) {
                                ++classcnt;
                                continue;
                        }

			if (mp->flag & NOTADRV)
				continue;

			if (mp->flag & FUNMOD) {
				++fmodcnt;
				if (!(mp->flag & (CHAR | BLOCK | FUNDRV)))
					continue;
			}

			if (mp->flag & (CHAR | FUNDRV))
				cdevcnt = max((long)cdevcnt, (long)(dp->int_major));
			if (mp->flag & BLOCK)
				bdevcnt = max((long)bdevcnt, (long)(dp->int_major));

			}
		}
		while (dp = dp->next);

			/* no null entry for exectype */
	++nfstype;
        ++classcnt;
	++cdevcnt;
	++bdevcnt;

#if DEBUG1
        printf("cdevcnt=%d  bdevcnt=%d fmodcnt=%d nfstype=%d nexectype= %d classcnt=%d\n", cdevcnt, bdevcnt, fmodcnt, nfstype, nexectype, classcnt);

#endif

	/*
	 * Print configuration table if auto-boot switch is off
	 */
	if (!QuietMode )
		print_configuration();

	/*
	 * allocate memory for load-string 
	 */
	alloc_loadstring(number_drivers);

	/*
	 * do the dirty work; load the kernel and all drivers
	 */
	relocatable(nsyms);

	/*
	 * create object file conf.o
	 */

	conf(conf_file);

	/* 
	 * Some really disastrous things could have happened. Example
	 * a user decides to build a totally mismatched KERNEL and
	 * modules. DO NOT attempt to create a unix in this case.
	 */

	if (donotload){ 
		ckconferror("The loading of your system will not be attempted due to earlier symbol referencing errors, bye\n");
		exit(-1);
	}
		 
	/*
	 * free up mmap'd space. Get swap reservation space down.
	 */
	free_sym_space();
	xfree();

	/*
	 * call loader.
	 */

	 load(); 

	/*
	 * patch up absolute.
	 */
	 patch();

}

void
elf_readboot (fd, nsyms)
int fd;
long *nsyms;
{
	int opthdrsize;
	Elf *elfd;
	Elf32_Ehdr *ehdrin;
	struct master *mp;
	struct config_note *config_note;
	struct stat st;

	/*
	**	we need to do this stat for mtime value.  we'll never
	**	exit here because a stat() was just done on this
	**	fd before we were called.
	*/

	if ((fstat(fd, &st)) == -1) {
		error(ER80, "elf_readboot");
		exit (1);
	}

	if ((elfd = elf_begin (fd, ELF_C_READ, NULL)) == NULL) {
		error (ER121, bootprogram.path, elf_errmsg(-1));
		exit (1);
	}

	if (!is_elf_reloc(elfd)) {
		error(ER51, bootprogram.path);
		exit (1);
	}

	if ((ehdrin = elf32_getehdr (elfd)) == NULL) {
		error(ER123, bootprogram.path, elf_errmsg(-1));
		exit (1);
	}

	if ((*nsyms = elf_nsyms (elfd)) == -1) {
		/* No symbol table */
		error (ER122, bootprogram.path);
		exit (1);
	}

	bootprogram.timestamp = st.st_mtime;

	if (ehdrin->e_shnum == 0) {
		error (ER33, bootprogram.path);
		exit (1);
	}

	opthdrsize = ((config_note = elf_getconfig_note (elfd)) ? config_note->descsz : 0);
	if ((kernel = (struct kernel *) malloc ((unsigned int) (sizeof(struct kernel) +
	    max ((long) opthdrsize, (long) sizeof(struct master))))) == NULL) {
		fprintf (stderr, "No memory for kernel optional header\n");
		exit (1);
	}
	loader_type = O_ELF;
	kernel->otype = O_ELF;
	kernel->opthdr = mp = (struct master *) (kernel + 1);
	kernel->flag = 0;
	kernel->nctl = 0;
	kernel->int_major = 0;
	kernel->name = bootprogram.path;
	memcpy ((char *) mp, (char *) (config_note + 1), opthdrsize);
	if (opthdrsize == 0 || mp->magic != MMAGIC) {
		mp->flag = KERNEL;
		mp->vec = mp->ndev = mp->ndep = mp->nparam = mp->nrtn = mp->nvar = 0;
	} else if (! (mp->flag & KERNEL)) {
		error (ER47, bootprogram.path);
		exit (1);
	}
	if (driver->next == driver) {
		error (ER34);
		exit (1);
	}
	kernel->next = driver;
	check_ifile();
	elf_end (elfd);
}


/*
 * Do all the dirty work. initialize internal symbol tables for conf.o 
 * generation, build the io subsystem, create load string of drivers,
 * build sys3bsym and some other stuff.
 */

 static
 int
relocatable(nsyms)
	long nsyms;
	{

	long Xsize;
	register struct driver *dp;
	register struct rtname *rtn;
	int fd = -1;
	int number_drivers;		/* total number of drivers/modules to be loaded */
	int fscnt, clcnt;
	address temp;
	struct s3bsym *sys3bsym;


	if ((fd=open(bootprogram.path,O_RDONLY)) == -1){
		error(ER80,bootprogram.path);
		exit(1);
		}


	/*
	 * Load the kernel's symbol table (and allocate kernel BSS)
	 */
	Ksymread(nsyms);

	/*
	 * Initialize space for object file data structures.
	 *
	 */

	init_sym_space();

	/*
	 * Create .file .text and .data symbols plus auxillary entries.
	 *
	 */

	init_sym();




	for (rtn=rtname; rtn->name; ++rtn)
		{
		if ((rtn->symbol=Ksym_name(rtn->name)) == NULL)
			{
			/* <bootprogram.path>: routine <rtn->name>() not found */
			error(ER65, bootprogram.path, rtn->name);

			}
		ext_allocate(rtn->name);
		}

	/*
	 * build all interrupt routines, pcb's, kernel data structures for I/O, etc.
	 */
	build_io_subsys();


	/*
	 * allocate any variables required by the kernel
	 */
	alloc_variables((struct driver *)kernel);

	/*
	 * copy "sys3bboot"
	 */
	generate(G_DATA, "sys3bboot", sizeof(bootprogram), &bootprogram);


	close(fd);
	fd = -1;

	if (loader == NULL){
		if (loader_type == O_COFF)
			loader = "/usr/bin/ld";
		else if (loader_type == O_ELF)
			loader = "/usr/ccs/bin/ld";
	}



	if ((loader_type == O_COFF)  && (larg == NULL))
		larg = "-x";

	if (larg != NULL){	/* additional arguments to the loader */
		strcpy(lsp,larg);
		lsp += strlen(larg);
		strcat(lsp++," "); 
	}
	if (loader_type == O_ELF){ /* ELF loader has -dn -M option */
		strcpy(lsp,"-dn -M");
		lsp += strlen("-dn -M");
		strcat(lsp++," ");
	}
	strcpy(lsp,ifile);
	lsp += strlen(ifile);
	strcat(lsp++," ");
	strcpy(lsp,bootprogram.path);
	lsp += strlen(bootprogram.path);

	number_drivers = 0;
	classcnt = 1;

	dp = driver;

	do	{
		if (dp->flag & LOAD)
			{
                        if (! (dp->flag & INEDT || dp->flag & ISEBUS))
				++number_drivers;

			strcat(lsp++," ");
			strcpy(lsp,slash_boot);
			lsp += strlen(slash_boot);
			strcat(lsp++,"/");
			strcpy(lsp,dp->name);
			lsp += strlen(dp->name);
			loadriver(dp);
			}
		}
		while (dp = dp->next);

	/*
	 * place config at end of load string.
	 */
	strcat(lsp++," ");
	strcpy(lsp,conf_file);
	lsp += strlen(conf_file);

	*next_init++ = NULL;
	generate(G_UDATA, "io_init", (next_init-io_init)*sizeof(*io_init));

	*next_start++ = NULL;
	generate(G_UDATA, "io_start", (next_start-io_start)*sizeof(*io_start));

	*next_pwrclr++ = NULL;
	generate(G_UDATA, "pwr_clr", (next_pwrclr-pwr_clr)*sizeof(*pwr_clr));

        *next_poll++ = NULL;
	generate(G_UDATA, "io_poll", (next_poll-io_poll)*sizeof(*io_poll));

        *next_halt++ = NULL;
	generate(G_UDATA, "io_halt", (next_halt-io_halt)*sizeof(*io_halt));


        /*
         * Now that we have a good class cound generate nclass.
	 */
	generate(G_DATA, "nclass", sizeof(int), &classcnt);


	/*
	 * allocate the sys3bconfig structure
	 */
	alloc_sys3bconfig(edt_count + number_drivers);


	/*
	 * initialize the driver data structures (this may involve resolving
	 * address-of initializers which may need to be resolved with routine
	 * references in unloaded drivers; if so additional symbols will be
	 * created)
	 */
	initdata((struct driver*)kernel,0);

	dp = driver;

	fscnt = 0;
	clcnt = 0;
	do	{
	register struct master *mp1;
		mp1 = dp->opthdr;
		if ((mp1->flag & FSTYP) && (dp->flag & LOAD))
			fscnt++;
		if ((mp1->flag & SCHEDCLASS) && (dp->flag & LOAD))
			clcnt++;
		if (dp->flag & LOAD)
			initdata(dp,fscnt,clcnt);
		}
		while (dp = dp->next);

	Xsym_name("sys3bsym")->flag |= DEFER;

	Xsym_walk(NULL,Xsym_resolve);

	/*	If debugging mode was specified, then call
	**	Ksym_copyall.  This will copy all defined
	**	entries from Ksymtab to Xsymtab so that
	**	they will end up in the sys3b symbol table.
	*/

	if (DebugMode){
		Ksym_copyall();
		elf_debug_info(kernel);
	}

	/*
	 * allocate the space necessary for the sys3bsym symbol table; this
	 * is the last thing to create -- thus all remaining addresses can
	 * now be computed
	 */
	if (DebugMode)
		Xsize = Xsym_size;
	else 
		Xsize = 0;

	temp = allocate(&data_locctr.v_locctr, "sys3bsym", (long)(sizeof(struct s3bsym)+Xsize),2);

	/*
	 * copy symbol table to kernel data for sys3b(2) system call
	 */
	if (temp > (address)0) {
		sys3bsym = (struct s3bsym*) REAL(Xsym_name("sys3bsym")->value, data_locctr);
		sys3bsym->size = sizeof(struct s3bsym) + Xsize;
		sys3bsym->count = Xsize ? Xsym_count : 0;
		
		if (DebugMode)
			Xsym_copy((SYMBOL*)NULL, 0, sys3bsym); 

	}
	return;

	}



/*
 * see if the driver specified by major maj is on the driver linked list.
 *
 */
searchbymaj(maj,name)
int maj;
char **name;
{
	register struct driver *dp;
	register struct master *mp;
	register int i;
	
        if ((dp =driver) != NULL)
                do {
			mp = dp->opthdr;

                        if (! (dp->flag & LOAD) ||
                           mp->flag & NOTADRV)
                                continue;
                        if ((mp->flag & (CHAR | BLOCK | FUNMOD | FUNDRV)) == FUNMOD)
                                continue;
                        if (mp->soft == maj){
                        	*name= dp->name;
                                return(1);
                        }
                } while (dp=dp->next);
	return(0);
}


/*
 * check_ifile()
 * decide what system being built and choose appropriate
 * ifile.
 */

 static void
check_ifile()
{
	struct stat sb;
		 	
	if (ifile != NULL){	/* set by user with -i option */
		if (stat(ifile,&sb)){
			error(ER80,ifile);
			exit(1);
		}
	return;
	}


	switch (loader_type){
	case O_COFF:
		ifile = (char *) malloc( strlen(conf_dir) + strlen("/ifile4.0") + 1);
     		if ( ifile == NULL){
                        panic("Can't malloc space for ifile name\n");
                        exit(1);
		}
		strcpy(ifile,conf_dir);
		strcat(ifile,"/ifile4.0");
		return;
	case O_ELF:
		ifile = (char *) malloc( strlen(conf_dir) + strlen("/mapfile4.0") + 1);
     		if ( ifile == NULL){
                        panic("Can't malloc space for mapfile name\n");
                        exit(1);
		}
		strcpy(ifile,conf_dir);
		strcat(ifile,"/mapfile4.0");
		return;
	default:
		return;
	}

}

/*
 * Itoa(number)
 *
 * Convert number to right adjusted ascii string; pointer returned is to
 * beginning of char [15] array, with result right adjusted and left padded
 * with blanks (and an optional minus sign).
 */
 char *
itoa(number)
	register int number;
	{

	static char buffer[15+1];
	register boolean minus;
	register char *p;

	if (!(minus = number<0))
		/*
		 * compute using negative numbers to avoid problems at MAXINT
		 */
		number = -number;

	*(p=buffer+sizeof(buffer)-1) = '\0';

	do	{
		*--p = '0' - (number%10);
		}
		while ((number/=10) < 0);

	if (minus)
		*--p = '-';

	while (p > buffer)
		*--p = ' ';

	return(buffer);
	}

/*
 * Lcase(string)
 *
 * Convert upper case letters in string to lower case.  Return the string.
 */
 char *
lcase(s)
	char *s;
	{

	register char *p;
	register c;

	for (p=s; c = *p; ++p)
		if (isascii(c) && isupper(c))
			*p = tolower(c);

	return(s);
	}

/*
 * Print_configuration()
 *
 * Print the configuration table; this is only done if the auto-boot
 * switch is off and QuietMode is FALSE
 */
 static
 void
print_configuration()
	{

	register struct driver *dp;
	register struct master *mp;
	register int i;
	register boolean issued;
	char buffer[DIRSIZ+1];


	if (QuietMode )
		return;

	sleep(3);	/* give user a chance to see earlier error messages if any */

	printf("\nCONFIGURATION SUMMARY\n=====================\n     ----driver---- #devices major\n");
	/*                                                     \n     DIRSIZ__LENGTH nnnnn   nnnn,nnnn */

	if ((dp = driver) != NULL)
		{
		/*
		 * handle drivers
		 */
		do	{
			if (! (dp->flag & LOAD) ||
			   ((mp=dp->opthdr)->flag & NOTADRV))
				continue;
			if ((mp->flag & (CHAR | BLOCK | FUNMOD | FUNDRV)) == FUNMOD)
				continue;


			strcpy(buffer, dp->name);
			strncat(buffer, "              ", (int)(sizeof(buffer)-1-strlen(buffer)));

			printf("     %s %s   ", buffer, itoa((int)dp->nctl)+10);

			if (! (mp->flag & (BLOCK|CHAR|FUNDRV)))
				printf("\n");
			else
				if (mp->flag & SOFT)
					printf("%s\n", itoa((int)mp->soft)+11);
				else
					{
					for (i=0; i<(int)dp->nctl; ++i)
						printf((i+1==dp->nctl)?"%s\n":"%s,", itoa((int)dp->maj[i])+11);
					}
			}
			while (dp = dp->next);

		/*
		 * handle modules
		 */
		dp = driver;

		issued = FALSE;
		do	{
			mp = dp->opthdr;
			if (!(dp->flag & LOAD) ||
			   !(mp->flag & (NOTADRV | FUNMOD)))
				continue;

			if (QuietMode )
				{
				printf("\n");
				return;
				}

			if (! issued)
				/*
				 * print heading for first module found
				 */
				{
				issued = TRUE;
				printf("\n     ----module----\n");
				/*       \n     DIRSIZ__LENGTH */
				}

			printf("     %s\n", dp->name);
			}
			while (dp = dp->next);
		}

	printf("\n\n");
	printf("    ----device info----\n");
	printf("              major    minor\n");
	printf("    rootdev   %5d   %5d\n",getemajor(rootdev),geteminor(rootdev));
  	if (swapdev != NODEV)
		printf("    swapdev   %5d   %5d	  %s\n", getemajor(swapdev),
                        geteminor(swapdev), swapfile.bo_name);
	else
            	printf("    swapdev   %s\n", swapfile.bo_name);

	printf("\n");
	}

#if DEBUG2
/*
 * Print_driver()
 *
 * Print the kernel and driver linked-list
 */

static void print_master();
static void print_expression();
static char *print_flag();

 void
print_driver()
	{

	register struct driver *dp;
	register int i;


	printf("\nKernel:\n");

	if (kernel != NULL)
		print_master(kernel->opthdr);

	printf("\nDriver linked-list:\n");

	if ((dp = driver) == NULL || dp->next == driver)
		return;

	do	{
		printf("%9s  flag=%4X  #C=%d  #M=%d  maj(sys)=", dp->name, dp->flag, dp->nctl, dp->int_major);

		for (i=0; i<(int)dp->nctl; ++i)
			printf("0x%X(0x%X)%s", dp->maj[i], dp->sys_bits[i], (i+1==dp->nctl)?"":",");
		printf("\n           #syms=%D   symptr=0x%lX\n", dp->nsyms, dp->symptr);

		printf("           text: vaddr=0x%6lX sz=0x%lX sptr=0x%lX #rel=%d rptr=0x%lX\n",
			dp->shdr[0].s_vaddr, dp->shdr[0].s_size, dp->shdr[0].s_scnptr, dp->shdr[0].s_nreloc, dp->shdr[0].s_relptr);
		printf("           data: vaddr=0x%6lX sz=0x%lX sptr=0x%lX #rel=%d rptr=0x%lX\n",
			dp->shdr[1].s_vaddr, dp->shdr[1].s_size, dp->shdr[1].s_scnptr, dp->shdr[1].s_nreloc, dp->shdr[1].s_relptr);
		printf("           bss:  vaddr=0x%6lX sz=0x%lX\n", dp->shdr[2].s_vaddr, dp->shdr[2].s_size);

		print_master(dp->opthdr);
		}
		while (dp = dp->next);
	}



/*
 * Print_master()
 *
 * Print the master file optional header built by mkboot(1M)
 */
 static
 void
print_master(master)
	register struct master *master;
	{

	register int i, j;


	if (master == NULL)
		return;

	printf("           flag=");
	if (master->vec)
		printf("%d %s\n", master->vec, print_flag(master->flag));
	else
		printf("%s\n", print_flag(master->flag));

	if (! (master->flag & KERNEL))
		{
		printf("           nvec=%d  prefix=%s  software=%d  ndev=%d  ipl=%d\n",
			master->nvec, master->prefix, master->soft,
			master->ndev, master->ipl);

		if (master->ndep)
			{
			register struct depend *dp = (struct depend *) POINTER(master->o_depend, master);
			for (i=0; i<master->ndep; ++i, ++dp)
				printf("           dependency=%s\n", POINTER(dp->name,master));
			}
		}

	if (master->nparam)
		{
		char name[PARAMNMSZ+1];
		register struct param *pp = (struct param *) POINTER(master->o_param, master);
		for (i=0; i<master->nparam; ++i, ++pp)
			{
			if (pp->name[0] == '\0')
				continue;
			printf("           parameter: %s = ",  strncat(strcpy(name,""),pp->name,PARAMNMSZ));
			if (pp->type == '"')
				printf("\"%s\"\n", POINTER(pp->value.string, master));
			else
				printf("0x%X\n", pp->value.number);
			}
		}

	if (! (master->flag & KERNEL))
		{
		if (master->nrtn)
			{
			static char *id[] = { "", "nosys", "nodev", "true", "false", "fsnull", "fsstray", "nopkg", "noreach" };
			register struct routine *rp = (struct routine *) POINTER(master->o_routine, master);
			for (i=0; i<master->nrtn; ++i, ++rp)
				printf("           routine %s() {%s}\n", POINTER(rp->name,master), id[rp->id]);
			}
		}

	if (master->nvar)
		{
		register struct variable *vp = (struct variable *) POINTER(master->o_variable, master);

		for (i=0; i<master->nvar; ++i, ++vp)
			{
			printf("           variable %s", POINTER(vp->name, master));
			if (vp->dimension)
				{
				printf("[");
				print_expression((union element *) POINTER(vp->dimension, master));
				printf("]");
				}
			printf("(0x%X)\n", vp->size);
			if (vp->ninit)
				{
				static char *type[] = { "0x%X", "c", "s", "i", "l", "%dc" };
				register struct format *fp = (struct format *) POINTER(vp->initializer, master);
				for (j=0; j<vp->ninit; ++j,++fp)
					{
					printf("                 ={ %s", "%");
					printf(type[fp->type&FTYPE], ((fp->type&FTYPE)!=FSTRING)? fp->value : fp->strlen);
					if (fp->type & FEXPR)
						{
						printf(" expression=");
						print_expression((union element *) POINTER(fp->value, master));
						printf(" }\n");
						continue;
						}
					if (fp->type & FSKIP)
						{
						printf(" }\n");
						continue;
						}
					printf(" 0x%X }\n", fp->value);
					}
				}
			}
		}
	}


/*
 * Print_expression(expression)
 *
 * Print an expression
 */
 static
 void
print_expression(expression)
	register union element *expression;
	{

	static union element *xp;
	register int temp;
	char c[2];

	if (expression == NULL)
		expression = xp;

	switch (c[1]='\0', c[0]=expression->operator)
		{
		char name[PARAMNMSZ+1];
	case '+':
	case '-':
	case '*':
	case '/':
		xp = XBUMP(expression,operator);
		print_expression((union element *)NULL);
		printf(c);
		print_expression((union element *)NULL);
		break;
	case '>':
	case '<':
		printf((c[0]=='<')? "min(" : "max(");
		xp = XBUMP(expression,function);
		print_expression((union element *)NULL);
		printf(",");
		print_expression((union element *)NULL);
		printf(")");
		break;
	case 'C':
	case 'D':
	case 'M':
		printf("#%s", c);
		if (expression->nC[1])
			printf("(%s)", expression->nC+1);
		xp = XBUMP(expression,nC);
		break;
	case '#':
	case '&':
		printf("%s%s", c, expression->size_of+1);
		xp = XBUMP(expression,size_of);
		break;
	case 'I':
		printf("%s", strncat(strcpy(name,""),expression->identifier+1,PARAMNMSZ));
		xp = XBUMP(expression,identifier);
		break;
	case '"':
		printf("\"%s\"", expression->string+1);
		xp = XBUMP(expression,string);
		break;
	case 'N':
		temp = ((expression->literal[1]<<8 | expression->literal[2] & 0xFF)<<8 | expression->literal[3] & 0xFF)<<8 | expression->literal[4] & 0xFF;
		printf("%d", temp);
		xp = XBUMP(expression,literal);
		break;
		}
	}


/*
 * print_flag(flag)
 *
 * Print the flags symbolically
 */
 static
 char *
print_flag(flag)
	register unsigned short flag;
	{
	static char buffer[80];

	strcpy(buffer, "");

	if (flag == 0)
		return("none");

	if (flag & KERNEL)
		strcat(buffer, ",KERNEL");

	if (flag & ONCE)
		strcat(buffer, ",ONCE");

	if (flag & REQ)
		strcat(buffer, ",REQ");

	if (flag & BLOCK)
		strcat(buffer, ",BLOCK");

	if (flag & CHAR)
		strcat(buffer, ",CHAR");

	if (flag & FUNDRV)
		strcat(buffer, ",FUNDRV");

	if (flag & REQADDR)
		strcat(buffer, ",REQADDR");

	if (flag & TTYS)
		strcat(buffer, ",TTYS");

	if (flag & SOFT)
		strcat(buffer, ",SOFT");

	if (flag & FUNMOD)
		strcat(buffer, ",FUNMOD");

	if (flag & NOTADRV)
		strcat(buffer, ",NOTADRV");

	if (flag & FSTYP)
		strcat(buffer, ",FSTYP");

        if (flag & NEWDRV)
		strcat(buffer, ",NEWDRV");

	return(buffer+1);
	}
#endif



/*
 * allocate memory for load_string.
 */

alloc_loadstring(number_drivers)
{
	register int size;

	size= ((strlen(slash_boot) + DIRSIZ + 1) * number_drivers + strlen(loader) + strlen(ifile) + strlen(unix_abs) + 10);
	if ((lsp = load_string = (char *)malloc(size)) == NULL){
		panic("malloc failed, no memory \n");
		exit(1);
	}
}



/*
 * load() , finish up load_string and exec loader.
 *
 */
load()
{
	int stat_loc;
	char *out=" -o ";

	strcpy(lsp,out);
	lsp += strlen(out);
	strcpy(lsp,unix_abs);
	execld();
	wait(&stat_loc);
	if (stat_loc != 0){
		ckconferror("load failed, bye \n");
		exit(1);
	}
}
		
execld()
{
	int pid,i;
	char *argv[200],*s;


	if ((pid = fork()) == 0){
		argv[0] = loader;
		i = 1;
		s = load_string;
		while (*s){
			argv[i++] = s;
			while ( *s != ' ' && *s != '\0')
				s++;
			if ( *s == '\0')
				break;
			*s++ = '\0';
		}
		argv[i] = (char *)0;
		if (i > 199){
			ckconferror(stderr,"max arguments exceed \n");
			exit(1);
		}
		i = 0;
		if (!QuietMode){
			while( argv[i])
				printf("%s ",argv[i++]);{
			printf("\n");
			}
		}

		if (execv(loader,argv) == -1){
			ckconferror("exec of loader failed\n");
			perror("error");
			exit(1);
		}
	 }
	if (pid == -1){
		ckconferror("unable to fork\n");
		perror("error");
		exit(1);
	}
}
		

/*
 * Read with error checking
 *
 */
 void
read_and_check(fildes, buf, nbyte)
	int fildes;
	char *buf;
	register unsigned nbyte;
	{

	register int rbyte;

	if ((rbyte = read(fildes,buf,nbyte)) == -1)
		{
		error(ER7);
		exit(1);
		}
	else
		if (rbyte != nbyte)
			{
			error(ER49, "read_and_check", "");
			exit(1);
			}
	}



/*
 * Write with error checking; 
 *
 */
 void
fwrite_and_check(stream, buf, nbyte)
	FILE *stream;
	char *buf;
	register unsigned nbyte;
	{


	if (fwrite(buf,nbyte,1,stream) == -1)
		{
		error(ER7);
		exit(1);
		}
	}




/*
 * Lseek with error checking; if an I/O error occurs, condition IOERROR is
 * signalled.
 */
 void
seek_and_check(fildes, foffset, whence)
	int fildes;
	long foffset;
	int whence;
	{

	if (lseek(fildes,foffset,whence) == -1L)
		{
		error(ER7);
		exit(1);
		}
	}


object_file_type(fd)
int fd;
{
	short magic;

	lseek(fd, 0, 0);
	read_and_check(fd, &magic, sizeof(short));
	lseek(fd, 0, 0);
	if (magic == FBOMAGIC)
		return(O_COFF);
	if (magic == 0x7f45)
		return(O_ELF);
	return(0);
}


 static void
ckconferror(text)
char *text;
{

	fprintf(stderr,"error: %s", text);
	if (!confdebug)
		unlink(conf_file);
}
