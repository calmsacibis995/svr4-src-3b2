/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/olboot/basicio.c	11.9"

#include	"sys/param.h"
#include	"sys/types.h"
#include	"sys/filsys.h"
#include	"sys/inode.h"
#include	"sys/ino.h"
#include	"sys/iobuf.h"
#include	"sys/elog.h"
#include	"sys/sysmacros.h"
#include	"sys/firmware.h"
#include	"sys/boot.h"
#include	"sys/lboot.h"
#include 	"sys/sbd.h"
#include 	"sys/psw.h"
#include 	"sys/immu.h"
#include 	"sys/nvram.h"
#include	"sys/fsiboot.h"

int	Ind1b,	/* current block # in IND1 */
	Ind2b,	/* current block # in IND2 */
	Ind3b,	/* current block # in IND3 */
	Inodeb;	/* current block # in INODE */

extern void ndisk_acs();

/*
**	This file contains basic I/O routines to unpack inodes and to read
**	a block in a file given the inode.
*/


/*
**	lbmap - Little bmap.
**
**	This routine maps a block number within a file to a block number within
**	a file system.  Zero is returned if the block is not allocated.
**
**	This code is a stripped version of bmap(os/subr.c).
*/

lbmap(ip, bn)
struct inode	*ip;	/* inode of file being referenced */
register int	bn;	/* block # */
{
	register int	nb,	/* next block wanted */
			j;	/* indirect block indicator */
	int		sh,	/* shift count */
			*bap;	/* ptr to indirect block */

	/* 0..NADDR-4 are direct blocks. */
	if (bn < NADDR - 3)
		return(ip->i_addr[bn]);

	/* Addresses NADDR-3, NADDR-2, and NADDR-1 have single, double, and
		triple indirect blocks.  Determine how many levels. */
	sh = 0;
	nb = 1;
	bn -= NADDR - 3;
	for (j = 3; j > 0; j--) {
		sh += FsNSHIFT(fstype);
		nb <<= FsNSHIFT(fstype);
		if (bn < nb)
			break;
		bn -= nb;
	}
	if (j == 0) {
		/* File too big. */
                PRINTF("FILE TOO BIG!\n");
                return(CFAIL);
        }

	/* Fetch through the (CFAIL)indirect blocks. */
	for (nb = ip->i_addr[NADDR - j]; j <= 3; j++) {
		if (nb == 0)
			break;
		switch (j) {
		case 3:

			/* Single indirect. */
			if (Ind1b != nb) {
				Ind1b = nb;
				switch(fstype) {
				case Fs1b:
					ndisk_acs(Fso + nb, IND1);
					break;
				case Fs2b:
					ndisk_acs(Fso + 2*nb, IND1);
					ndisk_acs(Fso + 2*nb + 1, IND1 + BSIZE);
					break;
				case Fs4b:
					ndisk_acs(Fso + 4*nb, IND1);
					ndisk_acs(Fso + 4*nb + 1, IND1 + BSIZE);
					ndisk_acs(Fso + 4*nb + 2, IND1 + 2*BSIZE);
					ndisk_acs(Fso + 4*nb + 3, IND1 + 3*BSIZE);
					break;
				}
			}
			bap = (int *)(IND1);
			break;
		case 2:

			/* Double indirect. */
			if (Ind2b != nb) {
				Ind2b = nb;
				switch(fstype) {
				case Fs1b:
					ndisk_acs(Fso + nb, IND2);
					break;
				case Fs2b:
					ndisk_acs(Fso + 2*nb, IND2);
					ndisk_acs(Fso + 2*nb + 1, IND2 + BSIZE);
					break;
				case Fs4b:
					ndisk_acs(Fso + 4*nb, IND2);
					ndisk_acs(Fso + 4*nb + 1, IND2 + BSIZE);
					ndisk_acs(Fso + 4*nb + 2, IND2 + 2*BSIZE);
					ndisk_acs(Fso + 4*nb + 3, IND2 + 3*BSIZE);
					break;
				}
			}
			bap = (int *)(IND2);
			break;
		case 1:

			/* Triple indirect. */
			if (Ind3b != nb) {
				Ind3b = nb;
				switch(fstype) {
				case Fs1b:
					ndisk_acs(Fso + nb, IND3);
					break;
				case Fs2b:
					ndisk_acs(Fso + 2*nb, IND3);
					ndisk_acs(Fso + 2*nb + 1, IND3 + BSIZE);
					break;
				case Fs4b:
					ndisk_acs(Fso + 4*nb, IND3);
					ndisk_acs(Fso + 4*nb + 1, IND3 + BSIZE);
					ndisk_acs(Fso + 4*nb + 2, IND3 + 2*BSIZE);
					ndisk_acs(Fso + 4*nb + 3, IND3 + 3*BSIZE);
					break;
				}
			}
			bap = (int *)(IND3);
			break;
		}
		sh -= FsNSHIFT(fstype);
		nb = bap[(bn >> sh) & FsNMASK(fstype)];
	}
	return(nb);
}

