/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.sys:sys/session.h	1.12"

#ifndef _SYS_SESSION_H
#define _SYS_SESSION_H

typedef struct sess {
	short s_procs; 			/* reference count */
	short s_mode;			/* session permissions */
	pid_t s_sid;			/* session ID */
	uid_t s_uid;			/* user ID */
	gid_t s_gid;			/* group ID */
	dev_t s_dev;			/* session device number */
	pid_t	*s_sidp;		/* tty's copy of session ID */
	pid_t	*s_fgidp;		/* foreground process group ID */
	struct vnode *s_vp;		/* tty's vnode */
} sess_t;

extern sess_t session0;

/*
 * Enumeration of the types of access that can be requested for a 
 * controlling terminal under job control.
 */

enum jcaccess {
	JCREAD,			/* read data on a ctty */
	JCWRITE,		/* write data to a ctty */
	JCSETP,			/* set ctty parameters */
	JCGETP			/* get ctty parameters */
};


#if defined(__STDC__)

dev_t	cttydev(proc_t *);
void	freectty(sess_t *);
void	forksession(proc_t *, sess_t *);
void	exitsession(proc_t *);
void	newsession();
void	closctty();
void	alloctty(proc_t *, vnode_t *, pid_t *, pid_t *);
void	realloctty(vnode_t *);
int	accsctty(vnode_t *, enum jcaccess, int);

#else

dev_t	cttydev();
void	freectty();
void	forksession();
void	exitsession();
void	newsession();
void	closctty();
void	alloctty();
void	realloctty();
int	accsctty();

#endif

#endif
