/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nifg:cg/mau/stasg.c	1.13"

#include "mfile2.h"

/* Structure assignment and arguments.
**
** The idea here is to do a real good job of moving words
** from one place to another.  We assume that we are handed
** operands that are pointers to the left (and right, if
** assignment) side.  The shapes matched are those things we
** can add offsets to:  REG, (+ REG ICON), &STK, ICON (named).
** Other stuff gets resolved into a REG.
**
** The obvious approach is to try to get the "from" and "to"
** pointers into registers at run-time, then do a block-move.
** A slightly more elegant solution is to do specific moves
** in place of the block move when the number of words to be
** moved is small.
**
** This approach requires these steps:
**	1)  Determine whether the move is above threshold.
**	2a) For offset moving, generate the moves.
**	2b) For block move:  move operands to registers,
**		then do the move.  Unroll the loop by the
**		factor UNROLL, never do fewer than 2 loops.
*/

static void dooff(), movaw();
static void dooffreg();

/* register number corresponding to macro An */
#define	A(n)	(resc[n-1].tn.rval)
/* print-name for that register */
#define	RNAMES(n) (rnames[scratch[n]])

/* Number of words to move before block move pays off */
#ifndef	THRESH
#define	THRESH	4
#endif

/* Amount to unroll a loop. */
#ifndef	UNROLL
#define	UNROLL	6
#endif

#define WORDSIZE	(SZINT/SZCHAR)	/* size of word in bytes */


/* macro returning TRUE if node is a scratch register */

#define	istnode(p) (p->in.op == REG && istreg(p->tn.rval))


/* We need to find the scratch registers to use for from, to, and count
** in the face of sharing.  Use this array to keep track of what's what.
*/

static int scratch[NRGS];
/* These defines give indexes in "sharing". */
#define	R_TO	0	/* "to" */
#define	R_FROM	1	/* "from" */
#define	R_COUNT	2	/* word count */


/* This routine figures out the assigning of registers to scratch
** registers, accounting for sharing.
*/
static void
getscratch(node, share)
NODE * node;
int share;				/* one of LSHARE or RSHARE */
{
    NODE * sharenode;
    /* assume simple case:  no sharing occurs */

    scratch[R_TO] = A(1);
    scratch[R_FROM] = A(2);
    scratch[R_COUNT] = A(3);

    sharenode = (share == LSHARE) ? node->in.left : node->in.right;

    /* The shared node is an STAWD:  R, &STK, R+C, R-C.  Get to the
    ** register, if any.
    */
    if (sharenode->in.op == PLUS || sharenode->in.op == MINUS)
	sharenode = sharenode->in.left;

    if (istnode(sharenode)) {		/* possibly a shared node */
	int sharereg = sharenode->tn.rval;
	int i;

	for (i = 0; i < NRGS; ++i)
	    if (scratch[i] == sharereg)
		break;
	
	/* If we are, in fact, sharing a register, make sure it's the operand
	** we want.  If not, interchange with the "wrong" one.
	*/
	if (i <= R_COUNT) {

	    if (share == LSHARE && i != R_TO) {
		scratch[i] = scratch[R_TO];
		scratch[R_TO] = sharereg;
	    }
	    else {			/* RSHARE */
		scratch[i] = scratch[R_FROM];
		scratch[R_FROM] = sharereg;
	    }
	}
    }
    return;
}


/* Generate code to do structure copy for assignment.  The node must be a
** STASG node.  The left and right sides are assumed to be recognizable
** addressing shapes as described above.  Assume $3 is specified, so there
** are three scratch registers available.  Further assume that $> was set,
** which means the right side could be one of the scratch registers.
*/

