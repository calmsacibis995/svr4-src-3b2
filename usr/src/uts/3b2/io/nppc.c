/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * PPC Peripheral (3B2) STREAMS PORT Controller Driver
 */
#ident	"@(#)kernel:io/nppc.c	1.28"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/iu.h"
#include "sys/file.h"
#include "sys/fcntl.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "sys/firmware.h"
#include "sys/devcode.h"
#include "sys/cmn_err.h"
#include "sys/strpump.h"
#include "sys/cio_defs.h"
#include "sys/pp_dep.h"
#include "sys/queue.h"	
#include "sys/strlla_ppc.h"
#include "sys/ppc_lla.h"
#include "sys/systm.h"
#include "sys/errno.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/strtty.h"
#include "sys/strppc.h"
#include "sys/debug.h"
#include "sys/eucioctl.h"
#include "sys/cred.h"
#include "sys/ddi.h"

extern int	npp_bnbr;
clock_t npp_tm_delay = 30; /* Time delay for terminal to settle after self test */
int nppdevflag = 0;	/* Indicates new interface for open and close to cunix */

/*
 * function return codes
 */
#define SUCCESS		0	/* successful return value */
#define	E_NO_LBUFS	1	/* No Large stream buffers */
#define	E_NO_SBUFS	2	/* No message header stream buffers */
#define	E_LLA_QUEUE	3	/* Lla queue out of buffers */
/*
 * Xflag modes
 */
#define X_ITIME		0x004		/* enable ITIME and IFS */
/*
 * raw mode parameters (1200-19200 baud)
 */
#define ITIME	2	/* intercharacter timer: 25-50 msec */
#define IFS	64	/* input field size: 64 bytes */

#define SYSG_TIME 1500  	/* timeout for sysgen  change*/
#define CIO_TIME  1500	  	/* timeout for LLC(CIO) commands to complete*/
#define CL_TIME   500	 	/* Time to complete output drain on port close*/
/*
 * intercharacter timer values
 * for low baud rates
 */
char	npptime[] = {
	0, 	/* 0 */
	18, 	/* 50 */
	12, 	/* 75 */
	9, 	/* 110 */
	7, 	/* 134.5 */
	6, 	/* 150 */
	5, 	/* 200 */
	3, 	/* 300 */
};

long	nversno;

char	nppc_speeds[16] = {
	0, 0, B75BPS, B110BPS, B134BPS, B150BPS,
	0, B300BPS, B600BPS, B1200BPS, B1800BPS,
	B2400BPS, B4800BPS, B9600BPS, B19200BPS, 0
};

SG_DBLK nsg_dblk;	/* Data block for sysgen. See queue.h */

int	pmpopen = 0;	/* global pump flag */
int	nversflag;
int	nloadflag;

#define INITCB		1	/* extra recv bufs allocated on first open */
#define	CB_PER_PPC	4	/* recv bufs per open TTY */

/*
 * Minor device definition
 */
#define TP(b,p)	&npp_tty[b*5+p] 	/* tp from board and port */

/*
 * nppsetparm slpind values
 */
#define	CANSLEEP	1	/* Sleep possible because of user context */
#define	DONTSLEEP	0	/* Sleep impossible because of lack of user context */

extern int drv_priv();
extern int drv_setparm();
extern int nlla_xfree();
extern int nlla_regular();
extern int nlla_ldeuld();
extern int nlla_reset();
extern void drv_usecwait();
extern time_t drv_hztousec();
extern int nlla_sysgen();
extern int nlla_cqueue();
extern int splx();
extern int splstr();
extern paddr_t kvtophys();
extern int bufawake();
extern void nlla_express();

STATIC int putioc();
STATIC int srvioc();
STATIC int nppsetparm();
STATIC void nttflush();
STATIC int nversreq();
STATIC int npp_ba_mctl();
STATIC int abuf_to_ppc();
STATIC int copy_to_lbuf();
STATIC int getoblk();

/************************
 * Streams Declarations *
 ************************/
STATIC int	nppopen(), nppclose(), nppoput(), npposrv(), nppisrv();

struct module_info ppc_info = { 99, "pp", 0, 512, 512, 256};

/* init: put	srv	open	close	admin stat */
STATIC struct qinit npprinit = { putq, nppisrv, nppopen, nppclose, NULL, &ppc_info, NULL};
STATIC struct qinit nppwinit = { nppoput, npposrv, NULL, NULL, NULL, &ppc_info, NULL};

struct streamtab nppinfo = { &npprinit, &nppwinit, NULL, NULL};


/*
 * open routine called through qinit structure entry
 */
STATIC int
nppopen( q, dev, oflag, sflag, credp)
register queue_t *q;
register dev_t *dev;
int	oflag;
int	sflag;
cred_t	*credp;
{
	register struct strtty *tp;
	register struct ppcboard *ppcp;
	register mblk_t *bp;

	struct stroptions *sop;

	mblk_t *mop;

	int	sx;
	int	nbufs;		/* number of buffers allocated on PPC */
	int	mdev;		/* the minor device number */
	int	i;		/* for loop variable */
	int	npp_ret;	/* return from nppsetparm() */
	int	lmaj;		/* lastmajor, used in itoemajor */ 


	/*
	 * sflag is 0 for a normal driver open
	 */
	if ( sflag)
		return( ENXIO);

	if ( geteminor( *dev) == A_MINOR) {
		if ( drv_priv( credp) != 0)
			return( EPERM);

		for ( i = 0, lmaj = -1; i < npp_bnbr; i++) {
			lmaj = (int)itoemajor( getmajor( *dev), lmaj);
			if ( lmaj == getemajor( *dev))
				break;
		}
		if ( i == npp_bnbr)
			return( ENXIO);
		else
			mdev = 5*i;

	} else
		mdev = getminor( *dev);

	ppcp = &npp_board[nppcbid[mdev]];

	/*
	 * Check for SYSGEN failure
	 */
	if ( ppcp->b_state & SYSGFAIL)
		return( ENXIO);

	sx = splstr();
	tp = &npp_tty[mdev]; 		/* get tty structure */
	q->q_ptr = (caddr_t)tp;
	(WR(q))->q_ptr = (caddr_t)tp;
	tp->t_rdqp = q;		   	/* store queue pointer */
	tp->t_dev = mdev;	     	/* store minor device number */

	if ( geteminor( *dev) == A_MINOR) {
		/*
		 * opening only for pump and sysgen 
		 * don't initialize anything.
		 */
		pmpopen = 1;
		splx( sx);
		return( 0);
	}

	/*
	 * get entry for device enable
	 */
	while (( !nlla_xfree( nppcbid[mdev], nppcpid[mdev])) || ( tp->t_state & TTIOW)) {
		tp->t_dstat |= WENTRY;
		tp->t_dstat |= OPDRAIN;
		if ( sleep( (caddr_t)&tp->t_cc[1], TTOPRI | PCATCH)) {
			tp->t_dstat &= ~( OPDRAIN | WENTRY);
			splx( sx);
			return( EINTR);
		}

		if ( !( tp->t_dstat & OPDRAIN)) {
			tp->t_dstat &= ~( OPDRAIN | WENTRY);
			splx( sx);
			return( ENXIO);
		}
	}
	tp->t_dstat &= ~OPDRAIN;

	/*
	 * If this port is not open, initialize its parameter
	 */
	if ( !( tp->t_state & ISOPEN)) {
		/*
		 * Pass back to the Stream Head the setup options
		 * to assign the hi and low water marks and 
		 * controlling terminal.
		 */
		while (( mop = allocb( sizeof( struct stroptions), BPRI_HI)) == NULL) {
			bufcall( sizeof( struct stroptions), BPRI_MED, bufawake, (long)tp);
			if ( sleep( (caddr_t)&tp->t_cc[3], TTIPRI | PCATCH)) {
				splx( sx);
				return( EINTR);
			}
		}
		mop->b_datap->db_type = M_SETOPTS;
		mop->b_wptr += sizeof(struct stroptions);
		sop = (struct stroptions *)mop->b_rptr;
		sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
		sop->so_hiwat = 512;
		sop->so_lowat = 256;
		putnext( q, mop);

		/*
		 * set tty flag values to RAW on open
		 */
		tp->t_line = 0;
		tp->t_iflag = 0;
		tp->t_lflag = 0;
		tp->t_cflag = SSPEED | CS8 | CREAD | HUPCL;
		tp->t_oflag = 0;
		bzero( (caddr_t)tp->t_cc, NCCS);

		/*
		 * Send CONNECT request to the PPC
		 */
		if ( nlla_regular( nppcbid[mdev], TRUE, 0, nppcpid[mdev], PPC_CONN, 0, 0, 0) != PASS) {
			splx( sx);
			return( ENXIO);
		}

		tp->t_ioctlp = NULL;
		splx( sx);
		if ( npp_ret = nppsetparm( tp, CANSLEEP))
			return( npp_ret);
	} else {
		/*
		 * Send CONNECT request to the PPC
		 */
		if ( nlla_regular( nppcbid[mdev], TRUE, 0, nppcpid[mdev], PPC_CONN, 0, 0, 0) != PASS) {
			splx( sx);
			return( ENXIO);
		}
	}

	sx = splstr();

	if ( !( oflag & (FNDELAY|FNONBLOCK))) {
		/*
		 * If "delay on open" is set, wait until "carrier on" interrupt
		 */
		while ( !( tp->t_state & CARR_ON)) { /* Wait for Carrier */ 
			tp->t_state |= WOPEN;
			if ( sleep( (caddr_t)&tp->t_line, TTIPRI | PCATCH)) {
				/*
				 * Free the message buffer that was
				 * allocated in nppsetparm()
				 */
				if ( tp->t_in.bu_bp != NULL) {
					freeb( tp->t_in.bu_bp);
					tp->t_in.bu_bp = NULL;
					tp->t_in.bu_ptr = NULL;
				}
				tp->t_state &= ~WOPEN;
				/*
				 * Flush and disconnect the PPC.
				 */
				nlla_regular( nppcbid[tp->t_dev], TRUE, 0, nppcpid[tp->t_dev], PPC_DISC, 0, (char)0, GR_DTR|GR_CREAD);
				splx( sx);
				return( EINTR);
			}
		}
	}
	/*
	 * set up to allocate receive buffers
	 */
	if ( !( tp->t_dstat & SUPBUF) && nppcpid[mdev] != CENTRONICS) {

		if ( ppcp->dcb == 0)	/* first open */
			ppcp->dcb += INITCB;
		ppcp->dcb += CB_PER_PPC;

		tp->t_dstat |= SUPBUF;

		tp->t_in.bu_ptr = NULL;
		tp->t_in.bu_cnt = 0;

		/*
		 * Allocate some recv blocks and send the buf ptrs to the PPC.
		 */
		nbufs = 0;
		while ( ppcp->dcb > ppcp->qcb) {
			if (( bp = allocb( PPBUFSIZ, BPRI_MED)) == NULL) {
				/*
				 * wait until a buffer becomes available
				 */
				tp->t_state |= TTIOW;
				bufcall( PPBUFSIZ, BPRI_MED, bufawake, (long)tp);
				if ( sleep( (caddr_t)&tp->t_cc[3], TTIPRI | PCATCH)) {
					tp->t_state &= ~TTIOW;
					tp->t_dstat &= ~SUPBUF;
						
					/*
					 * Flush and disconnect the PPC.
					 */
					nlla_regular( nppcbid[tp->t_dev], TRUE, 0, nppcpid[tp->t_dev], PPC_DISC, 0, (char)nbufs, GR_DTR|GR_CREAD);

					ppcp->dcb -= CB_PER_PPC;
					if ( ppcp->dcb == INITCB)
						ppcp->dcb -= INITCB;

					splx( sx);
					return( EINTR);
				}
			}
			if ( abuf_to_ppc( tp, bp) == E_LLA_QUEUE) {
				/*
				 * Hardware failure
				 */
				tp->t_dstat &= ~SUPBUF;
					
				/*
				 * Flush and disconnect the PPC.
				 */
				nlla_regular( nppcbid[tp->t_dev], TRUE, 0, nppcpid[tp->t_dev], PPC_DISC, 0, (char)nbufs, GR_DTR|GR_CREAD);

				ppcp->dcb -= CB_PER_PPC;
				if ( ppcp->dcb == INITCB)
					ppcp->dcb -= INITCB;
				splx( sx);
				return( EIO);
			}
		nbufs++;
		ppcp->qcb++;
		}
	}
	tp->t_state |= ISOPEN;  /* Mark TTY as open. */
	splx( sx);
	return( 0);
}


