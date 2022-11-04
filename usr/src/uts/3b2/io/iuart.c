/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/iuart.c	1.30"

/*
 *	3B2 Integral UART Streams Driver
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/iu.h"
#include "sys/csr.h"
#include "sys/dma.h"
#include "sys/immu.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/fcntl.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "sys/firmware.h"
#include "sys/nvram.h"
#include "sys/inline.h"
#include "sys/cmn_err.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/strtty.h"
#include "sys/debug.h"
#include "sys/eucioctl.h"
#include "sys/systm.h"
#include "sys/cred.h"
#include "sys/ddi.h"

#ifdef KPERF     /* KPERF is for kernel performance measurment tool */
#include "sys/proc.h"
#include "sys/disp.h"
#endif /* KPERF */

#define	IUCONSOLE   0	/* minor device number for system board console uart */
#define	IUCONTTY    1	/* minor device number for system board second uart */
#define	IU_CNT      2	/* number of iuarts on the system board (one duart) */


/*  Values used to designate whether characters should be queued or
 *  processed depending upon the rate characters are being received.
 */
#define IU_IDLE		0	/* no characters have been rcv for 30ms */
#define IU_TIMEOUT	1	/* state was active but no chars rcv for 30ms */
#define IU_ACTIVE	2	/* character was rcv within last 30ms */
#define	IU_SCAN		3	/* clock ticks for the iuart scan timeout */
#define	IU_BRK		2	/* set in t_dstat if a break of 0.25 sec is to be sent */

#define ISSUE_PIR9	0x01	/* sets the CSR bit for UART lo-IPL handler */

/*
 *  Register defines for the interrupt mask register and the 
 *  output port configuration register used during DMA jobs.
 */

#define INIT_IMR	0x00	/*  initializes interrupt mask register */
#define DCD_INT0	0x01	/*  interrupt on dcd change for console */
#define DCD_INT1	0x02	/*  interrupt on dcd change for contty */
#define BAUD_SEL	0x80	/*  sets baud rate generator to 1  */
#define	COUNTER		0x30	/*  selects crystal counter (CLK/16)  */
#define DCD_CHG		0x80	/*  allows interrupt on input port change in state  */
#define INIT_OPCR	0xc0	/*  initializes output port config register */

#define RXINT0		0x03	/*  iuart 0 xmt/rcv interrupt enab/dis  */
#define RXINT1		0x30	/*  iuart 1 xmt/rcv interrupt enab/dis  */
#define	CNTRINT		0x08	/*  counter/timer interrupt enable  */
#define	XINT0		0x01	/*  iuart 0 transmitter interrupt enab/dis */
#define	XINT1		0x10	/*  iuart 1 transmitter interrupt enab/dis */

#define IU0TC		0x04	/*  iuart 0 terminal count in dmac  */
#define IU1TC		0x08	/*  iuart 1 terminal count in dmac  */
#define DIS_DMAC	0x04	/*  disables dmac chan for requested chan */
#define	IUCTRHZ		230525	/*  counter/timer frequency (CLK/16)  */
#define	IUCTRMAX	0xffff	/*  maximum counter/timer value (16 bits) */

extern struct duart duart;	/* system board duart structure */
extern struct dma dmac;		/* system board dma controller structure */
extern int sbdrcsr;		/* used to access the sbd CSR for reading */
extern int sbdwcsr;		/* used to access the sbd CSR for writing */
extern char uartflag;		/* used to generate pir-9 interrupt */
extern struct strtty iu_tty[];	/* tty data structures */
extern int	Demon_cc;	/* Control character for entry to demon. */

extern rnvram();		/* function called to read NVRAM */
extern wnvram();		/* function called to write NVRAM */
extern void iudelay();
extern paddr_t kvtophys();
extern int drv_setparm();
extern void drv_usecwait();
extern time_t drv_hztousec();
extern void iuscan();
extern int call_demon();
extern void iurstrt();

STATIC void iuproc();
STATIC void iuputioc();
STATIC void iuflush();
STATIC int iugetoblk();
STATIC void iusrvioc();
STATIC void iuparam();
STATIC void iurint();
STATIC void iuxint();
STATIC void iubufwake();
STATIC mblk_t *iu_get_buffer();

void iuinit();

clock_t iu_tm_delay = 50;	/* time to allow terminal to settle after self test */

/* NVRAM subfield from physical mode */
unsigned short nvramcons = CS8 | B9600 | CREAD;	/* console characteristics  */

unsigned char	imaskreg;	/* local copy of intr mask reg */
unsigned char	opcrmask;	/* local copy of output port conf reg */
unsigned char	acrmask;	/* local copy of aux config reg */

char	iu_speeds[16] = {
	0, 0, 0, B110BPS, B134BPS, 0,
	0, B300BPS, B600BPS, B1200BPS, B1800BPS,
	B2400BPS, B4800BPS, B9600BPS, B19200BPS, 0
};

/*
 *  structure used to reference uart 0/1 registers indirectly
 *  without checking which device is being referenced
 */
struct {
	unsigned char	*uart_csr;	/*  ptr to duart.[ab]_sr_csr  */
	unsigned char	*uart_cmnd;	/*  ptr to duart.[ab]_cmnd    */
	unsigned char	*uart_data;	/*  ptr to duart.[ab]_data    */
	char		dmajob;	
	char		dmac_chan;	/*  DMAC channel (CH2IUA or CH3IUB)  */
	char		INT;		/*  XINT1 or XINT0  */
} dual[IU_CNT] = {
	&duart.a_sr_csr, &duart.a_cmnd, &duart.a_data, 0, CH2IUA, XINT0,
	&duart.b_sr_csr, &duart.b_cmnd, &duart.b_data, 0, CH3IUB, XINT1
	         };

/* MACROS used to access the specific uart DCD and DTR signals */
#define DCD(dev)	(dev ? DCDB : DCDA)
#define DTR(dev)	(dev ? DTRB : DTRA)

