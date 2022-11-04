/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/cmn_err.c	1.15.1.12"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/time.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/panregs.h"
#include "sys/sbd.h"
#include "sys/sit.h"
#include "sys/csr.h"
#include "sys/immu.h"
#include "sys/edt.h"
#include "sys/cmn_err.h"
#include "sys/nvram.h"
#include "sys/firmware.h"
#include "sys/tty.h"
#include "sys/reg.h"
#include "sys/inline.h"
#include "sys/strlog.h"
#include "sys/iu.h"
#include "sys/debug.h"


#define	DMD_DELAY	0x1000

STATIC void pansave();
STATIC void printn();

STATIC int panic_level = 0;
STATIC int dmd_delay = DMD_DELAY;

/*
 * A delay is required when outputting many lines to the console
 * if it is a DMD 5620 terminal.  The following value has been
 * chosen empirically.  If your console is a normal terminal, set
 * the delay to 0.  Note that dmd_delay can be set to 0 on the
 * running system with DEMON.
 */

void
dodmddelay()
{
	int	delay;

	if (dmd_delay) {
		delay = dmd_delay;
		while (delay--) ;
	}
}

/*
 * Save output in a buffer where we can look at it with DEMON
 * or crash.  If the message begins with a '!', then only put
 * it in the buffer, not out to the console.
 */
extern	char	putbuf[];
extern	int	putbufsz;
extern	int	putbufrpos;
extern	int	putbufwpos;
extern	int	conslogging;
short		prt_where;
static	short	not_cmn;	
static	int	cmntype;
static	char	consbuf[256];
static	int	conspos;

#define	output(c) {					\
	if (prt_where & PRW_BUF) {			\
		if (++putbufwpos >= putbufsz)		\
			putbufwpos = 0;			\
		putbuf[putbufwpos] = c;		\
		putbufrpos = putbufwpos;		\
	}						\
	if (prt_where & PRW_CONS) {			\
		if (conslogging)			\
			consbuf[conspos++ % 256] = c;	\
		else					\
			(void)iuputchar(c);		\
	}						\
}

char	*panicstr;

extern char	*mmusrama;
extern char	*mmusramb;
extern char	*mmufltar;
extern char	*mmufltcr;
extern int	*save_r0ptr;
extern int	sdata;
extern int	bssSIZE[];

/* return contents of r0 */

#if defined(lint)
extern int r0();
#else
int r0() {}
#endif

#if defined(__STDC__)
#define MOVW(x, r, m)	asm(" MOVW " #r ",%r0"); x = (m)r0();
#define MOVAW(x, r, m)  asm(" MOVAW " #r ",%r0"); x = (m)r0();
#else
#define MOVW(x, r, m)	asm(" MOVW r,%r0"); x = (m)r0();
#define MOVAW(x, r, m)	asm(" MOVAW r,%r0"); x = (m)r0();
#endif

#define IUCONSOLE	0
#define SEC		300000	/* approx # loops per second */

/*
 * Scaled down version of C Library printf.
 * Only %s %u %d (==%u) %o %x %D are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much suspended.
 * Printf should not be used for chit-chat.
 */

static void
xprintf(fmtp)
	char **fmtp;
{
	VA_LIST ap;
	register char *fmt;
	register char	c;
	register char	*s;
	register int	opri;
	int oldlog;
	extern int in_demon;

	fmt = *fmtp;
	ap = (char *)(fmtp + 1);

	opri = splhi();
	if (in_demon) {
		oldlog = conslogging;
		conslogging = 0;
	} else {
		bzero(consbuf, 256);
		conspos = 0;
	}

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0') {
			if (not_cmn)	/* cmn_err not used */
				cmntype = 0;

			/*
			 * If message was 257 bytes long, conspos will be 0
			 * and no message will be output.  Too bad.
			 */
			if (conspos > 0)
			    (void)strlog(0, 0, 0, SL_CONSOLE|cmntype, 
				consbuf, 0);
			if (in_demon)
				conslogging = oldlog;
			splx(opri);
			return;
		}
		output(c);
	}
	c = *fmt++;
	if (c == 'D' || c == 'd' || c == 'u' || c == 'o' || c == 'x')
		printn(VA_ARG(ap, long), c=='o'? 8: (c=='x'? 16:10));
	else if (c == 's') {
		s = VA_ARG(ap, char *);
		while ((c = *s++) != 0)
			output(c);
	}
	goto loop;
}

