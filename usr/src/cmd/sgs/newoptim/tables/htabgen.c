/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:tables/htabgen.c	1.3"

#include <stdio.h>
#include "optab.h"
#include "OpTabTypes.h"

#define CLIM (BUPPER + AUPPER-ALOWER-1 + OUPPER-OLOWER-1)/2

struct htabent htab[HTABSZ];

struct collide {
	unsigned int oi, hi;
	};
static struct collide ctab[ CLIM ];
static int ci = 0;

main()
{
	extern struct opent optab[];
	extern unsigned int hash();
	void do_hash();
	struct htabent *p;
	unsigned int hi;
	int i;

	/* primary hash. put initial hash elements into hash table.
	 * save collisions in ctab for now.
	 */
	for(i=0; i < BUPPER; ++i)
		do_hash(i);
	for(i=ALOWER+1; i < AUPPER; ++i)
		do_hash(i);
	for(i=OLOWER+1; i < OUPPER; ++i)
		do_hash(i);

	/* find empty slots in htab to put collisions, and thread them.
	 */
	hi = 0;
	for(i=0; i < ci; ++i){
		while(htab[hi].op != NULL)
			if(hi >= HTABSZ)
				fatal("out of hash table space");
			else
				hi++;
		htab[hi].op = &optab[ctab[i].oi];
		htab[hi].next = NULL;
		/* find end of current chain */
		for(p = &htab[ctab[i].hi]; p->next != NULL; p = p->next)
			;
		p->next = &htab[hi];
	}
	
	/* output hash table.
	 */
	printf("#include \"OpTabTypes.h\"\n\n");
	printf("extern struct opent optab[];\n\n");
	printf("struct htabent htab[HTABSZ] = {\n  ");
	for(i=0; i < HTABSZ; ++i){
		if(htab[i].op != NULL){
			printf("&optab[%d],", htab[i].op - optab);
			if(htab[i].next != NULL)
				printf("&htab[%d], ", htab[i].next - htab);
			else
				printf("0, ");
		}
		else
			printf("0,0, ");
		if((i & 03) == 03) /* i % 4 == 3 */
			printf("\n  ");
	}
	printf("\n};\n");
	exit(0);	/* need this for invocation inside makefile ! */
}

void
do_hash(i)
{
	extern struct opent optab[];
	extern unsigned int hash();
	int hi;

	hi = hash(optab[i].oname);
	if( htab[hi].op == NULL ){
		htab[hi].op = &optab[i];
		htab[hi].next = NULL;
	}
	else{
		ctab[ci].oi = i;
		ctab[ci].hi = hi;
		++ ci;
		if(ci >= CLIM)
			fatal("too many collisions");
	}
}

fatal(s)
char *s;
{
	fprintf(stderr,"%s\n",s);
	exit(1);
}

