/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getut.c	1.19"

/*	Routines to read and write the /etc/utmp file.			*/
/*									*/
#ifdef __STDC__
	#pragma weak endutent = _endutent
	#pragma weak getutent = _getutent
	#pragma weak getutid = _getutid
	#pragma weak getutline = _getutline
	#pragma weak pututline = _pututline
	#pragma weak setutent = _setutent
	#pragma weak utmpname = _utmpname
#endif
#include	"synonyms.h"
#include "shlib.h"
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<utmpx.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<string.h>
#include	<stdlib.h>

#define	MAXFILE	79	/* Maximum pathname length for "utmp" file */

#ifdef ut_time
#undef ut_time
#endif

#ifdef	DEBUG
#undef	UTMP_FILE
#define	UTMP_FILE "utmp"
#endif

extern long	lseek();
extern void	setutent();
extern int	stat(), write(), read(), close();

static int fd = -1;	/* File descriptor for the utmp file. */
static const char *utmpfile = UTMP_FILE;	/* Name of the current */
static const char *utmpxfile = UTMPX_FILE;	/* "utmp" like file.   */

#ifdef ERRDEBUG
static long loc_utmp;	/* Where in "utmp" the current "ubuf" was found.*/
#endif

static struct utmp ubuf;	/* Copy of last entry read in. */


/* "getutent" gets the next entry in the utmp file.
 */

struct utmp *getutent()
{
	extern int fd;
	extern struct utmp ubuf;
	register char *u;
	register int i;

/* If the "utmp" file is not open, attempt to open it for
 * reading.  If there is no file, attempt to create one.  If
 * both attempts fail, return NULL.  If the file exists, but
 * isn't readable and writeable, do not attempt to create.
 */

	if (fd < 0) {
		/* Make sure files are in synch */
		if (synchutmp(utmpfile, utmpxfile))
			return(NULL);
		
		if ((fd = open(utmpfile, O_RDWR|O_CREAT, 0644)) < 0) {

/* If the open failed for permissions, try opening it only for
 * reading.  All "pututline()" later will fail the writes.
 */
		if ((fd = open(utmpfile, O_RDONLY)) < 0)
				return(NULL);
		}
	}

/* Try to read in the next entry from the utmp file.  */
	if (read(fd,&ubuf,sizeof(ubuf)) != sizeof(ubuf)) {

/* Make sure ubuf is zeroed. */
		for (i=0,u=(char *)(&ubuf); i<sizeof(ubuf); i++) *u++ = '\0';
		return(NULL);
	}

/* Save the location in the file where this entry was found. */
	(void) lseek(fd,0L,1);
	return(&ubuf);
}

/*	"getutid" finds the specified entry in the utmp file.  If	*/
/*	it can't find it, it returns NULL.				*/

struct utmp *getutid(entry)
register const struct utmp *entry;
{
	extern struct utmp ubuf;
	struct utmp *getutent();
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
	} while (getutent() != NULL);

/* Return NULL since the proper entry wasn't found. */
	return(NULL);
}

/* "getutline" searches the "utmp" file for a LOGIN_PROCESS or
 * USER_PROCESS with the same "line" as the specified "entry".
 */

struct utmp *getutline(entry)
register const struct utmp *entry;
{
	extern struct utmp ubuf,*getutent();
	register struct utmp *cur;

/* Start by using the entry currently incore.  This prevents */
/* doing reads that aren't necessary. */
	cur = &ubuf;
	do {
/* If the current entry is the one we are interested in, return */
/* a pointer to it. */
		if (cur->ut_type != EMPTY && (cur->ut_type == LOGIN_PROCESS
		    || cur->ut_type == USER_PROCESS) && strncmp(&entry->ut_line[0],
		    &cur->ut_line[0],sizeof(cur->ut_line)) == 0) return(cur);
	} while ((cur = getutent()) != NULL);

/* Since entry wasn't found, return NULL. */
	return(NULL);
}

/*	"pututline" writes the structure sent into the utmp file.	*/
/*	If there is already an entry with the same id, then it is	*/
/*	overwritten, otherwise a new entry is made at the end of the	*/
/*	utmp file.							*/