/*
 * Awakens an open waiting for receive buffers to become available.
 */
int
bufawake( tp)
register struct strtty *tp;
{
	tp->t_state &= ~(TTIOW);
	wakeup( (caddr_t) & tp->t_cc[3]);
}


/*
 * Close routine called through qinit structure entry.
 */
/* ARGSUSED */
STATIC int
nppclose( qp, oflag, credp)
register queue_t *qp;
register int oflag;
register cred_t	*credp;
{
	register struct strtty *tp;
	register struct ppcboard *ppcp;

	int	sx;
	int	eflush;

	char	dcode;


	/*
	 * Get the tty structure for the terminal
	 */
	tp = (struct strtty *)qp->q_ptr;

	if ( !( tp->t_state & ISOPEN))	/* See if it's closed already. */
		return( 0);

	pmpopen = 0;
	nversflag = 0;

	ppcp = &npp_board[nppcbid[tp->t_dev]];

	if ( !( oflag & (FNDELAY|FNONBLOCK)))
		/*
		 * Drain queued output to the user's terminal.
		 */
		while (( tp->t_state & CARR_ON) && ( WR( qp)->q_first != NULL
			|| tp->t_state & ( BUSY | WIOC) 
			|| tp->t_out.bu_bp != NULL)) {

			if ( getoblk( tp) == 0) {
				sx = splstr();
				tp->t_state |= TTIOW;
				if ( sleep( (caddr_t)&tp->t_oflag, PZERO + 1|PCATCH)) {
					tp->t_state &= ~TTIOW;
					splx( sx);
					break;
				}
				splx( sx);
			}
		}

	sx = splstr();
	if ( tp->t_state & CARR_ON) {
		if ( tp->t_dstat & OPDRAIN) {
			tp->t_dstat &= ~(OPDRAIN);
			wakeup( (caddr_t)&tp->t_cc[1]);
		}
	} else {
		if ( tp->t_state & BUSY)
			tp->t_state &= ~(BUSY);
		nlla_ldeuld( nppcbid[tp->t_dev], nppcpid[tp->t_dev]);
	}

	tp->t_state &= ~(ISOPEN);	/* TTY marked closed. */
	/*
	 * deallocate buffers
	 */
	if ( nppcpid[ tp->t_dev] != CENTRONICS) {
		if ( tp->t_lbuf != NULL) {
			freeb( tp->t_lbuf);
			tp->t_lbuf = NULL;
		}

		if ( tp->t_in.bu_bp != NULL) {
			freeb( tp->t_in.bu_bp);
			tp->t_in.bu_bp = NULL;
		}

		tp->t_in.bu_ptr = NULL;

		ppcp->dcb -= CB_PER_PPC;
		if ( ppcp->dcb <= INITCB)
			ppcp->dcb -= INITCB;

		if ( ppcp->dcb <= 0) 	/* this is the last close on the board */
			ppcp->dcb = 0;
	}

	if ( tp->t_ioctlp != NULL) {	/* dump the ioctl buffer */ 
		freemsg( tp->t_ioctlp);
		tp->t_ioctlp = NULL;
	}

	if ( tp->t_out.bu_bp != NULL) {	/* dump the xmit buffer */ 
		freemsg( tp->t_out.bu_bp);
		tp->t_out.bu_bp = NULL;
	}
	tp->t_out.bu_ptr = NULL;

	tp->t_dstat &= ~(SUPBUF);	/* No bufs requested for this TTY. */

	/*
	 * Send the final disconnect request to the PORTS board.
	 */
	while ( !( nlla_xfree( nppcbid[tp->t_dev], nppcpid[tp->t_dev]))) {
		tp->t_dstat |= WENTRY;
		if ( sleep( (caddr_t)&tp->t_cc[1], TTOPRI | PCATCH)) {
			tp->t_dstat &= ~WENTRY;
			splx( sx);
			return( 0);
		}
	}

	/*
	 * Calculate the number of message buffers to free out of the PPC. 
	 * For each buffer that is released by the PCC board a
	 * PPC_RECV will be received( see nppint()) by the host after the
	 * PPC board has received the Disconnect(PPC_DISC) from the host.
	 */
	if (( eflush = ppcp->qcb - ppcp->dcb) < 0)
		eflush = 0;
	tp->t_state &= ~(CARR_ON);
	if ( tp->t_cflag & HUPCL)	/* Hang up on disconnect. */
		dcode = (GR_DTR | GR_CREAD);
	else
		dcode = GR_CREAD;
	/*
	 * May need to unset the WOPEN flag since one
	 * process could have opened the ports line with
	 * O_NDELAY off and another with the flag on.
	 */
	 tp->t_state &= ~WOPEN;

	/*
	 * Flush and disconnect the PPC.
	 */
	nlla_regular( nppcbid[tp->t_dev], TRUE, 0, nppcpid[tp->t_dev], PPC_DISC, 0, (char)eflush, dcode);

	splx( sx);
	return( 0);
}

/****************************************
 *                                      *
 *  Output Routines.  Also see nppint() *
 *  later in the file.                  *
 *                                      *
 ****************************************/

/*
 * Write queue put procedure.
 */
