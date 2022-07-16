/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ttymon:tmpeek.c	1.7"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include "ttymon.h"
#include "tmstruct.h"
#include "tmextern.h"

#define BRK	1
#define DEL	2

static	struct	strbuf *do_peek();
static	int	process();
static	void	sigint();

static	int	interrupt;

/*
 *	poll_data	- it polls the device, waiting for data
 *			- return BADSPEED it <brk> is received
 *			- return the result of process if data is received
 *			- write a newline if <del> is received
 *			- exit if hangup is received
 */
poll_data(pfirst)
int	pfirst;	/* if TRUE, will do peek before poll to improve performance */
{
	int	 j;
	register struct	strbuf *ptr;
	struct 	 pollfd fds[1];

#ifdef	DEBUG
	debug("in poll_data");
#endif
	if (pfirst) {
		ptr = do_peek(0, 255);
		if (ptr != NULL) {
			return(process(0,ptr));
		}
	}
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	(void)sigset(SIGINT, sigint); 
	for (;;) {
		interrupt = 0;
		if ((j = poll(fds,1,-1)) == -1) {
			if (interrupt == BRK) {
				return(BADSPEED);
			}
			if (interrupt == DEL) {
				(void)write(1,"\r\n",2);
			}
		}
		else if (j > 0) {
			if (fds[0].revents & POLLHUP) {
				log( "POLLHUP received, about to exit");
				exit(1);
			}
			if (fds[0].revents & POLLIN) {
				ptr = do_peek(fds[0].fd, 255);
				if (ptr != NULL) {
					return(process(fds[0].fd,ptr));
				}
			}
		}
	}
}

/*
 *	process	- process the data 
 *		- return GOODNAME if it is a non-empty line 
 *		- return NONAME if a <CR> is received
 *		- return BADNAME if zero byte is detected
 *		- except the case of GOODNAME, data will be pulled out
 *		  of the stream 
 */
static int
process(fd,ptr)
int	fd;			/* fd to read data from if necessary 	*/
register struct strbuf *ptr;	/* ptr that holds data in ptr->buf 	*/
{
	unsigned i;
	char	junk[BUFSIZ];

	for (i = 0; i < ptr->len; i++) {
		if (ptr->buf[i] == '\0') {
			(void)read(fd, junk, i+1);
			return(BADSPEED);
		}
		else if ((ptr->buf[i] == '\n') || (ptr->buf[i] == '\r')) {
			if (i == 0) {
				(void)read(fd, junk, ptr->len);
				return(NONAME);
			}
			else
				return(GOODNAME);
		}
	}	/* end for loop */
	/* end of input is encountered */
#ifdef	DEBUG
	debug("in process: EOF encountered");
#endif
	exit(1);
	/*NOTREACHED*/
}

/*
 *	do_peek	- peek at the stream to get the data
 *		- this only called when POLLIN is detected,
 *		- so there should always be something there 
 *		- return a ptr to the buf that contains the data
 *		- return NULL if nothing to peek at
 */
static	struct	strbuf	*
do_peek(fd,n)
int	fd;	/* fd to do the ioctl on */
int	n;	/* maxlen of data to peek at */
{
	int	 ret;
	static 	 struct strpeek peek;
	register struct strpeek *peekp;
	static	 char	buf[BUFSIZ];

#ifdef	DEBUG
	debug("in do_peek");
#endif

	peekp = &peek;
	peekp->flags = 0;
	/* need to ask for ctl info to avoid bug in I_PEEK code */
	peekp->ctlbuf.maxlen = 1;
	peekp->ctlbuf.buf = buf;
	peekp->databuf.maxlen = n;
	peekp->databuf.buf = buf;
	ret = ioctl(fd, I_PEEK, &peek);
	if (ret == -1) {
		(void)sprintf(Scratch,"do_peek: I_PEEK failed, errno = %d", 
				errno);
		log(Scratch);
		exit(1);
	}
	if (ret == 0) {
		return( (struct strbuf *)NULL );
	}
	return(&(peekp->databuf));
}

/*
 *	sigint	- this is called when SIGINT is caught
 */
static	void
sigint()
{
	struct strbuf *ptr;
	char   junk[2];

#ifdef	DEBUG
	debug("in sigint");
#endif
	ptr = do_peek(0, 1);
	if (ptr == NULL) {	/* somebody type <del> */
		interrupt = DEL;
	}
	else {
		if (ptr->buf[0] == '\0') {
			/* somebody type <brk> or frame error */
			(void)read(0,junk,1);
			interrupt = BRK;
		}
	}
}

