/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_USER_H
#define _SYS_USER_H

#ident	"@(#)head.sys:sys/user.h	11.40"

#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/siginfo.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/mau.h"
#include "sys/resource.h"

/*
 * The user structure; one allocated per process.  Contains all the
 * per-process data that doesn't need to be referenced while the
 * process is swapped.
 *
 * The user block is USIZE*click bytes long; resides at virtual kernel
 * address 0xC0000000 on the 3B2; contains the system stack per user;
 * is cross referenced with the proc structure for the same process.
 *
 *			NOTE  --  NOTE
 *
 * 	WHEN THIS HEADER CHANGES, BOTH ml/misc.s AND ml/ttrap.s MUST BE
 *	   EXAMINED FOR ANY DEPENDENCIES UPON OFFSETS WITHIN THE UBLOCK.
 *	   IN PARTICULAR - .SET's ARE DONE ON:
 *				u_iop,
 *				u_pcb,
 *				u_kpcb,
 *				u_pcbp,
 *				u_stack,
 *				u_flist,
 *				u_tracepc,
 *				u_caddrflt,
 *				u_sigevpend,
 *				pr_base,
 *				pr_size,
 *				pr_off, and
 *				pr_scale
 *
 **********************************************************************
 * 
 */

/*
 * User file descriptors are allocate dynamically, in multiples
 * of NFPCHUNK.  WARNING: ml/misc.s:vstart_s knows about the
 * size of struct ufchunk.  If this changes, or in NFPCHUNK is
 * changed, remember to update misc.s.
 */

#define NFPCHUNK 24

struct ufchunk {
	struct file *uf_ofile[NFPCHUNK];
	char uf_pofile[NFPCHUNK];
	struct ufchunk *uf_next;
};

#define UF_FDLOCK	0x2	/* file descriptor locked (SysV-style) */

#define MAXSYSARGS	8	/* Maximum # of arguments passed to a syscall */ 
#define PSARGSZ		80	/* Space for exec arguments (used by ps(1)) */ 

#define	PSCOMSIZ	14

typedef struct {		/* kernel syscall set type */
	long	word[16];
} k_sysset_t;

/* flags for u_sigflag field */
#define SOMASK 		0x001	/* use u_sigoldmask on return from signal */