STATIC int
nppoput( qp, bp)
register queue_t *qp;
register mblk_t *bp;
{
	register mblk_t	*msgbp;
	register struct strtty *tp;

	int	sx;


	tp = (struct strtty *)qp->q_ptr;

	switch ( bp->b_datap->db_type) {

	case M_DATA:
		/*
		 * Delay sending data to terminal if carrier not
		 * present
		 */
		if ( !( tp->t_state & CARR_ON)) {
			putq( qp, bp);
			return( 0);
		}
		while ( bp != NULL) {
			msgbp = unlinkb( bp);
			bp->b_cont = NULL;
			if (( bp->b_wptr - bp->b_rptr) <= 0)
				freeb( bp);
			else 
				putq( qp, bp);

			bp = msgbp;
		}
		if ( qp->q_first != NULL)
			getoblk( tp);
		break;

	case M_IOCTL:
		putioc( qp, bp);
		if ( qp->q_first != NULL)
			getoblk( tp);
		break;

	case M_FLUSH:
		sx = splstr();
		if ( *bp->b_rptr & FLUSHW) {
			nttflush( tp, FWRITE);
			*bp->b_rptr &= ~FLUSHW;
		}
		if ( *bp->b_rptr & FLUSHR) {
			nttflush( tp, FREAD);
			putnext( RD( qp), bp);
		} else
			freemsg( bp);

		splx( sx);
		break;

	case M_START:
		sx = splstr();
		nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_RES);
		tp->t_state &= ~(BUSY | TBLOCK | WIOC);
		splx( sx);
		freemsg( bp);
		getoblk( tp);
		break;

	case M_STOP:
		sx = splstr();
		nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_SUS);
		tp->t_state |= BUSY;
		splx( sx);
		freemsg( bp);
		break;

	case M_STARTI:
		sx = splstr();
		nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_UNB);
		tp->t_state &= ~(TBLOCK);
		splx( sx);
		freemsg( bp);
		break;

	case M_STOPI:
		sx = splstr();
		nlla_express( nppcbid[tp->t_dev], 0, 0,  nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_BLK);
		tp->t_state |= TBLOCK;
		splx( sx);
		freemsg( bp);
		break;

	case M_CTL:

		/*
		 * put message back on queue so that buffer allocation failures
		 * can be handled easier
		 */
		putbq( qp, bp);
		npp_ba_mctl( qp);
		break;

	default:
		freemsg( bp);
		break;
	}
	return( 0);
}


/*
 * Sent the block to be transmitted to the PPC board.
 */
STATIC int
npp_out( tp)
register struct strtty *tp;
{
	int	sx;


	sx = splstr();

	if ( tp->t_out.bu_bp == NULL) {	/* nothing to send */
		tp->t_state &= ~(BUSY);
		tp->t_out.bu_ptr = NULL;
		tp->t_out.bu_cnt = 0;
		splx( sx);
		return( 0);
	}

	if ( !( nlla_xfree( nppcbid[tp->t_dev], nppcpid[tp->t_dev]))) {
		tp->t_out.bu_ptr = NULL;
		tp->t_out.bu_cnt = 0;
		splx( sx);
		return( 0);
	}

	if (( tp->t_out.bu_bp->b_wptr - tp->t_out.bu_bp->b_rptr) > PPBUFSIZ)
		tp->t_dstat |= SPLITMSG;

	if (( nlla_regular( nppcbid[tp->t_dev], TRUE, (unsigned short)(tp->t_out.bu_cnt - 1), nppcpid[tp->t_dev], PPC_XMIT, (long)kvtophys( (caddr_t)tp->t_out.bu_ptr),
	     0, 0)) == PASS) {
		splx( sx);
		return( 1);	/* At least some of the buffer was sent. */
	} else {
		splx( sx);
		return( 0);
	}
}

/*
 * Pull a block off the write queue.
 * Called by Xmit Complete interrupt handler and bufcall() routines.
 */
STATIC int
getoblk( tp)
register struct strtty *tp;
{
	register mblk_t	*msgbp;
	register queue_t *qp;

	int	sx;

	sx = splstr();

	if ( tp->t_state & (BUSY | WIOC)) {
		splx(sx);
		return( 0);
	}

	qp = WR( tp->t_rdqp);
	if ( qp->q_first == NULL) {
		/*
		 * wakeup close write queue drain
		 */
		if ( tp->t_state & TTIOW) {
			tp->t_state &= ~(TTIOW);
			wakeup((caddr_t) &tp->t_oflag);
		}
		splx( sx);
		return( 1);	/* Nothing to send. */
	} else 
		msgbp = getq( qp);

	switch ( msgbp->b_datap->db_type) {
	case M_DATA:
		tp->t_state |= BUSY;
		tp->t_out.bu_bp = msgbp;
		tp->t_out.bu_ptr = msgbp->b_rptr;
		tp->t_out.bu_cnt = (ushort)(msgbp->b_wptr - msgbp->b_rptr);
		if ( tp->t_out.bu_cnt > PPBUFSIZ)
			tp->t_out.bu_cnt = PPBUFSIZ;
		if (( npp_out(tp) == 0) && ( tp->t_out.bu_bp != NULL)) {
			putbq( qp, msgbp);
			tp->t_out.bu_bp = NULL;
			tp->t_out.bu_ptr = NULL;
			tp->t_out.bu_cnt = 0;
			tp->t_state &= ~(BUSY);
		}
		break;

	case M_IOCTL:
		srvioc( qp, msgbp);
		break;

	default:
		freemsg( msgbp);
		break;
	}
	splx( sx);
	return( 1);
}


/*
 * ioctl handler for output PUT procedure
 */
