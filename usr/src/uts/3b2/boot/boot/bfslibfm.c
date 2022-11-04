/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)boot:boot/boot/bfslibfm.c	1.6"
#include	"sys/types.h"
#include	"sys/psw.h"
#include	"sys/elog.h"
#include	"sys/iobuf.h"
#include	"sys/boot.h"
#include	"sys/firmware.h"
#include	"sys/id.h"
#include	"sys/sbd.h"
#include 	"sys/csr.h"
#include 	"sys/immu.h"
#include	"sys/nvram.h"
#include	"sys/vtoc.h"
#include	"sys/edt.h"
#include	"sys/extbus.h"
#include	"sys/sys3b.h"
#include	"sys/dma.h"
#include	"sys/vnode.h"
#include	"sys/vfs.h"
#include	"sys/fs/bfs.h"
#include	"sys/fsiboot.h"
#include 	"sys/libfm.h"
#include 	"sys/elftypes.h"
#include	"sys/elf.h"

char data[SECTSIZE];
struct nfso nfso[V_NUMPAR]={0,0};


#define RND512(x)	((x) & ~(512 -1))
#define EXC512(x)	((x) & (512 -1))

#define  M32MAGIC	0560

struct filehdr {
	ushort	f_magic;	
	ushort	f_nscns;	/* number of sections */
	long	f_timdat;	/* time & date stamp */
	long	f_symptr;	/* file pointer to symtab */
	long	f_nsyms;	/* number of symtab entries */
	ushort	f_opthdr;	/* sizeof(optional hdr) */
	ushort	f_flags;
};

/*
 *  Common object file section header.
 */

/*
 * s_flags
 */
#define	STYP_TEXT	0x0020	/* section contains text only */
#define STYP_DATA	0x0040	/* section contains data only */
#define STYP_BSS	0x0080	/* section contains bss only  */
#define STYP_LIB	0x0800	/* section contains lib only  */

struct scnhdr {
	char	s_name[8];	/* section name */
	long	s_paddr;	/* physical address */
	long	s_vaddr;	/* virtual address */
	long	s_size;		/* section size */
	long	s_scnptr;	/* file ptr to raw	*/
				/* data for section	*/
	long	s_relptr;	/* file ptr to relocation */
	long	s_lnnoptr;	/* file ptr to line numbers */
	ushort	s_nreloc;	/* number of relocation	*/
				/* entries		*/
	ushort	s_nlnno;	/* number of line	*/
				/* number entries	*/
	long	s_flags;	/* flags */
};

Elf32_Ehdr elfhdr;
Elf32_Shdr eshdr;
struct filehdr coffhdr;
short headmagic;


/* The following globals are required by LBOOT's autoconfiguration code */

int xedtsect;
struct s3bconf sys3bconfig[120];
char xdev = 0;
static void confirm();
static void xconfirm();
static void edtscan();
static void read();

/* Load a BFS file to memory given the start block number and the file header */

 int
load(fd, bhdr, check_config)
off_t fd;
struct boothdr *bhdr;
char check_config;
{
	off_t offset;
	int bootstartaddr;
	register size;
	struct bootsect bsect;
	int s,nsect;
	char *mem;

 	bootstartaddr = 0x7fffffff;

	/* Loop through the sections loading the loadable ones. */

	for (nsect = 0; nsect < bhdr->nsect; nsect++){

		getsect(fd,&bsect,nsect);

		if ((bsect.type == CONFIG) && check_config){
			mem = (char *)sys3bconfig;
			size = bsect.size;
			offset = bsect.offset;
		} else if ((bsect.type == LOAD) && (bsect.size != 0)){
			size = bsect.size;
			mem = (char *)bsect.addr;
			offset = bsect.offset;
		} else
			continue;

		/* The start address of the program is the lowest section
		   load address within mainstore. */

		if (((int)mem < bootstartaddr) && ((int)mem > MAINSTORE))
			bootstartaddr = (int)mem;

		read(fd,offset,mem,size);

		if (bsect.type & CONFIG){
			s = do_edt();
			if (s)
				return (0);
		}
	}

	return (bootstartaddr);
}


