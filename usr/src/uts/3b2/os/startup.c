/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/startup.c	1.41"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/psw.h"
#include "sys/boot.h"
#include "sys/sbd.h"
#include "sys/firmware.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/nvram.h"
#include "sys/pcb.h"
#include "sys/systm.h"
#include "sys/signal.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/map.h"
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/utsname.h"
#include "sys/tty.h"
#include "sys/var.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"
#include "sys/disp.h"
#include "sys/class.h"
#include "sys/mman.h"
#include "sys/vnode.h"
#include "sys/session.h"
#include "sys/kmem.h"
#include "sys/sys3b.h"
#include "sys/file.h"
#include "sys/uio.h"

#include "vm/seg.h"
#include "vm/seg_kmem.h"
#include "vm/seg_vn.h"
#include "vm/seg_u.h"
#include "vm/seg_map.h"
#include "vm/page.h"

extern void dmainit();

#ifdef __STDC__
STATIC void datainit(void);
STATIC int sysseginit(int);
STATIC int kvm_init(int);
STATIC void p0init(void);
STATIC void devinit(void);
#else
STATIC void datainit();
STATIC int sysseginit();
STATIC int kvm_init();
STATIC void p0init();
STATIC void devinit();
#endif

STATIC int icdblk = 0;

/*
 * The following page table is used for the first
 * segment of the kernel's virtual space, that is,
 * the gate table.  This is result of the hardware
 * implementation.  It seems that when writing the
 * result of an SPOP instruction, write protection
 * doesn't work.  You can write anywhere that you
 * have read permission.  
*/

STATIC int gateptbl[NPGPT + 8];
int	*gateptbl0;

/*
 * Routine called before main, to initialize various data structures.
 * The arguments are:
 *	physclick - The click number of the first available page
 *		    of memory.
 */

void
mlsetup(physclick)
	int	physclick;
{
	register unsigned	nextclick;
	register unsigned	i;
	register int		*pte;
	register sde_t		*sde;
	extern int		sbss;		/* Start of kernel bss. */
	extern int		bssSIZE[];	/* Size of bss in bytes.*/
	extern int       	ptbl_init();
	int zero;

	/*
	 * Reinitialize the data area to support
	 * booting from an absolute file which
	 * was written after the system was
	 * initialized.
	 */

	datainit();

	/*
	 * Zero all BSS
	 */

	bzero((caddr_t)sbss, (int)bssSIZE);
	
	/*
	 * Set up the page table for segment 0.  First
	 * get the physical addresses.  Then build the
	 * page table.  Note that a page table must
	 * be aligned on a 32-byte boundary.
	 * Also, change protections on segment 0 based on value in s3btlc_state
	 * for trapping on low core accesses from user mode
	 */

	nextclick = btoc(svirtophys(0));
	pte = gateptbl;
	pte = (int *)(((int)pte + 31) & ~0x1f);
	gateptbl0 = pte;

	for(i = 0; i < NPGPT; i++, nextclick++)
		pte[i] = mkpte(PG_V | PG_W, nextclick);
	pte[i-1] |= PG_LAST;
	sde = (sde_t *)srama[0];
	ASSERT(sde->seg_flags & SDE_C_bit);
	sde->wd2.address = svirtophys(pte);
	sde->seg_flags &= ~SDE_C_bit;
	sde->seg_prot = (sde->seg_prot & KACCESS) | 
		((s3btlc_state == S3BTLC_DISABLE) ? URE : UNONE);
	flushmmu(0, NPGPT);

	if (fast32b())
		cache_on();
	else
		cache_off();

	/*
	 * Set up memory parameters.
	 */

 	rnvram((char *) &(UNX_NVR->icdsize), (char *)&icdblk, 4);
 	if (icdblk) {
		zero = 0;
 		wnvram((char *)&zero, (char *)&(UNX_NVR->icdsize), 4);
	 	i = btoct(VSIZOFMEM - icdblk * ICDBLKSZ);
	} else
		i = btoct(VSIZOFMEM);
	if (v.v_maxpmem  &&  (v.v_maxpmem < i))
		i = v.v_maxpmem;
	physmem = i;
	maxclick = btoc(MAINSTORE) + i;

	ASSERT(physclick < maxclick);

	/*
	 * Zero all unused physical memory.
	 * Note that all physical memory is
	 * mapped to the indentical address
	 * in section 0 of the kernel's virtual
	 * address space.
	 */

	bzero((caddr_t)ctob(physclick), ctob(maxclick - physclick));

	/*
	 * Initialize memory mapping for sysseg,
	 * the segment from which the kernel
	 * dynamically allocates space for itself.
	 */

	nextclick = sysseginit(physclick);

	/* Initialize the proc table */

	ptbl_init();

	/*
	 * Initialize the map used to allocate kernel virtual space.
	 */

	mapinit(sptmap, v.v_sptmap);
	mfree(sptmap, SYSSEGSZ, btoc(syssegs));

	/*
	 * Initialize kas stuff, including mmu segment/page tables
         * for the segkmap vm segment.
	 * At the point that kvseg is initialized,
	 * and the page pool is initialized, sptalloc becomes useable.
	 */

	nextclick = kvm_init(nextclick);

	/*
	 * Do scheduler initialization.
	 */

	dispinit();

	/*
	 * Initialize the high-resolution timer free list.
	 */
	
	hrtinit();

	/*
	 * Initialize the interval timers free list.
	 */

	itinit();

	/*
	 * Initialize process 0.
	 */

	p0init();
}