STATIC int
putioc( qp, bp)
register queue_t *qp;
register mblk_t *bp;
{
	register struct strtty *tp;
	register struct iocblk *iocbp;
	register struct ppcboard *ppcp;
	register struct pumpst *pump;

	mblk_t	*msgbp;
	mblk_t	*msgb1p;

	int	sx;
	int	err_ret;

	char	seqbit;

	paddr_t temp;


	iocbp = (struct iocblk *)bp->b_rptr;
	tp = (struct strtty *)qp->q_ptr;

	ppcp = &npp_board[nppcbid[tp->t_dev]];

	if ( iocbp->ioc_count == TRANSPARENT) {
		/*
		 * The PORTS driver does not handle TRANSPARENT ioctls
		 */
		bp->b_datap->db_type = M_IOCNAK;
		iocbp->ioc_error = EINVAL;
		iocbp->ioc_count = 0;
		qreply( qp, bp);
		return( 0);
	}

	switch ( iocbp->ioc_cmd) {
	case TCSETSW:
	case TCSETSF:
	case TCSETAW:
	case TCSETAF:
	case TCSBRK:
		/*
		 * drain the output
		 */
		if ( qp->q_first != NULL || ( tp->t_state & (WIOC | BUSY))) {
			putq( qp, bp);
			break;
		}
		srvioc( qp, bp);
		break;

	case TCSETA: {	/* immediate parm set   */
		register struct termio *cb;

		if ( tp->t_state & (WIOC | BUSY)) {
			putbq( qp, bp); /* queue these for later */
			break;
		}
		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD(qp), bp);
			break; 
		}
		cb = (struct termio *)bp->b_cont->b_rptr;
		tp->t_iflag = (tp->t_iflag & 0xffff0000 | cb->c_iflag);
		tp->t_oflag = (tp->t_oflag & 0xffff0000 | cb->c_oflag);
		tp->t_cflag = (tp->t_cflag & 0xffff0000 | cb->c_cflag);
		tp->t_lflag = (tp->t_lflag & 0xffff0000 | cb->c_lflag);
		tp->t_line = cb->c_line;
		bcopy( (caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCC);

		tp->t_ioctlp = NULL;
		if ( err_ret = nppsetparm( tp, DONTSLEEP)) {
			bp->b_datap->db_type = M_IOCNAK;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			iocbp->ioc_count = 0;
			iocbp->ioc_error = err_ret;
			putnext( RD(qp), bp);
			break; 
		}

		bp->b_datap->db_type = M_IOCACK;
		msgb1p = unlinkb(bp);
		freeb( msgb1p);
		iocbp->ioc_count = 0;
		putnext( RD(qp), bp);
		break;
	}

	case TCSETS:{	/* immediate parm set   */

		register struct termios *cb;

		if ( tp->t_state & (WIOC | BUSY)) {
			putbq( qp, bp); /* queue these for later */
			break;
		}

		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD(qp), bp);
			break; 
		}
		cb = (struct termios *)bp->b_cont->b_rptr;

		tp->t_lflag = cb->c_lflag;
		tp->t_iflag = cb->c_iflag;
		tp->t_cflag = cb->c_cflag;
		tp->t_oflag = cb->c_oflag;
		tp->t_line = 0;
		bcopy( (caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCCS);

		tp->t_ioctlp = NULL;
		if ( err_ret = nppsetparm( tp, DONTSLEEP)) {
			bp->b_datap->db_type = M_IOCNAK;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			iocbp->ioc_count = 0;
			iocbp->ioc_error = err_ret;
			putnext( RD(qp), bp);
			break; 
		}

		bp->b_datap->db_type = M_IOCACK;
		msgb1p = unlinkb( bp);
		freeb( msgb1p);
		iocbp->ioc_count = 0;
		putnext( RD(qp), bp);
		break;
	}

	case TCGETA: {	/* immediate parm retrieve */
		register struct termio *cb;

		if (( msgbp = allocb( sizeof(struct termio), BPRI_MED)) == NULL) {
			putbq( qp, bp);
			bufcall( sizeof( struct termio), BPRI_MED, getoblk, (long)tp);
			return( 0);
		}
		if ( bp->b_cont)
			freemsg( bp->b_cont);	/* Bad user formatted I_STR */
		bp->b_cont = msgbp;
		cb = (struct termio *)bp->b_cont->b_rptr;

		cb->c_iflag = (ushort)tp->t_iflag;
		cb->c_oflag = (ushort)tp->t_oflag;
		cb->c_cflag = (ushort)tp->t_cflag;
		cb->c_lflag = (ushort)tp->t_lflag;
		cb->c_line = tp->t_line;
		bcopy( (caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCC);

		bp->b_cont->b_wptr += sizeof(struct termio);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termio);
		putnext( RD(qp), bp);
		break;
	}

	case TCGETS:{	/* immediate parm retrieve */
		register struct termios *cb;

		if ( bp->b_cont)
			freemsg( bp->b_cont);	/* Bad user formatted I_STR */

		if (( msgbp = allocb( sizeof(struct termios), BPRI_MED)) == NULL) {
			putbq( qp, bp);
			bufcall( sizeof( struct termios), BPRI_MED, getoblk, (long)tp);
			return( 0);
		}
		bp->b_cont = msgbp;
		cb = (struct termios *)bp->b_cont->b_rptr;

		cb->c_iflag = tp->t_iflag;
		cb->c_oflag = tp->t_oflag;
		cb->c_cflag = tp->t_cflag;
		cb->c_lflag = tp->t_lflag;
		bcopy( (caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCCS);

		bp->b_cont->b_wptr += sizeof( struct termios);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof( struct termios);
		putnext( RD(qp), bp);
		break;
	}

	case PPC_VERS:

		if ( bp->b_cont)
			freemsg( bp->b_cont);	/* Bad user formatted I_STR */
		/*
		 * Restrict command to superuser
		 */
		if ( iocbp->ioc_uid != 0) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EPERM;
			putnext( RD( qp), bp);
			return( 0);
		}
		if ( tp->t_state & (WIOC | BUSY)) {
			putbq( qp, bp); /* queue these for later */
			break;
		}

		if (( msgbp = allocb( 4, BPRI_HI)) == NULL) {
			putbq( qp, bp);
			bufcall( 4, BPRI_HI, getoblk, (long)tp);
			return( 0);
		}
		bp->b_cont = msgbp;
		if ( err_ret = nversreq( nppcbid[tp->t_dev])) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = err_ret;
			putnext( RD( qp), bp);
			return( 0);
		}
		tp->t_ioctlp = bp;
		/*
		 * Note: The ACK to this M_IOCTL is done in nppint()
		 */
		break;

	case P_RST:	/* reset PPC board */
		/*
		 * Fail Reset attempt if device is not open for pumping
		 */
		if ( !pmpopen) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
			putnext( RD( qp), bp);
			return( 0);
		}
		/*
		 * Restrict command to superuser
		 */
		if ( iocbp->ioc_uid != 0) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EPERM;
			putnext( RD( qp), bp);
			return( 0);
		}
		if ( npp_bnbr <= 0) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENXIO;
			putnext( RD(qp), bp);
			break;
		}
		sx = splstr();
		nlla_reset( nppcbid[tp->t_dev]);
		drv_usecwait( drv_hztousec( (clock_t)50)); /* Delay 50 ticks */
		nlla_reset( nppcbid[tp->t_dev]);

		ppcp->b_state = 0;
		tp->t_ioctlp = bp;
		nsg_dblk.request = (long)&R_QUEUE(nppcbid[tp->t_dev]);
		nsg_dblk.complt = (long)&C_QUEUE(nppcbid[tp->t_dev]);
		nsg_dblk.req_size = RQSIZE;
		nsg_dblk.comp_size = CQSIZE;
		nsg_dblk.no_rque  = NUM_QUEUES;
		nlla_sysgen( nppcbid[tp->t_dev], &nsg_dblk);
		splx( sx);
		/*
		 * Note: The ACK to this M_IOCTL is done in nppint()
		 */
		break;

	case P_SYSGEN:
		/*
		 * Fail Sysgen attempt if device is not open for pumping
		 */
		if ( !pmpopen) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
			putnext( RD( qp), bp);
			return( 0);
		}
		/*
		 * Restrict command to superuser
		 */
		if ( iocbp->ioc_uid != 0) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EPERM;
			putnext( RD( qp), bp);
			return( 0);
		}
		sx = splstr();
		ppcp->b_state = 0;
		tp->t_ioctlp = bp;
		nsg_dblk.request = (long)&R_QUEUE(nppcbid[tp->t_dev]);
		nsg_dblk.complt = (long)&C_QUEUE(nppcbid[tp->t_dev]);
		nsg_dblk.req_size = RQSIZE;
		nsg_dblk.comp_size = CQSIZE;
		nsg_dblk.no_rque  = NUM_QUEUES;
		nlla_sysgen( nppcbid[tp->t_dev], &nsg_dblk);
		splx( sx);
		/*
		 * Note: The ACK to this M_IOCTL is done in nppint()
		 */
		break;

	case P_LOAD:	/* download pump code */
		/*
		 * Fail Loading attempt if device is not open for pumping
		 */
		if ( !pmpopen) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
			putnext( RD( qp), bp);
			return( 0);
		}
		/*
		 * Restrict command to superuser
		 */
		if ( iocbp->ioc_uid != 0) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EPERM;
			putnext( RD( qp), bp);
			return( 0);
		}
		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD( qp), bp);
			break; 
		}
		sx = splstr();
		if ( ppcp->b_state & CIOTYPE) {
			splx( sx);
			return( 0);
		}
		pump = (struct pumpst *)bp->b_cont->b_rptr;
		if (( temp = kvtophys( (caddr_t)pump->data)) == (paddr_t)0) {
			bp->b_datap->db_type = M_IOCNAK;
			putnext( RD( qp), bp);
			splx( sx);
			break;
		}
		ppcp->b_state |= CIOTYPE;
		tp->t_ioctlp = bp;
		nloadflag = 1;
		seqbit = (((int)pump->address) >= (1024 * 16));
		drv_usecwait( drv_hztousec( 50)); /* Delay 50 ticks */
		nlla_express( nppcbid[tp->t_dev], (unsigned short)((pump->size) - 1), seqbit, ((int)pump->address) / 256, DLM, (long)temp, 0);
		splx( sx);
		/*
		 * Note: The ACK to this M_IOCTL is done in nppint()
		 */
		break;

	case P_FCF:
		/*
		 * Fail FCF attempt if device is not open for pumping
		 */
		if ( !pmpopen) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
			putnext( RD( qp), bp);
			return( 0);
		}
		/*
		 * Restrict command to superuser
		 */
		if ( iocbp->ioc_uid != 0) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EPERM;
			putnext( RD( qp), bp);
			return( 0);
		}
		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD(qp), bp);
			splx( sx);
			break; 
		}
		sx = splstr();
		if ( ppcp->b_state & CIOTYPE) {
			splx( sx);
			return( 0);
		}
		ppcp->b_state |= CIOTYPE;
		tp->t_ioctlp = bp;
		nloadflag = 1;
		drv_usecwait( drv_hztousec( 50)); /* delay 50 ticks */
		nlla_express( nppcbid[tp->t_dev], 0, 0, 0, FCF, *((long *)bp->b_cont->b_rptr), 0);
		splx( sx);
		/*
		 * Note: The ACK to this M_IOCTL is done in nppint()
		 */
		break;

	case EUC_MSAVE:		
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		putnext( RD( qp), bp);
		break;

	default:
		if (( iocbp->ioc_cmd & IOCTYPE) == LDIOC) {
			bp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0;
		} else {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
		}
		putnext( RD(qp), bp);
		break;
	}
	return( 0);
}


/*
 * Ioctl processor for queued ioctl messages.
 *
 */