void	(*iutimefn)();		/* Function to call when timer goes off */
int	iutimepir;		/* Timer pir9 pending */
int	iuspurint;		/* Spurious CNTRINT count */

int	iudevflag = 0;	/* Indicates the new interface for open and close to cunix */

STATIC int iuopen(), iuclose(), iuoput(), iuisrv(), iuosrv();

struct module_info iu_info = { 42, "iu", 0, INFPSZ, 512, 128};
struct qinit iu_rint = { NULL, iuisrv, iuopen, iuclose, NULL, &iu_info, NULL};
struct qinit iu_wint = { iuoput, iuosrv, NULL, NULL, NULL, &iu_info, NULL};
struct streamtab iuinfo = { &iu_rint, &iu_wint, NULL, NULL};

#define READBUFSZ	128

void
iuinit()
{
	register dev;


	duart.a_cmnd = RESET_TRANS;
	duart.a_cmnd = RESET_RECV;

	duart.b_cmnd = RESET_TRANS;
	duart.b_cmnd = RESET_RECV;

	imaskreg = INIT_IMR | DCD_CHG | CNTRINT;
	duart.is_imr = imaskreg;

	opcrmask |= INIT_OPCR;
	duart.ip_opcr = INIT_OPCR;
	acrmask = DCD_INT0 | DCD_INT1 | BAUD_SEL | COUNTER;
	duart.ipc_acr = acrmask;

	for (dev = 0; dev < IU_CNT; dev++) {
		dual[dev].dmajob = 0;
		iumodem( dev, ON);
	}
}  /* iuinit */


/* ARGSUSED */
STATIC int
iuopen( q, dev, flag, sflag, credp)
register struct queue *q;
register dev_t *dev;
int flag;
int sflag;
register cred_t *credp;
{
	register int	mdev;
	register struct strtty *tp;

	int retval;

	char *conflag;
	char *conbaud;

	unsigned short consbaud;

	int oldpri;

	struct stroptions *sop;

	mblk_t *mop;


	mdev = (int)getminor( *dev);

	if ( mdev >= IU_CNT) 
		return( ENXIO);

	oldpri = splstr();
	tp = &iu_tty[mdev];
	q->q_ptr = (caddr_t)tp;
	WR(q)->q_ptr = (caddr_t)tp;
	tp->t_rdqp = q;
	tp->t_dev = mdev;

	if (( tp->t_state & (ISOPEN | WOPEN)) == 0) {
		/*
		 * Pass up to the Stream Head the setup options
		 * needed to assign the hi and low water marks and
		 * controlling terminal.
		 */
		while (( mop = allocb( sizeof( struct stroptions), BPRI_MED)) == NULL) {
			if ( flag & (FNDELAY | FNONBLOCK)) {
				splx( oldpri);
				return( EAGAIN);
			}
			bufcall( (uint)sizeof( struct stroptions), BPRI_MED, iubufwake, tp);
			if ( sleep( (caddr_t)&tp->t_cc[3], TTIPRI | PCATCH)) {
				splx( oldpri);
				return( EINTR);
			}
		}
		mop->b_datap->db_type = M_SETOPTS;
		mop->b_wptr += sizeof( struct stroptions);
		sop = (struct stroptions *)mop->b_rptr;
		sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
		sop->so_hiwat = 512;
		sop->so_lowat = 256;
		putnext( q, mop);

		if ( mdev == IUCONSOLE) {
			tp->t_iflag |= IXON|IXANY|BRKINT|IGNPAR;
			/*
			 * get control modes from the nvram
			 */
			conflag = (char *)(&UNX_NVR->consflg);
			conbaud = (char *)(&consbaud);
			retval = rnvram( conflag, conbaud, sizeof(UNX_NVR->consflg));
			if ( retval != 1)
				tp->t_cflag = nvramcons;
			else
				tp->t_cflag = consbaud;
		} else	/* default contty setting */
			tp->t_cflag |= SSPEED | CS8 | CREAD | HUPCL;
		iuparam( mdev);
	}

	if ( tp->t_cflag & CLOCAL || iumodem( mdev, ON))
		tp->t_state |= CARR_ON;
	else
		tp->t_state &= ~CARR_ON;

	if ( !( flag & ( FNDELAY | FNONBLOCK)))
		while (( tp->t_state & CARR_ON) == 0) {
			tp->t_state |= WOPEN;
			if ( sleep( (caddr_t)tp->t_rdqp, TTIPRI|PCATCH)) {
				q->q_ptr = NULL;
				WR(q)->q_ptr = NULL;
				tp->t_state &= ~(WOPEN|BUSY);
				tp->t_rdqp = NULL;

				splx( oldpri);
				return( EINTR);
			}
		}

	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;
	splx( oldpri);
	return( 0);
}  /* iuopen */

/* ARGSUSED */
STATIC int
iuclose( q, oflag, credp)
register struct queue *q;
register int oflag;
register cred_t	*credp;
{
	register struct strtty *tp;
	register int	oldpri;


	tp = (struct strtty *)q->q_ptr;

	if ( !( tp->t_state & ISOPEN))  /* See if it's closed already */
		return( 0);

	oldpri = splstr();
	if ( !( oflag & (FNDELAY|FNONBLOCK)))
		/*
		 * Drain queued output to the console/contty line.
		 */
		while (( tp->t_state & CARR_ON) && ( WR(q)->q_first != NULL
			|| tp->t_state & BUSY || tp->t_out.bu_bp != NULL)) {

			if ( iugetoblk( tp) == 0) {
				tp->t_state |= TTIOW;
				if ( sleep((caddr_t)&tp->t_oflag, PZERO + 1| PCATCH)) {
					tp->t_state &= ~TTIOW;
					break;
				}
			}
		}

	if ( tp->t_cflag & HUPCL)
		iumodem(tp->t_dev, OFF);

	/*
	 * reinitialize the duart registers for the closing
	 * device and turn off xmt and rcv interrupts
	 */
	if ( tp->t_dev == IUCONSOLE) {
		duart.a_cmnd = RESET_TRANS;
		duart.a_cmnd = RESET_RECV;
		imaskreg &= ~RXINT0;
	} else {
		duart.b_cmnd = RESET_TRANS;
		duart.b_cmnd = RESET_RECV;
		imaskreg &= ~RXINT1;
	}
	duart.is_imr = imaskreg;
	if ( tp->t_out.bu_bp != NULL) {
		/*
		 * Dump the transmit buffer
		 */
		freeb( tp->t_out.bu_bp);
		tp->t_out.bu_bp = NULL;
	}
	tp->t_state &= ~(ISOPEN|BUSY);
	tp->t_rdqp = NULL;
	splx( oldpri);
	return( 0);

}  /* iuclose */