/*
 * The unix object file is written by a user program
 * (mkunix) after the system is booted.  Therefore,
 * the data has been modified from its initial
 * state.  This routine saves the initial state
 * on the first (config) boot and restores it
 * on all subsequent boots.
*/

STATIC void
datainit()
{
	static int		first_time = 1;
	static struct var	copyv = { 0 };

	if (first_time)
		copyv = v;
	else {
		v = copyv;
	}
	first_time = 0;
}

/*
 * Allocate page tables for the kernel segment sysseg and
 * and initialize the segment table for it.
 */

STATIC int
sysseginit(physclick)
	register int	physclick;
{
	register int		pgcnt;
	register sde_t		*sptr;
	register char		*ptptr;
	sde_t			sdeproto;

	/*
	 * Set up a prototype with the permissions and flags we want.
	 */

	sdeproto.seg_prot  = KRWE;
	sdeproto.seg_len   = ctomo(NCPS);
	sdeproto.seg_flags = SDE_V_bit | SDE_P_bit;

	/*
	 * Now loop through all of the segment table
	 * entries for sysseg and initialize them
	 * to point to the correct kernel page
	 * tables.
	 */

	sptr = kvtokstbl(syssegs);
	ptptr = (char *)ctob(physclick);

	for (pgcnt = 0; pgcnt < SYSSEGSZ; pgcnt += NPGPT, sptr++) {
		*sptr = sdeproto;
		sptr->wd2.address = (int)ptptr;
		ptptr += sizeof(ptbl_t);
	}

	/*
	 * Set the address of the kernel page table.
	 * We are actually using a physical address.
	 * This works since all of physical memory
	 * is mapped 1-to-1 into the kernel's virtual
	 * address space.
	 */

	kptbl = (pte_t *)ctob(physclick);

	return btoc(ptptr);
}

STATIC int smsegs = 0;
STATIC int susegs = 0;

struct	seg *segkmap;		/* kernel generic mapping segment */
pte_t	*ksegmappt;
pte_t	*eksegmappt;
struct	seg *segu;		/* u area mapping segment */
pte_t	*ksegupt;
pte_t	*eksegupt;
int	page_hashsz;		/* Size of page hash table (power of 2) */
struct	page **page_hash;	/* page hash table */

