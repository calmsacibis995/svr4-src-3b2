/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/mon.c	2.18"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 *	Environment variable PROFDIR added such that:
 *		If PROFDIR doesn't exist, "mon.out" is produced as before.
 *		If PROFDIR = NULL, no profiling output is produced.
 *		If PROFDIR = string, "string/pid.progname" is produced,
 *		  where name consists of argv[0] suitably massaged.
 *
 *
 *
 *	Monitor(), in coordination with mcount() and mnewblock(), 
 *	maintains a series of one or more blocks of prof-profiling 
 *	information.  These blocks are added in response to calls to
 *	monitor(), explicitly or via mcrt0's _start, and, via mcount()'s
 *	calls to mnewblock().  The blocks are tracked via a linked list 
 *	of block anchors, which each point to a block.
 *
 *
 *	An anchor points forward, backward and 'down' (to a block).
 *	A block has the profiling information, and consists of
 *	three regions: a header, a function call count array region,
 *	and an optional execution histogram region, as illustrated below.
 *
 *
 *		 "anchor"
 *		+========+
 *	prior<--|        |-->next anchor
 *	anchor	|        |
 *		+========+
 *		 |
 *		 |
 *		 V "block"
 *		+-----------+
 *		+  header   +
 *		+-----------+
 *		+           +
 *		+ fcn call  +	// data collected by mcount
 *		+  counts   +
 *		+  array    +
 *		+           +
 *		+-----------+
 *		+           +
 *		+ execution +	// data collected by system call,
 *		+ profile   +	// profil(2) (assumed ALWAYS specified
 *		+ histogram +	// by monitor()-caller; never specified
 *		+           +	// by mnewblock() ).
 *		+-----------+
 *
 *	The first time monitor() is called, it sets up the chain
 *	by allocating an anchor and initializing countbase/limit.
 *	Jes aboot EVERYONE assumes that they start out 0ed.
 *
 *	When a user (or _start from mcrt0) calls monitor(), they
 *	register a buffer which contains the third region (either with
 *	a meaningful size, or so short that profil-ing is being shut off).
 *	Mcount() parcels out the fcn call count entries from the
 *	current block, until they are exausted; then it calls mnewblock().
 *	Mnewbloc() allocates a block Without the third region, and
 *	links in a new associated anchor.  Each new mnewblock() or
 *	user block, is added to the list as it comes in.
 *
 *	When monitor() is called to close up shop, it writes out
 *	a summarizing header, ALL the fcn call counts from ALL
 *	the blocks, and the Last specified execution histogram
 *	(currently there is no neat way to accumulate that info).
 *	This preserves all call count information, even when
 *	new blocks are specified.
 *
 *	NB - no block passed to monitor() may be freed, until
 *	it is called to clean up!!!!
 *
 *	NB - Only monitor(), mcount() and mnewblock() may change
 *	countbase and countlimit!!!!
 */
#ifdef __STDC__
	#pragma weak monitor = _monitor
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <mon.h>
#define PROFDIR	"PROFDIR"


#ifdef DEBUG
	void dumpAnchors(), showArray();
#endif


char **___Argv = NULL; /* initialized to argv array by mcrt0 (if loaded) */

	/* countbase and countlimit are used to parcel out
	 * the pc,count cells from the current block one at 
	 * a time to each profiled function, the first time 
	 * that function is called.
	 * When countbase reaches countlimit, mcount() calls
	 * mnewblock() to link in a new block.
	 * Only monitor/mcount/mnewblock() should change these!!
	 */
char *countbase;	/* address of next pc,count cell to use in this block */
char *_countlimit;	/* address limit for cells (addr after last cell)  */


typedef struct anchor	ANCHOR;

struct anchor
{
	ANCHOR  *next, *prior;	/* forward, backward ptrs for list */
	struct hdr  *monBuffer;	/* 'down' ptr, to block */
	short  flags;		/* indicators - has histogram designation */

	int  histSize;		/* if has region3, this is size. */
};

#define HAS_HISTOGRAM	0x0001		/* this buffer has a histogram */


static ANCHOR 	*curAnchor = NULL;	/* addr of anchor for current block */
static ANCHOR    firstAnchor;		/* the first anchor to use - hopefully */
					/* the Only one needed */
					/* a speedup for most cases. */