typedef	struct	user {
	struct 	ipcb  u_ipcb;
	
	pcb_t	u_pcb;			/* pcb, save area when switching */
	
	struct pcb 	*u_pcbp;	/* %pcbp while not running */
	struct kpcb	u_kpcb;		/* kernel per-process pcb */
	int		u_r0tmp;	/* user %r0 during CALLPS */
#ifdef DEBUG
	long	u_aoutstamp;
	char	u_filler1[36];
#else
     	char	u_filler1[40];
#endif
	long	u_spop;			/* support processor being used */
	mau_t 	u_mau;

	/*
	 * Pointer to function to handle user-space external
	 * memory faults encountered in the kernel.
	 */
	int	u_caddrflt;

	/*
	 * Number of shared-memory segments currently attached.
	 */
	char	u_nshmseg;

	/*
	 * Flag to indicate there is a signal or event pending to
	 * the current process.  Used to make a quick check just
	 * prior to return from kernel to user mode.
	 */
	char	u_sigevpend;

	char	u_filler2[34];		/* Padding */

	/*
	 * The following for RFS.  u_srchan is needed because ioctls on
	 * the server can hit RF_SERVER() hooks in copyin and copyout,
	 * and rcopying/rcopyout need the implicit parameter.
	 */
	ushort		rfs_pad0;
	ushort 		rfs_pad1;
	int 		u_syscall;	/* system call number */
	struct sndd	*u_srchan;	/* server channel back to client */
	long		rfs_pad2;

	long	u_bsize;		/* block size of device */
	char 	u_psargs[PSARGSZ];	/* arguments from exec */	
	char	u_filler3[16];		/* Padding for swap compatibility */
	char	*u_tracepc;		/* Return PC if tracing enabled */
	int	u_sysabort;		/* Debugging: if set, abort syscall */
	int	u_systrap;		/* Are any syscall mask bits set? */ 
	k_sysset_t u_entrymask;		/* syscall stop-on-entry mask */
	k_sysset_t u_exitmask;		/* syscall stop-on-exit mask */

	/*
	 * This filler area is reserved for expanding the debugger interface
	 * definitions (above) and the signal interface definitions (below).
	 * No other use should be made of it.
	 */
	char	u_filler4[296];		/* Padding for swap compatibility */

	int	u_sigflag;		/* per-process signal flags */
	struct ucontext *u_oldcontext;	/* previous user context */
	stack_t u_sigaltstack;		/* sp & on-stack state variable */
	k_sigset_t u_signodefer;	/* signals defered when caught */
	k_sigset_t u_sigonstack;	/* signals taken on alternate stack */
	k_sigset_t u_sigresethand;	/* signals reset when caught */
	k_sigset_t u_sigrestart;	/* signals that restart system calls */
	k_sigset_t u_sigoldmask;	/* for sigsuspend */
	k_sigset_t u_sigmask[MAXSIG];	/* signals held while in catcher */
	void	(*u_signal[MAXSIG])();	/* Disposition of signals */

	int	u_pgproc;		/* Used by the MAU driver */
	int	u_fpovr;		/* flag for FP overflow workaround */
	label_t	u_qsav;			/* longjmp label for quits and intrs */

	char	u_dmm;			/* Double-mapped memory */

	/*
	 * The following (u_segflg and u_error) are obsolete.
	 */
	char	u_segflg;		/* 0=user D, 1=system, 2=user I */
	char	u_error;		/* return error code */

	/*
	 * The following four fields are for backward compatibility
	 * with old device drivers; the actual user credentials are
	 * found through p_cred in struct proc. These fields may
	 * not support valid uid's or gid's when the system is
	 * configured with large user id's.
	 */

	o_uid_t	u_uid;		/* effective user id */
	o_gid_t	u_gid;		/* effective group id */
	o_uid_t	u_ruid;	/* real user id */
	o_gid_t	u_rgid;	/* real group id */

	proc_t *u_procp;		/* pointer to proc structure */

	int	*u_ap;			/* pointer to arglist */

	/*
	 * The following (u_r through u_rablock) are all obsolete.
	 */
	union {
		struct	{
			int	r_v1;
			int	r_v2;
		} r_reg;
		off_t	r_off;
		time_t	r_time;
	} u_r;
	caddr_t	u_base;			/* base address for IO */
	unsigned u_count;		/* bytes remaining for IO */
	off_t	u_offset;		/* offset in file for IO */
	short	u_fmode;		/* file mode for IO */
	ushort	u_pbsize;		/* Bytes in block for IO */
	ushort	u_pboff;		/* offset in block for IO */
	char	u_filler4_1[2];		/* padding */
	daddr_t	u_rablock;		/* read ahead block address */

	short	u_errcnt;		/* syscall error count */

	struct vnode *u_cdir;		/* current directory */

	struct vnode *u_rdir;		/* root directory */

	char	u_filler5[24];		/* Padding */

	int	*u_stack;		/* Ptr to start of kernel stack. */
	char	u_filler6[4];		/* filler pointer */

	int	u_arg[MAXSYSARGS];	/* arguments to current system call */

	struct rlimit u_rlimit[RLIM_NLIMITS];     /* resource usage limits */

	char	u_filler7[28];		/* Padding for swap compatibility */

	uint	u_tsize;		/* text size (clicks) */
	uint	u_dsize;		/* data size (clicks) */
	uint	u_ssize;		/* stack size (clicks) */

	k_siginfo_t u_siginfo;		/* debugger interface: stop-on-fault */
	char	u_filler8[52];		/* Padding for swap compatibility */

	/*
	 * This offset must be reflected in misc.s and ttrap.s.
	 */
	/* Secret 3B2 illegal opcode for FP simulation */
	int	*u_iop;

	/*
	 * u_utime, u_stime, u_cutime, u_cstime have been moved to proc table
	 */

	clock_t	u_uservirt;		/* User virtual time */
	clock_t	u_procvirt;		/* Process virtual time */
	char	u_filler9[8];		/* Padding */

	int	*u_ar0;			/* address of user's saved R0 */

	/*
	 * These offsets must be reflected in ttrap.s and misc.s.
	 */

	struct prof {			/* profile arguments */
		short	*pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} u_prof;

		/* WARNING: the definitions for u_ttyp and
		** u_ttyd will be deleted at the next major
		** release following SVR4.
		*/

	o_pid_t *u_ottyp;		/* for binary compatibility only ! */
	o_dev_t  u_ttyd;		/* for binary compatibility only -
					** NODEV will be assigned for large
					** controlling terminal devices.
					*/
	

	char	u_filler10[2];

	int	u_execid;
	long	u_execsz;

	char	u_filler11[52];

	/*
	 * Executable file info.
	 */
	struct exdata {
		struct    vnode  *vp;
		size_t    ux_tsize;	/* text size */
		size_t    ux_dsize;	/* data size */
		size_t    ux_bsize;	/* bss size */
		size_t    ux_lsize;  	/* lib size */
		long      ux_nshlibs; 	/* number of shared libs needed */
		short     ux_mag;   	/* magic number MUST be here */
		off_t     ux_toffset;	/* file offset to raw text */
		off_t     ux_doffset;	/* file offset to raw data */
		off_t     ux_loffset;	/* file offset to lib sctn */
		caddr_t   ux_txtorg;	/* start addr of text in mem */
		caddr_t   ux_datorg;	/* start addr of data in mem */
		caddr_t	  ux_entloc;/* entry location */
	} u_exdata;

	char	u_comm[PSCOMSIZ];

	time_t	u_start;
	clock_t	u_ticks;
	long	u_mem;
	long	u_ior;
	long	u_iow;
	long	u_iosw;
	long	u_ioch;
	char	u_acflag;
	mode_t	u_cmask;		/* mask for file creation */
	short	u_lock;			/* process/text locking flags */
	int u_nofiles;			/* number of open file slots */
	struct ufchunk u_flist;		/* open file list */
	pid_t	*u_ttyp;		/* temporary pointer until we work
					** compatibility issues on ttyp
					*/
} user_t;

extern struct user u;
/* floating u area support */
#define KERNSTACK	0x1000		/* 2 pages */
struct seguser {
	struct user segu_u;
	char segu_stack[KERNSTACK];
};
#define KUSER(seg)	(&((seg)->segu_u))

#define	u_rval1	u_r.r_reg.r_v1
#define	u_rval2	u_r.r_reg.r_v2
#define	u_roff	u_r.r_off
#define	u_rtime	u_r.r_time

#define	u_cred	u_procp->p_cred

#define u_altflags	u_sigaltstack.ss_flags
#define u_altsp		u_sigaltstack.ss_sp
#define u_altsize	u_sigaltstack.ss_size

/* ioflag values: Read/Write, User/Kernel, Ins/Data */
#define	U_WUD	0
#define	U_RUD	1
#define	U_WKD	2
#define	U_RKD	3
#define	U_WUI	4
#define	U_RUI	5

/* u_spop values */
#define U_SPOP_MAU	0x1L

#ifdef _KERNEL

#if defined(__STDC__)
extern void addupc(void(*)(), struct prof *, int);
#else
extern void addupc();
#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _SYS_USER_H */