STATIC int
kvm_init(nextfree)
	int nextfree;
{
	register int	m;
	register int	i;
	register sde_t	*sptr;
	register char	*ptptr;
	sde_t		sdeproto;
	struct page *pp;
	struct segmap_crargs a;
	extern int	stext;		/* Start of kernel text. */
	extern int	sbss;		/* Start of kernel bss. */
	extern int	bssSIZE[];	/* Size of bss in bytes.*/
	register int		pgcnt;
	extern int	kvsegmap[];	/* kv segmap */
	extern int	kvsegu[];	/* u area segment */
	uint page_base;

	/*
	 * First, set up basic kernel vm segments.
	 */

	(void) seg_attach(&kas, 0, MAINSTORE+VSIZOFMEM, &kpseg);
	(void) segkmem_create(&kpseg, (caddr_t)NULL);
	(void) seg_attach(&kas, (caddr_t)stext,
		(unsigned)bssSIZE + (unsigned)sbss - (unsigned)stext,
		&ktextseg);
	(void) segkmem_create(&ktextseg, (caddr_t)NULL);
	(void) seg_attach(&kas, syssegs, ctob(SYSSEGSZ), &kvseg);
	(void) segkmem_create(&kvseg, kptbl);

	/*
	 * Now take the user-level style (pt followed by mapping table)
	 * page tables for the segmap kv space.
	 */

	smsegs = ctost(maxclick) - ctos(nextfree);
	if (smsegs <= 0)
		cmn_err(CE_PANIC, "No space for mapping files");

	if (smsegs < 1)
		smsegs = 1;

	i = stoc(smsegs);

	/*
	 * Set up a prototype with the permissions and flags we want.
	 */

	sdeproto.seg_prot  = KRWE;
	sdeproto.seg_len   = ctomo(NCPS);
	sdeproto.seg_flags = SDE_V_bit | SDE_P_bit;

	/*
	 * Now loop through all of the segment table
	 * entries for segmap and initialize them
	 * to point to the correct kernel page
	 * tables.
	 */

	sptr = kvtokstbl(kvsegmap);
	ptptr = (char *)ctob(nextfree);

	for (pgcnt = 0; pgcnt < i; pgcnt += NPGPT, sptr++) {
		*sptr = sdeproto;
		sptr->wd2.address = (int)ptptr;
		ptptr += 2*sizeof(ptbl_t);
	}

	/*
	 * Set the address of the kernel page table.
	 * We are actually using a physical address.
	 * This works since all of physical memory
	 * is mapped 1-to-1 into the kernel's virtual
	 * address space.
	 */
	ksegmappt = (pte_t *)ctob(nextfree);

	/* LINTED */
	eksegmappt = (pte_t *)ptptr;
	nextfree = btoc(ptptr);

	/* floating u area support */
	susegs = ctos(v.v_proc * USIZE);
	if (susegs <= 0)
		cmn_err(CE_PANIC, "No space for mapping u areas");

	i = stoc(susegs);

	/*
	 * Set up a prototype with the permissions and flags we want.
	 */

	sdeproto.seg_prot  = KRWE;
	sdeproto.seg_len   = ctomo(NCPS);
	sdeproto.seg_flags = SDE_V_bit | SDE_P_bit;

	/*
	 * Now loop through all of the segment table
	 * entries for segu and initialize them
	 * to point to the correct kernel page
	 * tables.
	 */

	sptr = kvtokstbl(kvsegu);
	ptptr = (char *)ctob(nextfree);

	for (pgcnt = 0; pgcnt < i; pgcnt += NPGPT, sptr++) {
		*sptr = sdeproto;
		sptr->wd2.address = (int)ptptr;
		ptptr += 2*sizeof(ptbl_t);
	}


	/*
	 * Set the address of the kernel page table.
	 * We are actually using a physical address.
	 * This works since all of physical memory
	 * is mapped 1-to-1 into the kernel's virtual
	 * address space.
	 */

	ksegupt = (pte_t *)ctob(nextfree);

	/* LINTED */
	eksegupt = (pte_t *)ptptr;
	nextfree = btoc(ptptr);

	/*
	 * Allocate space for the page accounting.
	 */

	/*
	 * Compute hash size and round up to power of 2.
	 */
	m = (maxclick - nextfree) / PAGE_HASHAVELEN;
	while (m & (m - 1))
		 m = (m | (m - 1)) + 1;
	page_hashsz = m;
	i = m * sizeof(struct page *);
	m = maxclick - nextfree;
	i += m * sizeof(struct page);
	i = btoc(i);
	page_base = nextfree + i;	/* so page_numtookpp()
					 * returns NULL to segkmem_mapin().
					 */
	pp = ((struct page *)sptalloc(i, PG_V, (caddr_t)nextfree, 0));
	page_hash = (struct page **) (pp + m);
	nextfree +=i;
	hat_init();
	maxmem = maxclick - nextfree;
	page_init(pp, maxmem, page_base);
	memialloc(nextfree, maxclick);
	availrmem = maxmem;
	availsmem = maxmem;

	kmem_init();

	segkmap = seg_alloc(&kas, kvsegmap, stob(smsegs));
	if (segkmap == NULL)
		cmn_err(CE_PANIC, "cannot allocate segkmap");
	a.prot = PROT_READ | PROT_WRITE;
	if (segmap_create(segkmap, (caddr_t)&a) != 0)
		cmn_err(CE_PANIC, "segmap_create segkmap");

	/* floating u area support */
	segu = seg_alloc(&kas, kvsegu, stob(susegs));
	if (segu == NULL)
		cmn_err(CE_PANIC, "cannot allocate segu");
	a.prot = PROT_READ | PROT_WRITE;
	if (segu_create(segu, (caddr_t)&a) != 0)
		cmn_err(CE_PANIC, "segu_create segu");

	/*
	 * Return next available physical page.
	 */

	return nextfree;
}

