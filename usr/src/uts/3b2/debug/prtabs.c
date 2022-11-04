/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debug:debug/prtabs.c	1.21"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/signal.h"
#include "sys/proc.h"
#include "sys/fs/s5dir.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/var.h"
#include "sys/evecb.h"
#include "sys/hrtcntl.h"
#include "sys/cmn_err.h"
#include "sys/hrtsys.h"
#include "sys/inline.h"
#include "sys/swap.h"
#include "sys/immu.h"
#include "vm/vm_hat.h"
#include "vm/as.h"
#include "vm/page.h"
#include "vm/seg.h"
#include "vm/hat.h"
#include "vm/seg_vn.h"
#include "vm/anon.h"

char	prtabs_buf[200];

char	*prproc_hdg =
	" SLOT    ADDR    PID PPID STATUS   WCHAN  FLAGS              \tCMD\n\n";

char	*prprocalarm_hdg = "    CLOCK       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";
                                                                            
char	*prhralarm_hdg = "    PROCP       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";
                                                                            
char	*prproc_stat[] = {
		"UNUSED ",
		"SLEEP  ",
		"RUN    ",
		"ZOMBIE ",
		"STOP   ",
		"IDLE   ",
		"ONPROC ",
		"SXBRK  ",
		"GOOFED1",
		"GOOFED2",
	};

char	*prproc_flgs[] = {
				"Sys ",
				"Trc ",
				"Prwake ",
				"Nwake ",
				"Load ",
				"Lock ",
				"Rsig ",
				"Poll ",
				"Prstop ",
				"Proctr ",
				"Procio ",
				"Prfork ",
				"Propen ",
				"Uload ",
				"Runlcl ",
				"Nostop ",
				"Ptrx ",
				"Asleep ",
				"Uswap ",
				"Uwant ",
				"", /* don't need to print SEXECED */
				"Detached ",
				"",
				"",
				"Jcproc ",
				"Nowait ",
				"Vfork ",
				"Vfdone ",
				"Swlocks ",
				"Xstart ",
				"Pstart ",
				"Goofed3 ",
			};
int	prproc_flgs_sz = sizeof(prproc_flgs) / sizeof(char *);
 
printprocs()
{
	register proc_t	**pp;
	register int	lc;
	register user_t	*up;
	int		winaddr_save;
	char		buf[20];
	int		slot;

	cmn_err(CE_CONT, "^%s", prproc_hdg);

	for (slot = 0, pp = &nproc[0] ; pp < v.ve_proc ; slot++, pp++) {
		if (*pp == NULL || (*pp)->p_stat == 0)
			continue;
		sprintf(prtabs_buf, " %3d   %.8x ", slot, *pp);
		sprintf(buf, "%4d ", (*pp)->p_pid);
		strcat(prtabs_buf, buf);
		sprintf(buf, "%4d ", (*pp)->p_ppid);
		strcat(prtabs_buf, buf);
		strcat(prtabs_buf, prproc_stat[(*pp)->p_stat]);

		sprintf(buf, "%.8x ", (*pp)->p_wchan);
		strcat(prtabs_buf, buf);

		for (lc = 0  ;  lc < prproc_flgs_sz  ;  lc++) {
			if ((*pp)->p_flag & (1 << lc))
				strcat(prtabs_buf, prproc_flgs[lc]);
		}

		if ((*pp)->p_stat == SZOMB  ||  (*pp)->p_stat == SIDL) {
			cmn_err(CE_CONT, "^%s\n", prtabs_buf);
			continue;
		}

		if ((*pp)->p_flag & SULOAD) {
			up = (user_t *)KUSER((*pp)->p_segu);
			cmn_err(CE_CONT, "^%s\t%s\n", prtabs_buf, up->u_psargs);
		} else
			cmn_err(CE_CONT, "^%s\tSWAPPED\n", prtabs_buf);
		dodmddelay();

	}
}

char	*proc_clk[] = {
		"CLK_STD",
		"CLK_USERVIRT",
		"CLK_PROCVIRT"
};