get_fs()
{

	off_t Fso=0;
	unsigned long logstrt;
	register int i;

	switch(P_CMDQ->b_dev){
	case FLOPDISK:
		Fso = FFSO;
		break;

	case HARDDISK0:
	case HARDDISK1:
		{
		logstrt = PHYS_INFO[P_CMDQ->b_dev - HARDDISK0].logicalst;
		break;
		}
	default:
		logstrt = MYPDINFO->logicalst;
		break;
	}

	if (P_CMDQ->b_dev != FLOPDISK){
		for (i=0; i < V_NUMPAR; i++)
			if (MYVTOC->v_part[i].p_tag == V_STAND){
				if (Fso == 0){
					Fso = MYVTOC->v_part[i].p_start + logstrt;
					hd_mapinit(P_CMDQ->b_dev - HARDDISK0, Fso, MYVTOC->v_part[i].p_size);
				}
				nfso[i].start = MYVTOC->v_part[i].p_start + logstrt;
				nfso[i].size = MYVTOC->v_part[i].p_size;
			}


		xedtsect = MYVTOC->v_part[7].p_start + MYVTOC->v_part[7].p_size
					 - XEDTSIZE -1;
		xedtsect += logstrt;
	}
	return (Fso);
}

/* ALL The following code is required for AUTO-CONFIGURATION, and is
			 taken from LBOOT */

do_edt()
{
	register int i;
	struct s3bc *s3bc;

	edtscan(EDT_START, 0, confirm);
	if (xdev)
		xconfirm();

	for (i=0, s3bc=sys3bconfig->driver; i<sys3bconfig->count;++i, ++s3bc){
		char buffer[40];


		if (!strcmp(s3bc->name, "SBD"))
		{
			if (s3bc->timestamp > SIZOFMEM)
			{
				PRINTF("%s configured for too much memory.\n",
					P_CMDQ->b_name);
				restart();
			}

			if (s3bc->timestamp < SIZOFMEM)
			{
				PRINTF("%s configured for too little memory.\n",
					P_CMDQ->b_name);
				restart();
			}
		}

		if (s3bc->flag & S3BC_IGN || s3bc->board == 0)
		/* device is ignored, it exists now, or this is a software driver */
			continue;

		strncat(strcpy(buffer,""), s3bc->name, sizeof(s3bc->name));

		if (s3bc->board & 0xF0 && !(s3bc->flag & S3BC_TCDRV))
		{
			PRINTF("Device %s previously configured on LBE (board code %d",
				buffer, (unsigned int)(s3bc->board&0xf0) >> 4);
			PRINTF(") at ELB board code %d\n", s3bc->board&0x0f);
			PRINTF("A new unix will be re-configured\n");
			return (1);
		}

		else
		{
			PRINTF("Device %s previously configured at board code %d\n",
					buffer, s3bc->board);
			PRINTF("A new unix will be re-configured\n");
			return (1);
		}

	}

	return (0);
}

 static void
edtscan(base, lba, function)
register struct edt *base;
register int lba;
register int (*function)();
{
	register struct edt *edtp;
	register i;
	int elb;

	for (i=0, edtp=base; i < NUM_EDT; ++i)
	{
		elb = edtp->opt_slot;

		(*function)(edtp++, lba, elb);
	}
}

 static
 void
confirm(edtp, lba, elb)
register struct edt *edtp;
int lba;
int elb;
{
	register struct s3bc *s3bc;
	register i;

	/*
	 * find the matching device for this EDT entry
	 */
	for (i=0; i<sys3bconfig->count; ++i)
	{
		s3bc = &sys3bconfig->driver[i];

		if (0 != STRCMP(edtp->dev_name,s3bc->name))
			continue;

		if (s3bc->flag & S3BC_IGN)
			/* ignored before, therefore ignore it now */
			return;

		if (s3bc->board == ((lba<<4) | elb))
			/* found it, therefore mark it found */
			{
			s3bc->board = 0;
			PRINTF("found board %s\n",s3bc->name);
			if(edtp->indir_dev)
				/* load bus gen program for configured
				 * HA device.
				 */
				xdev++;
			return;
			}
		}

}

/*
 * xconfirm()
 *
 * Confirm that TC devices that exist now were configured at the time the
 * sys3bconfig structure was created
 */