/*
 * The following pcb is used to initialize the u_kpcb
 * field of the u-block.  This is so that the CALLPS
 * done in ttrap.s to switch to the kernel stack for
 * a system call will go off to the system call
 * handler, systrap, in trap.c
 */

extern void	systrap();

struct kpcb	kpcb_syscall = {

			/*
			 * The initial PSW with CM = kernel,
			 * PM = user, and the I and R bits
			 * set.  This is followed by the
			 * initial PC and initial SP.
			 */
			
			{ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 0, 0, 0 },
			systrap,
			0 },

			/*
			 * Save area for PSW, PC, and SP.
			 */

			ZPSW, 0, 0,

			/*
			 * Stack lower bound and stack upper bound.
			 */

			0,
			(int *)0xC0000000 + ctob(USIZE) / sizeof(int),

			/*
			 * Savearea for ap, fp, r0 - r10.
		 	*/

			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

			/*
			 * The following indicates no block moves.
			 */

			0,
	} ;


/*
 * Set up proc 0.
 */

psw_t	p0init_psw = IPSW;
struct  seguser *p0seguser;

STATIC void
p0init()
{
	register int	ii;
	register pte_t	*ptptr;
	extern int	userstack[];
	register proc_t	*pp;
	register pte_t  *uptr;
	register sde_t	*sdeptr;
	caddr_t 	addr;

	/*
	 * Fix up the syscall pcb.
	 */

	kpcb_syscall.ipcb.sp = u.u_stack;
	kpcb_syscall.slb = u.u_stack;

	/*
	 * Initialize proc 0's ublock
	 */

	u.u_pcb.psw = p0init_psw;
	u.u_pcb.sp  = userstack;
	u.u_pcb.pc  = (void (*)())UVTEXT;
	u.u_pcb.slb = (int *)UVBASE;
	u.u_pcb.sub = 0;
	u.u_pcb.regsave[K_AP] = (unsigned) userstack;
	u.u_pcb.regsave[K_FP] = (unsigned) userstack;
	u.u_tsize   = 0;
	u.u_dsize   = 0;
	u.u_ssize   = 0;
	u.u_ar0     = &u.u_pcb.regsave[K_R0];
	u.u_kpcb    = kpcb_syscall;
	pp = (struct proc *)kmem_zalloc(sizeof(struct proc), KM_SLEEP);
	if(pp == NULL){
		cmn_err(CE_PANIC, "process 0 - creation failed\n");
	}
	nproc[0]    = pp;
	u.u_procp   = pp;
	u.u_cmask   = (mode_t) CMASK;

	bcopy((caddr_t)rlimits, (caddr_t)u.u_rlimit, 
		sizeof(struct rlimit) * RLIM_NLIMITS);
	/*
	 * Initialize process data structures
	 */

	curproc = pp;
	curpri = v.v_maxsyspri;

	pp->p_stat = SONPROC;
        pp->p_flag = SLOAD | SSYS | SLOCK | SULOAD | SDETACHED;
        pp->p_pri = v.v_maxsyspri;
        pp->p_cid = 0;
        pp->p_clfuncs = class[0].cl_funcs;
        pp->p_clproc = (caddr_t)pp;

	forksession(nproc[0],&session0);

	pp->p_segu = (struct seguser *)p0seguser;
	addr = (caddr_t)p0seguser;
	ptptr = (pte_t *)ubptbl(nproc[0]);
	for (ii = 0; ii < USIZE; ii++, ptptr++) {
		ptptr->pg_pte =
			mkpte(PG_V, btoc(kvtophys((char *)&u + ctob(ii))));
		sdeptr = (sde_t *)kvtokstbl(addr);
		uptr = (pte_t *)vatopte(addr, sdeptr);
		uptr->pg_pte = ptptr->pg_pte;
		addr += PAGESIZE;
	}
}