/*
**	liread - Little iread.
**
**	This routine reads in and unpacks the inode pointed to by ip.  The
**	root file system is assumed.  The inode # is taken from the inode.
**	Other fields are filled in from disk.
**
**	This is a stripped version of iread(os/iget.c).
*/

struct dinode    disk_inode;

void
liread(ip)
register struct inode	*ip;	/* ptr to inode to fill in */
{
	register struct dinode	*dp;	/* ptr to disk version of inode */
	register int		i;	/* loop control */
	register char		*p1,	/* pointers for unpacking address */
				*p2;	/*	fields of inode		  */
	int			b;	/* inode block # */


	/* Get inode block. */
	b = FsITOD(fstype, ip->i_number);
	if (b != Inodeb) {
		Inodeb = b;
		switch(fstype) {
		case Fs1b:
			ndisk_acs(Fso + b, INODE);
			break;
		case Fs2b:
			ndisk_acs(Fso + 2*b, INODE);
			ndisk_acs(Fso + 2*b + 1, INODE + BSIZE);
			break;
		case Fs4b:
			ndisk_acs(Fso + 4*b, INODE);
			ndisk_acs(Fso + 4*b + 1, INODE + BSIZE);
			ndisk_acs(Fso + 4*b + 2, INODE + 2*BSIZE);
			ndisk_acs(Fso + 4*b + 3, INODE + 3*BSIZE);
			break;
		}
	}

	/* Set up ptr to the disk copy of the inode. */
	dp = (struct dinode *)(INODE);
	dp += FsITOO(fstype, ip->i_number);

/*	Save the disk inode for comparison with
	absolute unix mtime */

	p1 = (char *)&disk_inode;
	for (i=0; i < sizeof(struct dinode); i++)
		*(p1+i) = *((char *)dp+i);

	/* Unpack the inode. */
	ip->i_mode = dp->di_mode;
	ip->i_nlink = dp->di_nlink;
	ip->i_size = dp->di_size;
	p1 = (char *)ip->i_addr;
	p2 = (char *)dp->di_addr;
	for (i = 0; i < NADDR; i++) {
		*p1++ = '\0';
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = *p2++;
	}
}

/*
**	rb - Read a block.
**
**	This routine reads the block from the inode pointed to by ip that
**	contains file byte offset faddr into the I/O buffer given by maddr.
*/

unsigned short
rb(maddr, faddr, ip)
paddr_t		maddr;	/* I/O buffer address */
off_t		faddr;	/* file byte offset */
struct inode	*ip;	/* ptr to inode */
{
	register int	bn;	/* file system block # */

	if (bn = lbmap(ip, (int)(faddr >> FsBSHIFT(fstype)))) {
		switch(fstype) {
		case Fs1b:
			ndisk_acs(Fso + bn, maddr);
			break;
		case Fs2b:
			ndisk_acs(Fso + 2*bn, maddr);
			ndisk_acs(Fso + 2*bn + 1, maddr + BSIZE);
			break;
		case Fs4b:
			ndisk_acs(Fso + 4*bn, maddr);
			ndisk_acs(Fso + 4*bn + 1, maddr + BSIZE);
			ndisk_acs(Fso + 4*bn + 2, maddr + 2*BSIZE);
			ndisk_acs(Fso + 4*bn + 3, maddr + 3*BSIZE);
			break;
		}
	}
	else {
		PRINTF("BLOCK NOT MAPPED  offset = %x, buffer addr = %x\n", faddr, maddr);
		return(CFAIL);
	}
	return(CPASS);
}