static
void
xconfirm()
{

	register struct s3bc *s3bc;
	char *mem,*xbuf;
	long sect,size,max_bytes,count;
	register i,j,k,l;
	B_TC *xtcp;
	B_EDT *xedtp;
	long *sanity,*xhead;



	/*
         * We know we always transfer whole sectors. No need to check for
	 * misaligned read.
	 */

	mem = (char *)0x02188000; /* This should be a sector aligned address */
			          /* Enough space left for 16K of bss        */
	xbuf = mem;
	sect = xedtsect;
	size = XEDTSIZE * SECTSIZE;

	while (size){
		max_bytes = size;
		if ((((int)mem & MSK64K) + size) > MSK64K)
			max_bytes = MSK64K - ((int)mem & MSK64K);
		count= big_read(sect, mem, max_bytes/SECTSIZE);
		size -= count;
		sect += count/SECTSIZE;
		mem += count;
	}

	sanity = (long *)xbuf;
	xhead = (long *)xbuf + 1;
 	if (*sanity != XEDTMAGIC) /* no extended EDT on disk */
		return;

	for (j=0; j< 15; j++){
		l=k=0;
		if (xhead[j] == (long)0)
			return;
		else	{
		
			xedtp = (B_EDT *)(xbuf + xhead[j]);

			xtcp = xedtp->tc;

			while (k < (int)xedtp->n_tcs && l < (int)xedtp->max_tc){
			   l++;
			   if (xtcp->equip_stat){
				k++;
                		/*
                 		* find the matching device for this XEDT entry
                 		*/
                		for (i=0; i<sys3bconfig->count; ++i){
					s3bc = &sys3bconfig->driver[i];
					if (0 != strcmp(xtcp->name,s3bc->name)
					     || s3bc->flag & S3BC_IGN)
						continue;
		
					if (s3bc->board == xtcp->maj){
                                	/* found it, therefore mark it found */
		
						PRINTF("found board %s\n",s3bc->name);
                                		s3bc->board = 0;
                                		break;
					}
				}
			   }
			xtcp = (B_TC *)((caddr_t)xtcp + xedtp->tc_size);
			}
		}
	}
}


 off_t
nextfso(fso)
off_t fso;
{
	int i,j,k;

	for(i=0; i< V_NUMPAR && nfso[i].start != fso; i++);

	if (i == V_NUMPAR)
		return(fso);
	for (j=i,k=0;k < V_NUMPAR ;k++){
		j = ++j % V_NUMPAR;
		if (nfso[j].start != 0){
			PRINTF("Switching to partition %d\n",j);
			hd_mapinit(P_CMDQ->b_dev - HARDDISK0, nfso[j].start, nfso[j].size);
			return(nfso[j].start);
		}
	}
}



static void
read(fd,foffset,buf,numbytes)
off_t fd;
off_t foffset;
char *buf;
int numbytes;
{


	int i;
	long nbytes,size,max_bytes,sbyte,count;
	off_t offset;
	char *mem,*bp;


	mem= buf;
	size = numbytes;

	/*
	 * for bfs a fd is the offset of the file
 	 */
	offset = fd + foffset;

	while (size){
		/* If this read doesn't start on a sector boundary we must

		 * read in the entire sector and copy part of it to memory. 
		 */

		if ((offset % SECTSIZE) != 0){
			nbytes = SECTSIZE - (offset % SECTSIZE);
			max_bytes = min(nbytes, size);
			nbytes = max_bytes;
			sbyte = offset % SECTSIZE;
			bp = data + sbyte;
			read_onesect(offset / SECTSIZE, data);
			for (i=nbytes; i--; *mem++ = *bp++);
			size -= nbytes;
			offset += nbytes;

			continue;
		}

		max_bytes = size;

		/* 
		 * This read starts on a sector boundary.  We will attempt
		 * to have the controller do an entire read at once.  We 
		 *  cannot allow the controller to attempt to read past a
		 *  64K address boundary as the DMA cannot handle it. 
		 */

		if ((((int)mem & MSK64K) + size) > MSK64K)
			max_bytes = MSK64K - ((int)mem & MSK64K);

		/* 
		 * Misaligned read.  Again we must read the entire sector
		 * and copy part of it. 
		 */

		if (max_bytes < SECTSIZE){
			read_onesect(offset / SECTSIZE, data);
			bp = data;
			for (i=max_bytes; i-- ; *mem++ = *bp++);
			size -= max_bytes;
			offset += max_bytes;
			continue;
		}


		/* Now attempt to read multiple sectors at once */

		count = big_read(offset/SECTSIZE, mem, max_bytes/SECTSIZE);
		size -= count;
		offset += count;
		mem += count;
	}

}

/*
 * open a file and return a fd. In BFS a fd is the byte offset of the file
 * on the disk.
 */
off_t
open(fso, fname, battrp)
off_t fso;
char *fname;
struct bootattr *battrp;
{
	char onekbuf[SECTSIZE *2];
	struct bdsuper bd;
	struct bfs_dirent *dir;
	struct bfs_ldirs *ld;
	off_t byte_off;
	off_t sect_off;
	off_t buff_off;
	off_t offset;
	off_t endofdir;
	off_t dirlastblk;
	register int i;
	register int j;
	register int k = 0;
	register found = 0;

