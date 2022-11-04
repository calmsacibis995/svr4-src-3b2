/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debug:debug/trace.c	1.10"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/fs/s5dir.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/cmn_err.h"
#include "sys/sys3b.h"
#include "sys/inline.h"
#include "sys/var.h"

#include "sys/immu.h"
#include "vm/as.h"
#include "vm/vm_hat.h"

#define MAXARGS		20

int	debugtrace;
char	tracebuf[200];
char	*s3blookup();
int	strcmp();

trace(arg)
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
	
		if ((proc_t **)paddr >= nproc && (proc_t **)paddr < v.ve_proc) {
			paddr = *((proc_t **)paddr);
		} else {
			if ((paddr != curproc) && (paddr != opaddr)) {
				flag = 0;
				for (j = 0; j < v.v_proc ; j++)
					if (paddr == nproc[j]) {
						++flag;
						break;
					}
				if (flag == 0) {
					cmn_err(CE_CONT, "^%x is not a proc table address\n", paddr);
					return;
				}
			}
		}
		opaddr = paddr;
		if (paddr->p_stat == 0 || paddr->p_stat == SIDL
		  || paddr->p_stat == SZOMB)
			cmn_err(CE_CONT, "^Inactive Process\n");
		else if ((paddr->p_flag & SULOAD) == 0)
			cmn_err(CE_CONT, "^Process was swapped out\n");
			else {
			dotrace(paddr);
			}

		if (argc > 1)
			cmn_err(CE_CONT, "^-------------------------------------------------------------------------------\n\n");

	} while (--argc);
}

dotrace(pp)
register proc_t		*pp;
{
	register uint	oldubptbl;
	register sde_t	*sdep;
	extern proc_t	*curproc;

	if(pp == curproc){
		oldubptbl = NULL;
	} else {
		sdep = (sde_t *)*(srama + 3);
		oldubptbl = sdep->wd2.address;
		sdep->wd2.address = phys_ubptbl(pp->p_ubptbl);
		flushmmu((caddr_t)&u, USIZE);
	}


	tracepcb(pp, u.u_pcbp);

	if(oldubptbl != NULL){
		sdep->wd2.address = oldubptbl;
		flushmmu((caddr_t)&u, USIZE);
	}
}

