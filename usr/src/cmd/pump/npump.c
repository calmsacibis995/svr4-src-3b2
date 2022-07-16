/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pump:npump.c	1.3"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <filehdr.h>
#include <scnhdr.h>
#include <ldfcn.h>
#include <sys/stropts.h>
#include <sys/strpump.h>

#define TIMEOUT	120		/* I_STR timeout value		*/
#define MAXBUF  128000		/* max size of firmware RAM 	*/

struct strioctl ioctlst;	/* STREAMS I_STR control str	*/
struct pumpst pumpst;

char buffer[MAXBUF];		/* firmware RAM image		*/
extern int errno;

main(argc, argv)
int argc;
char *argv[];
{
	register int i;
	unsigned short sect;
	int fd, bstart, bend, size;
	long startaddr;
	char *pumpfile, *device;
	LDFILE *ldptr;
	SCNHDR secthdr;

/*
 * Check for proper number of arguments
 */

	if( argc > 3 )
	{
		fprintf(stderr, "Too many arguments to npump\n");
		fprintf(stderr, "syntax: npump pump_device pump_file\n");
		exit(1);
	}
	
	device = argv[1];
	pumpfile = argv[2];

	if( (fd = open( device, O_WRONLY)) < 0 )
	{
		perror("Cannot open device to pump");
		exit(1);
	}

/*
 * RESET the board
 */

	ioctlst.ic_cmd    = P_RST;	/* RESET */
	ioctlst.ic_timout = TIMEOUT;
	ioctlst.ic_len    = 0;
	ioctlst.ic_dp     = NULL;

	if( ioctl(fd, I_STR, &ioctlst) < 0 )
	{
		perror("Unable to reset device");
		exit(1);
	}
/*
 * Open the file to be downloaded, and check its type
 */

	if( (ldptr = ldopen(pumpfile, ldptr)) == NULL )
	{
		perror("Cannot open pump code file");
		exit(1);
	}
	if( (HEADER(ldptr).f_magic != B16MAGIC)  &&
	    (HEADER(ldptr).f_magic != X86MAGIC)  )
	{
		perror("Bad magic number in pump code file");
		exit(1);
	}

/*
 * Read the entire pump file into an internal buffer and download
 */

        startaddr = -1;
	bstart = MAXBUF;
	bend   = 0;

	for( sect = 1; sect <= HEADER(ldptr).f_nscns; sect++ )
	{
		if( ldshread(ldptr, sect, &secthdr) == FAILURE )
		{
			perror("Read error on pump code file");
			exit(1);
		}

		/*
		 * Look for the address at which to start exectution
		 */
		if( strcmp(".start", secthdr.s_name) == 0 )
		{
			startaddr = secthdr.s_paddr;
		}

		/*
		 * Ignore empty sections
		 */
		if( (i = secthdr.s_size) == 0 )
		{
			continue;
		}

		/*
		 * Record the high and low buffer addresses
		 */
		if( (i += secthdr.s_paddr) > MAXBUF )
		{
			perror("MAXBUF size exceeded");
			exit(1);
		}
		if( bstart > secthdr.s_paddr )
		{
			bstart = secthdr.s_paddr;
		}
		if( bend < i )
		{
			bend = i;
		}

		/*
		 * For .bss-type sections, generate zero values to
		 * download
		 */
		if( secthdr.s_scnptr == 0 )
		{
			buffer[secthdr.s_paddr] = 0;
			memcpy(&buffer[secthdr.s_paddr+1], &buffer[secthdr.s_paddr], secthdr.s_size-1);
			continue;
		}

		/*
		 * Read a section into an internal buffer
		 */
		if( ldsseek(ldptr, sect) == FAILURE )
		{
			perror("Lseek failure on pump code file");
			exit(1);
		}
		if( FREAD(&buffer[secthdr.s_paddr], 1, (int) secthdr.s_size, ldptr) == NULL )
		{
			perror("FREAD failure on pump code file");
			exit(1);
		}
	}

/*
 * Download the firmware in 3*256 byte pieces:
 *
 *	1. STREAMS automatically partitions ioctl() data into blocks of
 *		at most 1024 bytes, and the EMD does NOT want to handle
 *		multiple data blocks
 *	2. 3B2 CIO requires that the starting firmware address for any
 *		download operation be on a 256-byte boundry
 */

	size = bend - bstart;
	if( (bstart % 256) != 0 )
	{
		i = 256 - (bstart % 256);
		bstart -= i;
		size += i;
	}

	pumpst.address = (char *)bstart;
	while( size > PUMPBSIZE )
	{
		ioctlst.ic_cmd    = P_LOAD;	/* DOWNLOAD */
		ioctlst.ic_timout = TIMEOUT;
		ioctlst.ic_len    = sizeof(pumpst);
		ioctlst.ic_dp     = (char *)&pumpst;

		pumpst.flags   = 0;
		pumpst.size    = PUMPBSIZE;

		memcpy(pumpst.data,&buffer[(int) pumpst.address],pumpst.size);

		if( ioctl(fd, I_STR, &ioctlst) < 0 )
		{
			perror("LOAD ioctl failed (a)");
			exit(1);
		}

		pumpst.address += PUMPBSIZE;
		size -= PUMPBSIZE;
	}
	if( size > 0 )
	{
		ioctlst.ic_cmd    = P_LOAD;
		ioctlst.ic_timout = TIMEOUT;
		ioctlst.ic_len    = sizeof(pumpst);
		ioctlst.ic_dp     = (char *)&pumpst;

		pumpst.flags   = 0;
		pumpst.size    = size;

		memcpy(pumpst.data,&buffer[(int) pumpst.address],pumpst.size);

		if( ioctl(fd, I_STR, &ioctlst) < 0 )
		{
			perror("LOAD ioctl failed (b)");
			exit(1);
		}
	}

	ldclose(ldptr);

/*
 * There must have been a section called ".start"
 */

	if( startaddr == -1 )
	{
		perror("Invalid start address");
		exit(1);
	}


/*
 * Supply the address to start execution
 */

	ioctlst.ic_cmd    = P_FCF;	/* FORCE CALLED FUNCTION */
	ioctlst.ic_timout = TIMEOUT;
	ioctlst.ic_len    = sizeof(startaddr);
	ioctlst.ic_dp     = (char *)&startaddr;

	if( ioctl(fd, I_STR, &ioctlst) < 0 )
	{
		perror("FCF ioctl failed");
		exit(1);
	}
	sleep(2);

/*
 * SYSGEN the board
 */

	ioctlst.ic_cmd    = P_SYSGEN;	/* SYSGEN */
	ioctlst.ic_timout = TIMEOUT;
	ioctlst.ic_len    = 0;
	ioctlst.ic_dp     = NULL;

	if( ioctl(fd, I_STR, &ioctlst) < 0 )
	{
		perror("SYSGEN ioctl failed");
		exit(1);
	}
	sleep(2);

	if( close(fd) < 0 )
	{
		perror("Close of device failed");
		exit(1);
	}

	exit(0);
}