printprocalarms()
{
	register int	type;
	timer_t	*hrp;
	timer_t	*nhrp;
	extern	 proc_t	*curproc;
	char		buf[30];

	type = 0;

	cmn_err(CE_CONT, "^%s", prprocalarm_hdg);
	while (type < 2) {
		hrp = curproc->p_italarm[type];
		for(; hrp != NULL; hrp = nhrp) {
			nhrp = hrp->hrt_next;
			sprintf(prtabs_buf, "%s", proc_clk[type + 1]);	
			sprintf(buf, "% 7d", hrp->hrt_time);
			strcat(prtabs_buf, buf);
			sprintf(buf, " %11d", hrp->hrt_int);
			strcat(prtabs_buf, buf);
			sprintf(buf, " %7d", hrp->hrt_cmd);
			strcat(prtabs_buf, buf);
			sprintf(buf, " %6d", hrp->hrt_ecb.ecb_eid);
			strcat(prtabs_buf, buf);
			sprintf(buf, " %13x", hrp->hrt_prev);
			strcat(prtabs_buf, buf);
			sprintf(buf, " %10x", hrp->hrt_next);
			strcat(prtabs_buf, buf);

			cmn_err(CE_CONT, "^%s\n", prtabs_buf);
		}
		type++;
	}
	dodmddelay();
}
	
printhralarms()
{
	register timer_t	*hrp;
	register timer_t	*nhrp;
	char			buf[40];
	extern	 timer_t	hrt_active;

	cmn_err(CE_CONT, "^%s", prhralarm_hdg);

	hrp = hrt_active.hrt_next;
	for (; hrp != &hrt_active; hrp = nhrp) {
		nhrp = hrp->hrt_next;
		sprintf(prtabs_buf, "%12x", hrp->hrt_proc);	
		sprintf(buf, "% 7d", hrp->hrt_time);
		strcat(prtabs_buf, buf);
		sprintf(buf, " %11d", hrp->hrt_int);
		strcat(prtabs_buf, buf);
		sprintf(buf, " %7d", hrp->hrt_cmd);
		strcat(prtabs_buf, buf);
		sprintf(buf, " %6d", hrp->hrt_ecb.ecb_eid);
		strcat(prtabs_buf, buf);
		sprintf(buf, " %13x", hrp->hrt_prev);
		strcat(prtabs_buf, buf);
		sprintf(buf, " %10x", hrp->hrt_next);
		strcat(prtabs_buf, buf);

		cmn_err(CE_CONT, "^%s\n", prtabs_buf);
	}
}

char	*segperms[] = {
		"NONE",
		"EO",
		"RE",
		"RWE",
	};

printaddrinfo(pp, adr, cnt)
register proc_t	*pp;
register uint	adr;
register int	cnt;
{
	register int	ii;

	ii = argcount();

	if(ii < 2){
		cmn_err(CE_CONT, "^%s%s\n",
			"usage: printaddrinfo <proc tbl adr> ",
			"<vaddr> [<page cnt>]");
		return;
	}
	if(ii < 3)
		cnt = 1;
	
	prtaddrinfo(pp, adr);
	for(ii = 1 ; ii < cnt ; ii++){
		adr += ctob(1);
		cmn_err(CE_CONT, "^\n");
		prtaddrinfo(pp, adr);
	}
}