STATIC int
srvioc( qp, bp)
register queue_t *qp;
register mblk_t *bp;
{
	register struct strtty *tp;
	register struct iocblk *iocbp;
	register mblk_t	*msgbp;
	register mblk_t	*msgb1p;

	int	err_ret;


	iocbp = (struct iocblk *)bp->b_rptr;
	tp = (struct strtty *)qp->q_ptr;

	if ( iocbp->ioc_count == TRANSPARENT) {
		/*
		 * The PORTS driver does not handle TRANSPARENT ioctls
		 */
		bp->b_datap->db_type = M_IOCNAK;
		iocbp->ioc_error = EINVAL;
		iocbp->ioc_count = 0;
		qreply( qp, bp);
		return( 0);
	}

	switch ( iocbp->ioc_cmd) {
		/*
		 * At this point output has drained
		 */
	case TCSETAF: {

		register struct termio *cb;


		nttflush( tp, FREAD);
		putctl1( RD(qp)->q_next, M_FLUSH, FLUSHR);

		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD(qp), bp);
			break; 
		}
		cb = (struct termio *)bp->b_cont->b_rptr;

		tp->t_iflag = (tp->t_iflag & 0xffff0000 | cb->c_iflag);
		tp->t_oflag = (tp->t_oflag & 0xffff0000 | cb->c_oflag);
		tp->t_cflag = (tp->t_cflag & 0xffff0000 | cb->c_cflag);
		tp->t_lflag = (tp->t_lflag & 0xffff0000 | cb->c_lflag);
		tp->t_line = cb->c_line;
		bcopy( (caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCC);

		if ( iocbp->ioc_cmd != TCSETA)
			tp->t_ioctlp = bp;
		else
			tp->t_ioctlp = NULL;

		if (( err_ret = nppsetparm( tp, DONTSLEEP)) == EAGAIN) {
			tp->t_ioctlp = NULL;
			putbq( qp, bp);
			bufcall( sizeof( Options), BPRI_MED, getoblk, (long)tp);
			return( 0);
		} else if ( err_ret != 0) {
			tp->t_ioctlp = NULL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = err_ret;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			putnext( RD( qp), bp);
		} else if ( iocbp->ioc_cmd == TCSETA) {
			/*
			 * ACK TCSETAs immediately, dont wait for the board
			 * to set the options
			 */
			bp->b_datap->db_type = M_IOCACK;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			iocbp->ioc_count = 0;
			putnext( RD( qp), bp);
		}
		/*
		 * ACK other ioctls when the board has set the option
		 * ( cf. nppint)
		 */
		break;
	}

	case TCSETAW:
	case TCSETA: {
		register struct termio *cb;

		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD(qp), bp);
			break; 
		}
		cb = (struct termio *)bp->b_cont->b_rptr;

		tp->t_iflag = (tp->t_iflag & 0xffff0000 | cb->c_iflag);
		tp->t_oflag = (tp->t_oflag & 0xffff0000 | cb->c_oflag);
		tp->t_cflag = (tp->t_cflag & 0xffff0000 | cb->c_cflag);
		tp->t_lflag = (tp->t_lflag & 0xffff0000 | cb->c_lflag);
		tp->t_line = cb->c_line;
		bcopy( (caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCC);

		if ( iocbp->ioc_cmd != TCSETA)
			tp->t_ioctlp = bp;
		else
			tp->t_ioctlp = NULL;

		if (( err_ret = nppsetparm( tp, DONTSLEEP)) == EAGAIN) {
			tp->t_ioctlp = NULL;
			putbq( qp, bp);
			bufcall( sizeof( Options), BPRI_MED, getoblk, (long)tp);
			return( 0);
		} else if ( err_ret != 0) {
			tp->t_ioctlp = NULL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = err_ret;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			putnext( RD( qp), bp);
		} else if ( iocbp->ioc_cmd == TCSETA) {
			/*
			 * ACK TCSETAs immediately, dont wait for the board
			 * to set the options
			 */
			bp->b_datap->db_type = M_IOCACK;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			iocbp->ioc_count = 0;
			putnext( RD( qp), bp);
		}
		/*
		 * ACK other ioctls when the board has set the option
		 * ( cf. nppint)
		 */
		break;
	}
 
	case TCSETSF: {

		register struct termios *cb;


		nttflush( tp, FREAD);
		putctl1( RD(qp)->q_next, M_FLUSH, FLUSHR);

		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD(qp), bp);
			break; 
		}
		cb = (struct termios *)bp->b_cont->b_rptr;

		tp->t_lflag = cb->c_lflag;
		tp->t_iflag = cb->c_iflag;
		tp->t_cflag = cb->c_cflag;
		tp->t_oflag = cb->c_oflag;
		tp->t_line = 0;
		bcopy( (caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCCS);

		if ( iocbp->ioc_cmd != TCSETS)
			tp->t_ioctlp = bp;
		else
			tp->t_ioctlp = NULL;

		if (( err_ret = nppsetparm( tp, DONTSLEEP)) == EAGAIN) {
			tp->t_ioctlp = NULL;
			putbq( qp, bp);
			bufcall( sizeof( Options), BPRI_MED, getoblk, (long)tp);
			return( 0);
		} else if ( err_ret != 0) {
			tp->t_ioctlp = NULL;
			bp->b_datap->db_type = M_IOCNAK;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			iocbp->ioc_count = 0;
			iocbp->ioc_error = err_ret;
			putnext( RD(qp), bp);
		} else if ( iocbp->ioc_cmd == TCSETS) {
			/*
			 * ACK TCSETSs immediately, dont wait for the board
			 * to set the options
			 */
			bp->b_datap->db_type = M_IOCACK;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			iocbp->ioc_count = 0;
			putnext( RD( qp), bp);
		}
		/*
		 * ACK othe ioctls when the board has set the option
		 * ( cf. nppint)
		 */
		break;
	}

	case TCSETSW:
	case TCSETS:{
		register struct termios *cb;

		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD(qp), bp);
			break; 
		}
		cb = (struct termios *)bp->b_cont->b_rptr;

		tp->t_lflag = cb->c_lflag;
		tp->t_iflag = cb->c_iflag;
		tp->t_cflag = cb->c_cflag;
		tp->t_oflag = cb->c_oflag;
		tp->t_line = 0;
		bcopy( (caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCCS);

		if ( iocbp->ioc_cmd != TCSETS)
			tp->t_ioctlp = bp;
		else
			tp->t_ioctlp = NULL;

		if (( err_ret = nppsetparm( tp, DONTSLEEP)) == EAGAIN) {
			tp->t_ioctlp = NULL;
			putbq( qp, bp);
			bufcall( sizeof( Options), BPRI_MED, getoblk, (long)tp);
			return( 0);
		} else if ( err_ret != 0) {
			tp->t_ioctlp = NULL;
			bp->b_datap->db_type = M_IOCNAK;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			iocbp->ioc_count = 0;
			iocbp->ioc_error = err_ret;
			putnext( RD(qp), bp);
		} else if ( iocbp->ioc_cmd == TCSETS) {
			/*
			 * ACK TCSETSs immediately, dont wait for the board
			 * to set the options
			 */
			bp->b_datap->db_type = M_IOCACK;
			msgb1p = unlinkb( bp);
			freeb( msgb1p);
			iocbp->ioc_count = 0;
			putnext( RD( qp), bp);
		}
		/*
		 * ACK othe ioctls when the board has set the option
		 * ( cf. nppint)
		 */
		break;
	}

	case TCGETA: {
		register struct termio *cb;

		if (( msgbp = allocb( sizeof( struct termio), BPRI_MED)) == NULL) {
			putbq( qp, bp);
			bufcall( sizeof(struct termio), BPRI_MED, getoblk, (long)tp);
			return( 0);
		}
		if ( bp->b_cont)
			freemsg( bp->b_cont);	/* Bad user formatted I_STR */
		bp->b_cont = msgbp;
		cb = (struct termio *)bp->b_cont->b_rptr;

		cb->c_iflag = (ushort)tp->t_iflag;
		cb->c_oflag = (ushort)tp->t_oflag;
		cb->c_cflag = (ushort)tp->t_cflag;
		cb->c_lflag = (ushort)tp->t_lflag;
		cb->c_line = tp->t_line;
		bcopy( (caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCC);

		bp->b_cont->b_wptr += sizeof( struct termio);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof( struct termio);
		putnext( RD( qp), bp);
		break;
	}

	case TCGETS: {
		register struct termios *cb;

		if (( msgbp = allocb( sizeof(struct termios), BPRI_MED)) == NULL) {
			putbq( qp, bp);
			bufcall( sizeof(struct termios), BPRI_MED, getoblk, (long)tp);
			return( 0);
		}
		if ( bp->b_cont)
			freemsg( bp->b_cont);	/* Bad user formatted I_STR */
		bp->b_cont = msgbp;
		cb = (struct termios *)bp->b_cont->b_rptr;

		cb->c_iflag = tp->t_iflag;
		cb->c_oflag = tp->t_oflag;
		cb->c_cflag = tp->t_cflag;
		cb->c_lflag = tp->t_lflag;
		bcopy( (caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCCS);

		bp->b_cont->b_wptr += sizeof(struct termios);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termios);
		putnext( RD(qp), bp);
		break;
	}

	case TCSBRK: {
		/*
		 * drain the output
		 */
		register int arg;


		if ( !bp->b_cont) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			putnext( RD(qp), bp);
			break; 
		}
		arg = *(int *)bp->b_cont->b_rptr;
		tp->t_state |= WIOC;
		tp->t_ioctlp = bp;
		nlla_regular( nppcbid[tp->t_dev], TRUE, 0, nppcpid[tp->t_dev], PPC_BRK, 0, arg, 0);
		/*
		 * ACK this ioctl when the board interrupts the host
		 */
		break;
	}

	default:
		bp->b_datap->db_type = M_IOCNAK;
		putnext( RD(qp), bp);
	}
	return( 0);
}


/*
 * Sets parameters on the PPC board.
 * Called from putioc() and srvioc().
 * NOTE: this routine can be called form routines that may or may not
 *	have a user context
 */
