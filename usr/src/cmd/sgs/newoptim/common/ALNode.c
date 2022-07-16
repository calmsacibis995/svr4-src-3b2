/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/ALNode.c	1.3"

#include	<stdio.h>
#include	<malloc.h>
#include	"defs.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"ALNodeType.h"

static ALN_Id global_list = NULL;

	void
DoAlias(list)		/* make aliased REGALs nonNAQs. */
register ALN_Id list;
{
 register AN_Id an_id;
 AN_GnaqType gty;

 for(; list != NULL; list = list->next){
	an_id = list->name;
	switch(gty=GetAdGnaqType(an_id)){
	case NAQ:	/* was it a REGAL? */
	case SNAQ:
	case SENAQ:
	case ENAQ:
		list->naqtype = gty;
		if(IsAdArg(an_id) || IsAdAuto(an_id))
			PutAdGnaqType(an_id, SV);
		else
			PutAdGnaqType(an_id, Other);
		endcase;
	default:	/* no, skip it. */
		endcase;
	}
 }
}

	void
DoExtAlias()
{
 extern void DoAlias();

 DoAlias(global_list);
}
	void
FreeAlias(list)
ALN_Id *list;
{
 register ALN_Id l;
 register ALN_Id n;

 l = *list;
 *list = NULL;
 while(l != NULL){
	n = l;
	l = l->next;
	Free(n);
 }
}

	boolean
IsAliased(list, an_id)
register ALN_Id list;
register AN_Id an_id;
{
 while(list != NULL){
	if(list->name == an_id)
		return(TRUE);
	list = list->next;
 }
 return(FALSE);
}
	void
PutAlias(list, an_id)
ALN_Id *list;
AN_Id an_id;
{
 extern boolean IsAliased();
 extern int errno;
 extern void fatal();
 ALN_Id newnode;

 if(!IsAliased(*list, an_id)){
	newnode = (ALN_Id) Malloc(sizeof(struct Alias));
	if(newnode == NULL)
		fatal("PutAliasExt: Malloc failed (%d).\n",errno);
	newnode->name = an_id;
	newnode->next = *list;
	*list = newnode;
 }
}

	void
PutExtAlias(an_id)
AN_Id an_id;
{
 extern void PutAlias();

 PutAlias(&global_list, an_id);
}
	void
UndoAlias(list)
register ALN_Id list;
{
 for(; list != NULL; list = list->next)
	switch(list->naqtype){
	case ENAQ:
	case SENAQ:
		PutAdGnaqType(list->name,list->naqtype);
		endcase;
	default:	/* ignore NAQs and SNAQs */
		endcase;
	}
}