/*
 * This is the console's write put procedure
 */
STATIC int
iuoput( q, bp)
register struct queue *q;
register struct msgb *bp;
{
	register struct msgb *bp1;
	register struct strtty *tp;

	int s;

	
	tp = (struct strtty *)q->q_ptr;

	switch(bp->b_datap->db_type) {
	case M_DATA:

		/*
		 * If no carrier then just queue the message on the
		 * driver's write queue
		 */
		if ( !( tp->t_state & CARR_ON)) {
			putq( q, bp);
			return( 0);
		}
		while( bp) {
			bp->b_datap->db_type = M_DATA;
			bp1 = unlinkb( bp);
			if (( bp->b_wptr - bp->b_rptr) <= 0)
				freeb( bp);
			else
				putq( q,bp);
			bp = bp1;
		}
		if ( q->q_first != NULL)
			iugetoblk( tp);

		break;

	case M_IOCTL:

		iuputioc( q, bp);
		if ( q->q_first != NULL)
			iugetoblk( tp);
		break; 

	case M_FLUSH:
		s = splstr();
		if ( *bp->b_rptr & FLUSHW) {
			iuflush( tp, FWRITE);
			*bp->b_rptr &= ~FLUSHW;
		}
		if ( *bp->b_rptr & FLUSHR) {
			iuflush( tp, FREAD);
			putnext( RD(q), bp);
		} else
			freemsg( bp);
		splx( s);
		break;

	case M_START:
		s = splstr();
		iuproc( tp, T_RESUME);
		splx( s);
		freemsg( bp);
		iugetoblk( tp);
		break;

	case M_STOP:
		s = splstr();
		iuproc( tp, T_SUSPEND);
		splx( s);
		freemsg( bp);
		break;

	case M_DELAY:
		s = splstr();
		tp->t_state |= TIMEOUT;
		timeout( iudelay, (caddr_t)tp, ((int)*(bp->b_rptr))*HZ/60);
		splx( s);
		freemsg( bp);
		break;

	case M_STARTI:
		s = splstr();
		iuproc( tp, T_UNBLOCK);
		splx( s);
		freemsg( bp);
		break;

	case M_STOPI:
		s = splstr();
		iuproc( tp, T_BLOCK);
		splx( s);
		freemsg( bp);
		break;
		
	default:
		freemsg( bp);
		break;
	}
	return( 0);
}

/*
 * iugetoblk is called at interrupt level when
 * and by the write put procedure when
 * messages are to be sent to the uart.
 */
STATIC int
iugetoblk( tp)
register struct strtty *tp;
{
	register int s;
	register struct queue *q;
	register struct msgb	*bp;

	unsigned char chan, sr;

	paddr_t addr;


	if ( tp->t_rdqp == NULL)	/* Check if driver is open */
		return( 0);
	q = WR(tp->t_rdqp);

	if ( tp->t_state & BUSY) 
		return( 0);

	s = splstr();

	if ( q->q_first == NULL) {
		/*
		 * wakeup close write queue drain
		 */
		if ( tp->t_state & TTIOW) {
			tp->t_state &= ~(TTIOW);
			wakeup( (caddr_t)&tp->t_oflag);
		}
		splx( s);
		return( 1);	/* Nothing to send. */
	} else
		bp = getq( q);

	switch ( bp->b_datap->db_type) {

	case M_DATA:
		if ( tp->t_state & (TTSTOP | TIMEOUT)) {
			putbq( q, bp);
			splx( s);
			return( 0);
		}
		tp->t_state |= BUSY;
		/*
		 * turn off transmitter interrupt for dev
		 */
		imaskreg &= ~(dual[tp->t_dev].INT);
		duart.is_imr = imaskreg;
		*( dual[tp->t_dev].uart_cmnd) = ENB_TX;

		/*
		 * set dma job flag, and set channel
 	 	 */
		dual[tp->t_dev].dmajob = 1;
		chan = dual[tp->t_dev].dmac_chan;

		/*
		 * calculate physical address for dmac
		 */
		addr = kvtophys( (caddr_t)bp->b_rptr);

		/*
		 * setup dma job when transmitter is ready
		 */
		sr = *( dual[tp->t_dev].uart_csr);
		if ( sr & XMTRDY) {
			dma_access( chan, (uint)addr, (uint)( bp->b_wptr - bp->b_rptr), SNGLMOD, RDMA);
			tp->t_out.bu_bp = bp;
		} else {
			putbq( q, bp);
			tp->t_state &= ~BUSY;
		}
		break;

	case M_IOCTL:

		iusrvioc( q, bp);
		break;

	default:
		freemsg( bp);
		break;
	}

	splx( s);
	return( 0);
}

		
/**************************************
 *                                    *
 *   ioctl-handling routines          *
 *                                    *
 **************************************/

/*
 * ioctl handler for output PUT procedure
 */