STATIC int
nppsetparm( tp, slpind)
register struct strtty *tp;
register int	slpind;	/* = CANSLEEP if sleep possible, = DONTSLEEP otherwise */
{
	register Options *optp;
	register mblk_t *bp;

	int	sx;


	sx = splstr();
	if (( tp->t_cflag & CBAUD) == 0) {
		/*
		 * If baud is 0 then by convention this is a disconnect
		 */
		tp->t_cflag |= HUPCL;
		nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_DIS);
		splx( sx);
		return( 0);
	}

	while ( !nlla_xfree( nppcbid[tp->t_dev], nppcpid[tp->t_dev])) {
		tp->t_dstat |= (SETOPT | WENTRY);
		/*
		 * Wait until the board is free to accept a change
		 * parameter setting
	 	 */
		if ( slpind == CANSLEEP) {
			if ( sleep( (caddr_t)&tp->t_cc[1], TTOPRI + 1 | PCATCH)) {
				/*
				 * Sleep called with PCATCH so that on user
				 * interupt of the open call the processor
				 * priority is reset before control is returned
				 * to the user
				 */
				
				tp->t_dstat &= ~(SETOPT | WENTRY);
				splx( sx);
				return( EINTR);
			}
		}
	}

	tp->t_state |= WIOC;
	if (( bp = allocb( sizeof( Options), BPRI_HI)) == NULL) {
		tp->t_state &= ~WIOC;
		splx( sx);
		return( EAGAIN);
	}
	optp = (Options *)bp->b_rptr;
	optp->line = 0; /* line discipline 0 */
	optp->ld.zero.iflag = tp->t_iflag;
	optp->ld.zero.oflag = tp->t_oflag;
	optp->ld.zero.cflag = tp->t_cflag;
	optp->ld.zero.lflag = X_ITIME;	/* no canonical processing on ppc */
	optp->ld.zero.vcount = tp->t_cc[VMIN];/* user defined char limit */
	optp->ld.zero.vtime = tp->t_cc[VTIME];/* user defined inter char timer*/

	/*
	 * convert baud rate to duart register specification
	 */
	optp->ld.zero.cflag &= ~CBAUD; /* zero baud bits */
	optp->ld.zero.cflag |= nppc_speeds[tp->t_cflag&CBAUD] & 0xF;
	if ((( optp->ld.zero.cflag) & 0xF) == 0) {
		freeb( bp);
		tp->t_state &= ~WIOC;
		splx( sx);
		return( EIO);
	}

	if ( nppcpid[tp->t_dev] == CENTRONICS)
		optp->ld.zero.cflag &= ~CREAD;

	if (( tp->t_cflag & CBAUD) <= B300)
		optp->ld.zero.itime = npptime[tp->t_cflag&CBAUD];
	else
		optp->ld.zero.itime = ITIME;

	/*
	 * send options to the ppc
	 */
	nlla_regular( (short)nppcbid[tp->t_dev], TRUE, (unsigned short)sizeof(Options),
		 (char)nppcpid[tp->t_dev], PPC_OPTIONS, (long)kvtophys((caddr_t)optp), 0, 0);

	tp->t_in.bu_bp = bp; /* use input blk ptr to store setopts block */
	splx( sx);
	return( 0);
}

/*****************************************
 *                                       *
 *  input routine.  see also: nppint()    *
 *                                       *
 *****************************************/

/*
 * Get a written buffer from the PPC and send it up.
 * Called from the RECV interrupt handler, and by
 * bufcall() after block allocation failures.
 */
STATIC int
npp_in( tp)
register struct strtty *tp;
{
	register mblk_t *bp;
	register mblk_t *mp;
	register struct ppcboard *ppcp;
	register int i;

	int sx;
	int fn_ret;


	if ( nppcpid[tp->t_dev] == CENTRONICS)
		/*
		 * Don't want input from printer
		 */
		return( SUCCESS);

	ppcp = &npp_board[nppcbid[tp->t_dev]];
	/*
	 * Determine which buffer data came in on
	 */
	sx = splstr();
	for ( i = 0; i < MAX_RBUF; i++)
		if (( ppcp->rbp[i].bp != NULL) && ( ppcp->rbp[i].sp == (long)tp->t_in.bu_ptr)) {
			bp = ppcp->rbp[i].bp;
			ppcp->rbp[i].bp = NULL;
			break;
		}

	tp->t_in.bu_ptr = NULL;

	/*
	 * Copy buffer just received to a large buffer
	 */
	if (( fn_ret = copy_to_lbuf( tp, bp)) != SUCCESS) {
		ppcp->rbp[i].bp = bp;
		splx( sx);
		return( fn_ret);
	}
	/*
	 * Dup the large buffer and queue the dup'ed message
	 */
	if (( mp = dupb( tp->t_lbuf)) == NULL) {
		/*
		 *  Have to undo the copy_to_lbuf
		 */
		tp->t_lbuf->b_wptr -= tp->t_in.bu_cnt;
		ppcp->rbp[i].bp = bp;
		splx( sx);
		return( E_NO_SBUFS);
	}
	
	/*
	 * Return the current buffer address to the ppc
	 */
	if (( fn_ret = abuf_to_ppc( tp, bp)) != SUCCESS) {
		tp->t_lbuf->b_wptr -= tp->t_in.bu_cnt;
		freeb( mp);
		splx( sx);
		return( fn_ret);
	}
	tp->t_lbuf->b_rptr = tp->t_lbuf->b_wptr;

	/*
	 * Queue the message. The message will be delivered
	 * upstream in the service procedure.
	 */
	putq( tp->t_rdqp, mp);

	splx( sx);
	return( SUCCESS);
}


/*************************************
 *                                   *
 *  interrupt handler                *
 *                                   *
 *************************************/

/*
 *  Interrupt Handler for the Ports and Hiports Boards
 */
