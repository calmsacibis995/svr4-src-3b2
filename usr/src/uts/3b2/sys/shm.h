/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifndef _SYS_SHM_H
#define _SYS_SHM_H

#ident	"@(#)head.sys:sys/shm.h	11.19"

/*
**	IPC Shared Memory Facility.
*/

/*
**	Implementation Constants.
*/

#define	SHMLBA	stob(1)	/* segment low boundary address multiple */
			/* (SHMLBA must be a power of 2) */

/*
**	Permission Definitions.
*/

#define	SHM_R	0400	/* read permission */
#define	SHM_W	0200	/* write permission */

/*
**	ipc_perm Mode Definitions.
*/

#define SHM_LOCKED      001000	/* shmid locked */
#define SHM_LOCKWAIT    010000	/* shmid wanted */

#define PSHM    (PZERO + 1)     /* sleep priority */

/* define resource locking macros */
#define SHMLOCK(sp) { \
        while ((sp)->shm_perm.mode & SHM_LOCKED) { \
                (sp)->shm_perm.mode |= SHM_LOCKWAIT; \
                (void) sleep((caddr_t)(sp), PSHM); \
        } \
        (sp)->shm_perm.mode |= SHM_LOCKED; \
}

#define SHMUNLOCK(sp) { \
        (sp)->shm_perm.mode &= ~SHM_LOCKED; \
        if ((sp)->shm_perm.mode & SHM_LOCKWAIT) { \
                (sp)->shm_perm.mode &= ~SHM_LOCKWAIT; \
                wakeprocs((caddr_t)(sp), PRMPT); \
        } \
}

/*
**	Message Operation Flags.
*/

#define	SHM_RDONLY	010000	/* attach read-only (else read-write) */
#define	SHM_RND		020000	/* round attach address to SHMLBA */

/*
**	Structure Definitions.
*/

/*
**	There is a shared mem id data structure (shmid_ds) for each 
**	segment in the system.
*/

#if defined(_KERNEL) || defined(_KMEMUSER)
struct shmid_ds {
	struct ipc_perm shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	struct anon_map	*shm_amp;	/* segment anon_map pointer */
	ushort		shm_lkcnt;	/* number of times it is being locked */
	pid_t		shm_lpid;	/* pid of last shmop */
	pid_t		shm_cpid;	/* pid of creator */
	ulong		shm_nattch;	/* used only for shminfo */
	ulong		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	long		shm_pad1;	/* reserved for time_t expansion */
	time_t		shm_dtime;	/* last shmdt time */
	long		shm_pad2;	/* reserved for time_t expansion */
	time_t		shm_ctime;	/* last change time */
	long		shm_pad3;	/* reserved for time_t expansion */
	long		shm_pad4[4];	/* reserve area  */
};

/* SVR3 structure */
struct o_shmid_ds {
	struct o_ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	struct anon_map	*shm_amp;	/* segment anon_map pointer */
	ushort		shm_lkcnt;	/* number of times it is being locked */
	char 		pad[2];		
	o_pid_t		shm_lpid;	/* pid of last shmop */
	o_pid_t		shm_cpid;	/* pid of creator */
	ushort		shm_nattch;	/* used only for shminfo */
	ushort		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	time_t		shm_dtime;	/* last shmdt time */
	time_t		shm_ctime;	/* last change time */
};
#else	/* user definition */
#if !defined(_STYPES)
/* this maps to the kernel struct shmid_ds */
struct shmid_ds {
	struct ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	struct anon_map	*shm_amp;	/* segment anon_map pointer */
	ushort		shm_lkcnt;	/* number of times it is being locked */
	pid_t		shm_lpid;	/* pid of last shmop */
	pid_t		shm_cpid;	/* pid of creator */
	ulong		shm_nattch;	/* used only for shminfo */
	ulong		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	long		shm_pad1;	/* reserved for time_t expansion */
	time_t		shm_dtime;	/* last shmdt time */
	long		shm_pad2;	/* reserved for time_t expansion */
	time_t		shm_ctime;	/* last change time */
	long		shm_pad3;	/* reserved for time_t expansion */
	long		shm_pad4[4];	/* reserve area  */
};
#else	/* NON EFT */
/* old struct for compatibility */
struct shmid_ds {
	struct ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	struct anon_map	*shm_amp;	/* segment anon_map pointer */
	ushort		shm_lkcnt;	/* number of times it is being locked */
	char 		pad[2];		
	o_pid_t		shm_lpid;	/* pid of last shmop */
	o_pid_t		shm_cpid;	/* pid of creator */
	ushort		shm_nattch;	/* used only for shminfo */
	ushort		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	time_t		shm_dtime;	/* last shmdt time */
	time_t		shm_ctime;	/* last change time */
};
#endif	/* end defined(_LTYPES) */
#endif	/* end defined(_KERNEL */

struct	shminfo {
	int	shmmax,		/* max shared memory segment size */
		shmmin,		/* min shared memory segment size */
		shmmni,		/* # of shared memory identifiers */
		shmseg;		/* max attached shared memory	  */
				/* segments per process		  */
};


/*
 * Shared memory control operations
 */

#define SHM_LOCK	3	/* Lock segment in core */
#define SHM_UNLOCK	4	/* Unlock segment */

#if defined(__STDC__) && !defined(_KERNEL)
int shmctl(int, int, ...);
int shmget(key_t, int, int);
void *shmat(int, void *, int);
int shmdt(void *);
#endif

typedef struct segacct {
	struct segacct	*sa_next;
	caddr_t		 sa_addr;
	size_t		 sa_len;
	struct anon_map *sa_amp;
} segacct_t;

#endif	/* _SYS_SHM_H */