prtaddrinfo(p, adr)
proc_t	*p;
register uint	adr;
{
	register sde_t	*sp;
	register pte_t	*pt;
	register int	cnt;
	struct	page	*pp;
	SRAMB		csramb;
	extern proc_t	*curproc;
	int tmp;

	cnt = adr >> 30;
	cmn_err(CE_CONT, "^Proc = 0x%x, addr = 0x%x, sect %d.\n",
		p, adr, cnt);
	dodmddelay();
	switch(cnt){
		case 0:
		case 1:
			sp = (sde_t *)srama[cnt];
			csramb = sramb[cnt];
			break;

		case 2:
		case 3:
			sp = (sde_t *)(p->p_as->a_hat.hat_srama[cnt - 2]);
			csramb = (p->p_as->a_hat.hat_sramb[cnt - 2]);
			break;
	}


	cnt = csramb.SDTlen + 1;
	sprintf(prtabs_buf, "Seg tbl at %#x, sramb: %.8x, nsegs = %d.",
		sp, *(int *)(&csramb), cnt);
	cmn_err(CE_CONT, "^%s\n", prtabs_buf);
	dodmddelay();

	if(SEGNUM(adr) > cnt){
		cmn_err(CE_CONT, "^Segment nbr (%d) too big.\n",
			SEGNUM(adr));
		dodmddelay();
		return;
	}
	sp += SEGNUM(adr);

	sprintf(prtabs_buf, "sde at %#x: K%s U%s lng = %d (%#x) bytes, ", 
		sp, segperms[(unsigned)sp->seg_prot >> 6],
		segperms[sp->seg_prot & 0x3],
		SD_SEGBYTES(sp), SD_SEGBYTES(sp));
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	if(sp->seg_flags & SDE_I_bit)
		cmn_err(CE_CONT, "^I ");
	if(sp->seg_flags & SDE_V_bit)
		cmn_err(CE_CONT, "^V ");
	if(sp->seg_flags & SDE_T_bit)
		cmn_err(CE_CONT, "^T ");
	if(sp->seg_flags & SDE_C_bit)
		cmn_err(CE_CONT, "^C ");
	if(sp->seg_flags & SDE_P_bit)
		cmn_err(CE_CONT, "^P ");
	cmn_err(CE_CONT, "^\n");
	dodmddelay();

	if(SEGOFF(adr) >= SD_SEGBYTES(sp)){
		cmn_err(CE_CONT, "^Seg offset (%d) >= seg size (%d).\n",
			SEGOFF(adr), SD_SEGBYTES(sp));
		dodmddelay();
		return;
	}
	if((sp->seg_flags & (SDE_P_bit | SDE_V_bit))  !=
	   (SDE_P_bit | SDE_V_bit)){
		cmn_err(CE_CONT, "^Segment invalid or not present.\n");
		dodmddelay();
		return;
	}

	pt = (pte_t *)sp->wd2.address;
	if(sp->seg_flags & SDE_C_bit){
		cmn_err(CE_CONT, "^Contig seg data at 0x%x\n", pt);
		dodmddelay();
		cmn_err(CE_CONT, "^0x%x = 0x%x\n", adr,
			(int)pt + SEGOFF(adr));
		dodmddelay();
		return;
	}
	cmn_err(CE_CONT, "^page tbl at 0x%x, ", pt);
	pt += PAGNUM(adr) & PNUMMASK;
	sprintf(prtabs_buf, "pte at %#x: ", 
		pt, *(int *)pt);
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	if(pt->pgm.pg_lock)
		cmn_err(CE_CONT, "^LK ");
	if(pt->pgm.pg_ndref)
		cmn_err(CE_CONT, "^NR ");
	if(pt->pgm.pg_ref)
		cmn_err(CE_CONT, "^R ");
	if(pt->pgm.pg_w)
		cmn_err(CE_CONT, "^CW ");
	if(pt->pgm.pg_last)
		cmn_err(CE_CONT, "^L ");
	if(pt->pgm.pg_mod)
		cmn_err(CE_CONT, "^M ");
	if(pt->pgm.pg_v)
		cmn_err(CE_CONT, "^P ");
	cmn_err(CE_CONT, "^\n");
	dodmddelay();

	if(pt->pgm.pg_v == 0){
		cmn_err(CE_CONT, "^Page not present.\n");
		dodmddelay();
		return;
	}
	cmn_err(CE_CONT, "^pfn = %x, ", pt->pgm.pg_pfn);
	cmn_err(CE_CONT, "^0x%x = 0x%x, ", adr,
		ctob(pt->pgm.pg_pfn) + PAGOFF(adr));
	pp = page_numtopp(pt->pgm.pg_pfn);
	cmn_err(CE_CONT, "^page struct at 0x%x.\n", pp);
	dodmddelay();
}

printbases(args)
int	args;
{
	register int	*ap;
	register int	nargs;
	register int	cnt;

	ap = &args;
	nargs = argcount();

	for(cnt = 0 ; cnt < nargs ; cnt++, ap++){
		sprintf(prtabs_buf, "%#x = %d = %#o",
			*ap, *ap, *ap);
		cmn_err(CE_CONT, "^%s\n", prtabs_buf);
		if((cnt + 1) % 10  ==  0)
			dodmddelay();
	}
}

extern struct as kas;
extern int	kvsegmap[];
extern pte_t	*ksegmappt;
extern pte_t	*eksegmappt;
extern int	kvsegu[];
extern pte_t	*ksegupt;
extern pte_t	*eksegupt;

