/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/xmalloc.c	1.3"

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>


/*
 * fast personal malloc routine for cunix. currently manages up to 4 megabytes.
 */

#define XNUMSEGS 32
#define XSEGSIZE 128 * 1024

static char *xseg[XNUMSEGS];

static int totsize = 0;
static int remainder = 0;
static char *addr = (char *)0;
static int xfd = -1;
static int curseg = -1;

 char *
xmalloc(nbytes)
unsigned int nbytes;
{

	char *retaddr;

	if (nbytes == 0 || nbytes > XSEGSIZE)
		return(0);

	nbytes = (nbytes + sizeof(int) - 1 ) & ~(sizeof(int) - 1);

	if (xfd == -1)
		if ((xfd = open("/dev/zero",O_RDWR)) == -1){
			panic("can't open /dev/zero for xmalloc\n");
			exit(1);
		}
	
	if ( (remainder - (int)nbytes) < 0){
		curseg++;
		if (curseg >= XNUMSEGS){
			panic("xmalloc failure, upper limit exceeded\n");
			exit(1);
		}
		xseg[curseg] = (char *)mmap(0, XSEGSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, xfd, 0);
		if ((addr = xseg[curseg]) == (char *)-1){
			panic("xmalloc: mmap failure\n");
			exit(1);
		}
		remainder = XSEGSIZE;
	}

	retaddr = addr;
	addr += nbytes;
	totsize += nbytes;
	remainder -= nbytes;
	return(retaddr);
}


 void
xfree()
{
	int i;

	for (i=0; i < (curseg + 1); i++){
		(void)munmap(xseg[i],XSEGSIZE);
	}

	if (xfd != -1)
		close(xfd);
	totsize = 0;
	remainder = 0;
	curseg = -1;
	addr = (char *)0;
}