void
nppint( bid)
register short	bid;
{
	register struct strtty *tp;
	register struct ppcboard *ppcp;
	register struct iocblk *iocbp;

	extern	int npp_in();

	CENTRY cqe;

	char	*cc_p;

	mblk_t	*mp;
	mblk_t	*bp;
	mblk_t	*msgb1p;

	int	ocnt;
	int 	i;
	int	npp_ret;
	int	sx;


	if ( bid >= npp_bnbr)
		return;

	ppcp = &npp_board[bid];	/* get completion queue entry */

	while ( nlla_cqueue( bid, &cqe) == PASS) {
		if ( ppcp->b_state & SYSGFAIL)
			return;

		tp = TP( bid, cqe.common.codes.bits.subdev);

		switch ( cqe.common.codes.bytes.opcode) {

		case PPC_RECV:

			drv_setparm( SYSRINT, 1);

			if (( tp->t_in.bu_ptr = (unsigned char *)cqe.common.addr) == NULL)
				break;

			if ( cqe.appl.pc[0] & (RC_FLU | RC_DSR) /* buffers flushed or disrupted */
				|| !( tp->t_state & ISOPEN)) {
				/*
				 * On a disconnect(cf nppclose()) the PPC board will send the
				 * host a PPC_RECV with the "disrupted buffer"(RC_DSR).
				 * Return buffer back to the ppc.
				 * The same applies if the stream is closed.
				 */
				sx = splstr();
				for ( i = 0; i < MAX_RBUF; i++) {
					if (( ppcp->rbp[i].bp != NULL) && ( ppcp->rbp[i].sp == (long)tp->t_in.bu_ptr)) {
						freeb( ppcp->rbp[i].bp);
						ppcp->rbp[i].bp = NULL;
						ppcp->qcb--;
						break;
					}
				}
				splx( sx);
				return;
			} else
				tp->t_in.bu_cnt = (ushort)cqe.common.codes.bytes.bytcnt + 1;

			if ( cqe.appl.pc[0] & RC_BRK) { /* BREAK received */
				if ( tp->t_state & CARR_ON)
					tp->t_state |= BUSY;

				putctl( tp->t_rdqp->q_next, M_BREAK);
				
				sx = splstr();
				/*
				 * Give the buffer back to the PPC
				 */
				for ( i = 0; i < MAX_RBUF; i++)
					if (( ppcp->rbp[i].bp != NULL) && ( ppcp->rbp[i].sp == (long)tp->t_in.bu_ptr)) {
						bp = ppcp->rbp[i].bp;
						ppcp->rbp[i].bp = NULL;
						if ( abuf_to_ppc( tp, bp) == E_LLA_QUEUE) {
							putctl1( tp->t_rdqp->q_next, M_ERROR, EIO);
							cmn_err( CE_WARN, "PORTS: Hardware error board %d, port %d. Can't get buffer\n", nppcbid[tp->t_dev], nppcpid[tp->t_dev]);
						}
						break;
					}

				splx( sx);
				nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_ABX);
				return;
			}
			if (( npp_ret = npp_in( tp)) != SUCCESS) {
				if ( npp_ret == E_LLA_QUEUE) {
					putctl1( tp->t_rdqp->q_next, M_ERROR, EIO);
					nttflush( tp, FREAD|FWRITE);
					cmn_err( CE_WARN, "PORTS: Hardware error board %d, port %d. Can't get buffer\n", nppcbid[tp->t_dev], nppcpid[tp->t_dev]);
				} else if ( npp_ret == E_NO_LBUFS) /* Ran out of "large" STREAMS buffers */
					bufcall( LARGEBUFSZ, BPRI_MED, npp_in, (long)tp);
				else  /* Ran out of STREAM message blocks */
					bufcall( sizeof( mblk_t), BPRI_MED, npp_in, (long)tp);
			}
			break;

		case PPC_XMIT:

			drv_setparm( SYSXINT, 1);
			if ( tp->t_dstat & WENTRY) {
				tp->t_dstat &= ~(SETOPT | WENTRY);
				wakeup((caddr_t) & tp->t_cc[1]);
			}

			if ( tp->t_dstat & SPLITMSG
				 && !(cqe.appl.pc[0] & (GC_DSR | GC_FLU))) {
				/*
				 * incomplete xmit last time
				 * don't get a new buffer
				 */
				ocnt = (int)tp->t_out.bu_cnt;
				tp->t_out.bu_bp->b_rptr += (char)tp->t_out.bu_cnt;
				tp->t_out.bu_ptr = tp->t_out.bu_ptr + ocnt;
				tp->t_out.bu_cnt = (ushort)(tp->t_out.bu_bp->b_wptr - tp->t_out.bu_bp->b_rptr);
				if ( tp->t_out.bu_cnt > PPBUFSIZ)
					tp->t_out.bu_cnt = PPBUFSIZ;
				else
					tp->t_dstat &= ~(SPLITMSG);

				if (( tp->t_state & WIOC) || ( npp_out( tp) == 0)) {
					tp->t_out.bu_bp->b_rptr -= ocnt;
					putbq( WR(tp->t_rdqp), tp->t_out.bu_bp);
					tp->t_out.bu_bp = NULL;
					tp->t_state &= ~(BUSY);
					getoblk( tp);	/* try again */
				}
			} else {
				if ( tp->t_out.bu_bp != NULL) {
					freeb( tp->t_out.bu_bp);  /* free the xmit buffer */
					tp->t_out.bu_bp = NULL;
				}
				tp->t_out.bu_ptr = NULL;
				tp->t_out.bu_cnt = 0;
				tp->t_state &= ~(BUSY);
				getoblk( tp);	/* get another block to send */
			}
			break;

		case PPC_ASYNC:
			switch ( cqe.appl.pc[0]) {

			case AC_BRK:
				drv_setparm( SYSRINT, 1);

				putctl( tp->t_rdqp->q_next, M_BREAK);
				if ( tp->t_state & CARR_ON)
					tp->t_state |= BUSY;

				nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_ABX);

				break;

			case AC_DIS:
				drv_setparm( SYSMINT, 1);

				tp->t_state &= ~(CARR_ON);
				if ( tp->t_state & ISOPEN)
					putctl( tp->t_rdqp->q_next, M_HANGUP);
				nttflush( tp, (FREAD | FWRITE));
				break;

			case AC_CON:
				drv_setparm( SYSMINT, 1);

				tp->t_state |= CARR_ON;
				if ( tp->t_state & WOPEN) {
					tp->t_state &= ~WOPEN;
					wakeup( (caddr_t) & tp->t_line);
				}
				if (( WR( tp->t_rdqp)->q_first) != NULL) {
					drv_usecwait( drv_hztousec( npp_tm_delay));	/* delay npp_tm_delay ticks */
					getoblk( tp);
				}
				break;

			case AC_FLU:
				if ( tp->t_dstat & WENTRY) {
					tp->t_dstat &= ~(SETOPT | WENTRY);
					wakeup( (caddr_t) & tp->t_cc[1]);
				}
				tp->t_state &= ~BUSY;
				if (( WR( tp->t_rdqp)->q_first) != NULL)
					getoblk( tp);
				break;
			}
			break;

		case PPC_OPTIONS:
			/*
			 * Ack original TCSET??? ioctl here, since the
			 * queues on the board are certainly drained
			 * at this point
			 */
			bp = tp->t_ioctlp;
			if ( bp != NULL) {
				tp->t_ioctlp = NULL;
				iocbp = (struct iocblk *)bp->b_rptr;
				msgb1p = unlinkb( bp);
				freeb( msgb1p);
				bp->b_datap->db_type = M_IOCACK;
				iocbp->ioc_count = 0;
				putnext( tp->t_rdqp, bp);
			}

			if (( cc_p = (char *)cqe.common.addr) != NULL)
				if (( tp->t_in.bu_bp != NULL) && (long)kvtophys( (caddr_t)tp->t_in.bu_bp->b_rptr) == (long)cc_p) {
					freeb( tp->t_in.bu_bp);
					tp->t_in.bu_bp = NULL;
				}

			if ( tp->t_dstat & WENTRY) {
				tp->t_dstat &= ~(SETOPT | WENTRY);
				wakeup( (caddr_t) & tp->t_cc[1]);
			}

			tp->t_state &= ~(WIOC);
			if (( WR( tp->t_rdqp)->q_first) != NULL)
				getoblk( tp);
			break;

		case PPC_DISC:
		case PPC_CONN:
			if ( tp->t_dstat & WENTRY) {
				tp->t_dstat &= ~(SETOPT | WENTRY);
				wakeup( (caddr_t) & tp->t_cc[1]);
			}
			if (( WR( tp->t_rdqp)->q_first) != NULL)
				getoblk( tp);
			break;

		case PPC_DEVICE:
			/*
			 * DTR drop ioctl request ACK'ed here
			 */
			bp = tp->t_ioctlp;
			if ( bp != NULL) {
				tp->t_ioctlp = NULL;
				iocbp = (struct iocblk *)bp->b_rptr;
				msgb1p = unlinkb( bp);
				freeb( msgb1p);
				bp->b_datap->db_type = M_IOCACK;
				iocbp->ioc_count = 0;
				putnext( tp->t_rdqp, bp);
			}
			break;

		case PPC_BRK:
			tp->t_state &= ~(WIOC);
			/*
			 * Ack original TCSBRK ioctl here, since the
			 * queues on the board are certainly drained
			 * at this point
			 */
			bp = tp->t_ioctlp;
			if ( bp != NULL) {
				iocbp = (struct iocblk *)bp->b_rptr;
				msgb1p = unlinkb( bp);
				freeb( msgb1p);
				bp->b_datap->db_type = M_IOCACK;
				iocbp->ioc_count = 0;
				putnext( tp->t_rdqp, bp);
				tp->t_ioctlp = NULL;
			}

			if (( WR( tp->t_rdqp)->q_first) != NULL)
				getoblk( tp);
			break;

		case SYSGEN:
			ppcp->b_state |= ISSYSGEN;
			/*
			 * tp reassigned because of SYSGEN problem
			 */
			tp = TP( bid, 0);
			if ( pmpopen && tp->t_ioctlp) {
				mp = tp->t_ioctlp;
				if ( mp->b_datap->db_type == M_IOCTL) {
					if ( mp->b_cont) {
						freemsg( mp->b_cont);
						mp->b_cont = NULL;
					}
					((struct iocblk *)mp->b_rptr)->ioc_count = 0;
					mp->b_datap->db_type = M_IOCACK;
					putq( tp->t_rdqp, mp);
					tp->t_ioctlp = NULL;
				}
			}
			break;

		case FAULT:
		case QFAULT:
		case NORMAL:

			if ( pmpopen && nloadflag) {
				tp = TP( bid, 0);
				mp = tp->t_ioctlp;
				if ( mp->b_datap->db_type == M_IOCTL) {
					if ( mp->b_cont) {
						freemsg( mp->b_cont);
						mp->b_cont = NULL;
					}
					((struct iocblk *)mp->b_rptr)->ioc_count = 0;
					mp->b_datap->db_type = M_IOCACK;
					putq( tp->t_rdqp, mp);
					tp->t_ioctlp = NULL;
					nloadflag = 0;
				}
			}
			if ( E_OPCODE(cqe) == QFAULT &&  !(E_ADDR(cqe) == (long) kvtophys( (caddr_t)&nversno) || E_ADDR(cqe) == (long)&nversno))
				/*
				 * QFAULT on PPC_VERS request means the ROMware
				 * doesn't support version number, default value
				 * should be used.  E_ADDR field is checked for
				 * PPC_VERS request instead of E_APPL(the opcode)
				 * because somehow E_APPL doesn't return the
				 * expected opcode(maybe a bug in CIO or ROMcode)
				 */
				cmn_err( CE_WARN, "PORTS: QFAULT - opcode= %d, board = %d, \n, subdev = %d, bytecnt = %d, buff address = %x\n\n",
					E_APPL(cqe, 0), nppcbid[tp->t_dev], cqe.common.codes.bytes.subdev, E_BYTCNT(cqe), E_ADDR(cqe));
			if ( E_OPCODE( cqe) == FAULT)
				cmn_err(CE_WARN, "PORTS: FAULT - opcode= %d, board = %d, \n, subdev = %d, bytecnt = %d, buff address = %x, \n\n",
				      E_APPL(cqe, 0), nppcbid[tp->t_dev], cqe.common.codes.bytes.subdev, E_BYTCNT(cqe), E_ADDR(cqe));
			if ( !( ppcp->b_state & CIOTYPE))
				break;
			ppcp->b_state &= ~CIOTYPE;
			tp->t_state &= ~WIOC;
			ppcp->retcode = cqe.common.codes.bytes.opcode;
			if ( pmpopen && nversflag) {
				mp = tp->t_ioctlp;
				iocbp = (struct iocblk *)mp->b_rptr;
				if ( mp->b_datap->db_type == M_IOCTL) {
					mp->b_datap->db_type = M_IOCACK;
					if ( ppcp->retcode != NORMAL)
						nversno = DEFAULTVER;
					*(int *)mp->b_cont->b_rptr = nversno;
					iocbp->ioc_count = 4;
					mp->b_cont->b_wptr += 4;
					nversflag = 0;
					putq( tp->t_rdqp, mp);
					tp->t_ioctlp = NULL;
				}
			}
			wakeup( (caddr_t)&ppcp->qcb);
			getoblk( tp);
			break;

		default:
			cmn_err( CE_WARN, "PORTS: unknown completion code: %d\n", cqe.common.codes.bytes.opcode);
			break;
		}
	}
}


/****************************************
 *                                      *
 *  miscellaneous device routines       *
 *                                      *
 ****************************************/

/*
 * flush TTY queues
 */