STATIC void
iuputioc( q, bp)
register queue_t *q;
register mblk_t *bp;
{
	register struct strtty *tp;
	register struct iocblk *iocbp;

	mblk_t *bp1;


	iocbp = (struct iocblk *)bp->b_rptr;
	tp = (struct strtty *)q->q_ptr;

	/*
	 * Only called for M_IOCTL messages.
	 */
	switch( iocbp->ioc_cmd) {
		case TCSETSW:
		case TCSETSF:
		case TCSETAW:
		case TCSETAF:
		case TCSBRK:
			/*
			 * Run these now if possible if no data
			 * queued of if the uart is not busy.
			 */
			if ( q->q_first != NULL || (tp->t_state & BUSY)) {
				putq( q, bp);
				break;
			}
			iusrvioc( q, bp);
			break;

		case TCSETA:	/* immediate parm set   */
		case TCSETS:

			if ( tp->t_state & BUSY) {
				putbq( q, bp); /* queue these for later */
				break;
			}
			iusrvioc( q, bp);
			break;

		case TCGETA:
		case TCGETS:	/* immediate parm retrieve */

			iusrvioc( q, bp);
			break;

		case EUC_MSAVE:
		case EUC_MREST:
		case EUC_IXLOFF:
		case EUC_IXLON:
		case EUC_OXLOFF:
		case EUC_OXLON:
			bp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0;
			putnext( RD(q), bp);
			break;

		default:
			if (( iocbp->ioc_cmd&IOCTYPE) == LDIOC) {
				bp->b_datap->db_type = M_IOCACK; /* ignore LDIOC cmds */
				bp1 = unlinkb(bp);
				if ( bp1)
					freeb( bp1);
				iocbp->ioc_count = 0;
			} else {
				/*
				 * Unknown IOCTLs. Just NAK them.
				 */
				iocbp->ioc_error = EINVAL;
				bp->b_datap->db_type = M_IOCNAK;
			}
			putnext( RD(q), bp);
			break;
	}
}

/*
 * General ioctl handling 
 *
 */
STATIC void
iusrvioc( q, bp)
register queue_t *q;
register mblk_t *bp;
{
	register struct strtty *tp;
	register struct iocblk *iocbp;

	int arg;

	mblk_t *bpr;
	mblk_t *bp1;


	iocbp = (struct iocblk *)bp->b_rptr;
	tp = (struct strtty *)q->q_ptr;


	switch ( iocbp->ioc_cmd) {
		/*
		 * The output has drained now.
		 */
		case TCSETAF: {

			register struct termio *cb;


			iuflush( tp, FREAD);
			putctl1( RD(q)->q_next, M_FLUSH, FLUSHR);

			if ( !bp->b_cont) {
				iocbp->ioc_error = EINVAL;
				bp->b_datap->db_type = M_IOCNAK;
				iocbp->ioc_count = 0;
				putnext( RD(q), bp);
				break;
			}
			cb = (struct termio *)bp->b_cont->b_rptr;

			tp->t_cflag = (tp->t_cflag & 0xffff0000 | cb->c_cflag);
			tp->t_iflag = (tp->t_iflag & 0xffff0000 | cb->c_iflag);

			iuparam( tp->t_dev);
			bp->b_datap->db_type = M_IOCACK;
			bp1 = unlinkb( bp);
			if ( bp1)
				freeb( bp1);
			iocbp->ioc_count = 0;
			putnext( RD(q), bp);
			break;
		}
		case TCSETA:
		case TCSETAW: {

			register struct termio *cb;

			if ( !bp->b_cont) {
				iocbp->ioc_error = EINVAL;
				bp->b_datap->db_type = M_IOCNAK;
				iocbp->ioc_count = 0;
				putnext( RD(q), bp);
				break;
			}
			cb = (struct termio *)bp->b_cont->b_rptr;

			tp->t_cflag = (tp->t_cflag & 0xffff0000 | cb->c_cflag);
			tp->t_iflag = (tp->t_iflag & 0xffff0000 | cb->c_iflag);

			iuparam( tp->t_dev);
			bp->b_datap->db_type = M_IOCACK;
			bp1 = unlinkb( bp);
			if ( bp1)
				freemsg( bp1);
			iocbp->ioc_count = 0;
			putnext( RD(q), bp);
			break;
		}
		case TCSETSF: {

			register struct termios *cb;


			iuflush( tp, FREAD);
			putctl1( RD(q)->q_next, M_FLUSH, FLUSHR);

			if ( !bp->b_cont) {
				iocbp->ioc_error = EINVAL;
				bp->b_datap->db_type = M_IOCNAK;
				iocbp->ioc_count = 0;
				putnext( RD(q), bp);
				break;
			}
			cb = (struct termios *)bp->b_cont->b_rptr;

			tp->t_cflag = cb->c_cflag;
			tp->t_iflag = cb->c_iflag;

			iuparam( tp->t_dev);
			bp->b_datap->db_type = M_IOCACK;
			bp1 = unlinkb( bp);
			if ( bp1)
				freeb( bp1);
			iocbp->ioc_count = 0;
			putnext( RD(q), bp);
			break;

		}
		case TCSETS:
		case TCSETSW:{

			register struct termios *cb;

			if ( !bp->b_cont) {
				iocbp->ioc_error = EINVAL;
				bp->b_datap->db_type = M_IOCNAK;
				iocbp->ioc_count = 0;
				putnext( RD(q), bp);
				break;
			}
			cb = (struct termios *)bp->b_cont->b_rptr;

			tp->t_cflag = cb->c_cflag;
			tp->t_iflag = cb->c_iflag;

			iuparam( tp->t_dev);
			bp->b_datap->db_type = M_IOCACK;
			bp1 = unlinkb( bp);
			if ( bp1)
				freeb( bp1);
			iocbp->ioc_count = 0;
			putnext( RD(q), bp);
			break;

		}
		case TCGETA:{	/* immediate parm retrieve */
			register struct termio *cb;

			if ( bp->b_cont) { /* Bad user supplied parameter */
				freemsg( bp->b_cont);
				bp->b_cont = NULL;
			}

			if (( bpr = allocb( sizeof(struct termio), BPRI_MED)) == NULL) {
				putbq( q, bp);
				bufcall( sizeof(struct termio), BPRI_MED, iugetoblk, (long)tp); 
				return;
			}

			bp->b_cont = bpr;
			cb = (struct termio *)bp->b_cont->b_rptr;

			cb->c_iflag = (ushort)tp->t_iflag;
			cb->c_cflag = (ushort)tp->t_cflag;

			bp->b_cont->b_wptr += sizeof(struct termio);
			bp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = sizeof(struct termio);
			putnext( RD(q), bp);
			break;
		}
		case TCGETS:{	/* immediate parm retrieve */

			register struct termios *cb;

			if ( bp->b_cont)	/* Bad user supplied parameter */
				freemsg( bp->b_cont);

			if (( bpr = allocb( sizeof(struct termios),BPRI_MED)) == NULL) {
				putbq( q, bp);
				bufcall( sizeof(struct termios), BPRI_MED, iugetoblk, (long)tp); 
				return;
			}
			bp->b_cont = bpr;
			cb = (struct termios *)bp->b_cont->b_rptr;

			cb->c_iflag = tp->t_iflag;
			cb->c_cflag = tp->t_cflag;

			bp->b_cont->b_wptr += sizeof(struct termios);
			bp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = sizeof(struct termios);
			putnext( RD(q), bp);
			break;
		}
 		case TCSBRK:
			if ( !bp->b_cont) {
				iocbp->ioc_error = EINVAL;
				bp->b_datap->db_type = M_IOCNAK;
				iocbp->ioc_count = 0;
				putnext( RD(q), bp);
				break;
			}
			arg = *(int *)bp->b_cont->b_rptr;
			if ( arg == 0)
				iuproc( tp, T_BREAK);
			bp->b_datap->db_type = M_IOCACK;
			bp1 = unlinkb( bp);
			if ( bp1)
				freeb( bp1);
			iocbp->ioc_count = 0;
			putnext( RD(q), bp);
			break;

		case EUC_MSAVE:
		case EUC_MREST:
		case EUC_IXLOFF:
		case EUC_IXLON:
		case EUC_OXLOFF:
		case EUC_OXLON:
			bp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0;
			putnext( RD(q), bp);
			break;

		default: /* unexpected ioctl type */
			if (( iocbp->ioc_cmd&IOCTYPE) == LDIOC) {
				/*
				 * ACK LDIOC ioctls
				 */
				bp->b_datap->db_type = M_IOCACK;
				bp1 = unlinkb( bp);
				if ( bp1)
					freeb( bp1);
				iocbp->ioc_count = 0;
			} else {
				/*
				 * Unknown ioctls. Just NAK them.
				 */
				iocbp->ioc_error = EINVAL;
				bp->b_datap->db_type = M_IOCNAK;
			}
			putnext( RD(q), bp);
			break;
	}
	return;
}


