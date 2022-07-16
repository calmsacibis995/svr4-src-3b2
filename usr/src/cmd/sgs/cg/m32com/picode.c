/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:m32com/picode.c	1.3"
/*
 *	picode.c - rewrite the tree for NAME, ICON, and FCON nodes.
*/

#include "mfile2.h"

static NODE *picode(), *build_in(), *build_tn();

void
myreader(p) NODE *p;
{
	if (picflag) {
                NODE *q = picode(p);

                if (p->in.op == FREE) {
                	*p = *q;
			q->in.op = FREE;
		}
        }
}

/* Rewrite the tree for NAME, ICON, and FCON nodes. */
static NODE*
picode(p) NODE *p;
{
	register ty, o;
	NODE *q;

	if((o = p->in.op) == INIT ) return p;
	ty = optype( o );

	switch (ty) {
	case BITYPE:
		if (o == PLUS) {
			NODE * q = p->in.right;

			/* when it is an array, build the tree for the offset */
			if (   q->in.op == ICON
			    && q->in.name != (char *)0
			    && q->in.lval != 0
			    && q->in.rval == NI_GLOBAL
			) {
			    NODE * l;
			    l = build_in(PLUS, p->in.type, p->in.left,
					build_tn(NAME,q->in.type, q->in.name, 0, 0));
			    l->in.right->in.strat = PIC_GOT;
			    q->in.name = (char *)0;
			    p->in.left = l;
			    p->in.left->in.left = picode(p->in.left->in.left);
			    return p;
			}
		}

		p->in.right = picode(p->in.right);
		if ((o == CALL || o == STCALL) && p->in.left->in.op == ICON ) 
		{
			if ( p->in.left->tn.name != ((char *)0) )
			    p->in.left->tn.strat |= PIC_PLT;
			return p;
		}
#ifdef IN_LINE
		else if (o == INCALL && p->in.left->in.op == ICON )
		{
			if ( p->in.left->in.name != ((char *)0) )
			    p->in.left->tn.strat |= PIC_PLT;
			return p;
		}
#endif
		else
			p->in.left = picode(p->in.left);
		return p;
	case UTYPE:
		if ((o == UNARY CALL || o == UNARY STCALL) && p->in.left->in.op == ICON)
		{
			if ( p->in.left->in.name != ((char *)0) )
			     p->in.left->tn.strat |= PIC_PLT;
			return p;
		}
#ifdef IN_LINE
		else if (o == UNARY INCALL && p->in.left->in.op == ICON )
		{
			if ( p->in.left->in.name != ((char *)0) )
			     p->in.left->tn.strat |= PIC_PLT;
			return p;
		}
#endif
		else
			p->in.left = picode(p->in.left);
		return p;
	case LTYPE:
		switch (o) {

		case FCON:
			fcons(p);		/* in nail.c */
			return picode(p);

		case ICON:
		case NAME: {
			register TWORD t = p->in.type;
			register flag = p->tn.rval;

			if (p->tn.name == ((char *)0)) return p;

			q = build_tn(NAME, t, p->tn.name, 0, 0);
			if (flag & NI_GLOBAL)
                            q->tn.strat = PIC_GOT;
			else 
			    q->tn.strat = PIC_PC;
			    
			/* when it is an array, build the tree for the offset */
			if (p->tn.lval != 0) {
			    if (flag & NI_GLOBAL) {
				q->tn.type = TPOINT;
                                q = build_in(PLUS, TPOINT, q,
                                         build_tn(ICON, TINT, (char *)0, p->tn.lval, 0));
			    }
			    else 
				q->tn.lval = p->tn.lval;
                        }

                        if (o == NAME) { 
			    if (flag & NI_GLOBAL) {  
				q->tn.type = TPOINT;
                                q = build_in(STAR, t, q, NIL);
			    }
                        }
			else if (!(flag & NI_GLOBAL))
				q = build_in(UNARY AND, t, q, NIL);

                        if (p->tn.strat & VOLATILE) {
                                q->in.strat |= VOLATILE;
                        }

			p->in.op = FREE;
			return q;
			}
		}
		return p;
	}
	/* NOTREACHED */

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*
 * build an interior tree node
 */
static NODE *
build_in( op, type, left, right )
        int op;         /* operator     */
        TWORD type;     /* result type  */
        NODE * left;    /* left subtree */
        NODE * right;   /* right subtree*/
{
        register NODE *q = talloc();
        q->in.op = op;
        q->in.type = type;
        q->in.name = (char *)0;
        q->in.left = left;
        q->in.right = right;
        return q;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*
 * build a terminal tree node
 */
static NODE *
build_tn( op, type, name, lval, rval )
        int op;         /* operator     */
        TWORD type;     /* result type  */
        char * name;    /* symbolic part of constant */
        CONSZ  lval;    /* left part of value */
        int    rval;    /* right part of value*/
{
        register NODE *q = talloc();
        q->tn.op = op;
        q->tn.type = type;
        q->tn.name = name;
        q->tn.lval = lval;
        q->tn.rval = rval;
        return q;
}