void
stasg(node, flag)
NODE * node;
int flag;			/* non-zero if left side pointer retained */
{
    NODE * left = node->in.left;
    NODE * right = node->in.right;

    int size = node->stn.stsize/SZINT;	/* number of WORDS to copy */

    int vol_opnd = 0;

#ifdef VOL_SUPPORT
    if (node->in.strat & VOLATILE)
	vol_opnd = (VOL_OPND1 | VOL_OPND2);
#endif

    /* Do the block move if the move is "too long". */

    if (size <= THRESH) {
	/* generate direct moves.  On 3b20 use movd for pairs, movw for last */
	int i;
	for (i = 0; i < size; i++ ) {
	    emit_str("	movw	");
	    dooff(right, i*4);		/* generate "from" offset */
	    putc(',', outfile);
	    dooff(left, i*4);		/* "to" offset */
	    putc('\n', outfile);
#ifdef VOL_SUPPORT
	    if (vol_opnd)  emit_str("#VOL_OPND\t1,2\n");
#endif
	}
    }
    else {
	int lastlab;
	int loops = size / UNROLL;
	int adjdest = 0;		/* amount to adjust destination at end */

	if (loops < 2)
	    loops = 0;			/* don't bother doing 1 "loop" */
	else if (flag)
	    adjdest = size * WORDSIZE;

	getscratch(node, RSHARE);	/* assume $> set in template */

	/* Handle right side first, since it could share */
	movaw(right, R_FROM);

	/* Move the left side to a temp reg unless it's in one already and
	** we don't need the left side later.
	*/
	if ( !istnode(left))		/* Register variable*/
	    movaw(left, R_TO);
	else
	    scratch[R_TO] = left->tn.rval; /* allow this scratch reg to share */
	
	if (loops) {
	    size -= loops * UNROLL;	/* what's left after unrolling */

	    /* get count into register */
	    fprintf(outfile,"	movw	&%d,%s\n", loops, RNAMES(R_COUNT));

	    /* now do the block move */
	    lastlab = getlab();

	    fprintf(outfile, ".L%d:\n", lastlab);

	    /* Generate block of moves. */
	    dooffreg(UNROLL*WORDSIZE, vol_opnd);

	    fprintf(outfile, "\taddw2\t&%d,%s\n", UNROLL*WORDSIZE, RNAMES(R_FROM));
	    fprintf(outfile, "\taddw2\t&%d,%s\n", UNROLL*WORDSIZE, RNAMES(R_TO));
	    fprintf(outfile, "\tsubw2\t&1,%s\n\tjpos\t.L%d\n", RNAMES(R_COUNT), lastlab);
	}

	if (size)
	    dooffreg(size*WORDSIZE, vol_opnd);

	/* If we need the destination register, adjust the pointer. */
	if (adjdest)
	    fprintf(outfile, "\tsubw2\t&%d,%s\n", adjdest, RNAMES(R_TO));
    }
    return;
}


/* Structure argument.  This is similar to structure assignment.  The left
** side of the implied assignment is the stack.
*/

void
starg(node)
NODE * node;
{
    NODE * left = node->in.left;
    int size = node->stn.stsize/SZINT;	/* words to move */

    int vol_opnd = 0;

#ifdef VOL_SUPPORT
    if (node->in.strat & VOLATILE)
	vol_opnd = VOL_OPND1;
#endif

    /* Same thing here as with assignment: if too many words, use block move. */

    if (size <= THRESH) {
	int i;

	for (i = 0; i < size; i++ ) {
	    emit_str("	pushw	");
	    dooff(left, i*4);
	    putc('\n', outfile);
#ifdef VOL_SUPPORT
	    if (vol_opnd) emit_str("#VOL_OPND\t1\n");
#endif
	}
    }
    else {
	int lastlab;
	int loops = size / UNROLL;

	if (loops < 2)
	    loops = 0;			/* do at least twice */

	getscratch(node, LSHARE);	/* assume $> set in template */


	/* Move the left side to a temp reg unless it's in one already and
	** we don't need the left side later.
	*/
        movaw(left, R_FROM);
	
	/*Prepare destination register*/
	fprintf(outfile, "\tmovw\t%%sp,%s\n", RNAMES(R_TO));

	/*Leave enough room on the stack for the arg*/
	fprintf(outfile, "\taddw2\t&%d,%%sp\n", size*4);

	if (loops) {
	    size -= loops * UNROLL;	/* what's left after unrolling */

	    /* get count into register */
	    fprintf(outfile, "	movw	&%d,%s\n", loops, RNAMES(R_COUNT));

	    /* now do the block move */
	    lastlab = getlab();
	    fprintf(outfile, ".L%d:\n", lastlab);

	    dooffreg(UNROLL*WORDSIZE, vol_opnd);

	    fprintf(outfile, "\taddw2\t&%d,%s\n", UNROLL*WORDSIZE, RNAMES(R_FROM));
	    fprintf(outfile, "\taddw2\t&%d,%s\n", UNROLL*WORDSIZE, RNAMES(R_TO));
	    fprintf(outfile, "\tsubw2\t&1,%s\n\tjpos\t.L%d\n", RNAMES(R_COUNT), lastlab);
	}

	if (size)
	    dooffreg(size*WORDSIZE, vol_opnd);
    }
    return;
}


