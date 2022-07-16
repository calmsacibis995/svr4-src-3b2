/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/file.c	1.2"

#include	<fcntl.h>
#include	<signal.h>
#include	<unistd.h>

#define	READ_SZ	512

extern long lseek();
extern int read(), write();
extern int rename(), lockf(), close();
extern unsigned int alarm();

static char buffer[ READ_SZ ];

/* copy file "from" to file "to" */
int
f_copy( to, from )
int to, from;
{
	register count = 0;

	(void) lseek( to, 0, 0 );
	(void) lseek( from, 0, 0 );

	while( (count != -1) && ((count = read( from, buffer, READ_SZ )) > 0) )
		count = write( to, buffer, (unsigned int)count );

	return( count != -1 );
}

static void
alrm( signal )
int signal;
{
}

int
f_update( to_name, from_name )
char *to_name, *from_name;
{
	register to_fd, from_fd, seconds, rc = 0;
	void	(*fcn)();
	
	/* First try rename */
	if( rename( from_name, to_name ) != -1 )
		return( 0 );

	/* Now try lock and copy */
	if( !(to_fd = open( to_name, O_WRONLY ))
		|| !(from_fd = open( from_name, O_RDONLY )) )
		return( -1 );
	
	seconds = alarm( (unsigned int) 0);
	fcn = sigset( SIGALRM, alrm );

	if( lockf( to_fd, F_LOCK, 0 ) != -1
		&& f_copy( to_fd, from_fd ) ) {

		(void) lockf( to_fd, F_ULOCK, 0 );

		(void) sigset( SIGALRM, fcn );
		if( seconds > 0 ) (void) alarm( (unsigned int) seconds );

	} else rc = -1;

	(void) close( to_fd );
	(void) close( from_fd );

	return( rc );
}
