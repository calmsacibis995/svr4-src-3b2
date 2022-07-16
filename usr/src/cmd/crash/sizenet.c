/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:sizenet.c	1.7.4.2"
/*
 * This file contains code for the crash function size.  The RFS and
 * Streams tables and sizes are listed here to allow growth and not
 * overrun the compiler symbol table.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#define _KERNEL
#include <sys/stream.h>
#include <sys/strsubr.h>
#undef _KERNEL
#include <sys/list.h>
#include <sys/rf_messg.h>
#include <sys/rf_comm.h>
#include <sys/nserve.h>
#include <sys/rf_cirmgr.h> 
#include <sys/rf_adv.h>
#include "crash.h"


struct sizenetable {
	char *name;
	char *symbol;
	unsigned size;
};
struct sizenetable sizntab[] = {
	"datab","dblock",sizeof (struct datab),
	"dblk","dblock",sizeof (struct datab),
	"dblock","dblock",sizeof (struct datab),
	"gdp","gdp",sizeof (struct gdp), 
	"linkblk","linkblk",sizeof (struct linkblk),
	"mblk","mblock",sizeof (struct msgb),
	"mblock","mblock",sizeof (struct msgb),
	"msgb","mblock",sizeof (struct msgb),
	"queue","queue",sizeof (struct queue),
	"rcvd","rcvd",sizeof (struct rcvd),
	"sndd","sndd",sizeof (struct sndd),
	"sr_mount","sr_mount",sizeof (struct sr_mount),
	"resource","resource",sizeof (struct rf_resource),
	"stdata","streams",sizeof (struct stdata),
	"streams","streams",sizeof (struct stdata),
	NULL,NULL,NULL
};	


/* get size from size table */
unsigned
getsizenetab(name)
char *name;
{
	unsigned size = 0;
	struct sizenetable *st;

	for(st = sizntab; st->name; st++) {
		if(!(strcmp(st->name,name))) {
			size = st->size;
			break;
		}
	}
	return(size);
}

/* print size */
int
prsizenet(name)
char *name;
{
	struct sizenetable *st;
	int i;

	if(!strcmp("",name)) {
		for(st = sizntab,i = 0; st->name; st++,i++) {
			if(!(i & 3))
				fprintf(fp,"\n");
			fprintf(fp,"%-15s",st->name);
		}
		fprintf(fp,"\n");
	}
}

/* get symbol name and size */
int
getnetsym(name,symbol,size)
char *name;
char *symbol;
unsigned *size;
{
	struct sizenetable *st;

	for(st = sizntab; st->name; st++) 
		if(!(strcmp(st->name,name))) {
			strcpy(symbol,st->symbol);
			*size = st->size;
			break;
		}
}