struct utmp *pututline(entry)
const struct utmp *entry;
{
	int fc;
	struct utmp *answer;
	extern struct utmp ubuf;
	extern struct utmp *getutid();
	extern int fd;
	struct utmp tmpbuf, savbuf;

/* Copy the user supplied entry into our temporary buffer to */
/* avoid the possibility that the user is actually passing us */
/* the address of "ubuf". */
	tmpbuf = *entry;
	(void)getutent();
	if (fd < 0) {
#ifdef	ERRDEBUG
		gdebug("pututline: Unable to create utmp file.\n");
#endif
		return((struct utmp *)NULL);
	}
/* Make sure file is writable */
	if ((fc=fcntl(fd, F_GETFL, NULL)) == -1
	    || (fc & O_RDWR) != O_RDWR) {
		return((struct utmp *)NULL);
	}

/* Find the proper entry in the utmp file.  Start at the current */
/* location.  If it isn't found from here to the end of the */
/* file, then reset to the beginning of the file and try again. */
/* If it still isn't found, then write a new entry at the end of */
/* the file.  (Making sure the location is an integral number of */
/* utmp structures into the file incase the file is scribbled.) */

	if (getutid(&tmpbuf) == NULL) {
#ifdef	ERRDEBUG
		gdebug("First getutid() failed.  fd: %d",fd);
#endif
		setutent();
		if (getutid(&tmpbuf) == NULL) {
#ifdef	ERRDEBUG
			loc_utmp = lseek(fd, 0L, 1);
			gdebug("Second getutid() failed.  fd: %d loc_utmp: %ld\n",fd,loc_utmp);
#endif
			fcntl(fd, F_SETFL, fc | O_APPEND);
		} else {
			lseek(fd, -(long)sizeof(struct utmp), 1);
		}
	} else {
		lseek(fd, -(long)sizeof(struct utmp), 1);
	}

/* Write out the user supplied structure.  If the write fails, */
/* then the user probably doesn't have permission to write the */
/* utmp file. */
	if (write(fd,&tmpbuf,sizeof(tmpbuf)) != sizeof(tmpbuf)) {
#ifdef	ERRDEBUG
		gdebug("pututline failed: write-%d\n",errno);
#endif
		answer = (struct utmp *)NULL;
	} else {
/* Save the user structure that was overwritten. Copy the new user  */
/* structure into ubuf so that it will be up to date in the future. */
		savbuf = ubuf;
		ubuf = tmpbuf;
		answer = &ubuf;

#ifdef	ERRDEBUG
		gdebug("id: %c%c loc: %x\n",ubuf.ut_id[0],ubuf.ut_id[1],
		    ubuf.ut_id[2],ubuf.ut_id[3],loc_utmp);
#endif
	}
/* update the parallel utmpx file */
	if (updutmpx(entry)) {
		lseek(fd, -(long)sizeof(struct utmp), 1);
		write(fd, &savbuf, sizeof(savbuf));
		answer = (struct utmp *)NULL;
	}

	fcntl(fd, F_SETFL, fc);
	return(answer);
}

/*	"setutent" just resets the utmp file back to the beginning.	*/

