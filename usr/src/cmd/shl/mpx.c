/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)shl:mpx.c	1.7.2.1"
#include	"defs.h"
#include	"unistd.h"
#include	"sys/stropts.h"

open_cntl_chan()
{
	register int fd;
	register char p;

	while ((fd = open(cntlf, O_RDWR | O_EXCL)) == SYSERROR)
	{
		if (errno == EBUSY)
		{
			chan += MAX_LAYERS;
			set_dev(chan);
		}
		else
		{
			fprintf(stderr, "no control channels available (errno=%d)\n", errno);
			return(SYSERROR);
		}
	}


	{	register int j;

		for (j=0; j < MAX_LAYERS; ++j)
		{
			set_dev(chan + j);
			if (chown(cntlf, uid, gid) != SYSERROR)
			{
				chmod(cntlf, 0622);
			}
			else
			{
				fprintf(stderr, "chown on %s failed (errno = %d)\n", cntlf, errno);
				close(fd);
				return(SYSERROR);
			}
		}

	}

	return(fd);
}

int cook;		/* returned from I_LINK ioctl */
multiplex()
{
	extern int real_tty_fd;

	if ((cook = ioctl(cntl_chan_fd, I_LINK, real_tty_fd)) == SYSERROR)
	{
		switch (errno)
		{
			case ENXIO:
				fprintf(stderr, "sxt driver not configured (errno = %d) \n");
				break;

			case ENOMEM:
				fprintf(stderr, "no memory for kernel configuration (errno = %d) \n");
				break;

			case ENOTTY:
				fprintf(stderr,"not using a tty device (errno = %d) \n");
				break;

			case EAGAIN:
				fprintf(stderr,"Out streams buffers (errno = %d) \n");
				break;

			default:
				fprintf(stderr, "multiplex failed (errno = %d)\n", errno);
		}

		return(0);
	}

	return(1);
}

close_cntl_chan()
{
	register int j;

	for (j=0; j < MAX_LAYERS; ++j)
	{
		set_dev(chan + j);
		if (chown(cntlf, ROOT_UID, SYS_GID) != SYSERROR)
		{
			chmod(cntlf, 0620);
		}
		else
		{
			fprintf(stderr, "chown on %s failed (errno = %d)\n", cntlf, errno);
			return(SYSERROR);
		}
	}
	return(0);
}
