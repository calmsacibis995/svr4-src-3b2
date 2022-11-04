/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getutx.c	1.1"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*	Routines to read and write the /etc/utmpx file.			*/

#ifdef __STDC__
	#pragma weak getutxent = _getutxent
	#pragma weak getutxid = _getutxid
	#pragma weak getutxline = _getutxline
	#pragma weak pututxline = _pututxline
	#pragma weak setutxent = _setutxent
	#pragma weak endutxent = _endutxent
	#pragma weak utmpxname = _utmpxname
	#pragma weak updutmp = _updutmp
	#pragma weak updwtmpx = _updwtmpx
#endif

#include	"synonyms.h"
#include	<stdio.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<utmpx.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<string.h>

#define	MAXFILE	79	/* Maximum pathname length for "utmpx" file */

#ifdef	DEBUG
#undef	UTMPX_FILE
#define	UTMPX_FILE "utmpx"
#undef	UTMP_FILE
#define	UTMP_FILE "utmp"
#endif

extern void	_setutxent();

static int fd = -1;	/* File descriptor for the utmpx file. */
static char utmpxfile[MAXFILE+1] = UTMPX_FILE;	/* Name of the current */
static char utmpfile[MAXFILE+1] = UTMP_FILE;	/* "utmpx" and "utmp"  */
						/* like file.          */

#ifdef ERRDEBUG
static long loc_utmp;	/* Where in "utmpx" the current "ubuf" was found.*/
#endif

static struct utmpx ubuf;	/* Copy of last entry read in. */


/* "getutxent" gets the next entry in the utmpx file.
 */

struct utmpx *getutxent()
{
	extern int fd;
	extern char utmpxfile[];
	extern struct utmpx ubuf;
	register char *u;
	register int i;

/* If the "utmpx" file is not open, attempt to open it for
 * reading.  If there is no file, attempt to create one.  If
 * both attempts fail, return NULL.  If the file exists, but
 * isn't readable and writeable, do not attempt to create.
 */

	if (fd < 0) {
		/* Make sure files are in synch */
		if (synchutmp(utmpfile, utmpxfile)) {
			return(NULL);
		}

		if ((fd = open(utmpxfile, O_RDWR|O_CREAT, 0644)) < 0) {

/* If the open failed for permissions, try opening it only for
 * reading.  All "pututxline()" later will fail the writes.
 */
		if ((fd = open(utmpxfile, O_RDONLY)) < 0)
				return(NULL);
		}
	}
/* Try to read in the next entry from the utmpx file.  */
	if (read(fd,&ubuf,sizeof(ubuf)) != sizeof(ubuf)) {

/* Make sure ubuf is zeroed. */
		for (i=0,u=(char *)(&ubuf); i<sizeof(ubuf); i++) *u++ = '\0';
		return(NULL);
	}

/* Save the location in the file where this entry was found. */
	(void) lseek(fd,0L,1);
	return(&ubuf);
}

/*	"getutxid" finds the specified entry in the utmpx file.  If	*/
/*	it can't find it, it returns NULL.				*/

struct utmpx *getutxid(entry)
const struct utmpx *entry;
{
	extern struct utmpx ubuf;
	register short type;

/* Start looking for entry.  Look in our current buffer before */
/* reading in new entries. */
	do {

/* If there is no entry in "ubuf", skip to the read. */
		if (ubuf.ut_type != EMPTY) {
			switch(entry->ut_type) {

/* Do not look for an entry if the user sent us an EMPTY entry. */
			case EMPTY:
				return(NULL);

/* For RUN_LVL, BOOT_TIME, OLD_TIME, and NEW_TIME entries, only */
/* the types have to match.  If they do, return the address of */
/* internal buffer. */
			case RUN_LVL:
			case BOOT_TIME:
			case OLD_TIME:
			case NEW_TIME:
				if (entry->ut_type == ubuf.ut_type) return(&ubuf);
				break;

/* For INIT_PROCESS, LOGIN_PROCESS, USER_PROCESS, and DEAD_PROCESS */
/* the type of the entry in "ubuf", must be one of the above and */
/* id's must match. */
			case INIT_PROCESS:
			case LOGIN_PROCESS:
			case USER_PROCESS:
			case DEAD_PROCESS:
				if (((type = ubuf.ut_type) == INIT_PROCESS
					|| type == LOGIN_PROCESS
					|| type == USER_PROCESS
					|| type == DEAD_PROCESS)
				    && ubuf.ut_id[0] == entry->ut_id[0]
				    && ubuf.ut_id[1] == entry->ut_id[1]
				    && ubuf.ut_id[2] == entry->ut_id[2]
				    && ubuf.ut_id[3] == entry->ut_id[3])
					return(&ubuf);
				break;

/* Do not search for illegal types of entry. */
			default:
				return(NULL);
			}
		}
	} while (getutxent() != NULL);

/* Return NULL since the proper entry wasn't found. */
	return(NULL);
}