static char mon_out[100];
static char progname[15];


#ifdef DEBUG
# define MNLISTMAX	(64)
	extern int _mcountcalls;
	extern int _mc__cells;
	extern int _mc__mnews;
	static int mnewblkcalls =  0;
	/* used by mnewblkcalls to track areas allocated - for debugging! */
	static ANCHOR *mnewlist[MNLISTMAX];

	int monitorCalls = 0;	/* #times monitor called */
	int monitorActive = 0;	/* 1-if active, 0-if inactive */
#endif

void
monitor(alowpc, ahighpc, buffer, bufsize, nfunc)
int	(*alowpc)(), (*ahighpc)(); /* boundaries of text to be monitored */
WORD	*buffer;	/* ptr to space for monitor data (WORDs) */
int	bufsize;	/* size of above space (in WORDs) */
int	nfunc;		/* max no. of functions whose calls are counted
			(default nfunc is 300 on PDP11, 600 on others) */
{
	int scale;
	long text;
	register char *s, *name = mon_out;
	struct hdr *hdrp;
	ANCHOR  *newanchp;
	int ssiz;
	char	*lowpc = (char *)alowpc;
	char	*highpc = (char *)ahighpc;

#ifdef DEBUG
	monitorCalls++; monitorActive=1;

	printf("\t***>\nentering monitor(): low/hipc=%x/%x (hex).\n", lowpc,highpc);
	printf("entering for %s-type call.\n",
		((lowpc==NULL) ? "EndCleanup" : "InitialSetup" )  );
	printf("number of calls to mcount,mnewblock so far, %d,%d\n",
		_mcountcalls, mnewblkcalls );
	printf("number of cells taken, calls to mnewblock, %d,%d\n", 
		_mc__cells, _mc__mnews );
	showArray();
	printf(" buffer %x, bufsize %d, nfunc %d\n", buffer, bufsize, nfunc );
#endif


	if (lowpc == NULL) {		/* true only at the end */
		if (curAnchor != NULL) { /* if anything was collected!.. */
			register pid_t pid;
			register int  n;

#ifdef DEBUG
			aNewReport();
#endif
			if (progname[0] != '\0') { /* finish constructing
						    "PROFDIR/pid.progname" */
			    /* set name to end of PROFDIR */
			    name = strrchr(mon_out, '\0');  
			    if ((pid = getpid()) <= 0) /* extra test just in case */
				pid = 1; /* getpid returns something inappropriate */
			    for (n = 10000; n > pid; n /= 10)
				; /* suppress leading zeros */
			    for ( ; ; n /= 10) {
				*name++ = pid/n + '0';
				if (n == 1)
				    break;
				pid %= n;
			    }
			    *name++ = '.';
			    (void)strcpy(name, progname);
			}
#ifdef DEBUG
	printf("write to file: %s\n", name);
	dumpAnchors();
#endif

			profil((char *)NULL, 0, 0, 0);
			if ( !writeBlocks() )
				perror(mon_out);
		}
#ifdef DEBUG
		monitorActive=0;
#endif
		return;
	}
	/*
	 * Ok - they want to submit a block for immediate use, for
	 *	function call count consumption, and execution profile
	 *	histogram computation.
	 * If the block fails sanity tests, just bag it.
	 * Next thing - get name to use. If PROFDIR is NULL, let's
	 *	get out now - they want No Profiling done.
	 *
	 * Otherwise:
	 * Set the block hdr cells.
	 * Get an anchor for the block, and link the anchor+block onto 
	 *	the end of the chain.
	 * Init the grabba-cell externs (countbase/limit) for this block.
	 * Finally, call profil and return.
	 */

	ssiz = (sizeof(struct hdr) + nfunc * sizeof(struct cnt))/sizeof(WORD);
	if (ssiz >= bufsize || lowpc >= highpc)
		return;

	if ((s = getenv(PROFDIR)) == NULL) /* PROFDIR not in environment */
		(void)strcpy(name, MON_OUT); /* use default "mon.out" */
	else if (*s == '\0') /* value of PROFDIR is NULL */
		return; /* no profiling on this run */
	else { /* set up mon_out and progname to construct
		  "PROFDIR/pid.progname" when done profiling */

		while (*s != '\0') /* copy PROFDIR value (path-prefix) */
			*name++ = *s++;
		*name++ = '/'; /* two slashes won't hurt */
		if (___Argv != NULL) /* mcrt0.s executed */
			if ((s = strrchr(___Argv[0], '/')) != NULL)
			    strcpy(progname, s + 1);
			else
			    strcpy(progname, ___Argv[0]);
	}


	hdrp = (struct hdr *)buffer;	/* initialize 1st region */
	hdrp->lpc = lowpc;
	hdrp->hpc = highpc;
	hdrp->nfns = nfunc;

					/* get an anchor for the block */
	newanchp =  (curAnchor==NULL) ?
		&firstAnchor  :
		(ANCHOR *) malloc( sizeof(ANCHOR) );

	if (newanchp == NULL)
	{
		perror("monitor");
		return;
	}

					/* link anchor+block into chain */
	newanchp->monBuffer = hdrp;		/* new, down. */
	newanchp->next  = NULL;			/* new, forward to NULL. */
	newanchp->prior = curAnchor;		/* new, backward. */
	if (curAnchor != NULL)
		curAnchor->next = newanchp;	/* old, forward to new. */
	newanchp->flags = HAS_HISTOGRAM;	/* note that it has a histgm area */

					/* got it - enable use by mcount() */
	countbase  = (char *)buffer + sizeof(struct hdr);
	_countlimit = countbase + (nfunc * sizeof(struct cnt));

						/* (set size of region 3) */
	newanchp->histSize = bufsize*sizeof(WORD) - (_countlimit-(char*)buffer);

#ifdef DEBUG
	printf("anchor %x:\n\tbuffer at %x\n\thistsize == %d (dec)\n", 
		newanchp, newanchp->monBuffer, newanchp->histSize);
	printf("countbase/limit: %x/%x\n", countbase, _countlimit );
#endif

					/* done w/regions 1 + 2: setup 3 */
					/* to activate profil processing. */

	buffer += ssiz;			/* move ptr past 2'nd region */
	bufsize -= ssiz;		/* no. WORDs in third region */
					/* no. WORDs of text */
	text = (highpc - lowpc + sizeof(WORD) - 1)/
			sizeof(WORD);
	/* scale is a 16 bit fixed point fraction with the decimal
	   point at the left */
	if (bufsize < text)  {
		/* make sure cast is done first! */
		double temp = (double)bufsize;
		scale = (temp * (long)0200000L) / text;
	} else  {
		/* scale must be less than 1 */
		scale = 0xffff;
	}
	bufsize *= sizeof(WORD);	/* bufsize into # bytes */
	ssiz = ssiz * sizeof(WORD) + bufsize;	/* size into # bytes */
	profil((char *)buffer, bufsize, (int)lowpc, scale);


	curAnchor = newanchp;		/* make latest addition, the cur anchor */

#ifdef DEBUG
		monitorActive=0;
#endif
}


	/* writeBlocks() - write accumulated profiling info, std fmt.
	 *
	 * This routine collects the function call counts, and the
	 * last specified profil buffer, and writes out one combined
	 * 'pseudo-block', as expected by current and former versions
	 * of prof.
	 */
