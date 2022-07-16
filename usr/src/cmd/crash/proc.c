/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:proc.c	1.15.16.1"
/*
 * This file contains code for the crash functions:  proc, defproc, hrt.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/sbd.h>
#include <sys/psw.h>
#include <sys/pcb.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/var.h>
#include <vm/as.h>
#include <sys/proc.h>
#include <sys/sys3b.h>
#include <sys/evecb.h>
#include <sys/hrtcntl.h>
#include <sys/hrtsys.h>
#include <sys/priocntl.h>
#include <sys/procset.h>
#include <sys/events.h>
#include <sys/vnode.h>
#include <sys/session.h>
#include <sys/evsys.h>
#include "crash.h"

extern int active;
extern struct user *ubp;		/* pointer to the ublock */
extern struct syment *Curproc;	/* namelist symbol pointers */
char *proc_clk[] = {
		"CLK_STD",
		"CLK_USERVIRT",
		"CLK_PROCVIRT"
};


/* get arguments for proc function */
int
getproc()
{
	int slot = -1;
	int all = 0;
	int full = 0;
	int phys = 0;
	int run = 0;
	int alarm = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	pid_t id = -1;
	int c;
	char *heading = "SLOT ST  PID  PPID  PGID   SID   UID PRI CPU   EVENT     NAME        FLAGS\n";
	char *prprocalarm_hdg = "    CLOCK       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";

	optind = 1;
	while((c = getopt(argcnt,args,"efparw:")) !=EOF) {
		switch(c) {
			case 'a' :	alarm = 1;
					break;
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'r' :	run = 1;
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	fprintf(fp,"PROC TABLE SIZE = %d\n",vbuf.v_proc);
	if(!full && !alarm)
		fprintf(fp,"%s",heading);
	if(alarm)
		fprintf(fp,"%s", prprocalarm_hdg);
	if(args[optind]) {
		all = 1;
		do {
			if(*args[optind] == '#') {
				if((id = (pid_t)strcon(++args[optind],'d')) == -1)
					error("\n");
				prproc(all,full,slot,id,phys,run,alarm,addr,heading);
			}
			else {
				getargs(vbuf.v_proc,&arg1,&arg2);
				if(arg1 == -1) 
					continue;
				if(arg2 != -1)
					for(slot = arg1; slot <= arg2; slot++)
						prproc(all,full,slot,id,phys,
							run,alarm,addr,heading);
				else {
					if((arg1 < vbuf.v_proc) && (arg1 >= 0))
						slot = arg1;
					else
						addr = arg1;
					prproc(all,full,slot,id,phys,run,alarm,
					    addr,heading);
				}
			}
			id = slot = addr = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else for(slot = 0; slot < vbuf.v_proc; slot++)
		prproc(all,full,slot,id,phys,run,alarm,addr,heading);
}


/* print proc table */
int
prproc(all,full,slot,id,phys,run,alarm,addr,heading)
int all,full,slot,phys,run,alarm;
pid_t id;
long addr;
char *heading;
{
	char ch,*typ;
	char cp[PSCOMSIZ+1];
	struct proc procbuf, *procaddr;
	struct cred uc;
	struct sess sess;
	struct evpd evpbuf;
	int i,j,cnt;
	extern long lseek();
	timer_t *hrp;
	timer_t hrtbuf;
	char buf[40];
	int type = 0;
	proc_t *slot_to_proc();

	if(id != -1) {
		for(slot = 0; ; slot++) {
			if(slot == vbuf.v_proc) {
				fprintf(fp,"%d not valid process id\n",id);
				return;
			}
			if (slot_to_pid(slot) == id) {
				procaddr = slot_to_proc(slot);
				break;
			}
		}
	} else if (slot != -1) 
		procaddr = slot_to_proc(slot);
	else for(slot = 0; ; slot++) {
		if(slot == vbuf.v_proc) {
			fprintf(fp,"%x not valid process address\n", addr);
			return;
		}
		procaddr = slot_to_proc(slot);
		if (phys) {
			if (addr==(vtop(procaddr,0)+MAINSTORE))
				break;
		} else {
			if(addr==(long)procaddr)
				break;
		}
	}
	if (!procaddr)
		return;

	readbuf(addr,(long)procaddr,phys,-1,
		    (char *)&procbuf,sizeof procbuf,"proc table");

	if(run)
		if(!(procbuf.p_stat == SRUN || procbuf.p_stat == SONPROC))
			return;

	if (alarm) {
		while (type < 2) {
			hrp=procbuf.p_italarm[type];
			for(; hrp!=NULL; hrp=hrtbuf.hrt_next) {
				readmem((long)hrp, 1, -1, (char *)&hrtbuf,
					sizeof hrtbuf, "process alarm");
				fprintf(fp, "%s %7d %11d %7d %6d %13x %10x\n",
					proc_clk[type + 1],
					hrtbuf.hrt_time,
					hrtbuf.hrt_int,
					hrtbuf.hrt_cmd,
					hrtbuf.hrt_ecb.ecb_eid,
					hrtbuf.hrt_prev,
					hrtbuf.hrt_next);
			}
			type++;
		}
		return;
	}

	if(full)
		fprintf(fp,"%s",heading);
	switch(procbuf.p_stat) {
	case NULL:   ch = ' '; break;
	case SSLEEP: ch = 's'; break;
	case SRUN:   ch = 'r'; break;
	case SIDL:   ch = 'i'; break;
	case SZOMB:  ch = 'z'; break;
	case SSTOP:  ch = 't'; break;
	case SONPROC:  ch = 'p'; break;
	case SXBRK:  ch = 'x'; break;
	default:     ch = '?'; break;
	}
	if(slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%4d",slot);
	fprintf(fp," %c %5u %5u %5u %5u %5u  %2u %3u",
		ch,
		slot_to_pid(slot),
		procbuf.p_ppid,
		readpid(procbuf.p_pgidp),
		readsid(procbuf.p_sessp),
		procbuf.p_uid,
		procbuf.p_pri,
		procbuf.p_cpu);
	if(procbuf.p_stat == SONPROC)
		fprintf(fp,"          ");
	else fprintf(fp," %08lx ",procbuf.p_wchan);
	for(i = 0; i < PSCOMSIZ+1; i++)
		cp[i] = '\0';
	if(procbuf.p_stat == SZOMB)
		strcpy(cp,"zombie");
	else
	if(active){
		if(sys3b(RDUBLK, slot_to_pid(slot), (char *)ubp,sizeof(struct user)))
			strncpy(cp, ubp->u_comm, PSCOMSIZ);
	}	
	else if(!(procbuf.p_flag & SULOAD))
		strcpy(cp, "swapped");
	else
	{
		i=((char*)ubptbl(procaddr) - (char*)procaddr -
			((char*)procbuf.p_ubptbl - (char*)&procbuf)) >> 2;
		for(cnt=0; cnt < (USIZE*NBPC); cnt += NBPC, i++) {
			/* seek from begining of memory to ith page of uarea */
			if(lseek(mem,(long)(procbuf.p_ubptbl[i].pgm.pg_pfn<<11)-
				MAINSTORE,0) == -1) {
				prerrmes("seek error on ublock address\n");
				return;
			}
			if(read(mem,(char *)ubp+cnt,NBPC) != NBPC) {
				prerrmes("read error on ublock\n");
				return;
			}
		}
		strncpy(cp,ubp->u_comm, PSCOMSIZ);
	}
	for(i = 0; i < 8 && cp[i]; i++) {
		if(cp[i] < 040 || cp[i] > 0176) {
			strcpy(cp,"unprint");
			break;
		}
	}
	fprintf(fp,"%-14s", cp);
	fprintf(fp,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		procbuf.p_flag & SLOAD ? " load" : "",
		(procbuf.p_flag & (SLOAD|SULOAD)) == SULOAD ? " uload" : "",
		procbuf.p_flag & SSYS ? " sys" : "",
		procbuf.p_flag & SLOCK ? " lock" : "",
		procbuf.p_flag & STRC ? " trc" : "",
 		procbuf.p_flag & SNWAKE ? " nwak" : "",
 		procbuf.p_flag & SPOLL ? " poll" : "",
 		procbuf.p_flag & SPRSTOP ? " prst" : "",
 		procbuf.p_flag & SPROCTR ? " prtr" : "",
 		procbuf.p_flag & SPROCIO ? " prio" : "",
 		procbuf.p_flag & SPRFORK ? " prfo" : "",
 		procbuf.p_flag & SPROPEN ? " prop" : "",
		procbuf.p_flag & SRUNLCL ? " runl" : "",
		procbuf.p_flag & SNOSTOP ? " nstp" : "",
		procbuf.p_flag & SPTRX ? " ptrx" : "",
		procbuf.p_flag & SASLEEP ? " aslp" : "",
		procbuf.p_flag & SUSWAP ? " uswp" : "",
		procbuf.p_flag & SNOWAIT ? " nowait" : "",
		procbuf.p_flag & SJCTL ? " jctl" : "",
		procbuf.p_flag & SVFORK ? " vfrk" : "",
		procbuf.p_flag & SSWLOCKS ? " swlk" : "",
		procbuf.p_flag & SXSTART ? " xstr" : "",
		procbuf.p_flag & SPSTART ? " pstr" : "");
	if(!full)
		return;

	readmem((long)procbuf.p_sessp,1,-1,(char *)&sess,sizeof sess,
		"session");
	fprintf(fp,"\tSession: ");
	fprintf(fp,"sid: %u, ctty: ", readsid(procbuf.p_sessp));
	if (sess.s_vp)
		fprintf(fp,"vnode(%x) maj(%4u) min(%5u)\n",
			sess.s_vp, getemajor(sess.s_dev), geteminor(sess.s_dev));
	else 
		fprintf(fp,"-\n");

	readmem((long)procbuf.p_cred,1,-1,(char *)&uc,sizeof uc,
		"process credentials");
	fprintf(fp,"\tProcess Credentials: ");
	fprintf(fp,"uid: %u, gid: %u, real uid: %u, real gid: %u\n",
		uc.cr_uid,
		uc.cr_gid,
		uc.cr_ruid,
		uc.cr_rgid);
 	fprintf(fp,"\tas: %x\n",
		procbuf.p_as);
 	fprintf(fp,"\twait code: %x, wait data: %x\n",
		procbuf.p_wcode,
		procbuf.p_wdata);
	fprintf(fp,"\tsig: %x, cursig: %d, clktim: %d\n",
		procbuf.p_sig,
		procbuf.p_cursig,
		procbuf.p_clktim);
	fprintf(fp,"\tlink: %x\tparent: %x\tchild: %x\tsibling: %x\n",
		procbuf.p_link,
		procbuf.p_parent,
		procbuf.p_child,
		procbuf.p_sibling);
	if(procbuf.p_link)
		fprintf(fp,"\tlink: %d\n", proc_to_slot(procbuf.p_link));
	fprintf(fp,"\tutime: %ld\tstime: %ld\tcutime: %ld\tcstime: %ld\n",
		procbuf.p_utime,procbuf.p_stime,procbuf.p_cutime,procbuf.p_cstime);
	fprintf(fp, "\tubptbl:  ");
	for(i = 0,j = 1;  i < USIZE + 7  ;  i++) 
		if(*(int *)&procbuf.p_ubptbl[i] != 0) {
			if(!(j++ & 3))
				fprintf(fp,"\n\t");
			fprintf(fp,"%d: %8x   ",i,procbuf.p_ubptbl[i]);
		}	
	fprintf(fp,"\n\tepid: %d, sysid: %x, rlink: %x\n",
		procbuf.p_epid,
		procbuf.p_sysid,
		procbuf.p_rlink);
	fprintf(fp,"\tsrwchan: %d, trace: %x, sigmask: %x,",
		procbuf.p_srwchan,
		procbuf.p_trace,
		procbuf.p_sigmask);
	fprintf(fp,"\thold: %x\n",
		procbuf.p_hold);
	fprintf(fp, "\twhystop: %d, whatstop: %d\n",
		procbuf.p_whystop,
		procbuf.p_whatstop);

	/* print class information */

	fprintf(fp, "\tclass: %d, clfuncs: %x\n", 
 		procbuf.p_cid, procbuf.p_clfuncs);
	fprintf(fp, "\tclproc: %x\n", procbuf.p_clproc);

	/* print asyncio information */

	fprintf(fp, "\taiocount: %d, aiowcnt: %d\n",
		procbuf.p_aiocount, procbuf.p_aiowcnt);

	/* print events information */

	readmem((long)procbuf.p_evpdp, 1, -1, (char *)&evpbuf,
		sizeof evpbuf, "per process events info");

	fprintf(fp, "\tEvents: ");
	fprintf(fp, "exprs: %x,  ppsexprs: %x\n", evpbuf.epd_exprs.lh_first,
			evpbuf.epd_ppsexprs.lh_first);
	fprintf(fp, "\tptsexprs: %x,  tids: %x\n", evpbuf.epd_ptsexprs.lh_first,
			evpbuf.epd_tids.lh_first);
	fprintf(fp, "\tsigset: %d,  sigignset: %d,  asexp: %x\n",
		evpbuf.epd_sigset, evpbuf.epd_sigignset, evpbuf.epd_asexp);
	fprintf(fp, "\tsigrp: %x,  exits: %x,  ntraps: %d\n",
		evpbuf.epd_sigrp, evpbuf.epd_exits, evpbuf.epd_ntraps);
	fprintf(fp, "\tmaxtraps: %d,  maxeterms: %d,  holdall: %d\n",
		evpbuf.epd_maxtraps, evpbuf.epd_maxeterms, evpbuf.epd_holdall);
	fprintf(fp, "\trestart: %d,  astk: %d,  onastk: %d\n",
		evpbuf.epd_restart, evpbuf.epd_astk, evpbuf.epd_onastk);
	fprintf(fp, "\tlvl: %d,  astkp: %x,  astks: %d\n",
		evpbuf.epd_lvl, evpbuf.epd_astkp, evpbuf.epd_astks);

	fprintf(fp,"\n");
}


/* get arguments for defproc function */
int
getdefproc()
{
	int c;
	int proc = -1;
	int reset = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"cw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) 
		if((proc = strcon(args[optind],'d')) == -1)
			error("\n");
	prdefproc(proc,reset);
}

/* print results of defproc function */
int
prdefproc(proc,reset)
int proc,reset;
{

	if(reset)
		Procslot = getcurproc();
	else if(proc > -1) {
		if((proc > vbuf.v_proc) || (proc < 0))
			error("%d out of range\n",proc);
		Procslot = proc;
	}
	fprintf(fp,"Procslot = %d\n",Procslot);
}

/* print the high resolution timers */
int
gethrt()
{
	int c;
	static struct syment *Hrt;
	timer_t hrtbuf;
	timer_t *hrp;
	extern timer_t hrt_active;
	char *prhralarm_hdg = "    PROCP       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";


	optind = 1;
	while((c=getopt(argcnt, args,"w:")) != EOF) {
		switch(c) {
			case 'w'  :	redirect();
					break;
			default   :	longjmp(syn,0);
		}
	}

	if (!(Hrt = symsrch("hrt_active")))
		fatal("hrt_active not found in symbol table\n");


	readmem ((long)Hrt->n_value, 1, -1, (char *)&hrtbuf,
		sizeof hrtbuf, "high resolution alarms");

	fprintf(fp, "%s", prhralarm_hdg);
	hrp=hrtbuf.hrt_next;
	for (; hrp != (timer_t *)Hrt->n_value; hrp=hrtbuf.hrt_next) {
		readmem((long)hrp, 1, -1, (char *)&hrtbuf,
			sizeof hrtbuf, "high resolution alarms");
		fprintf(fp, "%12x %7d%11d %7d %6d %13x %10x\n",
			hrtbuf.hrt_proc,
			hrtbuf.hrt_time,
			hrtbuf.hrt_int,
			hrtbuf.hrt_cmd,
			hrtbuf.hrt_ecb.ecb_eid,
			hrtbuf.hrt_prev,
			hrtbuf.hrt_next);
	}
}

int
readsid(sessp)
	struct sess *sessp;
{
	struct sess s;

	readmem((char *)sessp,1,getcurproc(),(char *)&s,sizeof(struct sess),
		"session structure");

	return readpid(s.s_sidp);
}

int
readpid(pidp)
	struct pid *pidp;
{
	struct pid p;

	readmem((char *)pidp,1,getcurproc(),(char *)&p,sizeof(struct pid),
		"pid structure");

	return p.pid_id;
}
