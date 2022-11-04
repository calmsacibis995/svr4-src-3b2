/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgenIO:g_init.c	1.3"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <sys/sys3b.h>
#include <stdio.h>
#include <string.h>
#include <ftw.h>
#include <libgenIO.h>

extern
char	*malloc();

static
char	*ctcpath;	/* Used to re-open file descriptor (3B2/CTC) */

static
struct stat	orig_st_buf;	/* Stat structure of original file (3B2/CTC) */
	
/*
 * g_init: Determine the device being accessed, set the buffer size,
 * and perform any device specific initialization.  For the 3B2,
 * a device with major number of 17 (0x11) is an internal hard disk,
 * unless the minor number is 128 (0x80) in which case it is an internal
 * floppy disk.  Otherwise, get the system configuration
 * table and check it by comparing slot numbers to major numbers.
 * For the special case of the 3B2 CTC several unusual things must be done.
 * First, the open file descriptor is stat(2)'d and ftw(3) is used to walk
 * through the /dev directory structure looking for the file coorresponding
 * to the open file descriptor.  This is necessary, because to enable
 * streaming mode on the CTC, the file descriptor must be closed, re-opened
 * (with O_RDWR and O_CTSPECIAL flags set), the STREAMON ioctl(2) command
 * issued, and the file descriptor re-re-opened either read-only or write_only.
 */

int
g_init(devtype, fdes)
int *devtype, *fdes;
{
	major_t maj;
	minor_t min;
	int bufsize, nflag, oflag, i, count, size, ident();
	struct s3bconf *buffer;
	struct s3bc *table;
	struct stat st_buf;
	struct statfs stfs_buf;

	*devtype = G_NO_DEV;
	bufsize = -1;
	if (fstat(*fdes, &st_buf) == -1)
		return(-1);
	if (!(st_buf.st_mode & S_IFCHR) && !(st_buf.st_mode & S_IFBLK)) {
		*devtype = G_FILE; /* find block size for this file system */
		if (fstatfs(*fdes, &stfs_buf, sizeof(struct statfs), 0) < 0) {
			bufsize = -1;
			errno = ENODEV;
		} else
			bufsize = stfs_buf.f_bsize;
		return(bufsize);

	/* We'll have to add a remote attribute to stat but this 
	** should work for now.
	*/
	} else if (st_buf.st_dev & 0x8000)	/* if remote  rdev */
		return (512);

	maj = major(st_buf.st_rdev);
	min = minor(st_buf.st_rdev);
	if (maj == 0x11) { /* internal hard or floppy disk */
		if (min & 0x80)
			*devtype = G_3B2_FD; /* internal floppy disk */
		else
			*devtype = G_3B2_HD; /* internal hard disk */
	} else {
		if (sys3b(S3BCONF, (struct s3bconf *)&count, sizeof(count)) == -1)
			return(-1);
		size = sizeof(int) + (count * sizeof(struct s3bconf));
		buffer = (struct s3bconf *)malloc((unsigned)size);
		if (sys3b(S3BCONF, buffer, size) == -1)
			return(-1);
		table = (struct s3bc *)((char *)buffer + sizeof(int));
		for (i = 0; i < count; i++) {
			if (maj == (int)table->board) {
				if (!strncmp(table->name, "CTC", 3)) {
					*devtype = G_3B2_CTC;
					break;
				} else if (!strncmp(table->name, "TAPE", 4)) {
					*devtype = G_TAPE;
					break;
				}
				/* other possible devices can go here */
			}
			table++;
		}
	}
	switch (*devtype) {
		case G_3B2_CTC:	/* do special CTC initialization */
			bufsize = 15872;
			if (fstat(*fdes, &orig_st_buf) < 0) {
				bufsize = -1;
				break;
			}
			if (ftw("/dev", ident, 6) <= 0) {
				bufsize = -1;
				break;
			}
			oflag = fcntl(*fdes, F_GETFL, 0);
			nflag = (O_RDWR | O_CTSPECIAL);
			(void)close(*fdes);
			if ((*fdes = open(ctcpath, nflag, 0666)) != -1)
				if (ioctl(*fdes, STREAMON) != -1) {
					(void)close(*fdes);
					nflag = (oflag == O_WRONLY) ? O_WRONLY : O_RDONLY;
					if ((*fdes = open(ctcpath, nflag, 0666)) == -1)
						bufsize = -1;
				}
			else
				bufsize = -1;
			break;
		case G_NO_DEV:
		case G_3B2_HD:
		case G_3B2_FD:
		case G_TAPE:
			bufsize = 512;
			break;
		case G_SCSI_HD: /* not developed yet */
		case G_SCSI_FD:
		case G_SCSI_9T:
		case G_SCSI_Q24:
		case G_SCSI_Q120:
		case G_386_HD:
		case G_386_FD:
		case G_386_Q24:
			bufsize = 512;
			break;
		default:
			bufsize = -1;
			errno = ENODEV;
	} /* *devtype */
	return(bufsize);
}

/*
 * ident: Used to determine if the pathname and stat(2) structure
 * passwd by ftw(3C) coorresponds to the device that was specified
 * as the standard input or standard output of the parent command.
 * This is necessary for streaming on the 3B2 CTC, as the device
 * must be closed and then re-opened by name to permit streaming.
 */

static
int
ident(path, new_stat, ftype)
char *path;
struct stat *new_stat;
int ftype;
{

	if (ftype != FTW_F || new_stat->st_mode != orig_st_buf.st_mode
		|| new_stat->st_rdev != orig_st_buf.st_rdev
		|| new_stat->st_uid != orig_st_buf.st_uid
		|| new_stat->st_gid != orig_st_buf.st_gid)
		return(0);
	if ((ctcpath = malloc((unsigned)strlen(path) + 1)) == (char *)NULL)
		return(-1);
	(void)strcpy(ctcpath, path);
	return(1);
}