STATIC void
iuflush( tp, cmd)
register struct strtty *tp;
{
	register queue_t *q;
	register int s;

	
	s = splstr();
	if ( cmd&FWRITE) {
		q = WR(tp->t_rdqp);
		flushq( q, FLUSHDATA);
		tp->t_state &= ~(BUSY); 
		tp->t_state &= ~(TBLOCK);
		if ( tp->t_state & TTIOW) {
			tp->t_state &= ~(TTIOW);
			wakeup( (caddr_t)&tp->t_oflag);
		}
	}
	if ( cmd&FREAD) {
		q = tp->t_rdqp;
		flushq( q, FLUSHDATA);
		tp->t_state &= ~(BUSY);
		tp->t_state &= ~(TBLOCK);
	}
	splx( s);
	iugetoblk( tp);
}  /* iuflush */


STATIC void
iuparam( dev)
register dev;
{
	register struct strtty *tp;
	register flags;

	unsigned char mr1;
	unsigned char mr2;
	unsigned char cr;
	
	int s;


	s = splstr();

	tp = &iu_tty[dev];
	flags = tp->t_cflag;

	if (( flags & CBAUD) == 0) {
		/*
		 * hang up modem
		 */
		iumodem( dev ,OFF);
		splx( s);
		return;
	}

	mr1 = CHAR_ERR;
	if (( flags & CSIZE) == CS8)
		mr1 |= BITS8;
	else if (( flags & CSIZE) == CS7)
		mr1 |= BITS7;
	else if (( flags & CSIZE) == CS6)
		mr1 |= BITS6;

	if (( flags & PARENB) == 0)  
		mr1 |= NO_PAR;
	if (( flags & PARODD) != 0)
		mr1 |= OPAR;	/* if not odd, then evenp assumed */

	/*
	 * construct mode register 2 
	 */
	mr2 = NRML_MOD;
	if ( flags & CSTOPB)
		mr2 |= TWOSB;
	else
		mr2 |= ONESB;

	iuproc( tp, T_SUSPEND);

	/*
	 * write the command register to reset selected options including
	 * pointer to mr1
	 */
	*(dual[dev].uart_cmnd) = STOP_BRK; 
	*(dual[dev].uart_cmnd) = RESET_TRANS; 
	*(dual[dev].uart_cmnd) = RESET_RECV; 

	*(dual[dev].uart_cmnd) = RESET_MR; 
	*(dual[dev].uart_cmnd) = RESET_ERR; 

	if ( dev == IUCONSOLE)
		imaskreg |= RXINT0;
	else
		imaskreg |= RXINT1;
	duart.is_imr = imaskreg;

	if ( dev == IUCONSOLE) {
		duart.mr1_2a = mr1;
		duart.mr1_2a = mr2;
		duart.a_sr_csr = iu_speeds[flags & CBAUD];
	} else {
		duart.mr1_2b = mr1;
		duart.mr1_2b = mr2;
		duart.b_sr_csr = iu_speeds[flags & CBAUD];
	}

	cr = (ENB_TX | RESET_MR);
	if ( flags & CREAD) 
		cr |= ENB_RX;
	else
		cr &= ~ENB_RX;

	*(dual[dev].uart_cmnd) = cr;

	iuproc( tp, T_RESUME);

	splx( s);
}  /* iuparam */


/*
 * iuint() is the console interrupt routine
 */