printptdat(pt)
u_int pt;
{
	register struct page *pp;
	register struct ptdat *ptd;
	register u_int pt_pfn;
	register u_int secseg;
	register u_int pagnum;

	pt_pfn = phystopfn(pt);
	pagnum = ((u_int)pt - ((u_int)pt & ~255)) >> 2;

	if ((pp = page_numtopp(pt_pfn)) == NULL) {
		/*
		 * Must be a kernel page table.
		 */
		if ((pt >= (u_int)ksegmappt) && (pt < (u_int)ksegupt)) {
			secseg = (SCN1 << 14) |
				((SEGNUM(kvsegmap) +
				 (((pt - (u_int)ksegmappt) >> 9))) << 1);
		} else if ((pt >= (u_int)ksegupt) && (pt < (u_int)eksegupt)) {
			secseg = (SCN1 << 14) |
				((SEGNUM(kvsegu) +
				 (((pt - (u_int)ksegupt) >> 9))) << 1);
		} else {
			cmn_err(CE_CONT, "printptdat: invalid pte ptr.\n");
			return;
		}

		cmn_err(CE_CONT, "as: %x (kas)  secseg %x  inuse %d  keepcnt %d  maps addr %x\n",
			&kas,
			secseg,
			NPGPT+1,
			NPGPT+1,
			((secseg << 16) | (pagnum << PNUMSHFT)) );
	} else {
		if (pp->p_ptdats == NULL) {
			cmn_err(CE_CONT, "printptdat: invalid pte, page doesn't have ptdats.\n");
			return;
		}

		ptd = (pp->p_ptdats) + (((u_int)pt >> 9) & 3);
		/* LINTED */
		if (ptd->pt_addr == ((paddr_t)pt & ~255)) {
			cmn_err(CE_CONT, "pte belongs to a free page table:  addr %x  pp %x\n",
				ptd->pt_addr,
				ptd->pt_pp);
		} else {
			cmn_err(CE_CONT, "as: %x  secseg %x  inuse %d  keepcnt %d  maps addr %x\n",
				ptd->pt_as,
				ptd->pt_secseg,
				ptd->pt_inuse,
				ptd->pt_keepcnt,
				((ptd->pt_secseg << 16) | (pagnum << PNUMSHFT)) );

			dodmddelay();
			{
			register proc_t	**pp;
			int		slot;

			cmn_err(CE_CONT, "proc:");
			slot = 0;
			pp = &nproc[0] ;
			for ( ; pp < v.ve_proc ; slot++, pp++) {
				if (*pp == NULL || (*pp)->p_as != ptd->pt_as)
					continue;
				cmn_err(CE_CONT, " %x", *pp);
			}
			cmn_err(CE_CONT, "\n");
			}
		}
	}
	dodmddelay();
}

printas(arg)
int arg;
{
	static proc_t	*opaddr;
	proc_t	*paddr;
	extern proc_t *curproc;
	proc_t  **argptr;
	int 	argc;
	int 	j, flag;

	argc = argcount();	/* get num of args */
	argptr = (proc_t **) &arg;		/* get ptr to first arg */

	do {
		if (argc != 0)
			paddr = *argptr++;

		else {
			if (opaddr == NULL) {
				cmn_err(CE_CONT, "^No proc addr set yet\n");
				return;
			}

			paddr = opaddr;
			argc = 1;
		}

		/*
			Validate the proc table address 
		*/
	
		if ((proc_t **)paddr >= nproc && (proc_t **)paddr < v.ve_proc)
			paddr = *((proc_t **)paddr);
		else
			if ((paddr != curproc) && (paddr != opaddr)) {
				flag = 0;
				for (j = 0; j < v.v_proc ; j++)
					if (paddr == nproc[j]) {
						++flag;
						break;
					}
				if (flag == 0) {
					cmn_err(CE_CONT, "^%x is not a nproc table address\n", paddr);
					return;
				}
			}
		opaddr = paddr;
		if (paddr->p_as == (struct as *)NULL)
			cmn_err(CE_CONT, "^process has no as\n");
		else
			doas(paddr->p_as);

		if (argc > 1)
			cmn_err(CE_CONT, "^-------------------------------------------------------------------------------\n\n");

	} while (--argc);
}

dumpas(arg)
int arg;
{
	static struct as	*paddr;
	struct as  **argptr;
	int 	argc;

	argc = argcount();	/* get num of args */
	argptr = (struct as **) &arg;		/* get ptr to first arg */

	do {
		if (argc != 0)
			paddr = *argptr++;
		else {
			if (paddr == NULL) {
				cmn_err(CE_CONT, "^No as addr set yet\n");
				return;
			}
			argc = 1;
		}

		/*
			Validate the as address 
		*/
		if (paddr == (struct as *)NULL)
			cmn_err(CE_CONT, "^no as\n");
		else
			doas(paddr);

		if (argc > 1)
			cmn_err(CE_CONT, "^-------------------------------------------------------------------------------\n\n");

	} while (--argc);
}

