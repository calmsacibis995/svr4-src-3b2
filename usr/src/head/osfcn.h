/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head:osfcn.h	1.9"

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
extern int dup(int);
extern int execl(const char *, const char *, ...);
extern int execle(const char *, const char *, ...);
extern int execlp(const char *, const char *, ...);
extern int execv(const char *, const char **);
extern int execve(const char *, const char **, const char**);
extern int execvp(const char *, const char **);
extern void exit(int);
extern void _exit(int);
extern pid_t fork(void);
extern gid_t getegid(void);
extern uid_t geteuid(void);
extern gid_t getgid(void);
extern pid_t getpid(void);
extern pid_t getppid(void);
extern pid_t getpgrp(void);
extern uid_t getuid(void);
extern int ioctl(int, int, ...);
extern int link(const char *, const char *);
extern int lockf(int, int, long);
extern long lseek(int, long, int);
extern int nice(int);
extern int pathconf(char *, int);
extern int fpathconf(int, int);
extern int pause(void);
extern int pipe(int *);
extern void profil(char *, int, int, int);
extern int ptrace(int, pid_t, int, int);
extern int read(int, void *, unsigned);
extern int rmdir(const char *);
extern void *sbrk(int);
extern int setgid(gid_t);
extern pid_t setpgrp(void);
extern int setuid(uid_t);
extern unsigned sleep(unsigned);
extern int stime(const time_t *);
extern void sync(void);
extern int sysconf(int);
extern long ulimit(int, long);
extern mode_t umask(mode_t);
extern int unlink(const char *);
extern int wait(int *);
extern int write(int, const void *, unsigned);

#else
extern unsigned alarm();
extern void exit();
extern void _exit();
extern unsigned short getegid();
extern unsigned short geteuid();
extern unsigned short getgid();
extern unsigned short getuid();
extern long lseek();
extern void profil();
extern char *sbrk();
extern unsigned sleep();
extern void sync();
extern long ulimit();

#endif