STATIC void
nttflush( tp, cmd)
register struct strtty *tp;
register int cmd;
{
	int	sx;


	sx = splstr();

	if ( cmd & FWRITE) {
		/*
		 * Discard all messages on the output queue.
		 */
		flushq( WR( tp->t_rdqp), FLUSHDATA);

		/*
		 * Dump the current xmit buffer
		 */
		if (( tp->t_out.bu_bp != NULL) && ( tp->t_dstat & SPLITMSG))
			tp->t_dstat &= ~(SPLITMSG);

		if ( tp->t_out.bu_bp != NULL) {
			freemsg( tp->t_out.bu_bp);
			tp->t_out.bu_bp = NULL;
		}

		tp->t_state &= ~(BUSY);
		/*
		 * Awaken the close waiting for output to drain
		 */
		if ( tp->t_state & TTIOW) {
			tp->t_state &= ~(TTIOW);
			wakeup((caddr_t) &tp->t_oflag);
		}

		/*
		 * Abort transmission on the PPC
		 */
		nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_ABX);
	}
	if ( cmd & FREAD) {
		/*
		 * Flush the input queue
		 */
		flushq( tp->t_rdqp, FLUSHDATA);

		/*
		 * Abort reception on the PPC
		 */
		nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_ABR);

		if ( tp->t_state & TBLOCK) {
			tp->t_state &= ~(TBLOCK);
			tp->t_state &= ~(WIOC);
			tp->t_state &= ~(BUSY);
			nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_UNB);
		}
	}
	splx( sx);
}


/*
 * This is the "start" routine called by the "start" table
 * when the device in initialized, and before the pump code
 * is downloaded.
 */
int
nppstart()
{
	register short	i;
	register short	j;
	register int	sx;
	register char	*p;


	sx = splstr();
	for ( i = 0; i < npp_bnbr; i++) {
		ncsbit[i] = 1;
		for ( p = (char *)&npp_board[i]; p < ((char *)&npp_board[i] + sizeof( npp_board[i])); p++)
			*p =  0;

		for ( j = 0; j < 5; j++) {
			nppcbid[(i*5)+j] = i;
			nppcpid[(i*5)+j] = j;
		}
	}
	splx( sx);
	return( 0);
}


/*
 * This is the "powerfail" routine called from the "powerfail" table
 * when the system has lost power and is crashing.
 */
int
nppclr()
{
	register int	bid;
	register int	pid;
	register int	sx;


	/*
	 * The system has detected a power failure, and is about to go down:
	 *
	 *	1. Send a special notice to the firmware
	 *	2. Mark all boards as down, so as to fail any further attempts
	 *		to reference them
	 *	3. Wake up any processes sleeping very deeply
	 */
	sx = splstr();
	for ( bid = 0; bid < npp_bnbr; bid++) {
		nlla_express( bid, 0, 0, 0, PPC_CLR, 0L, 0);
		npp_board[bid].b_state = SYSGFAIL;
		wakeup( (caddr_t)&npp_board[bid]);

		for ( pid = 0; pid < 5; pid++)
			wakeup( (caddr_t)&npp_board[bid].qcb);
	}
	splx( sx);
	return( 0);
}

STATIC int
nversreq( bid)
short	bid;
{
	register struct ppcboard *tb;
	register struct strtty *tp;


	tb = &npp_board[bid];
	tp = TP( bid, 0);
	if ( tb->b_state & CIOTYPE) {
		/* 
		 * there is a CIO type command already in process
		 */
		return( EIO);
	}
	tb->b_state |= CIOTYPE;
	tp->t_state |= WIOC;
	nversflag = 1;
	/*
   	 * PPC_VERS requests go by way of express queue
	 */
	nlla_express( bid, 0, 0 , 0, VERS, (long)kvtophys( (caddr_t)&nversno), 0);
	return( 0);
}

/*
 * Read service procedure.  Pass everything upstream.
 */
STATIC int
nppisrv( qp)
register queue_t *qp;
{
	register mblk_t *mp;
	register struct strtty *tp;

	int	sx;


	tp = (struct strtty *)qp->q_ptr;
	if (( tp->t_iflag & IXOFF) && ( tp->t_state & TBLOCK)) {
		sx = splstr();
		nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_UNB);
		tp->t_state &= ~(TBLOCK);
		splx(sx);
	}
	
	while (( mp = getq( qp)) != NULL) {
		/*
		 * Note: only put back non-prioity messages
		 *	send up all priority messages immediately these
		 *	cannot be queued
		 */
		if ((( mp->b_datap->db_type & QPCTL) == QNORM) && ( canput( qp->q_next) == 0)) {
			/*
			 * Should block the terminal from sending any more
			 * characters at this time. 
			 * Set state to T_BLOCK and block out transmission.
			 * this procedure will be back enabled when
			 * the module upstream executes a getq().
			 */
			putbq( qp, mp);
			if ( tp->t_iflag & IXOFF) {
				sx = splstr();
				nlla_express( nppcbid[tp->t_dev], 0, 0, nppcpid[tp->t_dev], PPC_DEVICE, 0L, DR_BLK);
				tp->t_state |= TBLOCK;
				splx( sx);
			}
			return( 0);
		} else 
			putnext( qp, mp);
	}
	return( 0);
}
/*
 * Write service procedure. For flow control. 
 */
/* ARGSUSED */
STATIC int
npposrv( q)
register queue_t *q;
{
	return( 0);
}

STATIC int
npp_ba_mctl( qp)
register queue_t *qp;
{
	register struct iocblk *qryp;
	register mblk_t	*msgb1p;
	register mblk_t	*bp;
	register struct termios *cb;

	
	if ( qp->q_first == NULL)
		return( 1);	/* Should not happen */
	else 
		bp = getq( qp);

	if ( bp->b_datap->db_type != M_CTL) {
		/*
		 * This functions handles M_CTLs only
		 */
		putbq( qp, bp);
		return( 1);
	}
	/*
	 * line discipline wants to know who is doing the
	 * cannonicalization
	 */
	if (( bp->b_wptr - bp->b_rptr) != sizeof( struct iocblk)) {
		cmn_err( CE_NOTE, "Non standard M_CTL received by PORTS\n");
		freemsg( bp);
		return( 1);
	}
	qryp = (struct iocblk *)bp->b_rptr;
	if ( qryp->ioc_cmd == MC_CANONQUERY) {
		if (( msgb1p = allocb( sizeof( struct termios), BPRI_MED)) == NULL) {
			putbq( qp, bp);
			bufcall( sizeof( struct termios), BPRI_MED, npp_ba_mctl, (long)qp);
			return( 1);
		}
		qryp->ioc_cmd = MC_PART_CANON;
		bp->b_cont = msgb1p;
		cb = (struct termios *)bp->b_cont->b_rptr;
		cb->c_iflag = IGNBRK|IGNPAR|PARMRK|INPCK|ISTRIP|
		      INLCR|IGNCR|ICRNL|IUCLC|IXON|IXANY|IXOFF;
		cb->c_oflag = OPOST|OLCUC|ONLCR|OCRNL|ONOCR|ONLRET|OFILL
			|OFDEL|CRDLY|TABDLY|NLDLY|BSDLY|VTDLY|FFDLY;
		cb->c_lflag = 0;
		bp->b_cont->b_wptr += sizeof( struct termios);
		qreply( qp, bp);
	} else
		freemsg( bp);

	return( 0);
}

/*
 * Tell ppc the address of the stream buffer bp->b_datap->db_base
 */

STATIC int
abuf_to_ppc( tp, bp)
register struct strtty	*tp;
register mblk_t	*bp;
{
	register int	i;
	register struct ppcboard *ppcp;

	int sx;


	sx = splstr();
	ppcp = &npp_board[nppcbid[tp->t_dev]];

	for ( i = 0; i < MAX_RBUF; i++) {
		if ( ppcp->rbp[i].bp == NULL) {
			ppcp->rbp[i].bp = bp;
			ppcp->rbp[i].sp = (long)kvtophys( (caddr_t)bp->b_datap->db_base);
			break;
		}
	}

	if ( nlla_regular( nppcbid[tp->t_dev], TRUE, (unsigned short)(PPBUFSIZ - 1), SUPPLYBUF,
	     PPC_RECV, (long)kvtophys( (caddr_t)bp->b_datap->db_base), 0, 0) != PASS) {
		splx( sx);
		return( E_LLA_QUEUE);
	}
	splx( sx);
	return( SUCCESS);
}

/*
 * Copy data from incoming buffer to the large buffer
 */

STATIC int
copy_to_lbuf( tp, bp)
register struct strtty	*tp;
register mblk_t	*bp;
{
	register int buf_sz;


	if ( !tp->t_lbuf)
		if (( tp->t_lbuf = allocb( LARGEBUFSZ, BPRI_MED)) == NULL)
			return( E_NO_LBUFS);

	/*
	 * Check if in coming buffer will fit
	 */
	buf_sz = tp->t_in.bu_cnt;
	if ( buf_sz > tp->t_lbuf->b_datap->db_lim - tp->t_lbuf->b_wptr) {
		/*
		 * If not, free old large buffer, and allocate another
		 */
		freeb( tp->t_lbuf);

		if (( tp->t_lbuf = allocb( LARGEBUFSZ, BPRI_MED)) == NULL)
			return( E_NO_LBUFS);
	}	
	/*
	 * Copy message into large buffer
	 */
	bcopy( (caddr_t)bp->b_rptr, (caddr_t)tp->t_lbuf->b_rptr, buf_sz);
	tp->t_lbuf->b_wptr += buf_sz;

	return( SUCCESS);
}
