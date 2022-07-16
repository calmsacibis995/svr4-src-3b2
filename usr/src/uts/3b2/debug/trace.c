/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debug:debug/trace.c	1.14"
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

int	debugtrace = 0;
char	tracebuf[320];
char	debugbuf[320];
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

	argc = argcount();		/* get num of args */
	argptr = (proc_t **) &arg;	/* get ptr to first arg */

	do {
		if (argc != 0) {
			paddr = *argptr++;
		} else {
			if (opaddr == NULL) {
				cmn_err(CE_CONT, "^No proc addr set yet\n");
				return;
			}

			paddr = opaddr;
			argc = 1;
		}

		if (procinval(paddr))
			return;

		opaddr = paddr;
		if (paddr->p_stat == 0 || paddr->p_stat == SIDL ||
		    paddr->p_stat == SZOMB)
			cmn_err(CE_CONT, "^Inactive Process\n");
		else if ((paddr->p_flag & SULOAD) == 0)
			cmn_err(CE_CONT, "^Process was swapped out\n");
		else
			dotrace(paddr);

		if (argc > 1)
			cmn_err(CE_CONT, "^-------------------------------------------------------------------------------\n\n");

	} while (--argc);
}

dotrace(pp)
	register proc_t		*pp;
{
	extern proc_t	*curproc;

	register struct user *uptr;
	register u_int stack_base;
	register u_int stack_copy;
	register u_int pcbaddr;

	cmn_err(CE_CONT, "^Procp: %x\n\n", pp);

	if (pp != curproc) {
		uptr = (struct user *)(pp->p_segu);
		stack_base = (u_int)0xc0000000;
		stack_copy = (u_int)uptr;
		pcbaddr = (u_int)uptr +
			((u_int)uptr->u_pcbp - (u_int)0xc0000000);
	} else {
		uptr = (struct user *)&u;
		stack_base = 0;
		stack_copy = 0;
		pcbaddr = (u_int)uptr->u_pcbp;
	}

	if (uptr->u_pcbp == &u.u_pcb) {
		cmn_err(CE_CONT, "^Can't trace processes in user mode\n");
		return;
	}

	tracepcb(pcbaddr, stack_base, stack_copy);
}

tracepcb(pcbaddr, stack_base, stack_copy)
	pcb_t	*pcbaddr;
	u_int	stack_base;
	u_int	stack_copy;
{
	register u_int ap, fp, sp, pc;
	int *oldpcptr;
	int *argp;
	int first;
	char *s3bsp;
	int	oldpri;
	int	arg_count;

	oldpri = splhi();

	if (debugtrace) {
		sprintf(debugbuf,"pcbaddr %#x stack_base %#x stack_copy %#x",
			pcbaddr, stack_base, stack_copy);
		cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
	}

	cmn_err(CE_CONT, "^    AP		    FP		    PC		Function\n\n");

	ap = pcbaddr->regsave[K_AP];
	fp = pcbaddr->regsave[K_FP];
	pc = (int)pcbaddr->pc;
	sp = (int)pcbaddr->sp;

	if (debugtrace) {
		sprintf(debugbuf,"ap %#x fp %#x pc %#x", ap, fp, pc);
		cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
		sprintf(debugbuf,"sp %#x slb %#x sup %#x",
			sp, pcbaddr->slb, pcbaddr->sub);
		cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
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
				sprintf(debugbuf,"ap %#x fp %#x pc %#x %s()", 
					ap, fp, pc, s3bsp);
				cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
				sprintf(debugbuf,"sp %#x oldpcptr %#x",
					sp, oldpcptr);
				cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
				dodmddelay();
			}

			sp = (int)((int *)sp - 6);
			pc = *((int *)(stack_copy + sp - stack_base));
			s3bsp = s3blookup(pc);
			if (debugtrace) {
				sprintf(debugbuf,"nsp %#x npc %#x", sp, pc);
				cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
				dodmddelay();
			}

		}

		sprintf(tracebuf,"%#x	%#x	%#x	%s(",
			ap, fp, pc, s3bsp);
		
		if (fp > ap) {
			oldpcptr = (int *)(fp - 9 * sizeof(char *));
			if (debugtrace) {
				sprintf(debugbuf,"fp > ap : oldpcptr %#x",
					oldpcptr);
				cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
			}
		} else {
			oldpcptr = (int *)(sp - 2 * sizeof(char *));
			if (debugtrace) {
				sprintf(debugbuf,"fp <= ap : oldpcptr %#x",
					oldpcptr);
				cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
			}
		}

		/*
		 * Adjust oldpcptr to the actual stack we are traversing.
		 */
		oldpcptr = (int *)(stack_copy + ((u_int)oldpcptr - stack_base));
		argp = (int * )(stack_copy + (ap - stack_base));

		if (debugtrace) {
			sprintf(debugbuf,"argp %#x oldpcptr %#x",
				argp, oldpcptr);
			cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
		}

		for (arg_count = 1; argp < oldpcptr; argp++) {
			if (arg_count != 1)
				sprintf(tracebuf, "%s, ", tracebuf);

			if ( (int)argp & 0x3 ) {
				sprintf(tracebuf, "%s bad arg %#x", tracebuf, argp);
				break;
			}

			sprintf(tracebuf, "%s%#x", tracebuf, *argp);

			if (debugtrace) {
				sprintf(debugbuf,"argp %#x *argp %#x",
					argp, *argp);
				cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
			}

			if (++arg_count > MAXARGS) {
				cmn_err(CE_CONT, "^Trace Error, Too Many Args\n");
				goto out;
			}
		}
	
		cmn_err(CE_CONT, "^%s)\n", tracebuf);

		dodmddelay();

		if (debugtrace) {
			sprintf(debugbuf,"%#x	%#x", sp, oldpcptr);
			cmn_err(CE_CONT, "DEBUG: ^%s\n", tracebuf);
			dodmddelay();
		}

		if (ap > sp)	/* then we ran into residue of callps */
			break;

		pc = *oldpcptr++;
		sp = ap;
		ap = *oldpcptr++;

		if (fp > sp)
			fp = *oldpcptr;

		if (debugtrace) {
			sprintf(debugbuf,"ap %#x fp %#x pc %#x sp %#x oldpcptr %#x",
				ap, fp, pc, sp, oldpcptr);
			cmn_err(CE_CONT, "DEBUG: ^%s\n", debugbuf);
			dodmddelay();
		}
	}

out:
	splx(oldpri);
}

ptrc()
{
	register pcb_t *pcbaddr;

	asm("	MOVW	%pcbp,%r8");
	tracepcb(pcbaddr, 0, 0);
}

uptrc()
{
	tracepcb(u.u_pcbp, 0, 0);
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

