/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/lla_ppc.c	1.8"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/dir.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/pp_dep.h"
#include "sys/queue.h"
#include "sys/cmn_err.h"
#include "sys/cio_defs.h"
#include "sys/ppc_lla.h"
#include "sys/strlla_ppc.h"
#include "sys/stream.h"
#include "sys/strppc.h"
#include "sys/termio.h"
#include "sys/strtty.h"

/*
 * 3B2 PPC Kernel Driver (PORTS) LLA Interface Functions
 */

#define SIXTEENK	(16*1024)

#define IS_CEXPRESS(b)	(E_SEQBIT(CQEXPRESS(b)) == ncsbit[b])

extern int ncsbit[];		/* express entry sequence bit	*/
extern int nmaxsavexp;		/* Maximum number of express entries for savee */
extern RENTRY nsavee[];

extern int nsv_expq_load[];	/* load entry for saved express queue */
extern int nsv_expq_unload[];	/* unload entry for saved express queue */

extern void nlla_express();
extern paddr_t vtop();
extern int nlla_attn();
extern int splpp();
extern int splx();

extern char *npp_addr[];

int
nlla_sysgen( bid, sblk)
register short bid;
register SG_DBLK *sblk;
{
	register int i;
	extern unsigned char getvec();


	/*
	 * Complete the SYSGEN block by supplying the interrupt vector
	 * Convert the queue addresses from virtual to physical
	 */

	sblk->int_vect = getvec( (long)npp_addr[bid]);
	sblk->request = (long)vtop( (char *)sblk->request, NULL);
	sblk->complt = (long)vtop( (char *)sblk->complt, NULL);

	/*
	 * Initialize the data structures for this board:
	 *	Reset all CIO load/unload pointers to zero
	 */
	for ( i = 0; i < NUM_QUEUES; i++)
		RQPTRS(bid,i) = 0;

	CQPTRS(bid) = 0;
	E_SEQBIT(CQEXPRESS(bid)) = 0;
	ncsbit[bid] = 1;

	/*
	 * Have the board enter its "pre-sysgen" state. Note that this
	 * ASSUMES that any board reset, if necessary, has already been
	 * done
	 *
	 * Note the use of a "read" to trigger the interrupt, rather then a
	 * "write"
	 */
	i = *((char *)(npp_addr[bid] + 1));
	for ( i = 0; i < 10000; i++)
		;

	/*
	 * Put the pointer to the SYSGEN block in its appropriate place and
	 * notify the board
	 */
	*SG_PTR = (long)vtop( (char *)sblk, NULL);

	nlla_attn( bid);
	return( PASS);
}

int
nlla_attn( bid)
short bid;
{
	register int ignore;
/*
 * Send an "ATTENTION" interrupt (INT1) to a board
 */

/*
 * Note the use of a "read" to trigger the interrupt, rather then a
 * "write"
 */

	ignore = *((char *) (npp_addr[bid] + 3));
	ignore = ignore;	/* To satisfy lint only! */
	return( PASS);
}

int
nlla_reset( bid)
short bid;
{
	register int i;
/*
 * Perform a software RESET of a board
 */

/*
 * Note the use of a "cpu-hog" for loop, to give the board time to
 * set up its basic environment
 */

	i = *((char *)(npp_addr[bid] + 5));
	for ( i = 0; i < 10000; i++)
		;
	return( PASS);
}

