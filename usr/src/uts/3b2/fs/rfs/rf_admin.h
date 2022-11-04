/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_RFS_RF_ADMIN_H
#define _FS_RFS_RF_ADMIN_H

#ident	"@(#)fs:fs/rfs/rf_admin.h	1.2"

/*
 * declarations of RFS internal administrative interfaces and those related
 * to the rf_async daemons.
 */

/* rf_recovery opcodes */
#define REC_DISCONN	1	/* circuit gone */
#define REC_KILL	2	/* exit */
#define REC_FUMOUNT	3	/* forced unmount */
#define REC_MSG		4

/* rf_recovery_flag values */
#define RFRECDISCON	0x01
#define RFRECFUMOUNT	0x02
#define RFRECKILL	0x04
/*
 * rf_daemon_flag flag */
#define  RFDSERVE	0x01
#define  RFDDISCON	0x02
#define  RFDKILL	0x04

/* This is the structure passed to the user daemon */
#define ULINESIZ 120	/* want a 128-char buffer */
struct u_d_msg {
	int opcode;
	int count;
	char res_name[ULINESIZ];
};

/*
 * Structure that may be registered for rf_daemon processing via the
 * rfa_workenq operation.  rf_async always processes its work queue
 * before sleeping.  It dequeues a structure, calls rfaw_func with
 * the structure address, then does not reference the structure further.
 * The user is reponsible for ALL cleanup, including structure disposal.
 *
 * Passed by reference to rfa_workenq.  If canfail, caller must be prepared
 * to recover.
 *
 * NOTE: rfaw_func may be called with RFS in intermediate state,
 * because the work queue is cleaned up when RFS is started and stopped.
 */
typedef struct rfa_work {
	ls_elt_t	rfaw_elt;
	void		(*rfaw_func)();
	caddr_t		rfaw_farg;
	int		rfaw_canfail;
} rfa_work_t;

extern int	rfa_workenq();

extern void	rf_recovery();
extern void	rf_signal_serve();
extern void	rf_checkq();
extern void	rf_daemon();
extern void	rf_recovery();

extern int		rf_recovery_flag;
extern struct proc	*rf_recovery_procp;
extern struct rcvd	*rf_daemon_rd;
extern struct proc	*rf_daemon_procp;
extern int		rf_daemon_flag;
extern int		rf_daemon_lock;

/*
 * Structure passed into esballoc to have freeb queue up work for rf_async.
 *
 * These are allocated and freed by rffr_alloc() and RFFR_FREE(), respectively.
 *
 *	frtn.free_func == rfa_workenq
 *	frtn.free_arg == &work.rfaw_elt
 *	work.rfaw_func == func passed into rffr_alloc
 *	work.rfaw_farg == pointer passed into rffr_alloc
 *	work.rfaw_canfail == as passed into rffr_alloc
 *
 *	freeb will call rfa_workenq(&work.rfaw_elt)
 *	rf_async will call rfaw_func(&work)
 */
typedef struct rf_frtn {
	frtn_t		frtn;
	rfa_work_t	work;
} rf_frtn_t;

/* Convert address of rffr_work to address of enclosing structure */
#define WTOFR(wp)	\
	((rf_frtn_t *)((caddr_t)(wp) - (caddr_t)&((rf_frtn_t *)NULL)->work))

#define RFFR_FREE(rffrp)	kmem_free((rffrp), sizeof(rf_frtn_t))

extern rf_frtn_t	*rffr_alloc();

/*
 * Arg structure passed by rfesb_fbread into rffr_alloc for rfesb_fbrelse, 
 * which we use with esballoc().  We go to this trouble because we don't
 * want the provider to sleep in fbrelse.
 *
 *	fbp == &fbuf returned by fbread, to be passed to fbrelse
 *	rw == seg arg for fbrelse
 */
typedef struct rf_fba {
	struct fbuf	*fbp;
	enum seg_rw	rw;
} rf_fba_t;

extern int	rfesb_fbread();

#define NULLCADDR	((caddr_t)NULL)
#define NULLFRP		((frtn_t *)NULL)
#define NULLUIO		((uio_t *)NULL)
#define NULLCRED	((cred_t *)NULL)
#endif /* _FS_RFS_RF_ADMIN_H */