/*
 * Initialize uname info.
 * Machine-dependent code.
 */
void
inituname()
{
	struct vnode	*vp;
	int		resid;
	char 		buf[256];
	int		len;
	extern char	*release;
	extern char	*version;

	/*
	 * Get nodename if stored.
	 * NOTE:  Name of machine is stored in a file because the
	 * non-volatile RAM on the porting base is not large enough
	 * to hold an internet protocol hostname.  If a machine
	 * has non-volatile RAM which is large enough, however, it
	 * would be preferable to store the nodename there.
	 */
	if (vn_open("/etc/nodename", UIO_SYSSPACE, FREAD, 0, &vp) == 0) {
		if (vn_rdwr(UIO_READ, vp, (caddr_t)buf, SYS_NMLN-1, 0, 
	     	  UIO_SYSSPACE, 0, 0, u.u_cred, &resid) == 0) {
			len = (SYS_NMLN - 1) - resid;
			if (len != 0) {
				strncpy(utsname.nodename, buf, len);
				utsname.nodename[len] = '\0';
			}
		}
	}
	/*
	 * Get the release and version of the system.
	 */
	if (release[0] != '\0') {
		strncpy(utsname.release, release, SYS_NMLN-1);
		utsname.release[SYS_NMLN-1] = '\0';
	}
	if (version[0] != '\0') {
		strncpy(utsname.version, version, SYS_NMLN-1);
		utsname.version[SYS_NMLN-1] = '\0';
	}
	cmn_err(CE_CONT, "\nUNIX(R) System V Release %s AT&T %s Version %s\n",
	  utsname.release, utsname.machine, utsname.version);
	cmn_err(CE_CONT, "Node %s\n", utsname.nodename);
	cmn_err(CE_CONT, "Total real memory  = %d\n", ctob(physmem));
	return;
}
	

/*
 * Machine-dependent startup code.
 */

void
startup()
{
	/*
	 * Confirm that the configured number of supplementary groups
	 * is between the min and the max.  If not, print a message
	 * and assign the right value.
	 */
	if (ngroups_max < NGROUPS_UMIN){
		cmn_err(CE_NOTE, 
		  "Configured value of NGROUPS_MAX (%d) is less than \
min (%d), NGROUPS_MAX set to %d\n", ngroups_max, NGROUPS_UMIN, NGROUPS_UMIN);
		ngroups_max = NGROUPS_UMIN;
	}
	if (ngroups_max > NGROUPS_UMAX){
		cmn_err(CE_NOTE,
		  "Configured value of NGROUPS_MAX (%d) is greater than \
max (%d), NGROUPS_MAX set to %d\n", ngroups_max, NGROUPS_UMAX, NGROUPS_UMAX);
		ngroups_max = NGROUPS_UMAX;
	}

	devinit();
	dmainit();
}

/*
 * Adjust devs based on boot options.
 */

STATIC void
devinit()
{
	/*
	 * Fix up the devices if we were booted from floppy disk.
	 */
	if (VBASE->p_cmdqueue->b_dev == FLOPDISK) {
		rootdev = makedevice(getemajor(rootdev), FLOPMINOR);
	}
	if (VBASE->p_cmdqueue->b_dev == ICD) {
		extern dev_t icdmkdev();

		rootdev = icdmkdev(ICDROOT);
	}
}