static
int
writeBlocks()
{
	int fd;
	int ok;

	ANCHOR *ap;		/* temp anchor ptr */
	struct hdr *bp;		/* temp block ptr */
	struct hdr sum;		/* summary header (for 'pseudo' block) */

	ANCHOR *histp;		/* anchor with histogram to use */



	if ( (fd = creat(mon_out, 0666)) < 0 )
		return 0;

				/* this loop (1) computes # funct cts total */
				/*  (2) finds anchor of last block w/hist (histp) */
	histp=NULL;
	for(sum.nfns=0, ap=&firstAnchor; ap!=NULL; ap=ap->next)
	{
		sum.nfns += ap->monBuffer->nfns; /* accum num of cells */
		if( ap->flags & HAS_HISTOGRAM )
			histp=ap;		 /* remember lastone witha histgm */
	}


				/* copy pc range from effective histgm */
	sum.lpc = histp->monBuffer->lpc;
	sum.hpc = histp->monBuffer->hpc;

	ok = (write(fd, (char *)&sum, sizeof(sum)) == sizeof(sum)) ;

#ifdef DEBUG
	printf("lowpc, highpc = 0x%x, 0x%x - nfunc = %d\n", 
		sum.lpc, sum.hpc, sum.nfns );
#endif


	if (ok)			/* if the hdr went out ok.. */
	{
		unsigned int amt;
		char *p;

				/* write out the count arrays (region 2's) */
		for(ap=&firstAnchor; ok && ap!=NULL; ap=ap->next)
		{
			amt	= ap->monBuffer->nfns * sizeof(struct cnt);
			p	= (char *)ap->monBuffer + sizeof(struct hdr);

			ok = (write(fd, p, amt) == amt);
#ifdef DEBUG
			{int *pi; int i=0;
			printf("==>a count array, at addr %08x:\n", p);
			for(pi=(int *)p; amt>0; amt -= sizeof(int), pi++)
			{
				printf("  0x%08x", *pi);
				if( ++i == 4 )
				{ putchar('\n'); i=0; }
			}
			}
#endif
		}

				/* count arrays out; write out histgm area */
		if (ok)
		{
			p	= (char *)histp->monBuffer + sizeof(struct hdr) +
				  (histp->monBuffer->nfns * sizeof(struct cnt));
			amt	= histp->histSize;

			ok = (write(fd, p, amt) == amt);

#ifdef DEBUG
			{int *pi; int i=0;
			printf("==>the histgm area, at addr %08x:\n", p);
			for(pi=(int *)p; amt>0; amt -= sizeof(int), pi++)
			{
				printf("  0x%08x", *pi);
				if( ++i == 4 )
				{ putchar('\n'); i=0; }
			}
			}
#endif
		}
	}
	
	(void) close(fd);

	return( ok );	/* indicate success */
}




	/* mnewblock() - allocate and link in a new region1&2 block.
	 *
	 * This routine, called by mcount(), allocates a new block
	 * containing only regions 1 & 2 (hdr and fcn call count array),
	 * and an associated anchor (see header comments), inits the
	 * header (region 1) of the block, links the anchor into the
	 * list, and resets the countbase/limit pointers.
	 */