int
nlla_cqueue( bid, eptr)
register short bid;
register CENTRY *eptr;
{
	register int i;
/*
 * Return a COMPLETION QUEUE entry for board 'bid'
 *
 ***** NOTE ***** NOTE ***** NOTE ***** NOTE ***** NOTE *****
 *
 * It is assumed that this function is being called ONLY from within
 * a high-priority interrupt handler
 *
 * For that reason, it does NOT alter the priority level
 *
 ***** NOTE ***** NOTE ***** NOTE ***** NOTE ***** NOTE *****
 */

	if ( IS_CEXPRESS(bid)) {
		/*
		 * An EXPRESS entry is returned before any of the
		 * regular entries
		 */
		*eptr = CQEXPRESS(bid);
		BSTATE(bid) &= ~EXPRESS;
		/*
		 * Toggle the interal sequence bit value, so that the
		 * next entry can be recognized
		 */
		ncsbit[bid] ^= 1;

		/*
		 * If an EXPRESS entry has been queued for output,
		 * send it now
		 */
		if ( nsv_expq_load[bid] != nsv_expq_unload[bid]) {
			BSTATE(bid) |= EXPRESS;

			RQEXPRESS(bid) = nsavee[nmaxsavexp*bid + nsv_expq_unload[bid]];
			nsv_expq_unload[bid]++;
			if ( nsv_expq_unload[bid] == nmaxsavexp)
				nsv_expq_unload[bid] = 0;

			i = *((char *)(npp_addr[bid] + 1));
		}
		return( PASS);
	}

	/*
	 * This function fails if there are not entries to give the user
	 */
	if ( CQLOAD(bid) == CQULOAD(bid))
		return( FAIL);

	i = CQULOAD(bid)/sizeof(CENTRY);

	*eptr = CQENTRY(bid,i);
	if ( ++i == CQSIZE)
		i = 0;
	CQULOAD(bid) = i*sizeof(CENTRY);

	/*
	 * A COMPLETION QUEUE entry from the PPC can mean that the
	 * corresponding REQUEST QUEUE has an available slot
	 */
	switch ( (int)EP_OPCODE(eptr)) {
	case CFW_CONFIG:
	case CFW_IREAD:
	case CFW_IWRITE:
	case CFW_WRITE:
		/*
		 * All CFW-xxx functions refer to the REQUEST QUEUE
		 * for port 0
		 */
		i = 0;
		break;
	case PPC_OPTIONS:
	case PPC_XMIT:
	case PPC_CONN:
	case PPC_DISC:
	case PPC_BRK:
		/*
		 * These PPC-xxx functions specify specifically the
		 * port they reference
		 */
		i = EP_SUBDEV(eptr);

		/*
		 * These functions pass a cblock address to the
		 * board.  Convert this address back from physical to
		 * virtual
		 */
		EP_ADDR(eptr) = phystokv(EP_ADDR(eptr));
		break;

	case PPC_RECV:
		/*
		 * The PPC-RECV function always refers to the one
		 * common "supply buffer" queue
		 */
		i = SUPPLYBUF;

		/*
		 * This function passes a cblock address to the
		 * board.  Convert this address back from physical to
		 * virtual
		 */
		EP_ADDR(eptr) = phystokv(EP_ADDR(eptr));
		break;
	default:
		/*
		 * These functions do not "relate" to any REQUEST QUEUE
		 * entry
		 */
		return( PASS);
	}

	PNBR(bid,i)--;
	return( PASS);
}

int
nlla_ldeuld( bid, pid)
short bid;
short pid;
{
	RQLOAD(bid,pid) = RQULOAD(bid,pid);
	PNBR(bid,pid) = 0;

	return( PASS);
}

int
nlla_xfree( bid, pid)
short bid;
short pid;
{
	/*
	 * Return TRUE iff the REQUEST queue for port (bid,pid) has an available
	 * entry in it
	 */

	return( PNBR(bid,pid) < NUM_ELEMENTS);
}