void
ndisk_acs(nblock,addr)
	int nblock;
	unsigned long addr;
{


	switch (P_CMDQ->b_dev)
	{

	case HARDDISK0:
	case HARDDISK1:

		HD_ACS(P_CMDQ->b_dev - HARDDISK0, nblock, addr, DISKRD);
		break;

	case FLOPDISK:
	
		FD_ACS(nblock, addr, DISKRD, NOCHANGE);
		break;

	case ICD:

		icd_acs(nblock, addr, DISKRD);
		break;

	default:

		IOBLK_ACS(P_CMDQ->b_dev, nblock, addr, DISKRD);
		break;

	}
}

void
wndisk_acs(nblock,addr)
	int nblock;
	unsigned long addr;
	{
	switch (P_CMDQ->b_dev)
	{

	case HARDDISK0:
	case HARDDISK1:

	HD_ACS(P_CMDQ->b_dev - HARDDISK0, nblock, addr, DISKWT);
	break;

	case FLOPDISK:

	FD_ACS(nblock, addr, DISKWT, NOCHANGE);
	break;

	case ICD:

	icd_acs(nblock, addr, DISKWT);
	break;

	default:

	IOBLK_ACS(P_CMDQ->b_dev, nblock, addr, DISKWT);
	break;

	}
}

/*
 * routine to read or write a block from or to in-core disk
 * 	return 1 if successful, 0 if unsucessful.
 *
 *	input: block number on icd,
 *	       address of buffer to be acted upon,
 *	       common IO block read/write opcode.
 */

static	int icdsize = 0;
static	caddr_t icdstart;

icd_acs(block, address, rdwr)
long block;
caddr_t address;
unsigned char rdwr;
{
	caddr_t fraddr, toaddr, icdaddr;
	char *src, *dest;
	int n;

	if (!icdsize) {
		RNVRAM((char *) &(UNX_NVR->icdsize), (char *)&icdsize, 4);
		if (icdsize)
			icdstart = (caddr_t)(MAINSTORE + SIZOFMEM -icdsize * ICDBLKSZ);
		else
			return(FW_FAIL);
	}

	if (block >= icdsize || block < 0 )
		return(FW_FAIL);
	icdaddr = icdstart + (unsigned long)(block * ICDBLKSZ);
	if (rdwr == DISKRD) {
		fraddr = icdaddr;
		toaddr = address;
	} else {
		fraddr = address;
		toaddr = icdaddr;
	}
	for (src = (char *)fraddr, dest = (char *)toaddr, n = 1; n <= ICDBLKSZ; src++, dest++, n++)
		*dest = *src;
	return(FW_PASS);
}

/*
 * read specific number of bytes from the inode pointed to by
 * ip that contains file byte offset into buf
 */
int
eread(ip,offset,buf,numbytes)
struct inode *ip;
off_t offset;
char *buf;
int numbytes;
{
	long nbytes,size,sbyte;
	char *mem,*bp;
	int tc, i;

	/*
	PRINTF("debug: reading %x %x %x %x bytes\n",ip, offset, buf,numbytes);
	*/

	mem= (char *)buf;
	size = numbytes;

	while (size){
		/* If this read doesn't start on a sector boundary we must
		 * read in the entire sector and copy part of it to memory. 
		 */

		if ((offset % SECTSIZE) != 0){
			nbytes = SECTSIZE - (offset % SECTSIZE);
			if (nbytes > size)
				nbytes = size;
			sbyte = offset % SECTSIZE;
			bp = DATA + sbyte;
			if (!rb((paddr_t)&DATA, offset, ip))
				return(CFAIL);
			for (i=nbytes; i--; *mem++ = *bp++);
			size -= nbytes;
			offset += nbytes;

			continue;
		}

		/* 
		 * Misaligned read.  Again we must read the entire sector
		 * and copy part of it. 
		 */

		if (size < SECTSIZE){
			if (!rb((paddr_t)&DATA, offset, ip))
				return(CFAIL);
			tc = FsBSIZE(fstype) - (offset % FsBSIZE(fstype));
			bp = (char *)(DATA + FsBSIZE(fstype) -tc);
			for (; size > 0; *mem++ = *bp++, size--);
			continue;
		}


		/* Now attempt to read in all the whole sectors */

		while (size >= SECTSIZE) {
			if (!rb((paddr_t)mem, offset, ip))
				return(CFAIL);
			size -= SECTSIZE;
			mem += SECTSIZE;
			offset += SECTSIZE;
		}
	}
	return(CPASS);
}