	/*
	 * find out offset of the ROOT inode
	 */
	byte_off = BFS_INO2OFF(BFSROOTINO) + (fso * SECTSIZE);

	/*
	 * find the block offset. Must round down.
	 */
	sect_off = (RND512(byte_off)) / SECTSIZE;

	read_onesect(sect_off, onekbuf);

	/*
	 * Find out the offset into the buffer just read.
	 */
	buff_off = EXC512(byte_off);

	/*
	 * If all the ROOT inode is not in the first block
	 * we read, then read one more block.
	 */
	if ((SECTSIZE - buff_off) < sizeof(struct bfs_dirent))
		read_onesect(sect_off +1, onekbuf + SECTSIZE);

	dir = (struct bfs_dirent *) (onekbuf + buff_off);

	endofdir = dir->d_eoffset + (fso * SECTSIZE);
	dirlastblk = endofdir / SECTSIZE;
	offset = dir->d_sblock + fso;
	byte_off = offset * SECTSIZE;

	/*
	 * Search for the file name in the directory. If found, determine
	 * the offset on disk of where the inode of file is stored.
	 * Upon success, return the offset of the file in bytes.
	 * If failure, return -1
	 */
	for (i = offset; i < dirlastblk + 1 && found == 0; i += 2) {
		read_onesect(i, onekbuf);
		read_onesect(i + 1, onekbuf + SECTSIZE);

		for (j=0; j < sizeof(onekbuf) && byte_off < endofdir;
		     j += sizeof(struct bfs_ldirs),
		     byte_off += sizeof(struct bfs_ldirs) )  {

			ld = (struct bfs_ldirs *) (onekbuf + j);
			if (ld->l_ino != 0 && STRCMP(ld->l_name, fname) == 0) {
				PRINTF("Found file  %s\n", ld->l_name);
				found ++;
				break;
			}
		}
	}
	if (!found)
		return (-1);

	/*
	 * The file was found in the directory.
	 * Find out offset of the file inode
	 */
	byte_off = BFS_INO2OFF(ld->l_ino) + (fso * SECTSIZE);

	/*
	 * find the block offset. Must round down.
	 */
	sect_off = (RND512(byte_off)) / SECTSIZE;

	read_onesect(sect_off, onekbuf);

	/*
	 * Find out the offset into the buffer just read.
	 */
	buff_off = EXC512(byte_off);

	/*
	 * If all the file inode is not in the first block
	 * we read, then read one more block.
	 */
	if ((SECTSIZE - buff_off) < sizeof(struct bfs_dirent))
		read_onesect(sect_off +1, onekbuf + SECTSIZE);

	dir = (struct bfs_dirent *) (onekbuf + buff_off);
	battrp->size = dir->d_eoffset - dir->d_sblock +1;
	battrp->mtime = dir->d_fattr.va_mtime;
	battrp->ctime = dir->d_fattr.va_ctime;
	battrp->blksize = dir->d_fattr.va_blksize;
	return((dir->d_sblock + fso) * SECTSIZE);
}

/*
 * Function to list all of the files in /stand.  Reads two sectors at a time.
 */
void
list_dir(fso)
off_t fso;
{
	char onekbuf[SECTSIZE *2];
	struct bdsuper bd;
	struct bfs_dirent *dir;
	struct bfs_ldirs *ld;
	off_t byte_off;
	off_t sect_off;
	off_t buff_off;
	off_t offset;
	off_t endofdir;
	off_t dirlastblk;
	register int i;
	register int j;
	register int k = 0;

	/*
	 * find out offset of the ROOT inode
	 */
	byte_off = BFS_INO2OFF(BFSROOTINO) + (fso * SECTSIZE);

	/*
	 * find the block offset. Must round down.
	 */
	sect_off = (RND512(byte_off)) / SECTSIZE;

	read_onesect(sect_off, onekbuf);

	/*
	 * Find out the offset into the buffer just read.
	 */
	buff_off = EXC512(byte_off);

	/*
	 * If the all the ROOT inode is not in the first block
	 * we read, read one more block.
	 */
	if ((SECTSIZE - buff_off) < sizeof(struct bfs_dirent))
		read_onesect(sect_off +1, onekbuf + SECTSIZE);

	dir = (struct bfs_dirent *) (onekbuf + buff_off);

	endofdir = dir->d_eoffset + (fso * SECTSIZE);
	dirlastblk = endofdir / SECTSIZE;
	offset = dir->d_sblock + fso;
	byte_off = offset * SECTSIZE;

	PRINTF("\nThe files in /stand are:\n");

	for (i = offset; i < dirlastblk + 1; i += 2) {
		read_onesect(i, onekbuf);
		read_onesect(i + 1, onekbuf + SECTSIZE);

		for (j=0; j < sizeof(onekbuf) && byte_off < endofdir;
		     j += sizeof(struct bfs_ldirs),
		     byte_off += sizeof(struct bfs_ldirs) )  {

			ld = (struct bfs_ldirs *) (onekbuf + j);
			if (ld->l_ino == 0)
				continue;
			/*
			 * Print the filenames, six per line.
			 */
			PRINTF("%s     ", ld->l_name);
			if (++k == 6) {
				k = 0;
				PRINTF("\n");
			}
		}
	}
	PRINTF ("\n");
}