void
setutent()
{
	register char *ptr;
	register int i;
	extern int fd;
	extern struct utmp ubuf;

	if (fd != -1) lseek(fd,0L,0);

/* Zero the stored copy of the last entry read, since we are */
/* resetting to the beginning of the file. */

	for (i=0,ptr=(char*)&ubuf; i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*	"endutent" closes the utmp file.				*/

void
endutent()
{
	extern int fd;
	extern struct utmp ubuf;
	register char *ptr;
	register int i;

	if (fd != -1) close(fd);
	fd = -1;
	for (i=0,ptr= (char *)(&ubuf); i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*	"utmpname" allows the user to read a file other than the	*/
/*	normal "utmp" file.						*/
utmpname(newfile)
const char *newfile;
{
	static char *saveptr;
	static int savelen = 0;
	int len;

/* Determine if the new filename will fit.  If not, return 0. */
	if ((len = strlen(newfile)) >= MAXFILE) return (0);

	/* malloc enough space for utmp, utmpx, and null bytes */
	if (len > savelen)
	{
		if (saveptr)
			free(saveptr);
		if ((saveptr = malloc(2 * len + 3)) == 0)
			return (0);
		savelen = len;
	}

	/* copy in the new file name. */
	utmpfile = (const char *)saveptr;
	(void)strcpy(saveptr, newfile);
	utmpxfile = (const char *)saveptr + len + 2;
	(void)strcpy(saveptr + len + 2, newfile);
	strcat(saveptr + len + 2, "x");

/* Make sure everything is reset to the beginning state. */
	endutent();
	return(1);
}

/* "updutmpx" updates the utmpx file. Uses the same
 * search algorithm as pututline to make sure records
 * end up in the same place. 
 */
int updutmpx(entry)
struct utmp *entry;
{
	int fd_u, fc, type;
	struct stat stbuf;
	struct utmpx uxbuf, *uxptr = NULL;

	if ((fd_u = open(utmpxfile, O_RDWR|O_CREAT, 0644)) < 0) {
#ifdef ERRDEBUG
		gdebug("Could not open utmpxfile\n");
#endif
		return(1);
	}

	if ((fc = fcntl(fd_u, F_GETFL, NULL)) == -1)
		return(1);

	while (read(fd_u, &uxbuf, sizeof(uxbuf)) == sizeof(uxbuf)) {
		if (uxbuf.ut_type != EMPTY) {
			switch (entry->ut_type) {
				case EMPTY:
				    goto done;	
				case RUN_LVL:
				case BOOT_TIME:
				case OLD_TIME:
				case NEW_TIME:
				    if (entry->ut_type == uxbuf.ut_type) {
					uxptr = &uxbuf;
				        goto done;
				    }
				case INIT_PROCESS:
				case LOGIN_PROCESS:
				case USER_PROCESS:
				case DEAD_PROCESS:
				    if (((type = uxbuf.ut_type) == INIT_PROCESS
					|| type == LOGIN_PROCESS
					|| type == USER_PROCESS
					|| type == DEAD_PROCESS)
				      && uxbuf.ut_id[0] == entry->ut_id[0]
				      && uxbuf.ut_id[1] == entry->ut_id[1]
				      && uxbuf.ut_id[2] == entry->ut_id[2]
				      && uxbuf.ut_id[3] == entry->ut_id[3]) {
					uxptr = &uxbuf;
				        goto done;
				    }
			}
		}
	}

done:	
	if (uxptr)
		lseek(fd_u, -(long)sizeof(uxbuf), 1);
	else 
		fcntl(fd_u, F_SETFL, fc|O_APPEND);

	getutmpx(entry, &uxbuf);
	
	if (write(fd_u, &uxbuf, sizeof(uxbuf)) != sizeof(uxbuf)) {
#ifdef ERRDEBUG
		gdebug("updutmpx failed: write-%d\n", errno);
#endif
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
updwtmp(file, ut)
	char *file;
	struct utmp *ut;
{
	char filex[256];
	struct utmpx utx;
	int fd, fdx;

	strcpy(filex, file);
	strcat(filex, "x");

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
	
	write(fd, ut, sizeof(struct utmp));
	getutmpx(ut, &utx);
	write(fdx, &utx, sizeof(struct utmpx));

	close(fd);
	close(fdx);
}


/*
 * "getutmp" - convert a utmpx record to a utmp record.
 */

getutmp(utx, ut)
        struct utmpx *utx;
        struct utmp  *ut;
{
        strncpy(ut->ut_user, utx->ut_user, sizeof(ut->ut_user));
        strncpy(ut->ut_line, utx->ut_line, sizeof(ut->ut_line));
	(void) memcpy(ut->ut_id, utx->ut_id, sizeof(utx->ut_id));
        ut->ut_pid = utx->ut_pid;
        ut->ut_type = utx->ut_type;
        ut->ut_exit = utx->ut_exit;
        ut->ut_time = utx->ut_tv.tv_sec;
}


/*
 * "getutmpx" - convert a utmp record to a utmpx record.
 */

getutmpx(ut, utx)
	struct utmp *ut;
	struct utmpx *utx;
{
        strncpy(utx->ut_user, ut->ut_user, sizeof(ut->ut_user));
        utx->ut_user[sizeof(ut->ut_user)] = '\0';
        strncpy(utx->ut_line, ut->ut_line, sizeof(ut->ut_line));
        utx->ut_line[sizeof(ut->ut_line)] = '\0';
	(void) memcpy(utx->ut_id, ut->ut_id, sizeof(ut->ut_id));
        utx->ut_pid = ut->ut_pid;
        utx->ut_type = ut->ut_type;
        utx->ut_exit = ut->ut_exit;
        utx->ut_tv.tv_sec = ut->ut_time;
        utx->ut_tv.tv_usec = 0;
        utx->ut_host[0] = '\0';
}


/* "synchutmp" make sure utmp and utmpx files are in synch.
 * Returns an error code if the files are not multiples
 * of their respective struct size. Updates the out of 
 * date file.
*/
synchutmp(utf, utxf)
	char *utf, *utxf;
{
	struct stat stbuf, stxbuf;

	if (stat(utf, &stbuf) == 0 &&
				stat(utxf, &stxbuf) == 0) {
		/* Make sure file is a multiple of 'utmp'  entries long */
		if((stbuf.st_size % sizeof(struct utmp)) != 0 ||
		   (stxbuf.st_size % sizeof(struct utmpx)) != 0) {
			errno = EINVAL;
			return(1);
		}

		if (stbuf.st_size) {
			if (!stxbuf.st_size)
				return(updutxfile(utf, utxf));
		} else if (stxbuf.st_size)
			return(updutfile(utf, utxf));
				
		if (abs(stxbuf.st_mtime-stbuf.st_mtime) >= MOD_WIN) {
			/* files are out of sync */
			if (stxbuf.st_mtime > stbuf.st_mtime) 
				return(updutfile(utf, utxf));
			else 
				return(updutxfile(utf, utxf));
		}
		return(0);
	}
	return(1);
}



/* "updutfile" updates the utmp file using the contents of the
 * umptx file.
 */
updutfile(utf, utxf)
	char *utf, *utxf;
{
	struct utmpx utx;
	struct utmp  ut;
	int fd1, fd2, n;

	if ((fd1 = open(utf, O_RDWR|O_CREAT|O_TRUNC)) < 0 ||
	    (fd2 = open(utxf, O_RDONLY)) < 0) 
		return(1);

	while ((n = read(fd2, &utx, sizeof(utx))) == sizeof(utx)) {
		getutmp(&utx, &ut);
		if (write(fd1, &ut, sizeof(ut)) != sizeof(ut))
			return(1);
	}
	close(fd1);
	close(fd2);
	utime(utxf, NULL);
	return(0);
}


/* "updutxfile" updates the utmpx file using the contents of the 
 * utmp file. Tries to preserve the host information as much
 * as possible.
 */
updutxfile(utf, utxf)
	char *utf, *utxf;
{
	struct utmp  ut;
	struct utmpx utx;
	int fd1, fd2;
	int n1, n2, cnt=0;

	if ((fd1 = open(utf, O_RDONLY)) < 0 || 
	     (fd2 = open(utxf, O_RDWR)) < 0) 
		return(1);

	/* As long as the entries match, copy the records from the
	 * utmpx file to keep the host information.
	 */
	while ((n1 = read(fd1, &ut, sizeof(ut))) == sizeof(ut)) {
		if ((n2 = read(fd2, &utx, sizeof(utx))) != sizeof(utx)) 
			break;
		if (ut.ut_pid != utx.ut_pid || ut.ut_type != utx.ut_type 
		   || !memcmp(ut.ut_id, utx.ut_id, sizeof(ut.ut_id))
		   || ! memcmp(ut.ut_line, utx.ut_line, sizeof(ut.ut_line))) {
			getutmpx(&ut, &utx);
			lseek(fd2, -(long)sizeof(struct utmpx), 1);
			if (write(fd2, &utx, sizeof(utx)) != sizeof(utx))
				return(1);
			cnt += sizeof(struct utmpx); 
		}
	}

	/* out of date file is shorter, copy from the up to date file
	 * to the new file.
	 */
	if (n1 > 0) {
		do {
			getutmpx(&ut, &utx);
			if (write(fd2, &utx, sizeof(utx)) != sizeof(utx))
				return(1);
		} while ((n1 = read(fd1, &ut, sizeof(ut))) == sizeof(ut));
	} else {
		/* out of date file was longer, truncate it */
		truncate(utxf, cnt);
	}

	close(fd1);
	close(fd2);
	utime(utf, NULL);
	return(0);
}

#ifdef  ERRDEBUG
#include        <stdio.h>

gdebug(format,arg1,arg2,arg3,arg4,arg5,arg6)
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
