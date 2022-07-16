/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/boot/sys/libfm.h	1.2"



struct bootattr {
	ulong size;		/* size of file */
	time_t mtime;		/* modification time */
	time_t ctime;		/* creation time */
	ulong blksize;		/* blksize of FS */
};

enum lfsect { LOAD, NOLOAD, CONFIG };
typedef enum lfsect lfsect_t;

struct bootsect {
	lfsect_t type;		/* type of section */
	long addr;		/* address to load section */
	long size;		/* size of section */
	long offset;		/* offset in file */

};

enum lfhdr { COFF, ELF, NONE};
typedef enum lfhdr lfhdr_t;

struct boothdr {
	lfhdr_t type;		/* type of file */
	int nsect;		/* number of sections */
};


struct nfso {
	long start;
	long size;
};