/* print address offset by some number */

static void
dooff(node, offset)
NODE * node;
int offset;
{
    NODE dummy;				/* to call adrput() with */

    dummy.in.op = STAR;
    dummy.in.left = node;

    switch( node->in.op ) {
    case ICON:
	node->in.op = NAME;		/* make look like external temporarily */
	goto stack;
    case UNARY AND:			/* &STK */
	node = node->in.left;
stack:
	/* fake different offset from name */
	node->tn.lval += offset;
	adrput(node);
	node->tn.lval -= offset;
	if (node->in.op == NAME)
	    node->in.op = ICON;		/* fix up OP again */
	break;
    
    /* *(&(R+/-C)) ==> R +/- C */
    case MINUS:
	node->in.right->tn.lval -= offset;
	adrput(&dummy);
	node->in.right->tn.lval += offset;
	break;
    case PLUS:
	node->in.right->tn.lval += offset;
	adrput(&dummy);
	node->in.right->tn.lval -= offset;
	break;

    case REG:
	/* register offset */
	fprintf(outfile, "%d(%s)", offset, rnames[node->tn.rval]);
	break;
	
    default:
	cerror("screwy dooff() case");
    }

    return;
}

/* Move a block of words, from offset 0 to offset off, from the
** FROM register to the TO.  Also produce the volatile flag if
** necessary.
*/

static void
dooffreg(off, isvol)
int off;
int isvol;
{
    int i;

    for (i = 0; i < off; i += WORDSIZE) {
	fprintf(outfile, "\tmovw\t%d(%s),%d(%s)\n", i, RNAMES(R_FROM), i, RNAMES(R_TO));
#if VOL_SUPPORT
	if (isvol)
	    emit_str("VOL_OPND\t1,2\n");
#endif
    }
    return;
}


/* Generate move to get operand (address) into a register.
** Since addw3 is faster than movaw, generate the former.
*/

static void
movaw(node, regno)
NODE * node;				/* "from" operand */
int regno;				/* scratch reg. #:  R_TO - R_COUNT */
{
    char * regstring = (char *) 0;
    char * offname = (char *) 0;
    int offsign = 0;			/* offset sign:  1 means negate */
    long offset = 0;

    switch( node->in.op ) {
    case MINUS:
	offsign = 1;
	/*FALLTHRU*/
    case PLUS:
	regstring = rnames[node->in.left->tn.rval];
	node = node->in.right;
	/*FALLTHRU*/
    case ICON:
	offname = node->tn.name;
	offset = node->tn.lval;
	if (offsign) offset = -offset;	/* negate offset if necessary */
	break;

    case REG:
	/* for REG, do direct move if different register number */
	if (node->tn.rval == scratch[regno])
	    return;
	regstring = rnames[node->tn.rval];
	break;

    
    case UNARY AND:			/* &STK */
	node = node->in.left;
	switch( node->tn.op ){
	case VAUTO:	regstring = "%fp"; break;
	case VPARAM:	regstring = "%ap"; break;
	default:	cerror("confused movaw()");
	}
	offset = node->tn.lval;
	break;

    default:
	cerror("confused movaw()");
    }

    /* 		4	2	1	*/
    /*	    offname  offset  regstring	*/
    switch( (offname ? 4 : 0) + (offset ? 2 : 0) + (regstring ? 1 : 0)) {
    case 0+0+0:	cerror("movaw() case 0?");  /*FALLTHRU*/
    case 0+0+1: fprintf(outfile, "	movw	%s", regstring); break;
    case 0+2+0: cerror("movaw() case 2?");  /*FALLTHRU*/
    case 0+2+1: fprintf(outfile, "	movaw	%ld(%s)", offset, regstring); break;
    case 4+0+0: fprintf(outfile, "	movw	&%s", offname); break;
    case 4+0+1: fprintf(outfile, "	movaw	%s(%s)", offname, regstring); break;
    case 4+2+0: fprintf(outfile, "	movw	&%s%s%ld",
		    offname, (offset > 0 ? "+" : ""), offset); break;
    case 4+2+1: fprintf(outfile, "	movaw	%s%s%ld(%s)",
		    offname, (offset > 0 ? "+" : ""), offset, regstring); break;
    }
    fprintf(outfile, ",%s\n", RNAMES(regno));
    return;
}
