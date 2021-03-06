/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:ml/pstart.c	1.12"

#include "sys/types.h"
#include "sys/psw.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/sbd.h"
#include "sys/csr.h"
#include "sys/sit.h"
#include "sys/immu.h"
#include "sys/fs/s5dir.h"
#include "sys/user.h"
#include "sys/pcb.h"
#include "sys/firmware.h"
#include "sys/nvram.h"
#include "sys/var.h"
#include "sys/iobd.h"
#include "sys/debug.h"
 
#define SYSSPSZ		16
#define KSCRSZ		16
#define SBDREGS		ctob(2*NCPS)

/*
 * PCB used by physical portion of UNIX startup (initialized in uprt.s)
 */

#ifdef __STDC__
STATIC void wzero(int *, int);
STATIC void mmusetup(int, int, int, int, int);
#else
STATIC void wzero();
STATIC void mmusetup();
#endif

extern void pinset();
extern int crash_timestamp();
extern void cdump();

struct pcb phys_pcb = { KPSW0, 0, 0, 0, 0, {0,0,0,0,0,0,0,0,0,0,0}, 0};

/*
 * These addresses are set by cunix
 * based on the addresses indicated in the COFF
 * ifile or ELF mapfile.
 */

int sgate = 0x0;
int sboot = 0x0;
int stext = 0x0;
int sdata = 0x0;
int sbss  = 0x0;

extern int bootSIZE[], gateSIZE[];
extern int textSIZE[], dataSIZE[], bssSIZE[];

extern int	unxsbdst;
extern int	krnl_isp[];
extern char	*mmusrama;
extern char	*mmusramb;

extern struct s3bsym	sys3bsym;	/* Symbol table		*/
					/* produced by lboot.	*/

/* 
 *  Physical addresses
 */

int Sgate = 0;
int Egate = 0;
int Stext = 0;
int Etext = 0;
int Sdata = 0;
int Edata = 0;
int Sbss = 0;
int Ebss = 0;
int END = 0;

STATIC sde_t	*st_top0 = 0;	/* Base of segment table for section 0.	*/
STATIC sde_t	*st_top1 = 0;	/* Base of segment table for section 1.	*/
STATIC sde_t	*st_top3 = 0;	/* Base of segment table for section 3.	*/

STATIC int crash_sync = 0;	/* set to clock value first time this
				   function is entered so crash can sync
				   system images with namelist files */
STATIC int crashsw = 0;		/* software switch for crash dump 
				   Set to 1 in pstart. If already
				   set on entry to pstart, a crash 
				   dump may be made */


/*
 * Physical start-up routine;  this routine is the very first C routine
 * executed immediately after control is transferred to SPMEM from boot.
 */