static void
xpanic(msgp)
	char **msgp;
{
	extern void mtcrchk();
	extern void sync();
	VA_LIST ap;
	struct	xtra_nvr	nvram_copy;
	int			x;
	int			i;

	/* Stop interval timer. */
	((struct sit *)(&sbdpit))->command = ITINIT;

	/* Save panic string (needed for routines elsewhere). */
	panicstr = *msgp;
	ap = (char *)(msgp + 1);

	/* Get nvram contents. */
	rnvram(&sbdnvram+XTRA_OFSET, (caddr_t)&nvram_copy, sizeof(nvram_copy));

	/* Make room for new error in log. */
	for (i = NERRLOG-1; i >= 1; i--)
		nvram_copy.errlog[i] = nvram_copy.errlog[i-1];

	/* Enter new error in log. */
	nvram_copy.errlog[0].message = *msgp;
	nvram_copy.errlog[0].time    = hrestime.tv_sec;
	nvram_copy.errlog[0].param1  = VA_ARG(ap, int);
	nvram_copy.errlog[0].param2  = VA_ARG(ap, int);
	nvram_copy.errlog[0].param3  = VA_ARG(ap, int);
	nvram_copy.errlog[0].param4  = VA_ARG(ap, int);
	nvram_copy.errlog[0].param5  = VA_ARG(ap, int);

	/* Save system state. */
	pansave(&nvram_copy.systate);

	/* Update nvram. */
	nvram_copy.nvsanity = ~NVSANITY;
	wnvram((caddr_t)&nvram_copy, &sbdnvram+XTRA_OFSET, sizeof(nvram_copy));

	/* Mark as sane. */
	nvram_copy.nvsanity = NVSANITY;

	wnvram((caddr_t)&nvram_copy,
		(caddr_t)&(((struct xtra_nvr*)(_VOID*)
		(&sbdnvram+XTRA_OFSET))->nvsanity),
		sizeof(nvram_copy.nvsanity));

	/* Get execution level. */
	x = splhi();
	splx(x);

	/* "sync" */
	if (((psw_t *)&x)->IPL == 0)
		sync();

	/* Check for optional reboot. */
	mtcrchk();

	/* Return to firmware. */
	call_demon();
	rtnfirm();
}

static void
xcmn_err(level, fmtp)
	register int	level;
	char		**fmtp;

{
	register int	i;
	register int	x;

	/*
	 * Set up to print to putbuf, console, or both
	 * as indicated by the first character of the
	 * format.
	 */

	not_cmn = 0;

	if (**fmtp == '!') {
		prt_where = PRW_BUF;
		(*fmtp)++;
	} else if (**fmtp == '^') {
		prt_where = PRW_CONS;
		(*fmtp)++;
	} else
		prt_where = PRW_BUF | PRW_CONS;

	switch (level) {
		case CE_CONT:
			cmntype = 0;
			xprintf(fmtp);
			break;

		case CE_NOTE:
			cmntype = SL_NOTE;
			printf("\nNOTICE: ");
			xprintf(fmtp);
			printf("\n");
			break;

		case CE_WARN:
			cmntype = SL_WARN;
			printf("\nWARNING: ");
			xprintf(fmtp);
			printf("\n");
			break;

		case CE_PANIC: {
			switch (panic_level) {
			case 0: 
				x = splhi();

				/*
				 * Processes logging console messages
				 * will never run.  Force message to
				 * go to console.
				 */
				conslogging = 0;

				/* drop DTR to kill layers */

				(void)iumodem(IUCONSOLE, OFF);
				i = 2 * SEC;
				while (--i)
					;
				/* raise DTR before sending message */

				(void)iumodem(IUCONSOLE, ON);
				i = 2 * SEC;
				while (--i)
					;

				prt_where = PRW_CONS | PRW_BUF;
				panic_level = 1;
				printf("\nPANIC: ");
				xprintf(fmtp);
				printf("\n");
				splx(x);

				/*
				 * If the registers were not saved,
				 * save them now.
				 */

				xpanic(fmtp);
				/* NOTREACHED */

			case 1:
				panic_level = 2;
				prt_where = PRW_CONS | PRW_BUF;
				printf("\nDOUBLE PANIC: ");
				xprintf(fmtp);
				printf("\n");
				call_demon();
				rtnfirm();
				/* NOTREACHED */

			default:
				panic_level = 3;
				rtnfirm();
				/* NOTREACHED */
			}
		}

		default:
			cmn_err(CE_PANIC,
	  		  "unknown level: cmn_err(level=%d, msg=\"%s\")",
			   level, *fmtp);
	}
	not_cmn = 0;
}