void
iuint()
{
	register struct strtty *tp;
	register unsigned char sr;
	register dev;
	register unsigned char ignore1;
	register unsigned char ignore2;
	register int	ignore3;

#ifdef KPERF
	if (kpftraceflg)
		kperf_write(KPT_INTR, iuint, curproc);
#endif /* KPERF */

	/*
	 * check CSR to determine whether DMAC or DUART caused interrupt
	 */
	if ( Rcsr & CSRUART) {
		if ( duart.is_imr & CNTRINT) {
			ignore3 = duart.scc_ropbc;	/* Stop counter */
			ignore3 = ignore3;	/* To satisfy lint only! */
			if ( iutimefn) {
				iutimepir = 1;
				uartflag = 1;
				Wcsr->s_pir9 = ISSUE_PIR9;
			} else {
				++iuspurint;
				cmn_err( CE_NOTE, "spurious iu counter interrupt");
			}
		}
		for ( dev = 0; dev < IU_CNT; dev++) {
			tp = &iu_tty[dev];

			/*
			 * check for data set change;
			 * reverse logic used to check DCD bit in input port
			 */
			ignore2 = duart.ipc_acr & (DCD_INT0 | DCD_INT1);
			ignore2 = ignore2;	/* To satisfy lint only! */
			if ( tp->t_cflag & CLOCAL || !(duart.ip_opcr & DCD(dev))) {
				if ((tp->t_state & CARR_ON) == 0) {
					wakeup( (caddr_t)tp->t_rdqp);
					tp->t_state |= CARR_ON;
					/*
					 * May need to print prompt
					 */
					if ( tp->t_rdqp && WR( tp->t_rdqp)->q_first != NULL) {
						/*
						 * Delay a while before sending
						 * the prompt to allow the
						 * terminal to settle
						 */
						drv_usecwait( drv_hztousec( iu_tm_delay));
						iugetoblk( tp);
					}
				}
			} else {
				if (tp->t_state & CARR_ON) {
					register unsigned char bit;
					if ( tp->t_state & ISOPEN) {
						iuflush( tp, FREAD|FWRITE);
						putctl( tp->t_rdqp->q_next, M_HANGUP);
					 	bit = DTR(dev);
					 	duart.scc_ropbc = bit;
					}
					tp->t_state &= ~CARR_ON;
				}
			}

			/*
			 * check status register
			 */
			sr = *(dual[dev].uart_csr);

			if ( sr & RCVRDY) 
				iurint( dev);
			if ( sr & XMTRDY)
				iuxint( dev);
		}
	}

	if ( Rcsr & CSRDMA) {
		sr = dmac.RSR_CMD;
		ignore1 = duart.c_uartdma;
		ignore1 = ignore1;

		if (sr & IU0TC) {
			*(dual[IUCONSOLE].uart_cmnd) = DIS_TX;
			dmac.WMKR = (CH2IUA | DIS_DMAC);
			dual[IUCONSOLE].dmajob = 0;
			imaskreg |= XINT0;
			duart.is_imr = imaskreg;
			*(dual[IUCONSOLE].uart_cmnd) = ENB_TX;
		}

		if ( sr & IU1TC) {
			*(dual[IUCONTTY].uart_cmnd) = DIS_TX;
			dmac.WMKR = (CH3IUB | DIS_DMAC);
			dual[IUCONTTY].dmajob = 0;
			imaskreg |= XINT1;
			duart.is_imr = imaskreg;
			*(dual[IUCONTTY].uart_cmnd) = ENB_TX;
		}
	}
}  /* iuint */


/*
 * iurint() handles receive type interrupts and is called from iuint()
 */
STATIC void
iurint( dev)
register dev;
{
	register struct strtty *tp;
	register char c, stat;
	register unsigned char *sr;
	register mblk_t *bpt;

	char lbuf[12];
	char *lptr;

	int lcnt;
	int room_left;


	drv_setparm( SYSRINT, 1);
	if (dev >= IU_CNT)
		return;

	tp = &iu_tty[dev];
	lptr = lbuf;

	sr = dual[dev].uart_csr;

	while ((stat = *sr) & RCVRDY) {
		c = *( dual[dev].uart_data);

		/*
		 * Check for entry to demon.
		 */
		if ( dev == IUCONSOLE) {
			register int	ctmp;

			ctmp = c & 0177;
			if ( Demon_cc && Demon_cc == ctmp)
				if ( call_demon())
					return;
		}
		/*
		 * check if console is open: if not, just return
		 */
		if ( !( tp->t_rdqp))
			return;
			
		/*
		 * check for CSTART/CSTOP
		 */
		if ( tp->t_iflag & IXON) {
			register char ctmp;

			if ( tp->t_state & ISTRIP)
				ctmp = c & 0177;
			else
				ctmp = c;

			if ( tp->t_state & TTSTOP) {
				if ( ctmp == CSTART || tp->t_iflag & IXANY) 
					iuproc( tp, T_RESUME);
			} else {
				if ( ctmp == CSTOP) 
					iuproc( tp, T_SUSPEND);
			}
			if ( ctmp == CSTART || ctmp == CSTOP)
				continue;
		}

	        /*
		 * Check for errors
		 */
		if ( stat & (FE | PARERR | OVRRUN)) 
			*( dual[dev].uart_cmnd) = RESET_ERR;

		if ( stat & PARERR && !( tp->t_iflag & INPCK))
			stat &= ~PARERR;

		if ( stat & (RCVD_BRK | FE | PARERR | OVRRUN)) {
			if (( c & 0377) == 0) {
				if ( tp->t_iflag & IGNBRK)
					continue;

				putctl( tp->t_rdqp->q_next, M_BREAK);
				continue;
			} else {
				if ( tp->t_iflag & IGNPAR)
					continue;
			}

			if ( tp->t_iflag & PARMRK) {
				*lptr++ = 0377;
				*lptr++ = 0;
				drv_setparm( SYSRAWC, 2);
			} else
				c = 0;
		} else {
			if ( tp->t_iflag & ISTRIP)
				c &= 0177;
			else {
				c &= 0377;
				if ( c == 0377 && tp->t_iflag & PARMRK)
					*lptr++ = 0377;
			}
		}

		/*
		 * put input character on rcvq 
	 	 */
		*lptr++ = c;
	}

	lcnt = lptr - lbuf;
	if ( lcnt == 0)
		return;

	/*
	 * Buffering code.
	 */
	if (( bpt = iu_get_buffer( tp)) == NULL) 
		return;

	room_left = bpt->b_datap->db_lim - bpt->b_wptr;
	if ( room_left < lcnt) {
		/*
		 * No more room in this message block. Queue this one
		 * and allocate another one
		 */
		putq( tp->t_rdqp, tp->t_in.bu_bp);
		tp->t_in.bu_bp = NULL;
		if ( tp->t_rdqp->q_count > tp->t_rdqp->q_hiwat) {
			if (( tp->t_iflag & IXOFF) && !(tp->t_state & TBLOCK)) 
				iuproc( tp, T_BLOCK);
			if ( tp->t_rdqp->q_count > MAX_INPUT) 
				iuflush( tp, FREAD);
		}
		if (( bpt = iu_get_buffer( tp)) == NULL)
			return;
	}

	/*
	 * We have rooma for more data at this point.
	 * Copy the data to this buffer 
	 */
	bcopy( lbuf, (caddr_t)bpt->b_wptr, lcnt);
	bpt->b_wptr += lcnt;
	tp->t_in.bu_cnt += lcnt;
	if (!( tp->t_dstat & IU_TIMEOUT)) {
		timeout( iuscan, (caddr_t)tp, IU_SCAN);
		tp->t_dstat |= IU_TIMEOUT;
	}
	return;
}  /* iurint */