tracepcb(paddr, pcbaddr)
proc_t	*paddr;
pcb_t	*pcbaddr;
{
	register int ap, fp, sp, pc;
	int *oldpcptr;
	int *argp;
	int first;
	char *s3bsp;
	struct hat *hatp;
	uint srama_save;
	SRAMB sramb_save;
	sde_t sde_save;
	int	oldpri;
	int	arg_count;

	if (pcbaddr == &u.u_pcb) {
		cmn_err(CE_CONT, "^Can't trace processes in user mode\n");
		return;
	}

	oldpri = splhi();

	if (paddr != NULL) {
		srama_save = srama[SCN3];
		sramb_save = sramb[SCN3];
		sde_save = *((sde_t *)srama_save);

		if (paddr->p_as == (struct as *)NULL) {
			register int s;

			s = spl7();
			((sde_t *)dflt_sdt_p)->wd2.address =
				phys_ubptbl(paddr->p_ubptbl);
			srama[SCN3] = dflt_sdt_p;
			((int *)sramb)[SCN3] = 0;
			splx(s);
		} else {
			register struct hat *hatp = &paddr->p_as->a_hat;
			register sde_t *sde3;

			sde3 = (sde_t *)hatp->hat_srama[1];
			sde3->wd2.address = phys_ubptbl(paddr->p_ubptbl);
			loadmmu(hatp, SCN3);
		}
	}

	ap = pcbaddr->regsave[K_AP];
	fp = pcbaddr->regsave[K_FP];
	pc = (int)pcbaddr->pc;
	sp = (int)pcbaddr->sp;
	cmn_err(CE_CONT, "^    AP		    FP		    PC		Function	Procp: %x\n\n", paddr);

	if (debugtrace) {
		sprintf(tracebuf,"%#x	%#x	%#x	%s(", ap, fp, pc, "start");
		cmn_err(CE_CONT, "^%s)\n", tracebuf);
		sprintf(tracebuf,"					%#x	%#x", sp, oldpcptr);
		cmn_err(CE_CONT, "^%s\n", tracebuf);
		dodmddelay();
	}

	while (sp > (int)pcbaddr->slb && sp <= (int)pcbaddr->sub) {
		s3bsp = s3blookup(pc);

		/*
			Special case:  If the symbol is "nrmx_KK", then
			we took a fault in kernel mode.  So we want to
			step over the stuff on the stack left by the
			exception handler and pick up the PC on the
			stack that tells us where the fault occurred
		*/

		if (strcmp(s3bsp, "nrmx_KK") == 0) {
			if (debugtrace) {
				sprintf(tracebuf,"%#x	%#x	%#x	%s(", 
					ap, fp, pc, s3bsp);
				cmn_err(CE_CONT, "^%s)\n", tracebuf);
				sprintf(tracebuf,"					%#x	%#x", sp, oldpcptr);
				cmn_err(CE_CONT, "^%s\n", tracebuf);
				dodmddelay();
			}

			pc = *((int *)(sp) - 4);
			sp = (int)((int *)sp - 6);
			s3bsp = s3blookup(pc);
			if (debugtrace) {
				sprintf(tracebuf,"				npc	%#x	nsp	%#x", pc, sp);
				cmn_err(CE_CONT, "^%s\n", tracebuf);
				dodmddelay();
			}

		}

		sprintf(tracebuf,"%#x	%#x	%#x	%s(", ap, fp, pc, s3bsp);
		
		if (fp > ap) 
			oldpcptr = (int *)(fp - 9 * sizeof(char *));
		else {
			oldpcptr = (int *)(sp - 2 * sizeof(char *));
			if (debugtrace)
				sprintf(tracebuf, "%s!", tracebuf);
		}

		for (argp = (int * )ap, arg_count = 1; argp < oldpcptr; argp++) {
			if (arg_count != 1)
				sprintf(tracebuf, "%s, ", tracebuf);

			if ( (int)argp & 0x3 ) {
				sprintf(tracebuf, "%s bad arg %#x", tracebuf, argp);
				break;
			}

			sprintf(tracebuf, "%s%#x", tracebuf, *argp);

			if (++arg_count > MAXARGS) {
				cmn_err(CE_CONT, "^Trace Error, Too Many Args\n");
				goto out;
			}
		}
	
		cmn_err(CE_CONT, "^%s)\n", tracebuf);

		dodmddelay();

		if (debugtrace) {
			sprintf(tracebuf,"					%#x	%#x", sp, oldpcptr);
			cmn_err(CE_CONT, "^%s\n", tracebuf);
			dodmddelay();
		}

		if (ap > sp)	/* then we ran into residue of callps */
			break;

		pc = *oldpcptr++;
		sp = ap;
		ap = *oldpcptr++;

		if (fp > sp)
			fp = *oldpcptr;
	}

out:
	if (paddr != NULL) {
		*((sde_t *)srama_save) = sde_save;
		srama[SCN3] = srama_save;
		sramb[SCN3] = sramb_save;
	}

	splx(oldpri);
}

ptrc()
{
	register pcb_t *pcbaddr;

	asm("	MOVW	%pcbp,%r8");
	tracepcb(NULL, pcbaddr);
}

uptrc()
{
	tracepcb(NULL, u.u_pcbp);
}

prtpcbp()
{
	register pcb_t *pcbaddr;

	asm("	MOVW	%pcbp,%r8");
	cmn_err(CE_CONT, "^%x\n", pcbaddr);
}

printsp()
{
	register int sp;

	asm("	MOVW	%sp,%r8");
	cmn_err(CE_CONT, "^sp = %x\n", sp);
}

printpc()
{
	cmn_err(CE_CONT, "^%x\n", u.u_pcbp->pc);
}

testarg()
{
	int count;

	count = argcount();

	cmn_err(CE_CONT, "^arg count = %d\n", count);
}