/*PRINTFLIKE1*/
void 
#ifdef __STDC__
printf(char *fmt, ...)
#else
printf(fmt)
	char *fmt;
#endif
{
	xprintf(&fmt);
}

STATIC void
printn(n, b)
	long n;
	register b;
{
	register i, nd, c;
	int	flag;
	int	plmax;
	char d[12];

	c = 1;
	flag = n < 0;
	if (flag)
		n = (-n);
	if (b == 8)
		plmax = 11;
	else if (b == 10)
		plmax = 10;
	else if (b == 16)
		plmax = 8;
	if (flag && b == 10) {
		flag = 0;
		output('-');
	}
	for (i = 0; i < plmax; i++) {
		nd = n%b;
		if (flag) {
			nd = (b - 1) - nd + c;
			if (nd >= b) {
				nd -= b;
				c = 1;
			} else
				c = 0;
		}
		d[i] = (char)nd;
		n = n/b;
		if (n == 0 && flag == 0)
			break;
	}
	if (i == plmax)
		i--;
	for (; i >= 0; i--)
		output("0123456789ABCDEF"[d[i]]);
}

/*
 * Panic is called on unresolvable fatal errors.
 */

/*PRINTFLIKE1*/
void
#ifdef __STDC__
panic(char *msg, ...)
#else
panic(msg)
	char *msg;
#endif
{
	xpanic(&msg);
}

/*
 * prdev prints a warning message.
 * dev is a block special device argument.
 */

void
prdev(str, dev)
	char *str;
	dev_t dev;
{
	register major_t maj;

	maj = getmajor(dev);
	if (maj >= bdevcnt) {
		cmn_err(CE_WARN, "%s on bad dev 0x%x\n", str, dev);
		return;
	}
	if (*bdevsw[maj].d_flag & D_OLD)
		(*bdevsw[maj].d_print)(cmpdev(dev), str);
	else
		(*bdevsw[maj].d_print)(dev, str);
}

/*
 * Old-style drivers only
 * prcom prints a diagnostic from a device driver.
 * prt is device dependent print routine.
 */

void
prcom(prt, bp, er1, er2)
	int (*prt)();
	register struct buf *bp;
	int er1;
	int er2;
{
	(*prt)(bp->b_dev, "\nDevice error");
	cmn_err(CE_NOTE, "bn = %D er = %o,%o\n", bp->b_blkno, er1, er2);
}

STATIC void
pansave(p)
	struct	systate	*p;
{
	/*
	 * save_r0ptr is set in trap when trap is entered.
	 * save_r0ptr points to R0 in the stack after the trap.
	 * The format of the stack is in ttrap.s.
	 * ofp gets the fp of the process that caused the trap.
	 * lfp gets the last frame pointer (of pansave).
	 */
	
	p->csr = Rcsr;
	if (save_r0ptr != NULL) {
		*(int *)&p->psw = save_r0ptr[PS];
		p->r3 = save_r0ptr[R3];
		p->r4 = save_r0ptr[R4];
		p->r5 = save_r0ptr[R5];
		p->r6 = save_r0ptr[R6];
		p->r7 = save_r0ptr[R7];
		p->r8 = save_r0ptr[R8];
		p->oap = save_r0ptr[AP];
		p->opc = save_r0ptr[PC];
		p->osp = save_r0ptr[SP];
		p->ofp = save_r0ptr[FP];
	}
	MOVW(p->lfp, %fp, int)
	MOVW(p->isp, %isp, int)
	MOVW(p->pcbp, %pcbp, int)

	p->mmufltcr = *fltcr;
	p->mmufltar = *(long *)fltar;
	p->mmusrama[0] = *(SRAMA *)(srama);
	p->mmusrama[1] = *(SRAMA *)(srama + 1);
	p->mmusrama[2] = *(SRAMA *)(srama + 2);
	p->mmusrama[3] = *(SRAMA *)(srama + 3);
	p->mmusramb[0] = *(SRAMB *)(sramb);
	p->mmusramb[1] = *(SRAMB *)(sramb + 1);
	p->mmusramb[2] = *(SRAMB *)(sramb + 2);
	p->mmusramb[3] = *(SRAMB *)(sramb + 3);
}