void
nlla_express( bid, bytcnt, seqbit, subdev, opcode, addr, appl)
register short bid;
unsigned short bytcnt;
char seqbit;
char subdev;
char opcode;
long addr;
char appl;
{
	int i;
	int oldspl;

	RENTRY eentry;


	/*
	 * Build the REQUEST QUEUE EXPRESS entry
	 */
	E_BYTCNT(eentry) = bytcnt;
	E_SEQBIT(eentry) = seqbit;
	E_SUBDEV(eentry) = subdev;
	E_CMDSTAT(eentry) = 0;
	E_OPCODE(eentry) = opcode;
	E_ADDR(eentry) = addr;
	E_APPL(eentry, 0) = appl;
	E_APPL(eentry, 1) = 0;
	E_APPL(eentry, 2) = 0;
	E_APPL(eentry, 3) = 0;
	/*
	 * Put an "EXPRESS" entry into the REQUEST QUEUE, and notify the
	 * board
	 */
	oldspl = splpp();
	if ( BSTATE(bid) & EXPRESS) {
		/*
		 * There is already an "active" EXPRESS entry
		 * Put the new entry on an internal queue, and return
		 * Watch out for queue overflow: Drop this entry
		 */
		if ( nsv_expq_load[bid] != nsv_expq_unload[bid]) {
			if ( nsv_expq_load[bid] == 0 )
				i = nmaxsavexp - 1;
			else
				i = nsv_expq_load[bid] - 1;

			if ( E_SUBDEV(eentry) == E_SUBDEV(nsavee[nmaxsavexp*bid + i])
			    && E_OPCODE(eentry) == E_OPCODE(nsavee[nmaxsavexp*bid + i])) {
				/*
				 * Duplicate express command for the
				 * same device
				 */
				splx( oldspl);
				return;
			}
		}

		i = nsv_expq_load[bid] + 1;
		if ( i == nmaxsavexp)
			i = 0;

		if ( i == nsv_expq_unload[bid])
			cmn_err( CE_WARN, "PORTS: EXPRESS QUEUE OVERLOAD: One entry lost, bid = %d, pid = %d\n", bid, subdev);
		else {
			nsavee[nmaxsavexp*bid + nsv_expq_load[bid]] = eentry;
			nsv_expq_load[bid] = i;
		}

		splx( oldspl);
		return;
	}

	BSTATE(bid) |= EXPRESS;
	RQEXPRESS(bid) = eentry;

	splx( oldspl);
	/*
	 * Send an "EXPRESS" interrupt (INT0) to a board
	 * Note the use of a "read" to trigger the interrupt, rather then a
	 * "write"
	 */

	i = *((char *)(npp_addr[bid] + 1));
}

int
nlla_regular( bid, attn, bytcnt, subdev, opcode, addr, appl0, appl1)
register short bid;
int attn;
unsigned short bytcnt;
register char subdev;
char opcode;
long addr;
char appl0;
char appl1;
{
	int i;
	int splevel;

	RENTRY eentry;


	/*
	 * Build the REQUEST entry
	 */
	E_BYTCNT(eentry) = bytcnt;
	E_SEQBIT(eentry) = 0;
	E_SUBDEV(eentry) = subdev;
	E_CMDSTAT(eentry) = 0;
	E_OPCODE(eentry) = opcode;
	E_ADDR(eentry) = addr;
	E_APPL(eentry, 0) = appl0;
	E_APPL(eentry, 1) = appl1;
	E_APPL(eentry, 2) = 0;
	E_APPL(eentry, 3) = 0;
	/*
	 * Make sure there is an available REQUEST QUEUE entry available
	 * Simply return a FAIL if there is not enough space
	 */
	if ( PNBR(bid,subdev) == ((subdev == SUPPLYBUF) ? (RQSIZE-1) : NUM_ELEMENTS))
		return( FAIL);
	/*
	 * Add the new entry to the queue.
	 * Note the priority level change, to define a non-interruptable
	 * critical region.
	 */
	splevel = splpp();
	i = RQLOAD(bid,subdev)/sizeof(RENTRY);
	RQENTRY(bid,subdev,i) = eentry;
	if ( ++i == RQSIZE)
		i = 0;
	RQLOAD(bid,subdev) = i*sizeof(RENTRY);
	PNBR(bid,subdev)++;
	splx( splevel);
	/*
	 * If requested, send an "ATTENTION" interrupt to a board
	 */
	if ( attn)
		nlla_attn( bid);
	return( PASS);
}
