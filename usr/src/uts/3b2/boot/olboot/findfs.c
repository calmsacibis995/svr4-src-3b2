/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/olboot/findfs.c	11.4"

#include	"sys/param.h"
#include	"sys/types.h"
#include	"sys/inode.h"
#include	"sys/filsys.h"
#include	"sys/iobuf.h"
#include	"sys/elog.h"
#include	"sys/firmware.h"
#include	"sys/boot.h"
#include	"sys/lboot.h"

extern void ndisk_acs();

/*
 *	findfs - Find root file system.
 *
 *	Expand the inode for the root directory into Dinode.
 */

unsigned short
findfs()
{

	/* Get inode for root directory */

	Dinode.i_number = ROOTINO;
	liread(&Dinode);
	return(1);
}

/*
 *	readsb() - read superblock
 *
 */
void
readsb()
{
	struct filsys *fsptr;
	extern int 	Ind1b,	/* current block # in IND1 */
			Ind2b,	/* current block # in IND2 */
			Ind3b,	/* current block # in IND3 */
			Inodeb;	/* current block # in INODE */

	Ind1b = Ind2b = Ind3b = Inodeb = 0;

	fsptr = (struct filsys *)IOBASE;

	ndisk_acs(Fso+1, IOBASE);

	if (fsptr->s_magic == FsMAGIC)
		fstype = fsptr->s_type;
	else
		PRINTF("Not FsMAGIC:  fsptr->s_magic is %d   FsMAGIC is %d\n", fsptr->s_magic, FsMAGIC);
}