/*PRINTFLIKE2*/
void
#ifdef __STDC__
cmn_err(int level, char *fmt, ...)
#else
cmn_err(level, fmt)
	int level;
	char *fmt;
#endif
{
	pcb_t regsave_pcb;
	if (level == CE_PANIC) {
		int *reg;
		if (save_r0ptr == NULL) {
			save_r0ptr = &regsave_pcb.regsave[K_R0];
			MOVW(reg, %fp, int *);
			save_r0ptr[R0] = reg[0];
			save_r0ptr[R1] = reg[1];
			save_r0ptr[R2] = reg[2];
			MOVAW(reg, -9*4(%fp), int *);
			save_r0ptr[PC] = reg[0];
			save_r0ptr[AP] = reg[1];
			save_r0ptr[FP] = reg[2];
			save_r0ptr[R3] = reg[3];
			save_r0ptr[R4] = reg[4];
			save_r0ptr[R5] = reg[5];
			save_r0ptr[R6] = reg[6];
			save_r0ptr[R7] = reg[7];
			save_r0ptr[R8] = reg[8];
		} else {
			reg = save_r0ptr;
			save_r0ptr = &regsave_pcb.regsave[K_R0];
			save_r0ptr[PC] = reg[PC];
			save_r0ptr[PS] = reg[PS];
			save_r0ptr[AP] = reg[0];
			save_r0ptr[FP] = reg[1];
			save_r0ptr[R3] = reg[2];
			save_r0ptr[R4] = reg[3];
			save_r0ptr[R5] = reg[4];
			save_r0ptr[R6] = reg[5];
			save_r0ptr[R7] = reg[6];
			save_r0ptr[R8] = reg[7];
		}
	}

	xcmn_err(level, &fmt);
}

void
printnvram()
{
	struct	xtra_nvr	nvram_copy;
	register int		oldpri;

	asm("	PUSHW  %r0");
	asm("	PUSHW  %r1");
	asm("	PUSHW  %r2");

	oldpri = splhi();

	rnvram(&sbdnvram+XTRA_OFSET, (caddr_t)&nvram_copy, sizeof(nvram_copy));
	dumpnvram(&nvram_copy);

	splx(oldpri);

	asm("	POPW  %r2");
	asm("	POPW  %r1");
	asm("	POPW  %r0");
}

void
dumpnvram(p)
	struct	xtra_nvr	*p;
{
	int	i;
	static	char	*csr_names[] = {
		"i/o fault",
		"dma",
		"disk",
		"uart",
		"pir9",
		"pir8",
		"clock",
		"inhibit fault",
		"inhibit time",
		"unassigned",
		"floppy",
		"led",
		"alignment",
		"req reset",
		"parity",
		"bus timeout",
	};