void
iuscan( tp)
register struct strtty *tp;
{
	register int s;


	s = splstr();
	if ( tp->t_rdqp) {
		if ( tp->t_in.bu_cnt) {
			putq( tp->t_rdqp, tp->t_in.bu_bp);
			tp->t_in.bu_bp = NULL;
			iu_get_buffer( tp);
		}
	}
	tp->t_dstat &= ~IU_TIMEOUT;
	splx( s);
}

int
iupirint()
{
	register void (*fn)();


	if ( iutimepir) {
		if (( fn = iutimefn) == 0)
			cmn_err( CE_PANIC, "iutimefn");
		iutimepir = 0;
		iutimefn = 0;
		(*fn)();
	}
	return( 0);

}


/*
 * iuxint() handles transmit type interrupts and is called from iuint()
 */
STATIC void
iuxint( dev)
register dev;
{
	register struct strtty *tp;
	register unsigned char *sr;


	drv_setparm( SYSXINT, 1);
	tp = &iu_tty[dev];
	/*
	 * check for TTXON/TTXOFF 
	 */
	sr =  dual[dev].uart_csr;

	while (*sr & XMTRDY) { /* TX rdy */ 

		*(dual[dev].uart_cmnd) = DIS_TX;

		if ( tp->t_state & TTXON) {
			if ( dual[dev].dmajob == 1)
				dmac.WMKR = (dual[dev].dmac_chan | DIS_DMAC);
			*(dual[dev].uart_cmnd) = ENB_TX;
			*(dual[dev].uart_data) = CSTART;
			if ( dual[dev].dmajob == 1)
				dmac.WMKR = dual[dev].dmac_chan;  /* enable DMAC */

			tp->t_state &= ~TTXON;
			continue;
		}

		if ( tp->t_state & TTXOFF) {
			if (dual[dev].dmajob == 1)
				dmac.WMKR = (dual[dev].dmac_chan | DIS_DMAC);
			*(dual[dev].uart_cmnd) = ENB_TX;
			*(dual[dev].uart_data) = CSTOP;
			if ( dual[dev].dmajob == 1)
				dmac.WMKR = dual[dev].dmac_chan;  /* enable DMAC */

			tp->t_state &= ~TTXOFF;
			continue;
		}

		if ( dual[dev].dmajob == 1)
			*(dual[dev].uart_cmnd) = ENB_TX;
		else {
			if (( tp->t_state & BUSY) && ( tp->t_out.bu_bp != NULL)) {
				freeb( tp->t_out.bu_bp);
				tp->t_out.bu_bp = NULL;
				tp->t_state &= ~BUSY;
				iugetoblk( tp);
				continue;
			}
		}
		return;
	}
}  /* iuxint */


STATIC void
iuproc( tp, cmd)
register struct strtty *tp;
register cmd;
{
	register dev;
	register int s;


	dev = tp - iu_tty;
	switch (cmd) {
	case T_TIME:
		s = splstr();
		tp->t_state &= ~TIMEOUT;
		*(dual[dev].uart_cmnd) = STOP_BRK;
		if ( dev == IUCONSOLE)		
			imaskreg |= XINT0;	
		else				
			imaskreg |= XINT1;	
		duart.is_imr = imaskreg;	
		splx( s);
		iugetoblk( tp);
		break;

	case T_RESUME:	/* resume output */
		s = splstr();
		tp->t_state &= ~TTSTOP;
		if ( dual[dev].dmajob == 1)
			*(dual[dev].uart_cmnd) = ENB_TX;
		splx( s);
		iugetoblk( tp);
		break;


	case T_SUSPEND: /* suspend output */
		s = splstr();
		if ( dual[dev].dmajob == 1)
			*(dual[dev].uart_cmnd) = DIS_TX;
		tp->t_state |= TTSTOP;
		splx( s);
		break;


	case T_BREAK:
		s = splstr();
		if ( dev == IUCONSOLE)		
			imaskreg &= ~XINT0;	
		else				
			imaskreg &= ~XINT1;	
		duart.is_imr = imaskreg;	

		*(dual[dev].uart_cmnd) = ENB_TX;
		while (( *dual[dev].uart_csr & XMTRDY) == 0)
			;
		if ( tp->t_dstat & IU_BRK) {
			*(dual[dev].uart_cmnd) = STRT_BRK;	
			tp->t_state |= TIMEOUT;
			timeout( iurstrt, (caddr_t)tp, HZ/4);
		} else
			iugetoblk( tp);

		tp->t_dstat &= ~IU_BRK;
		splx( s);
		break;

	case T_BLOCK:

		tp->t_state &= ~TTXON;
		tp->t_state |= TBLOCK;
		if ( tp->t_state & BUSY)
			tp->t_state |= TTXOFF; 
		else {
			tp->t_state |= BUSY; 
			if ( dual[dev].dmajob == 1)
				dmac.WMKR = (dual[dev].dmac_chan | DIS_DMAC);
				*(dual[dev].uart_cmnd) = ENB_TX;
				*(dual[dev].uart_data) = CSTOP;
				if ( dual[dev].dmajob == 1)
					dmac.WMKR = dual[dev].dmac_chan;
		}
		break;

	 case T_UNBLOCK:
		 tp->t_state &= ~(TTXOFF | TBLOCK);
		 if ( tp->t_state & BUSY)
		 	tp->t_state |= TTXON;
		 else {
		 	tp->t_state |= BUSY;
			if ( dual[dev].dmajob == 1)
				dmac.WMKR = (dual[dev].dmac_chan | DIS_DMAC);
			*(dual[dev].uart_cmnd) = ENB_TX;
			*(dual[dev].uart_data) = CSTART;
			if ( dual[dev].dmajob == 1)
			dmac.WMKR = dual[dev].dmac_chan;  /* enable DMAC */
		}
		break;
	}  /* end of switch cmd */

}  /* iuproc */