void
pstart()
{
	register int	pclkno;
	register int	clksize;
	register int	physad;
	register int	tmp;
	register int	virtad;
	register int	mmst;
	int		sdtad0;
	int		sdtad1;
	int		sdtad3;
	struct user	*pu;

	/*
	 *	Set the physical addresses
	 */

	Sgate = (((sboot + 2047 >> 11) + 
	  (((int)bootSIZE + 2047) >> 11)) << 11);
	Stext = (Sgate + ((((int)gateSIZE + 2047) >> 11) << 11));
	Etext = Stext + (int)textSIZE -1;
	Sdata = (Stext + ((((int)textSIZE + 2047) >> 11) << 11));
	Edata = Sdata + (int)dataSIZE -1;
	Sbss  = (Sdata + ((((int)dataSIZE + 2047) >> 11) << 11));
	Ebss  = Sbss + (int)bssSIZE -1;
	END   = Sbss + (int)bssSIZE - 1;

	if (SERNO->serial0 > 0xB && SERNO->serial0 < 0x20
	  && ((int)SERNO) == 0xfff0)
		P_SYMTELL((char *)Sdata + ((char *)&sys3bsym - (char *)sdata));

	/*
	 * Check to see if this is the second time this point
	 * has been reached since boot. If so, generate a crash dump.
	 */

	if (crashsw == 1) {	/* generate crash dump */
		cdump();

		/* return to firmware control */
		RUNFLG = REENTRY;
		RST = ON;
	} else {		/* first time */
		crashsw = 1;
		/* set runflag in case of hw reset */

		RUNFLG = FATAL;

		/* set time stamp for crash  if not already set*/

		if (crash_sync == 0)
			crash_sync = crash_timestamp() ;
	}
	SBDSIT->command = ITINIT;	/* stop the interval timer */
					/* if running after a reset */
	tmp = SBDSIT->c_latch;		/* clear pending interrupt */



	/*
	 * Clear mmu configuration register so that ste
	 * reference and modify bits are mot set.
	 */

	*(int *)&mmucr = 0;
	*(int *)&mmufltcr = 0;
	*(int *)&mmufltar = 0;

	/*
	 * The first available free memory after the kernel
	 * .bss section 
	 */

	 mmst = btoc(END);

	/*
	 * setup MMU table space for Section 0
	 * segment tables must be aligned on a 32 byte physical address
	 * 3B2 requires segments to have a 2K boundary
	 * Using the ctob() conversion always guarantees that
	 * both of these conditions are met.
	 */

	sdtad0 = ctob(mmst);

	/* setup SRAMA and SRAMB for section 0. */

	srama[0] = sdtad0;
	sramb[0].SDTlen = NS0SDE - 1;

	st_top0 = (sde_t *)sdtad0;	/* for use in mmusetup */

	/*
	 * setup MMU table space for Section 1
	 * SDT 1 must also be aligned to a 32 byte physical address.
	 * Pad the size of SDT 0 (NS0SDE) to be a integral multiple 
	 * of 32 and the size of SDT 1 will be at the correct alignment.
	 */

	sdtad1 = sdtad0 + (NS0SDE * sizeof(sde_t));
	
	/* setup SRAMA and SRAMB for Section 1 */

	srama[1] = sdtad1;
	sramb[1].SDTlen = NS1SDE - 1;
	
	st_top1 = (sde_t *)sdtad1;

	/*
	 * setup MMU table space for Section 3
	 * SDT 3 must also be aligned to a 32 byte physical address.
	 * Pad the size of SDT 1 (NS1SDE) to be a integral multiple 
	 * of 32 and the size of SDT 3 will be at the correct alignment.
	 */

	sdtad3 = sdtad1 + (NS1SDE * sizeof(sde_t));
	
	/* setup SRAMA and SRAMB for Section 3 */

	srama[3] = sdtad3;
	sramb[3].SDTlen = NS3SDE - 1;
	
	st_top3 = (sde_t *)sdtad3;

	/* zero out section 0, 1, and 3 SDT  */

	wzero((int *)st_top0, NS0SDE * sizeof(sde_t));
	wzero((int *)st_top1, NS1SDE * sizeof(sde_t));
	wzero((int *)st_top3, NS3SDE * sizeof(sde_t));

	/*
	 * Pclkno will keep a count of physical memory clicks as they're
	 * allocated.  Skip over boot text and data.
	 */

	 pclkno = btoc((int)sboot) + btoc((int)bootSIZE);

	/*
	 * set up gate segment mapping
	 */

 	physad = ctob(pclkno);
	clksize = btoc((int)gateSIZE);

	mmusetup(0,physad,clksize,KRE|URE,1);
	pclkno += clksize;


	/*
	 * set up kernel text segment mapping
	 */

	physad = ctob(pclkno);
	clksize = btoc((int)textSIZE);

	mmusetup(stext,physad,clksize,KRE|UNONE,1);
	pclkno += clksize;

	/*
	 * set up kernel data segment
	 */

	physad = ctob(pclkno);
	clksize = btoc((int)dataSIZE);

	mmusetup(sdata,physad,clksize,KRWE|UNONE,1);
	pclkno += clksize;

	/*
	 * setup kernel bss segment
	 */

	physad = ctob(pclkno);
	clksize = btoc((int)bssSIZE);

	mmusetup(sbss,physad,clksize,KRWE|UNONE,1);

	/*
	 * skip memory for MMU tables (already set up - mapped
	 * in Mainstore)
	 */

	pclkno = btoc(sdtad0 +
			((NS0SDE + NS1SDE + NS3SDE) * sizeof(sde_t)));

	/*
	 * set up u_block for proc 0
	 */

	physad = ctob(pclkno);
	wzero((int *)physad, ctob(USIZE));   /* zero the u_block  */
	pu = (struct user *)physad;

	mmusetup(0xC0000000,physad,USIZE,KRWE|UNONE,1);
	pclkno += USIZE;


	pu->u_kpcb.sub = (int *)0xC0000000 + ctob(USIZE) / sizeof(int);


	/* set up SRAMA and SRAMB for section 2.  */

	srama[2] = (int)(NULL);
	sramb[2].SDTlen = NS2SDE - 1;

	/*
	 * set up demon text segment
	 */

	clksize = NCPS;
	mmusetup(VROM, 0, clksize, KRE|UNONE, 1);


	/*
	 * set up kernel's system board internal mapping
	 */

	mmusetup((int)&unxsbdst,SBDREGS,clksize,KRWE|URE,1);


	/*
	 *  setup memory mapping for i/o segments 1-E
	 *  	0x60000 - 1ffffff
	 */

	for (tmp=0, virtad=VKIO, physad=PKIO ; tmp<14 ; tmp++) {
		mmusetup(virtad,physad,KIOSZ,KRWE|UNONE,1);
		virtad += ctob(KIOSZ);
		
		/* leave an invalid segment */

		virtad += NBPS;
		physad += ctob(KIOSZ);
	}
	

	/*
	 * set up mainstore map
	 */

	clksize = btoc(SIZOFMEM);
	mmusetup(MAINSTORE,MAINSTORE,clksize,KRWE|UNONE,1);
 
	/*
	 * Pass the results to the virtual start-up routine
	 */

	pinset(pclkno);
}

STATIC void
mmusetup(vaddr, paddr, clks, prot, valid)
int vaddr, paddr, clks, prot, valid;
{
	register sde_t	*sde;
	int		i;

	/*
	 * set sde to point to correct segment descripter
	 */

	switch (SECNUM(vaddr)) {
		case 0:
			sde = st_top0 + SEGNUM(vaddr);
			break;
		case 1:
			sde = st_top1 + SEGNUM(vaddr);
			break;
		case 2:
		case 3:
			sde = st_top3 + SEGNUM(vaddr);
			break;
	}

	/*
	 *  Set up a segment descripter for each segment
	 */

	for (i = ctos(clks);i > 0; i--, sde++, paddr += NBPS) {
		sde->seg_prot = prot;
		if (clks > NCPS) {
			sde->seg_len = ctomo(NCPS);	/* full segment */
			clks -= NCPS;
		} else
			sde->seg_len = ctomo(clks);
		if (valid == 1)
			sde->seg_flags = SDE_kflags;
		else
			sde->seg_flags = 0;
		sde->wd2.address = paddr;
	}
}

/* Zero a word aligned area of memory.  We can't use
 * the bzero routine because it is in the virtual
 * address space which isn't mapped yet.
 *
 *	wzero(adr, byte_count);
 */

STATIC void
wzero(ptr, count)
	register int	*ptr;
	register int	count;
{
	count >>= 2;	/* Convert byte count to words. */

	while (count--)
		*ptr++ = 0;
}
