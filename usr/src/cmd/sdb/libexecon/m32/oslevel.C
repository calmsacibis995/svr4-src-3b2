//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libexecon/m32/oslevel.C	1.21"
#define lint	1	/* avoid asm's in mau.h */
#include	"oslevel.h"
#include	<osfcn.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<malloc.h>
#include	<string.h>
#include	<sys/reg.h>
#include	<sys/psw.h>
#include	<sys/pcb.h>
#include	<sys/siginfo.h>
#include	<sys/signal.h>
#include	<sys/syscall.h>
#include	<sys/fs/s5dir.h>
#define	MAXSIG	32
#include	<sys/user.h>

char	executable_name[100];

int
get_bytes( Key key, unsigned long offset, void * buf, int len )
{
	int		x;
	unsigned long	addr;
	int 		wdoff;
	int		sz;
	char *		s;
	char *		p;
	int		word;

	if ( key.fd != -1 )
	{
		do {
			::errno = 0;
			::lseek( key.fd, offset, 0 );
		} while ( ::errno == EINTR );
		if ( ::errno != 0 ) return 0;
		do {
			::errno = 0;
			x = ::read( key.fd, (char*)buf, len );
		} while ( ::errno == EINTR );
		return x;
	}
	else
	{
		p = (char *) buf;
		addr = offset & ~0x3;
		wdoff = offset - addr;
		sz = len;
		while ( sz > 0 )
		{
			s = (char*) &word;
			s += wdoff;
			::errno = 0;
			word = ::ptrace( 1, key.pid, addr, 0 );
			if ( ::errno != 0 ) return (int)(addr-offset+wdoff);
			switch (wdoff)
			{
				case 0:	*p = *s; ++p; ++s;
					if ((--sz) == 0) break;
				case 1:	*p = *s; ++p; ++s;
					if ((--sz) == 0) break;
				case 2:	*p = *s; ++p; ++s;
					if ((--sz) == 0) break;
				case 3:	*p = *s; ++p; ++s;
					if ((--sz) == 0) break;
			}
			addr += 4;
			wdoff = 0;
		}
		return len;
	}
}

int
put_bytes( Key key, unsigned long offset, void * buf, int len )
{
	int		x;
	unsigned long	addr;
	int 		wdoff;
	int		sz;
	char *		s;
	char *		p;
	int		word;

	if ( key.fd != -1 )
	{
		do {
			::errno = 0;
			::lseek( key.fd, offset, 0 );
		} while ( ::errno == EINTR );
		if ( ::errno != 0 ) return 0;
		do {
			::errno = 0;
			x = ::write( key.fd, (char*)buf, len );
		} while ( ::errno == EINTR );
		return x;
	}
	else
	{
		p = (char *) buf;
		addr = offset & ~0x3;
		wdoff = offset - addr;
		sz = len;
		if ( wdoff > 0 )
		{
			::errno = 0;
			word = ::ptrace( 1, key.pid, addr, 0 );
			s = (char*) &word;
			s += wdoff;
			switch (wdoff)
			{
				case 1:	*s = *p; ++s; ++p;
					if ((--sz) == 0) break;
				case 2:	*s = *p; ++s; ++p;
					if ((--sz) == 0) break;
				case 3:	*s = *p; ++s; ++p;
					if ((--sz) == 0) break;
			}
			::errno = 0;
			::ptrace( 4, key.pid, addr, word );
			addr += 4;
		}
		while ( sz >= 4 )
		{
			s = (char*) &word;
			*s = *p; ++s; ++p;
			*s = *p; ++s; ++p;
			*s = *p; ++s; ++p;
			*s = *p; ++s; ++p;
			::errno = 0;
			::ptrace( 4, key.pid, addr, word );
			addr += 4;
			sz -= 4;
		}
		if ( sz > 0 )
		{
			::errno = 0;
			word = ::ptrace( 1, key.pid, addr, 0 );
			s = (char*) &word;
			switch ( sz )
			{
				case 3:	*s = *p; ++s; ++p; --sz;
				case 2:	*s = *p; ++s; ++p; --sz;
				case 1:	*s = *p; ++s; ++p; --sz;
			}
			::errno = 0;
			::ptrace( 4, key.pid, addr, word );
		}
		return len;
	}
}