#define THISMANYFCNS	(MPROGS0*2)

		/* call malloc() to get an anchor & a regn1&2 block, together */
#define GETTHISMUCH	(sizeof(ANCHOR) + 	/* get an ANCHOR */  \
			 (sizeof(struct hdr) +	/* get Region 1 */   \
			  THISMANYFCNS*sizeof(struct cnt) /* Region 2 */  \
			 )			/* but No region 3 */\
			)

#ifdef DEBUG
	static int depth = 0;
#endif

void
_mnewblock()
{
	struct hdr  *hdrp;
	ANCHOR	    *newanchp;
	char *p;


#ifdef DEBUG
	if( ++depth > 1 )
		{
		puts("awwwk!\n"); exit();
		}

	mnewblkcalls++;		/* bump # of times called */
#endif


	countbase = NULL;	/* turn off profile data collection, */
				/* while we get space. Resetting base */
				/* below will turn it back 'on'. */


					/* get anchor And block, together */
	p = (char *)malloc( GETTHISMUCH );
	if (p == NULL)
	{
		perror("mnewblock");
		return;
	}

	newanchp = (ANCHOR *) p;
	hdrp = (struct hdr *)( p + sizeof(ANCHOR) );

					/* initialize 1st region to dflts */
	hdrp->lpc = 0;
	hdrp->hpc = 0;
	hdrp->nfns = THISMANYFCNS;

					/* link anchor+block into chain */
	newanchp->monBuffer = hdrp;		/* new, down. */
	newanchp->next  = NULL;			/* new, forward to NULL. */
	newanchp->prior = curAnchor;		/* new, backward. */
	if (curAnchor != NULL)
		curAnchor->next = newanchp;	/* old, forward to new. */
	newanchp->flags = 0;		/* note that it has NO histgm area */

					/* got it - enable use by mcount() */
	countbase  = (char *)hdrp + sizeof(struct hdr);
	_countlimit = countbase + (THISMANYFCNS * sizeof(struct cnt));

						/* (set size of region 3.. to 0) */
	newanchp->histSize = 0 ;

#ifdef DEBUG
	mnewlist[ mnewblkcalls-1 ] = newanchp;
#endif

	curAnchor = newanchp;		/* make latest addition, the cur anchor */
#ifdef DEBUG
	depth--;
#endif
}