gethead(fd, bhdr)
off_t fd;
struct boothdr *bhdr;
{
	register off_t offset, stroff;
	register unsigned short nscns;
	char edtstr[8];

	read(fd, 0L, (char *)&headmagic, sizeof(headmagic));

	switch (headmagic){
	case M32MAGIC:
		read(fd, 0L, (char *)&coffhdr, sizeof(coffhdr));
		bhdr->type = COFF;
		bhdr->nsect = coffhdr.f_nscns;
		return(0);
	case 0x7f45:
		read(fd, 0L, (char *)&elfhdr, sizeof(elfhdr));
		bhdr->type = ELF;
		bhdr->nsect = elfhdr.e_phnum;

		/*	find EDT section if it exists */

		/* Get string table for section table, first */

		offset = elfhdr.e_shentsize * elfhdr.e_shstrndx + elfhdr.e_shoff;

		read(fd, offset, (char *)&eshdr, elfhdr.e_shentsize);

		stroff = eshdr.sh_offset;	/* offset of str tab */

		/* Search for EDT section header */

		offset = elfhdr.e_shoff;

		for (nscns = 1; nscns < elfhdr.e_shnum; nscns++) {

			offset += elfhdr.e_shentsize;

			read(fd, offset, (char *)&eshdr, elfhdr.e_shentsize);

			read(fd, (stroff + eshdr.sh_name), edtstr, 6);

			if (strcmp(edtstr, "<EDT>") == 0) {
				bhdr->nsect += 1;
				break;
			}
		}

		return(0);

	default: 
		bhdr->type = NONE;
		bhdr->nsect = 0;
		return(0);
	}
}
		
	

getsect(fd, bsect, sectnum)
off_t fd;
struct bootsect *bsect;
int sectnum;
{

	struct scnhdr coffsect;
	Elf32_Phdr elfphdr;


	switch(headmagic){

	case M32MAGIC:

		if (coffhdr.f_magic != M32MAGIC)
			return(1);
		read(fd, (off_t)(sizeof(coffhdr) + coffhdr.f_opthdr +
			(sizeof(struct scnhdr) * sectnum)), (char *)&coffsect,
			sizeof(coffsect));
		
		if (coffsect.s_flags & (STYP_TEXT | STYP_DATA))
			bsect->type = LOAD;
		else if (STRCMP(coffsect.s_name,"<EDT>") == 0)
			bsect->type = CONFIG;
		else
			bsect->type = NOLOAD;

		bsect->addr = coffsect.s_paddr;
		bsect->size = coffsect.s_size;
		bsect->offset = coffsect.s_scnptr;
		return(0);

	case 0x7f45:

		if (elfhdr.e_ident[EI_MAG0] != 0x7f && elfhdr.e_ident[EI_MAG1] != 'E')
				return(1);

		if (sectnum == elfhdr.e_phnum) {
			bsect->addr = 0;
			bsect->size = eshdr.sh_size;
			bsect->offset = eshdr.sh_offset;
			bsect->type = CONFIG;
		} else {
			read(fd,elfhdr.e_phoff + (elfhdr.e_phentsize * sectnum),
				(char *)&elfphdr, elfhdr.e_phentsize);

			if (elfphdr.p_type == PT_LOAD)
				bsect->type = LOAD;
			else
				bsect->type =NOLOAD;
		
			bsect->addr = elfphdr.p_paddr;
			bsect->size = elfphdr.p_filesz;
			bsect->offset = elfphdr.p_offset;
		}

		return(0);

	default: 
		return(1);

	}
}
