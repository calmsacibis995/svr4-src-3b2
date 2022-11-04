/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UNISTD_H
#define _UNISTD_H

#ident	"@(#)head:unistd.h	1.14"

/* Symbolic constants for the "access" routine: */
#define	R_OK	4	/* Test for Read permission */
#define	W_OK	2	/* Test for Write permission */
#define	X_OK	1	/* Test for eXecute permission */
#define	F_OK	0	/* Test for existence of File */

#define F_ULOCK	0	/* Unlock a previously locked region */
#define F_LOCK	1	/* Lock a region for exclusive use */
#define F_TLOCK	2	/* Test and lock a region for exclusive use */
#define F_TEST	3	/* Test a region for other processes locks */


/* Symbolic constants for the "lseek" routine: */
#define	SEEK_SET	0	/* Set file pointer to "offset" */
#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */

/* Path names: */
#define	GF_PATH	"/etc/group"	/* Path name of the "group" file */
#define	PF_PATH	"/etc/passwd"	/* Path name of the "passwd" file */


/* command names for POSIX sysconf */
#define _SC_ARG_MAX	1
#define _SC_CHILD_MAX	2
#define _SC_CLK_TCK	3
#define _SC_NGROUPS_MAX 4
#define _SC_OPEN_MAX	5
#define _SC_JOB_CONTROL 6
#define _SC_SAVED_IDS	7
#define _SC_VERSION	8
#define _SC_PASS_MAX	9
#define _SC_LOGNAME_MAX	10
#define _SC_PAGESIZE	11

/* command names for POSIX pathconf */

#define _PC_LINK_MAX	1
#define _PC_MAX_CANON	2
#define _PC_MAX_INPUT	3
#define _PC_NAME_MAX	4
#define _PC_PATH_MAX	5
#define _PC_PIPE_BUF	6
#define _PC_NO_TRUNC	7
#define _PC_VDISABLE	8
#define _PC_CHOWN_RESTRICTED	9

/* compile-time symbolic constants,
** Support does not mean the feature is enabled.
** Use pathconf/sysconf to obtain actual configuration value.
** 
*/

#define _POSIX_JOB_CONTROL	1
#define _POSIX_SAVED_IDS	1

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE		0
#endif

#ifndef	NULL
#define NULL	0
#endif

#define	STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

/* Current version of POSIX */
#define _POSIX_VERSION		198808L


#if defined(__STDC__)

#include <sys/types.h>

extern int access(const char *, int);
extern int acct(const char *);
extern unsigned alarm(unsigned);
extern int brk(void *);
extern int chdir(const char *);
extern int chown(const char *, uid_t, gid_t);
extern int chroot(const char *);
extern int close(int);
extern char *ctermid(char *);
extern char *cuserid(char *);
extern int dup(int);
extern int dup2(int, int);
extern int execl(const char *, const char *, ...);
extern int execle(const char *, const char *, ...);
extern int execlp(const char *, const char *, ...);
extern int execv(const char *, const char **);
extern int execve(const char *, const char **, const char**);
extern int execvp(const char *, const char **);
extern void exit(int);
extern void _exit(int);
extern pid_t fork(void);
extern int fpathconf(int, int);
extern char *getcwd(char *, int);
extern gid_t getegid(void);
extern uid_t geteuid(void);
extern gid_t getgid(void);
extern int getgroups(int, gid_t *);
extern char *getlogin();
extern pid_t getpid(void);
extern pid_t getppid(void);
extern pid_t getpgrp(void);
extern char *getlogin();
extern uid_t getuid(void);
extern int ioctl(int, int, ...);
extern int isatty(int);
extern int link(const char *, const char *);
extern int lockf(int, int, long);
extern long lseek(int, long, int);
extern int nice(int);
extern int pathconf(char *, int);
extern int pause(void);
extern int pipe(int *);
extern void profil(char *, int, int, int);
extern int ptrace(int, pid_t, int, int);
extern int read(int, void *, unsigned);
extern int rmdir(const char *);
extern void *sbrk(int);
extern int setgid(gid_t);
extern int setpgid(pid_t, pid_t);
extern pid_t setpgrp(void);
extern pid_t setsid();
extern int setuid(uid_t);
extern unsigned sleep(unsigned);
extern int stime(const time_t *);
extern void sync(void);
extern int sysconf(int);
extern char *ttyname(int);
extern pid_t tcgetpgrp(int);
extern int tcsetpgrp(int, pid_t);
extern char *ttyname(int);
extern long ulimit(int, long);
extern int unlink(const char *);
extern int wait(int *);
extern int write(int, const void *, unsigned);

#else
extern int access();
extern int acct();
extern unsigned alarm();
extern int brk();
extern int chdir();
extern int chown();
extern int chroot();
extern int close();
extern char *ctermid();
extern char *cuserid();
extern int dup();
extern int dup2();
extern int execl();
extern int execle();
extern int execlp();
extern int execv();
extern int execve();
extern int execvp();
extern void exit();
extern void _exit();
extern pid_t fork();
extern int fpathconf();
extern char *getcwd();
extern gid_t getegid();
extern uid_t geteuid();
extern gid_t getgid();
extern int getgroups();
extern char *getlogin();
extern pid_t getpid();
extern pid_t getppid();
extern pid_t getpgrp();
extern uid_t getuid();
extern int ioctl();
extern int isatty();
extern int link();
extern int lockf();
extern long lseek();
extern int nice();
extern int pathconf();
extern int pause();
extern int pipe();
extern void profil();
extern int ptrace();
extern int read();
extern int rmdir();
extern void *sbrk();
extern int setgid();
extern int setpgid();
extern pid_t setpgrp();
extern pid_t setsid();
extern int setuid();
extern unsigned sleep();
extern int stime();
extern void sync();
extern int sysconf();
extern pid_t tcgetpgrp();
extern int tcsetpgrp();
extern char *ttyname();
extern long ulimit();
extern int unlink();
extern int wait();
extern int write();

#endif

#endif /* _UNISTD_H */