#ifdef DEBUG

void
dumpAnchors()
{
	int nanch,histno;
	ANCHOR *ap,*histp;

	if( curAnchor==NULL )
	{
		printf("No anchors initialized (curAnchor is NULL).\n");
		return;
	}

	nanch=0;
	histno=0; histp=NULL;
	for( ap = &firstAnchor; ap!=NULL; ap = ap->next )
	{
		printf("anchor at %x, next is at %x.\n", ap, ap->next );
		nanch++;
		if( ap->flags & HAS_HISTOGRAM )
		{
			histno=nanch;
			histp=ap;
		}
	}
	printf("%d anchor(s), %dTH has last histogram.\n", nanch, histno);
}

void
showArray()
{
	int i;
	ANCHOR *p, *lastp;

	printf("\nShowing values of curAnchor set by mnewblock..\n");
	lastp= NULL;
	for(i=0;i<mnewblkcalls;i++)
	{
		if( mnewlist[i] == NULL )
			printf("\t<%dTH entry, curAnchor set to null(?).>\n",i);
		else
		{
		   printf("\tanch @ %08x: nx %08x pr %08x dn %08x fl %d\n", 
				mnewlist[i],	mnewlist[i]->next,
						mnewlist[i]->prior,
						mnewlist[i]->monBuffer,
				mnewlist[i]->flags );

			  /* if not right, go lastp->forward, and here->back */
			if(lastp==NULL ||  (
			     (lastp != mnewlist[i]->prior)||
			     (lastp->next != mnewlist[i] )  
					   )
			   )
			{
				printf("Current PR != last NX!! Dump chains.\n");
				printf("\ttrack from prior, forward...\n");
				for( p=lastp; p!=mnewlist[i] && p!=NULL; p=p->next)
				  printf(
				     "\t\t@ %08x nx %08x pr %08x dn %08x fl %d\n",
					p, p->next, p->prior, p->monBuffer, 
					p->flags );
				printf("\ttrack from here, backward..\n");
				for( p=mnewlist[i]; p!=lastp && p!=NULL; p=p->prior)
				  printf(
				     "\t\t@ %08x nx %08x pr %08x dn %08x fl %d\n",
					p, p->next, p->prior, p->monBuffer, 
					p->flags );
			}
		}
		lastp= mnewlist[i];
	}
}




static int aNewCount = 0;
struct nu
{
	char *b, *l, *a;
	short curcalls, curactivity;
};

struct nu aNewList[128];

aNewBase(awp)
char *awp;	/* pointer to fcn's a_word */
{
	extern char *countbase, *_countlimit;

	aNewList[aNewCount].b = countbase ;
	aNewList[aNewCount].l = _countlimit ;
	aNewList[aNewCount].a = awp ;
	aNewList[aNewCount].curcalls = monitorCalls ;
	aNewList[aNewCount].curactivity = monitorActive ;

	aNewCount++;

}


aNewReport()
{
	extern char *countbase, *_countlimit;
	char *t1;
	int i;

	t1 = countbase;
	countbase = 0;

	printf("count is %d..\n\t-    countbase, countlimit, callerRetAd, #MonCalls, MonStatus\n",
			aNewCount);
	for(i=0;i<aNewCount;i++)
	{
		char *bp, *lp, *ap;
		int cl, ac;

		bp=aNewList[i].b;
		lp=aNewList[i].l;
		ap=aNewList[i].a;
		cl=aNewList[i].curcalls;
		ac=aNewList[i].curactivity;
		printf("\t- %2d %08x %08x %08x %2d %s\n", i, bp, lp, ap,
			cl, (ac==0 ? "Inactive":"Active!!")  );
	}


	countbase = t1;
}

#endif