doas(asp)
struct as *asp;
{
	register struct seg *seg, *sseg;

	pras(asp);
	prhat(&asp->a_hat);

	sseg = seg = asp->a_segs;
	if (seg != NULL) {
		sprintf(prtabs_buf, "s_base   s_size   s_next   s_prev   s_ops    s_data   \n");
		cmn_err(CE_CONT, "^%s", prtabs_buf);
		dodmddelay();
		do {
			prseg(seg, asp);
			seg = seg->s_next;
		} while (seg != sseg);
	}
}

pras(asp)
struct as *asp;
{

	/*
	sprintf(prtabs_buf, "|------- |------- |------- |------- | ------ \n");
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	*/
	sprintf(prtabs_buf, "as: %.8x\n", asp);
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	sprintf(prtabs_buf, "segs     seglast  rss      hat      \n");
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	sprintf(prtabs_buf, "%.8x %.8x %.8x %.8x\n",
		asp->a_segs, asp->a_seglast, asp->a_rss, asp->a_hat);
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();
}

prhat(hatp)
struct hat *hatp;
{

	sprintf(prtabs_buf, "srama[0] srama[1] sramb[0] sramb[1] \n");
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	sprintf(prtabs_buf, "%.8x %.8x %.8x %.8x\n",
		hatp->hat_srama[0], hatp->hat_srama[1],
		hatp->hat_sramb[0], hatp->hat_sramb[1]);
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();
}

prseg(segp, asp)
struct seg *segp;
struct as *asp;
{
	if (segp->s_as != asp)
		cmn_err(CE_CONT, "seg's s_as (%x) is incorrect\n", segp->s_as);
	sprintf(prtabs_buf, "%.8x %.8x %.8x %.8x %.8x %.8x\n",
		segp->s_base, segp->s_size, 
		segp->s_next, segp->s_prev, segp->s_ops, segp->s_data);
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();

}

extern struct swapinfo *swapinfo;

printswapinfo()
{
	register struct swapinfo *sip = swapinfo;

        /*
	 */
	if (sip != NULL)
	  do {
			prswap(sip);
	} while (sip = sip->si_next);
}

prswap(sip)
	register struct swapinfo *sip;
{
	
	cmn_err(CE_CONT, "^pathname:%s\n", sip->si_pname);
	dodmddelay();
	sprintf(prtabs_buf, "vp       soff     eoff     anon     eanon    free     allocs   next   \n");
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();
	sprintf(prtabs_buf, "%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		sip->si_vp, sip->si_soff, sip->si_eoff, sip->si_anon,
		sip->si_eanon, sip->si_free, sip->si_allocs, sip->si_next);
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();
	sprintf(prtabs_buf, "flags    npgs     nfpgs    svp\n");
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();
	sprintf(prtabs_buf, "%.8x %.8x %.8x %.8x\n\n",
		sip->si_flags, sip->si_npgs, sip->si_nfpgs, sip->si_svp);
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();
}

prsvd(svd)
	struct segvn_data *svd;
{
	struct anon_map *amp = svd->amp;

	if (amp != NULL) {
		pramp(amp);
	}
}

pramp(amp)
	struct anon_map *amp;
{
	sprintf(prtabs_buf, "         amp      refcnt   size     anon     swresv\n");
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();

	sprintf(prtabs_buf, "         %.8x %.8x %.8x %.8x %.8x\n",
		amp, amp->refcnt, amp->size, amp->anon, amp->swresv);
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();

	pranon(amp->anon, btoc(amp->size));

}