void
iurstrt( tp)
register struct strtty *tp;
{
	iuproc( tp, T_TIME);
	
}  /* iurstrt */

/*
 * iumodem()
 *
 * Toggle data terminal ready output. Uses the local "bit" variable
 * to avoid an SGS "spurious read" bug (which would play havoc with
 * these device registers).
 */
int
iumodem( dev, flag)
register dev, flag;
{
	register unsigned char	bit;

	bit = DTR(dev);
	if ( flag == OFF)
		duart.scc_ropbc = bit;
	else
		duart.scc_sopbc = bit;

	/*
	 * reverse logic used to check DCD on input port
	 */
	return( !( duart.ip_opcr & DCD(dev)));
}  /* iumodem */

int
#ifdef __STDC__
iuputchar( char c)
#else
iuputchar( c)
	char c;
#endif
{
	register s;

	s = splhi();
	duart.a_cmnd = ENB_TX;

	while (( duart.a_sr_csr & XMTRDY) == 0)
		;
	if ( c == 0) {
		splx( s);
		return( 0);
	}

	duart.a_data = c;
	if ( c == '\n')
		iuputchar( '\r');

	iuputchar( 0);
	splx( s);
	return( 0);
}  /* iuputchar */
/*
 * iutime()
 *
 * Call a given function after a specified number of milliseconds. The
 * given function is called at IPL 10 (via a pir9). The limit of 284
 * milliseconds is based on the clock rate (230525 Hz) and the width
 * of the timing hardware (16 bits). Note the lack of request queuing;
 * use of this function is now restricted to the integral floppy driver.
 * This could be made into a general-purpose high-resolution timer with
 * a bit of effort.
 */
void
iutime( ms, fn)
register unsigned int	ms;
void	(*fn)();
{
	register unsigned int	ticks;
	register int		s;
	register unsigned char	ignore;

	s = splstr();
	if ( iutimefn)
		cmn_err( CE_PANIC, "iudelay race");
	if ( ms > IUCTRMAX || ( ticks = (IUCTRHZ * ms) / 1000) > IUCTRMAX)
		cmn_err( CE_PANIC, "iudelay range");
	iutimefn = fn;
	ignore = duart.scc_ropbc;	/* Stop counter */
	duart.ctur = (ticks & 0xff00) >> 8;
	duart.ctlr = ticks & 0xff;
	ignore = duart.scc_sopbc;	/* Start counter */
	ignore = ignore;		/* To satisfy lint only! */
	splx( s);
}

/*
 * Input service procedure.  Pass everything upstream.
 */

STATIC int
iuisrv( q)
queue_t *q;
{
	register mblk_t *mp;
	register struct strtty *tp;

	int s;


	s = splstr();
	tp = (struct strtty *)q->q_ptr;

	while (( mp = getq( q)) != NULL) {
		if ( canput( q->q_next) == 0) {
			putbq( q, mp);
			splx( s);
			return( 0);
		}
		putnext( q, mp);
		if ( tp->t_state & TBLOCK)
			if ( q->q_count < q->q_lowat)
				iuproc( tp, T_UNBLOCK);
	}
	splx( s);
	return( 0);
}

/*
 * Output service procedure. Only there for flow control to work. 
 */

/* ARGSUSED */
STATIC int
iuosrv( q)
queue_t *q;
{
	return( 0);
}

/*
 * Called from timein() when callout table entry <= 0, using a PIR_9
 * interrupt. Used for carriage return delay processing. 
 */
void
iudelay( tp)
register struct strtty *tp;
{
	int s;


	s = splstr();
	tp->t_state &= ~TIMEOUT;
	splx( s);
	iugetoblk( tp);  /* Restart any ouput waiting on queue */
}
STATIC mblk_t *
iu_get_buffer( tp)
register struct strtty *tp;
{
	register mblk_t *bp;


	/*
	 * if no current message. allocate a block for it 
	 */
	if (( bp = tp->t_in.bu_bp) == NULL) {
		if (( bp = allocb( READBUFSZ, BPRI_HI)) == NULL) {
			cmn_err( CE_WARN, "iu_get_buffer: out of blocks");
			return( (mblk_t *)NULL);
		}
		tp->t_in.bu_bp = bp;
		tp->t_in.bu_cnt = 0;
	}
	return( bp);
}
/*
 * Wakeup sleep function calls sleeping for a STREAMS buffer
 * to become available
 */
STATIC void
iubufwake( tp)
register struct strtty *tp;
{
	wakeup( (caddr_t)&tp->t_cc[3]);
}