/* "getutxline" searches the "utmpx" file for a LOGIN_PROCESS or
 * USER_PROCESS with the same "line" as the specified "entry".
 */

struct utmpx *getutxline(entry)
const struct utmpx *entry;
{
	extern struct utmpx ubuf;
	register struct utmpx *cur;

/* Start by using the entry currently incore.  This prevents */
/* doing reads that aren't necessary. */
	cur = &ubuf;
	do {
/* If the current entry is the one we are interested in, return */
/* a pointer to it. */
		if (cur->ut_type != EMPTY && (cur->ut_type == LOGIN_PROCESS
		    || cur->ut_type == USER_PROCESS) && strncmp(&entry->ut_line[0],
		    &cur->ut_line[0],sizeof(cur->ut_line)) == 0) return(cur);
	} while ((cur = getutxent()) != NULL);

/* Since entry wasn't found, return NULL. */
	return(NULL);
}

/*	"pututxline" writes the structure sent into the utmpx file.	*/
/*	If there is already an entry with the same id, then it is	*/
/*	overwritten, otherwise a new entry is made at the end of the	*/
/*	utmpx file.							*/

struct utmpx *pututxline(entry)
const struct utmpx *entry;
{
	int fc;
	struct utmpx *answer;
	extern struct utmpx ubuf;
	extern int fd;
	struct utmpx tmpxbuf, savbuf;

/* Copy the user supplied entry into our temporary buffer to */
/* avoid the possibility that the user is actually passing us */
/* the address of "ubuf". */
	tmpxbuf = *entry;
	(void)getutxent();
	if (fd < 0) {
#ifdef	ERRDEBUG
		gxdebug("pututxline: Unable to create utmpx file.\n");
#endif
		return((struct utmpx *)NULL);
	}

/* Make sure utmpx file is writable */
	if ((fc=fcntl(fd, F_GETFL, NULL)) == -1
	    || (fc & O_RDWR) != O_RDWR) {
		return((struct utmpx *)NULL);
	}


/* Find the proper entry in the utmpx file.  Start at the current */
/* location.  If it isn't found from here to the end of the */
/* file, then reset to the beginning of the file and try again. */
/* If it still isn't found, then write a new entry at the end of */
/* the file.  (Making sure the location is an integral number of */
/* utmp structures into the file incase the file is scribbled.) */

	if (getutxid(&tmpxbuf) == NULL) {
#ifdef	ERRDEBUG
		gxdebug("First getutxid() failed.  fd: %d",fd);
#endif
		setutxent();
		if (getutxid(&tmpxbuf) == NULL) {
#ifdef	ERRDEBUG
			loc_utmp = lseek(fd, 0L, 1);
			gxdebug("Second getutxid() failed.  fd: %d loc_utmp: %ld\n",fd,loc_utmp);
#endif
			fcntl(fd, F_SETFL, fc | O_APPEND);
		} else {
			lseek(fd, -(long)sizeof(struct utmpx), 1);
		}
	} else {
		lseek(fd, -(long)sizeof(struct utmpx), 1);
	}


/* Write out the user supplied structure.  If the write fails, */
/* then the user probably doesn't have permission to write the */
/* utmpx file. */
	if (write(fd,&tmpxbuf,sizeof(tmpxbuf)) != sizeof(tmpxbuf)) {
#ifdef	ERRDEBUG
		gxdebug("pututxline failed: write-%d\n",errno);
#endif
		answer = (struct utmpx *)NULL;
	} else {
/* Save the user structure that was overwritten. Copy the new user  */
/* structure into ubuf so that it will be up to date in the future. */
		savbuf = ubuf;
		ubuf = tmpxbuf;
		answer = &ubuf;

#ifdef	ERRDEBUG
		gxdebug("id: %c%c loc: %x\n",ubuf.ut_id[0],ubuf.ut_id[1],
		    ubuf.ut_id[2],ubuf.ut_id[3],loc_utmp);
#endif
	}
	if (updutmp(entry)) {
		lseek(fd, -(long)sizeof(struct utmpx), 1);
		write(fd, &savbuf, sizeof(savbuf));
		answer = (struct utmpx *)NULL;
	}

	fcntl(fd, F_SETFL, fc);
	return(answer);
}

/*	"setutxent" just resets the utmpx file back to the beginning.	*/