	prt_where = PRW_CONS;
	printf("nvram status:\t");
	switch (p->nvsanity) {
	case NVSANITY:
		printf("sane");
		break;

	case ~NVSANITY:
		printf("incompletely updated");
		break;

	default:
		printf("invalid");
		break;
	}
	printf("\n\n");
	printf("csr:\t%x\t", p->systate.csr);
	for (i = 15; i >= 0; i--)
		if ((p->systate.csr&(1<<i)) != 0)
			printf("%s, ",csr_names[i]);
	printf("\n");
	printf("\n");
	dodmddelay();
 	printf("psw:\tCFD QIE CD OE NZVC TE IPL CM PM R I ISC TM FT\n");
	printf
	(
		"(hex)\t  %x   %x  %x  %x  %x    %x  %x   %x  %x  %x %x %x   %x  %x\n",
		p->systate.psw.CSH_F_D,
		p->systate.psw.QIE,
		p->systate.psw.CSH_D,
		p->systate.psw.OE,
		p->systate.psw.NZVC,
		p->systate.psw.TE,
		p->systate.psw.IPL,
		p->systate.psw.CM,
		p->systate.psw.PM,
		p->systate.psw.R,
		p->systate.psw.I,
		p->systate.psw.ISC,
		p->systate.psw.TM,
		p->systate.psw.FT
	);
	printf("\n");
	dodmddelay();
	printf("r3:\t%x\n", p->systate.r3);
	printf("r4:\t%x\n", p->systate.r4);
	printf("r5:\t%x\n", p->systate.r5);
	printf("r6:\t%x\n", p->systate.r6);
	printf("r7:\t%x\n", p->systate.r7);
	printf("r8:\t%x\n", p->systate.r8);
	dodmddelay();
	printf("oap:\t%x\n", p->systate.oap);
	printf("opc:\t%x\n", p->systate.opc);
	printf("osp:\t%x\n", p->systate.osp);
	printf("ofp:\t%x\n", p->systate.ofp);
	printf("isp:\t%x\n", p->systate.isp);
	printf("pcbp:\t%x\n", p->systate.pcbp);
	printf("\n");
	dodmddelay();
	printf("fltcr:\treqacc\txlevel\tftype\n");
	printf("\t%d\t%d\t%d\n", p->systate.mmufltcr.reqacc,
		p->systate.mmufltcr.xlevel,
		p->systate.mmufltcr.ftype);
	printf("fltar:\t0x%x\n", p->systate.mmufltar);
	printf("\n");
	printf("\tsrama\tsramb\n");
	dodmddelay();
	for (i = 0; i < 4; i++)
		printf("[%d]\t0x%x\t0x%x\n",
		  i, p->systate.mmusrama[i],
		  p->systate.mmusramb[i].SDTlen);
	printf("\n");
	dodmddelay();
	printf("time\tmessage\n");
	for (i = 0; i < NERRLOG; i++) {
		printf("\n");
		printf("-------------------------------------------\n");
		printf("%d\n", p->errlog[i].time);
		if ((unsigned)sdata <= (unsigned)p->errlog[i].message &&
		    (unsigned)p->errlog[i].message < (unsigned)sdata + (unsigned)bssSIZE)
			printf(p->errlog[i].message,
				p->errlog[i].param1,
				p->errlog[i].param2,
				p->errlog[i].param3,
				p->errlog[i].param4,
				p->errlog[i].param5);
		else
			printf("(0x%x, 0x%x, 0x%x)",
				p->errlog[i].message,
				p->errlog[i].param1,
				p->errlog[i].param2,
				p->errlog[i].param3,
				p->errlog[i].param4,
				p->errlog[i].param5);
		dodmddelay();
	}
	printf("\n");
	dodmddelay();
}

/*
 * The following is an interface routine for the drivers.
 */

/*PRINTFLIKE1*/
void
#ifdef __STDC__
dri_printf(char *fmt, ...)
#else
dri_printf(fmt)
	char *fmt;
#endif
{
	xcmn_err(CE_CONT, &fmt);
}

#ifdef DEBUG
int aask, aok;
#endif

/*
 * Called by the ASSERT macro in debug.h when an assertion fails.
 */

int
assfail(a, f, l)
	register char *a;
	register char *f;
	int l;
{
	/*
	 * Again, force message to go to console, not processes.
	 */
	conslogging = 0;

#ifdef DEBUG
	if (aask)  {
		cmn_err(CE_NOTE, "ASSERTION CAUGHT: %s, file: %s, line: %d",
		a, f, l);
		call_demon();
	}
	if (aok)
		return 0;	
#endif

	cmn_err(CE_PANIC, "assertion failed: %s, file: %s, line: %d", a, f, l);
	/* NOTREACHED */

}

void
nomemmsg(func, count, contflg, lockflg)
	register char	*func;
	register int	count;
	register int	contflg;
	register int	lockflg;
{
	cmn_err(CE_NOTE,
		"%s - Insufficient memory to%s %d%s page%s - %s",
		func, (lockflg ? " lock" : " allocate"),
		count, (contflg ? " contiguous" : ""),
		(count == 1 ? "" : "s"),
		"system call failed");
}

#ifdef DEBUG
sysin() {}
sysout() {}
sysok() {}
sysoops() {}

/* 
 ** set a breakpoint here to get back to demon at regular intervals.
 */
catchmenow() {}

printputbuf()
{
	register int		pbi;
	register int		cc;
	register int		lim;
	register int		opl;
	int			delay;
	struct tty		*errlayer();

	asm("	PUSHW  %r0");
	asm("	PUSHW  %r1");
	asm("	PUSHW  %r2");

	opl = splhi();

	pbi = putbufrpos % putbufsz;
	lim = putbufsz;

	while (1) {
		if (pbi < lim  &&  (cc = putbuf[pbi++])) {
			iuputchar(cc);
			if (cc == '\n')
				dodmddelay();
		} else {
			if (lim == putbufrpos % putbufsz) {
				break;
			} else {
				lim = putbufrpos % putbufsz;
				pbi = 0;
			}
		}
	}

	splx(opl);

	asm("	POPW  %r2");
	asm("	POPW  %r1");
	asm("	POPW  %r0");
}

#endif