pranon(app, cnt)
	struct anon **app;
	int cnt;
{
	int i;
	struct anon *old;
	struct vnode *vp;
	u_int off;
	int nullcnt = 0;

	sprintf(prtabs_buf, "                  refcnt   pagehint vp       offset   pp\n");
	cmn_err(CE_CONT, "^%s", prtabs_buf);
	dodmddelay();

	for (i=0; i < cnt; i++, app++) {
		if ((old = *app) == NULL) {
			nullcnt++;
			continue;
		}
		if (nullcnt) {
			cmn_err(CE_CONT, "^                  %d null anon\n",
				nullcnt);
			dodmddelay();
			nullcnt = 0;
		}
		swap_xlate(old, &vp, &off);
		sprintf(prtabs_buf, "                  %.8x %.8x %.8x %.8x ",
			old->an_refcnt, old->un.an_page,
			vp, off);
		cmn_err(CE_CONT, "^%s", prtabs_buf);
		dodmddelay();
		findpage(vp, off);
	}
	if (nullcnt) {
		cmn_err(CE_CONT, "^                  %d null anon\n",
			nullcnt);
		nullcnt = 0;
	}
	dodmddelay();
}

prxas(arg)
int arg;
{
	static proc_t	*opaddr;
	proc_t	*paddr;
	extern proc_t *curproc;
	proc_t  **argptr;
	int 	argc;
	int 	j, flag;

	argc = argcount();	/* get num of args */
	argptr = (proc_t **) &arg;		/* get ptr to first arg */

	do {
		if (argc != 0)
			paddr = *argptr++;
		else {
			if (opaddr == NULL) {
				cmn_err(CE_CONT, "^No proc addr set yet\n");
				return;
			}
			paddr = opaddr;
			argc = 1;
		}

		/*
			Validate the proc table address 
		*/
		if ((proc_t **)paddr >= nproc && (proc_t **)paddr < v.ve_proc)
			paddr = *((proc_t **)paddr);
		else
			if ((paddr != curproc) && (paddr != opaddr)) {
				flag = 0;
				for (j = 0; j < v.v_proc ; j++)
					if (paddr == nproc[j]) {
						++flag;
						break;
					}
				if (flag == 0) {
					cmn_err(CE_CONT, "^%x is not a nproc table address\n", paddr);
					return;
				}
			}
		opaddr = paddr;
		if (paddr->p_as == (struct as *)NULL)
			cmn_err(CE_CONT, "^process has no as\n");
		else
			prxasp(paddr->p_as);

		if (argc > 1)
			cmn_err(CE_CONT, "^-------------------------------------------------------------------------------\n\n");

	} while (--argc);
}

prxasp(asp)
struct as *asp;
{
	register struct seg *seg, *sseg;

	if (asp == NULL)
		return;
	pras(asp);
	prhat(&asp->a_hat);

	sseg = seg = asp->a_segs;
	if (seg != NULL) {
		sprintf(prtabs_buf, "s_base   s_size   s_next   s_prev   s_ops    s_data   \n");
		cmn_err(CE_CONT, "^%s", prtabs_buf);
		dodmddelay();
		do {
			prseg(seg, asp);
			if ((seg->s_ops == &segvn_ops) && seg->s_data)
				prsvd(seg->s_data);
			seg = seg->s_next;
		} while (seg != sseg);
	}
}

ckall()
{
	register proc_t	**pp;
	char		buf[20];
	int		slot;

	for (slot=0, pp=&nproc[0] ; pp < v.ve_proc ; slot++, pp++) {
		if (*pp != NULL && (*pp)->p_as != NULL)
			ckp(*pp);
	}
}

ckp(pp)
	register proc_t *pp;
{
	struct as *asp = pp->p_as;
	register struct seg *seg, *sseg;
	int bad = 0;

	if (asp == NULL)
		return;
	sseg = seg = asp->a_segs;
	if (seg != NULL) {
		do {
			if ((seg->s_ops == &segvn_ops) && seg->s_data)
				bad += cksvd(seg->s_data);
			seg = seg->s_next;
		} while (seg != sseg);
	}
	if (bad) {
		cmn_err(CE_CONT, "^proc %x has mising anon\n", pp);
		dodmddelay();
	}
}

cksvd(svd)
	struct segvn_data *svd;
{
	struct anon_map *amp = svd->amp;

	if (amp != NULL) {
		return ckanon(amp->anon, btoc(amp->size));
	}
	return 0;

}

ckanon(app, cnt)
	struct anon **app;
	int cnt;
{
	int i;
	struct anon *old;
	struct vnode *vp;
	u_int off;

	for (i=0; i < cnt; i++, app++) {
		if ((old = *app) == NULL) {
			continue;
		}
		swap_xlate(old, &vp, &off);
		if (xpage_find(vp, off))
			return 1;
	}
	return 0;
}