void
setutxent()
{
	register char *ptr;
	register int i;
	extern int fd;
	extern struct utmpx ubuf;

	if (fd != -1) lseek(fd,0L,0);

/* Zero the stored copy of the last entry read, since we are */
/* resetting to the beginning of the file. */

	for (i=0,ptr=(char*)&ubuf; i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*	"endutxent" closes the utmpx file.				*/

void
endutxent()
{
	extern int fd;
	extern struct utmpx ubuf;
	register char *ptr;
	register int i;

	if (fd != -1) close(fd);
	fd = -1;
	for (i=0,ptr= (char *)(&ubuf); i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*	"utmpxname" allows the user to read a file other than the	*/
/*	normal "utmpx" file.						*/

utmpxname(newfile)
const char *newfile;
{
	extern char utmpxfile[];
	extern char utmpfile[];
	int len;

/* Determine if the new filename will fit.  If not, return 0. */
	if ((len = strlen(newfile)) > MAXFILE-1) return (0);
/* The name of the utmpx file has to end with 'x' */
	if (newfile[len-1] != 'x') return(0);

/* Otherwise copy in the new file name. */
	else {
		(void)strcpy(&utmpxfile[0],newfile);
		(void)strcpy(&utmpfile[0],newfile);
		/* strip the 'x' */
		utmpfile[len-1] = '\0';
	}
/* Make sure everything is reset to the beginning state. */
	endutxent();
	return(1);
}

/* "updutmp" updates the utmp file, uses same algorithm as 
 * pututxline so that the records end up in the same spot.
 */
int updutmp(entry)
struct utmpx *entry;
{
	int fd_u, fc, type;
	struct stat stbuf;
	struct utmp ubuf, *uptr = NULL;

	if ((fd_u = open(utmpfile, O_RDWR|O_CREAT, 0644)) < 0) {
#ifdef ERRDEBUG
		gxdebug("Could not open utmpfile\n");
#endif
		return(1);
	}

	if ((fc = fcntl(fd_u, F_GETFL, NULL)) == -1)
		return(1);

	while (read(fd_u, &ubuf, sizeof(ubuf)) == sizeof(ubuf)) {
		if (ubuf.ut_type != EMPTY) {
			switch (entry->ut_type) {
				case EMPTY:
				    goto done;	
				case RUN_LVL:
				case BOOT_TIME:
				case OLD_TIME:
				case NEW_TIME:
				    if (entry->ut_type == ubuf.ut_type) {
					uptr = &ubuf;
				        goto done;
				    }
				case INIT_PROCESS:
				case LOGIN_PROCESS:
				case USER_PROCESS:
				case DEAD_PROCESS:
				    if (((type = ubuf.ut_type) == INIT_PROCESS
					|| type == LOGIN_PROCESS
					|| type == USER_PROCESS
					|| type == DEAD_PROCESS)
				      && ubuf.ut_id[0] == entry->ut_id[0]
				      && ubuf.ut_id[1] == entry->ut_id[1]
				      && ubuf.ut_id[2] == entry->ut_id[2]
				      && ubuf.ut_id[3] == entry->ut_id[3]) {
					uptr = &ubuf;
				        goto done;
				    }
			}
		}
	}

done:
	if (uptr) 
		lseek(fd_u, -(long)sizeof(ubuf), 1);
	else 
		fcntl(fd_u, F_SETFL, fc|O_APPEND);

	getutmp(entry, &ubuf);
	
	if (write(fd_u, &ubuf, sizeof(ubuf)) != sizeof(ubuf)) {
		return(1);
	}
	
	fcntl(fd_u, F_SETFL, fc);

	close(fd_u);

	return(0);
}


/*
 * If one of wtmp and wtmpx files exist, create the other, and the record.
 * If they both exist add the record.
 */
updwtmpx(filex, utx)
	char *filex;
	struct utmpx *utx;
{
	char file[MAXFILE+1];
	struct utmp ut;
	int fd, fdx;

	strcpy(file, filex);
	file[strlen(filex) - 1] = '\0';

	fd = open(file, O_WRONLY | O_APPEND);
	fdx = open(filex, O_WRONLY | O_APPEND);

	if (fd < 0) {
		if ((fdx < 0) || ((fd = open(file, O_WRONLY|O_CREAT)) < 0))
			return;
	} else if ((fdx < 0) && ((fdx = open(filex, O_WRONLY|O_CREAT)) < 0))
		return;


	/* Both files exist, synch them */
	if (synchutmp(file, filex))
		return;

	/* seek to end of file, in case synchutmp has appended to */
	/* the files. 					 	  */
	lseek(fd, 0, 2); lseek(fdx, 0, 2);

	getutmp(utx, &ut);
	write(fd, &ut, sizeof(struct utmp));
	write(fdx, utx, sizeof(struct utmpx));

	close(fd);
	close(fdx);
}



#ifdef  ERRDEBUG
#include        <stdio.h>

gxdebug(format,arg1,arg2,arg3,arg4,arg5,arg6)
char *format;
int arg1,arg2,arg3,arg4,arg5,arg6;
{
        register FILE *fp;
        register int errnum;

        if ((fp = fopen("/etc/dbg.getut","a+")) == NULL) return;
        fprintf(fp,format,arg1,arg2,arg3,arg4,arg5,arg6);
        fclose(fp);
}
#endif