int
send_sig( Key key, int sig )
{
	do {
		::errno = 0;
		::kill( key.pid, sig );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

#if PTRACE

int
get_key( int pid, Key & key )
{
	key.fd = -1;
	key.pid = pid;
	return 1;
}

int
live_proc( int )
{
	return 0;
}

Key
dispose_key( Key key )
{
	key.fd = -1;
	key.pid = -1;
	return key;
}

void
stop_self_on_exec()
{
	errno = 0;
	::ptrace( 0, 0, 0, 0 );
}

int
clear_pend_sig( Key )
{
	return 1;
}

int
run_proc( Key key, prrun &, int sig )
{
	do {
		::errno = 0;
		::ptrace( 7, key.pid, 1, sig );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
step_proc( Key key, prrun &, int sig )
{
	do {
		::errno = 0;
		::ptrace( 9, key.pid, 1, sig );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
stop_proc( Key , prstatus * )
{
	return 1;
}

int
wait_for( Key , prstatus & prstat )
{
	int	status;
	int	lobyte,hibyte;

	do {
		::errno = 0;
		::wait( &status );
	} while ( ::errno == EINTR );
	lobyte = status & 0xff;
	hibyte = status >> 8;
	if ( lobyte == 0x7f )
	{
		prstat.pr_why = PR_SIGNALLED;
		prstat.pr_what = hibyte;
		return 1;
	}
	else
	{
		return 0;
	}
}

int
kill_proc( Key key )
{
	do {
		::errno = 0;
		::kill( key.pid, 9 );
	} while ( ::errno == EINTR );
	if ( ::errno != 0 ) return 0;
	do {
		::errno = 0;
		::ptrace( 7, key.pid, 1, 0 );
	} while ( ::errno == EINTR );
	if ( ::errno == ESRCH )
		return 1;
	return ( ::errno == 0 );
}

int
catch_sigs( Key , sigset_t , prrun & )
{
	return 1;
}

int
get_status( Key , prstatus & )
{
	return 0;
}

int
open_object( Key key, unsigned long )
{
	int		fdobj;
/*
	char		buf[PSARGSZ+1];
	char *		p;
	long		offset, addr;
	int		wdoff;
	int		word;
	int		sz;
	char *		s;

	p = buf;
	offset = ((long)(((struct user *)0)->u_psargs));
	addr = offset & ~0x3;
	wdoff = offset - addr;
	sz = PSARGSZ;
	while ( sz > 0 )
	{
		s = (char*) &word;
		s += wdoff;
		word = ::ptrace( 3, key.pid, addr, 0 );
		switch (wdoff)
		{
			case 0:	*p = *s; ++p; ++s;
				if ((--sz) == 0) break;
			case 1:	*p = *s; ++p; ++s;
				if ((--sz) == 0) break;
			case 2:	*p = *s; ++p; ++s;
				if ((--sz) == 0) break;
			case 3:	*p = *s; ++p; ++s;
				if ((--sz) == 0) break;
		}
		addr += 4;
		wdoff = 0;
	}
	p = ::strchr( buf, ' ' );
	*p = '\0';
*/
	do {
		::errno = 0;
		fdobj = ::open( executable_name, O_RDONLY );
	} while ( ::errno == EINTR );
	return fdobj;
}

int
getfpset( Key , fpregset_t & )
{
	return 0;
}

char *
psargs( Key )
{
	return 0;	// not used if PTRACE
}

#else

int
get_key( int pid, Key & key )
{
	char	filename[12];

	::sprintf( filename, "/proc/%.5d", pid );
	key.pid = pid;
	do {
		::errno = 0;
		key.fd = ::open( filename, O_RDWR );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
live_proc( int fd )
{
	prstatus	p;

	::errno = 0;
	::ioctl( fd, PIOCSTATUS, &p );
	return ( ::errno == 0 );
}

Key
dispose_key( Key key )
{
	do {
		::errno = 0;
		::close( key.fd );
	} while ( ::errno == EINTR );
	key.fd = -1;
	return key;
}

void
stop_self_on_exec()
{
	int		pid;
	Key		key;
	sysset_t	scset;
	sigset_t	siset;

	premptyset( &scset );
	prfillset( &siset );
	praddset( &scset, SYS_exec );
	praddset( &scset, SYS_execve );
	do {
		::errno = 0;
		pid = ::getpid();
	} while ( ::errno == EINTR );
	get_key( pid, key );
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCSTRACE, &siset );
	} while ( ::errno == EINTR );
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCSEXIT, &scset );
	} while ( ::errno == EINTR );
	do {
		::errno = 0;
		::close( key.fd );
	} while ( ::errno == EINTR );
}

int
run_proc( Key key, prrun & pr_run, int signo )
{
	pr_run.pr_flags &= ~PRSTEP;
	if ( signo == 0 )
		pr_run.pr_flags |= PRCSIG;
	else
		pr_run.pr_flags &= ~PRCSIG;
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCRUN, &pr_run );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
step_proc( Key key, prrun & pr_run, int signo )
{
	pr_run.pr_flags |= PRSTEP;
	if ( signo == 0 )
		pr_run.pr_flags |= PRCSIG;
	else
		pr_run.pr_flags &= ~PRCSIG;
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCRUN, &pr_run );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
stop_proc( Key key, prstatus * prstat )
{
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCSTOP, prstat );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
wait_for( Key key, prstatus & prstat )
{
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCWSTOP, &prstat );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
kill_proc( Key key )
{
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCSSIG, 0 );
	} while ( ::errno == EINTR );
	if ( ::errno != 0 ) return 0;
	do {
		::errno = 0;
		::kill( key.pid, 9 );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
catch_sigs( Key key, sigset_t sset, prrun & pr_run )
{
	pr_run.pr_trace = sset;
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCSTRACE, &pr_run.pr_trace );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
get_status( Key key, prstatus & prstat )
{
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCSTATUS, &prstat );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

int
open_object( Key key, unsigned long addr )
{
	int		fdobj;
	caddr_t *	p;

	p = ( addr == 0 ) ? 0 : (caddr_t*)&addr;
	do {
		::errno = 0;
		fdobj = ::ioctl( key.fd, PIOCOPENM, p );
	} while ( ::errno == EINTR );
	return fdobj;
}

#if 0
int
open_object( Key , unsigned long )
{
	int		fdobj;

	do {
		::errno = 0;
		fdobj = ::open( executable_name, O_RDONLY );
	} while ( ::errno == EINTR );
	return fdobj;
}
#endif

int
getfpset( Key key, fpregset_t & fpset )
{
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCGFPREG, &fpset );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

char *
psargs( Key key )
{
	static prpsinfo_t psinfo;
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCPSINFO, &psinfo );
	} while ( ::errno == EINTR );
	if ( ::errno == 0 )
		return psinfo.pr_psargs;
	else
		return 0;
}

#endif
