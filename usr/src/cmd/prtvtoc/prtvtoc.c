/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtvtoc:prtvtoc.c	1.4.4.1"
/*
 * prtvtoc.c
 *
 * Print a disk partition map ("VTOC"). Avoids the standard
 * I/O library to conserve first-floppy space.
 */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vtoc.h>
#include <errno.h>
#include <sys/mkdev.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * Macros.
 */
#define	strsize(str)	\
		(sizeof(str) - 1)	/* Length of static character array */

#define	parttn(x)	(x & 0x0f)	/* assumes partition # is low 4 bits */
#define	noparttn(x)	(x & 0xf0) 	/* assumes partition # is low 4 bits */

/*
 * Definitions.
 */
#define	reg	register		/* Convenience */
#define	uint	unsigned int		/* Convenience */
#define	ulong	unsigned long		/* Convenience */
#define	ushort	unsigned short		/* Convenience */
#define	DECIMAL	10			/* Numeric base 10 */
#define	HEX	16			/* Numeric base 16 */

#define	NULL	0
#define	MNTTAB	"/etc/mnttab"
#define	VFSTAB	"/etc/vfstab"

/*
 * Standard I/O file descriptors.
 */
#define	STDOUT	1			/* Standard output */
#define	STDERR	2			/* Standard error */

/*
 * Disk freespace structure.
 */
typedef struct {
	ulong	fr_start;		/* Start of free space */
	ulong	fr_size;		/* Length of free space */
} Freemap;

struct mnttab {
	char	*mnt_special;
	char	*mnt_mountp;
	char	*mnt_fstype;
	char	*mnt_mntopts;
	char	*mnt_time;
};

struct vfstab {
	char	*vfs_special;
	char	*vfs_fsckdev;
	char	*vfs_mountp;
	char	*vfs_fstype;
	char	*vfs_fsckpass;
	char	*vfs_automnt;
	char	*vfs_mntopts;
};

/*
 * External functions.
 */
char	*strcat();
char	*strchr();
char	*strcpy();
char	*strrchr();
char	*strtok();
extern char	*fgets();	/* not stdio, but defined below */
#ifdef __STDC__
extern int	getmntent(int, struct mnttab *);
extern int	getvfsent(int, struct vfstab *);
#else
extern int	getmntent();
extern int	getvfsent();
#endif

/*
 * Internal functions.
 */
void	fatal();
Freemap	*findfree();
char	**getmntpt();
int	getopt();
char	*memstr();
int	partcmp();
void	prc();
char	*mkn();
void	prn();
void	prs();
int	prtvtoc();
void	putfree();
void	puttable();
int	readpd();
int	readvtoc();
char	*syserr();
void	usage();
int	warn();

/*
 * External variables.
 */
extern int	errno;			/* System error code */
extern char	*sys_errlist[];		/* Error messages */
extern int	sys_nerr;		/* Number of sys_errlist[] entries */

/*
 * Static variables.
 */
static short	fflag;			/* Print freespace shell assignments */
static short	hflag;			/* Omit headers */
static short	sflag;			/* Omit all but the column header */
static char	*fstab = VFSTAB;	/* Fstab pathname */
static char	*mnttab = MNTTAB;	/* mnttab pathname */
static char	*myname;		/* Last qualifier of arg0 */
static int	optind = 1;		/* Argument index */
static char	*optarg;		/* Option argument */

main(ac, av)
int		ac;
reg char	**av;
{
	reg int		idx;

	if (myname = strrchr(av[0], '/'))
		++myname;
	else
		myname = av[0];
	while ((idx = getopt(ac, av, "fhst:m:")) != -1)
		switch (idx) {
		case 'f':
			++fflag;
			break;
		case 'h':
			++hflag;
			break;
		case 's':
			++sflag;
			break;
		case 't':
			fstab = optarg;
			break;
		case 'm':
			mnttab = optarg;
			break;
		default:
			usage();
		}
	if (optind >= ac)
		usage();
	idx = 0;
	while (optind < ac)
		idx |= prtvtoc(av[optind++]);
	exit(idx == 0 ? 0 : 1);
	/* NOTREACHED */
}

/*
 * fatal()
 *
 * Print an error message and exit.
 */
static void
fatal(what, why)
reg char	*what;
reg char	*why;
{
	static char	between[] = ": ";
	static char	after[] = "\n";

	(void) write(STDERR, myname, (uint) strlen(myname));
	(void) write(STDERR, between, (uint) strlen(between));
	(void) write(STDERR, what, (uint) strlen(what));
	(void) write(STDERR, between, (uint) strlen(between));
	(void) write(STDERR, why, (uint) strlen(why));
	(void) write(STDERR, after, (uint) strlen(after));
	exit(1);
}

/*
 * findfree()
 *
 * Find free space on a disk. Returns a pointer to the static result.
 */
static Freemap *
findfree(pdinfo, vtoc)
reg struct pdinfo	*pdinfo;
reg struct vtoc		*vtoc;
{
	reg struct partition	*part;
	reg struct partition	**list;
	reg Freemap		*freeidx;
	ulong			fullsize;
	ulong			cylsize;
	struct partition	*sorted[V_NUMPAR + 1];
	static Freemap		freemap[V_NUMPAR + 1];

	cylsize = pdinfo->sectors * pdinfo->tracks;
	fullsize = (pdinfo->cyls - 1 - (pdinfo->logicalst + cylsize - 1) / cylsize) * cylsize;
	if (vtoc->v_nparts > V_NUMPAR)
		fatal("putfree()", "Too many partitions on disk!");
	list = sorted;
	for (part = vtoc->v_part; part < vtoc->v_part + vtoc->v_nparts; ++part)
		if (part->p_size && part - vtoc->v_part != 6)
			*list++ = part;
	*list = 0;
	qsort((char *) sorted, (uint) (list - sorted), sizeof(*sorted), partcmp);
	freeidx = freemap;
	freeidx->fr_start = 0;
	for (list = sorted; part = *list; ++list)
		if (part->p_start == freeidx->fr_start)
			freeidx->fr_start += part->p_size;
		else {
			freeidx->fr_size = part->p_start - freeidx->fr_start;
			(++freeidx)->fr_start = part->p_start + part->p_size;
		}
	if (freeidx->fr_start < fullsize) {
		freeidx->fr_size = fullsize - freeidx->fr_start;
		++freeidx;
	}
	freeidx->fr_start = freeidx->fr_size = 0;
	return (freemap);
}

/*
 * getmntpt()
 *
 * Get the filesystem mountpoint of each partition on the disk
 * from the fstab or mnttab . Returns a pointer to an array of pointers to
 * directory names (indexed by partition number).
 */
static char **
getmntpt(slot, nopartminor)
major_t		slot;
minor_t		nopartminor;
{
	reg char	*item;
	reg char	*line;
	reg char	*next;
	reg int		idx;
	reg int		fd;
	auto struct stat sb;
	auto char	devbuf[40];
	static char	*delimit = " \t";
	static char	devblk[] = "/dev/";
	static char	devraw[] = "/dev/r";
	static char	*list[V_NUMPAR];
	struct mnttab	mtab;
	struct vfstab	vtab;

	for (idx = 0; idx < V_NUMPAR; ++idx)
		list[idx] = 0;

	/* read mnttab for partition mountpoints */
	if ((fd = open(mnttab, O_RDONLY)) < 0)  {
		warn(mnttab, syserr());
	} else {
		while ( getmntent(fd, &mtab) == 0 ) {
			item = mtab.mnt_special;
			if ( item != NULL && mtab.mnt_mountp != NULL
			  && strncmp(item, devblk, strsize(devblk)) == 0
			  && stat(strcat(strcpy(devbuf, devraw),
			    item + strsize(devblk)), &sb) == 0
			  && (sb.st_mode & S_IFMT) == S_IFCHR
			  && major(sb.st_rdev) == slot
			  && noparttn(minor(sb.st_rdev)) == nopartminor )
				list[parttn(minor(sb.st_rdev))] = memstr(mtab.mnt_mountp);
		}
	}
	close(fd);

	if ((fd = open(fstab, O_RDONLY)) < 0)  {
		warn(fstab, syserr());
		return(list);
	}
	while (getvfsent(fd, &vtab) == 0) {
		item = vtab.vfs_special;
		if (item != NULL && vtab.vfs_mountp != NULL
		  && strncmp(item, devblk, strsize(devblk)) == 0
		  && stat(strcat(strcpy(devbuf, devraw),
		    item + strsize(devblk)), &sb) == 0
		  && (sb.st_mode & S_IFMT) == S_IFCHR
		  && major(sb.st_rdev) == slot
		  && noparttn(minor(sb.st_rdev)) == nopartminor
		  /* use mnttab if both tables have entries */
		  && list[parttn(minor(sb.st_rdev))] == 0
		  && (item = strtok((char *) 0, delimit))
		  && *item == '/')
			list[parttn(minor(sb.st_rdev))] = memstr(item);
	}
	close(fd);

	return (list);
}

/*
 * getopt()
 *
 * Parse options. Stolen from libc, with changes
 * to avoid standard I/O.
 */
static int
getopt(ac, av, options)
int		ac;
reg char	**av;
char		*options;
{
	reg int		c;
	reg char	*cp;
	static int	sp = 1;

	if (sp == 1)
		if (optind >= ac
		    || av[optind][0] != '-' || av[optind][1] == '\0')
			return (-1);
		else if (strcmp(av[optind], "--") == 0) {
			optind++;
			return (-1);
		}
	c = av[optind][sp];
	if (c == ':' || (cp = strchr(options, c)) == 0)
		usage();
	if (*++cp == ':') {
		if (av[optind][sp+1] != '\0')
			optarg = &av[optind++][sp+1];
		else if (++optind >= ac)
			usage();
		else
			optarg = av[optind++];
		sp = 1;
	} else {
		if (av[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = 0;
	}
	return (c);
}

/*
 * memstr()
 *
 * Copy a string into dynamic memory. Returns a pointer
 * to the new instance of the given string.
 */
static char *
memstr(str)
reg char	*str;
{
	reg char	*mem;

	if ((mem = (char *)malloc((uint) strlen(str) + 1)) == 0)
		fatal(str, "Out of memory");
	return (strcpy(mem, str));
}

/*
 * partcmp()
 *
 * Qsort() key comparison of partitions by starting sector numbers.
 */
static int
partcmp(one, two)
reg struct partition	**one;
reg struct partition	**two;
{
	return ((*one)->p_start - (*two)->p_start);
}

/*
 * prc()
 *
 * Print a character.
 */
static void
prc(byte)
char		byte;
{
	(void) write(STDOUT, &byte, 1);
}

/*
 * prn()
 *
 * Print a number.
 */
static void
prn(number, base, len, min)
reg ulong	number;
reg int		base;
reg ushort	len;
ushort	min;
{
	reg char	*idx;
	auto char	buf[64];

	idx=mkn(buf, sizeof(buf), number, base, len, min);
	prs(idx);
}

char *
mkn(buf, bufsize,  number, base, len, min)
char *		buf;
int		bufsize;
reg ulong	number;
reg int		base;
reg ushort	len;
ushort		min;
{
	reg char	*idx;

	idx = &buf[bufsize];
	*--idx = '\0';
	do {
		*--idx = "0123456789abcdef"[number % base];
		number /= base;
	} while (number);
	for (number = buf + bufsize - 1 - idx; number < min; ++number)
		*--idx = '0';
	for (number = buf + bufsize - 1 - idx; number < len; ++number)
		*--idx = ' ';
	return(idx);
}

/*
 * prs()
 *
 * Print a string.
 */
static void
prs(str)
reg char	*str;
{
	(void) write(STDOUT, str, (uint) strlen(str));
}

/*
 * prtvtoc()
 *
 * Read and print a VTOC.
 */
static int
prtvtoc(name)
char		*name;
{
	reg int		fd;
	reg int		idx;
	reg Freemap	*freemap;
	struct stat	sb;
	struct vtoc	vtoc;
	struct pdsector	pdsector;

	if (stat(name, &sb) < 0)
		return (warn(name, syserr()));
	if ((sb.st_mode & S_IFMT) != S_IFCHR)
		return (warn(name, "Not a raw device"));
	if ((fd = open(name, O_RDONLY)) < 0)
		return (warn(name, syserr()));
	idx = (readpd(fd, name, &pdsector) == 0
	    && readvtoc(fd, name, &pdsector.pdinfo, &vtoc) == 0);
	(void) close(fd);
	if (!idx)
		return (-1);
	freemap = findfree(&pdsector.pdinfo, &vtoc);
	if (fflag)
		putfree(&vtoc, freemap);
	else
		puttable(&pdsector.pdinfo, &vtoc, freemap, name,
		    getmntpt(major(sb.st_rdev), noparttn(minor(sb.st_rdev))));
	return (0);
}

/*
 * putfree()
 *
 * Print shell assignments for disk free space. FREE_START and FREE_SIZE
 * represent the starting block and number of blocks of the first chunk
 * of free space. FREE_PART lists the unassigned partitions.
 */
static void
putfree(vtoc, freemap)
reg struct vtoc		*vtoc;
reg Freemap		*freemap;
{
	reg Freemap	*freeidx;
	reg ushort		idx;

	prs("FREE_START=");
	prn(freemap->fr_start, DECIMAL, 0, 1);
	prs(" FREE_SIZE=");
	prn(freemap->fr_size, DECIMAL, 0, 1);
	for (freeidx = freemap; freeidx->fr_size; ++freeidx)
		;
	prs(" FREE_COUNT=");
	prn((ulong) (freeidx - freemap), DECIMAL, 0, 1);
	prs(" FREE_PART=");
	for (idx = 0; idx < vtoc->v_nparts; ++idx)
		if (vtoc->v_part[idx].p_size == 0 && idx != 3)
			if (idx < 10)
				prc('0' + idx);
			else
				prc('a' + (idx - 10));
	prs("\n");
}

/*
 * puttable()
 *
 * Print a human-readable VTOC.
 */
static void
puttable(pdinfo, vtoc, freemap, name, mtab)
reg struct pdinfo	*pdinfo;
reg struct vtoc		*vtoc;
reg Freemap		*freemap;
char			*name;
char			**mtab;
{
	reg ushort		idx;
	reg ulong	start;
	reg ulong	cylsize;
	int 		i;

	cylsize = pdinfo->sectors * pdinfo->tracks;
	start = (pdinfo->logicalst + cylsize - 1) / cylsize;
	if (!hflag && !sflag) {
		prs("* ");
		prs(name);
		if (*vtoc->v_volume) {
			prs(" (volume \"");
			for(i=0;i<8;i++) {
				if(vtoc->v_volume[i] != '\0')
					prc(vtoc->v_volume[i]);
				else
					break;
			}
			prs("\")");
		}
		prs(" partition map\n");
		prs("*\n* Dimensions:\n* ");
		prn(pdinfo->bytes, DECIMAL, 7, 1);
		prs(" bytes/sector\n* ");
		prn(pdinfo->sectors, DECIMAL, 7, 1);
		prs(" sectors/track\n* ");
		prn(pdinfo->tracks, DECIMAL, 7, 1);
		prs(" tracks/cylinder\n* ");
		prn(pdinfo->sectors * pdinfo->tracks, DECIMAL, 7, 1);
		prs(" sectors/cylinder\n* ");
		prn(pdinfo->cyls, DECIMAL, 7, 1);
		prs(" cylinders\n* ");
		prn(pdinfo->cyls - 1 - start, DECIMAL, 7, 1);
		prs(" accessible cylinders\n*\n* Flags:\n*  ");
		prn((ulong) V_UNMNT, HEX, 2, 1);
		prs(": unmountable\n*  ");
		prn((ulong) V_RONLY, HEX, 2, 1);
		prs(": read-only\n*\n");
		if (freemap->fr_size) {
			prs("* Unallocated space:\n");
			prs("*\tFirst     Sector    Last\n");
			prs("*\tSector     Count    Sector \n");
			do {
				prs("*   ");
				prn(freemap->fr_start, DECIMAL, 9, 1);
				prs(" ");
				prn(freemap->fr_size, DECIMAL, 9, 1);
				prs(" ");
				prn(freemap->fr_size + freemap->fr_start - 1, DECIMAL, 9, 1);
				prs("\n");
			} while ((++freemap)->fr_size);
			prs("*\n");
		}
	}
	if (!hflag)  {
		prs("*                          First     Sector    Last\n");
		prs("* Partition  Tag  Flags    Sector     Count    Sector  Mount Directory\n");
	}
	for (idx = 0; idx < vtoc->v_nparts; ++idx) {
		if (vtoc->v_part[idx].p_size == 0)
			continue;
		prs("      ");
		prn((ulong) idx, HEX, 2, 1);
		prs("  ");
		prn((ulong) vtoc->v_part[idx].p_tag, DECIMAL, 5, 1);
		prs("  ");
		prn((ulong) vtoc->v_part[idx].p_flag, HEX, 4, 2);
		prs("  ");
		prn((ulong) vtoc->v_part[idx].p_start, DECIMAL, 9, 1);
		prs(" ");
		prn((ulong) vtoc->v_part[idx].p_size, DECIMAL, 9, 1);
		prs(" ");
		prn((ulong) (vtoc->v_part[idx].p_start + vtoc->v_part[idx].p_size - 1), DECIMAL, 9, 1);
		if (mtab && mtab[idx]) {
			prs("   ");
			prs(mtab[idx]);
		}
		prs("\n");
	}
}

/*
 * readpd()
 *
 * Read physical device information.
 */
static int
readpd(fd, name, pdsector)
int		fd;
reg char	*name;
struct pdsector	*pdsector;
{
	struct io_arg	args;

	args.sectst = 0;
	args.memaddr = (unsigned long) pdsector;
	args.datasz = sizeof(struct pdsector);
	if (ioctl(fd, V_PDREAD, &args) < 0)
		return (warn(name, syserr()));
	if (args.retval == V_BADREAD)
		return (warn(name, "Unable to read device information sector"));
	return (0);
}

/*
 * readvtoc()
 *
 * Read a partition map.
 */
static int
readvtoc(fd, name, pdinfo, vtoc)
int		fd;
char		*name;
struct pdinfo	*pdinfo;
struct vtoc	*vtoc;
{
	struct io_arg	args;

	args.sectst = pdinfo->logicalst + 1;
	args.memaddr = (unsigned long) vtoc;
	args.datasz = sizeof(struct vtoc);
	if (ioctl(fd, V_PREAD, &args) < 0)
		return (warn(name, syserr()));
	if (args.retval == V_BADREAD)
		return (warn(name, "Unable to read VTOC"));
	if (vtoc->v_sanity != VTOC_SANE)
		return (warn(name, "No valid VTOC on device"));
	return (0);
}

/*
 * syserr()
 *
 * Return a pointer to a system error message.
 */
static char	err1[30] =  "Unknown error - ";
static char	err2[10] =  "         ";
static char *
syserr()
{
	return (errno <= 0 ? "No error (?)"
	    : errno < sys_nerr ? sys_errlist[errno]
	    : strcat( err1, mkn(err2,sizeof(err2),errno, DECIMAL, 5, 1)) );
}

/*
 * usage()
 *
 * Print a helpful message and exit.
 */
static void
usage()
{
	static char	before[] = "Usage:\t";
	static char	after[] = " [ -fhs ] [ -t fstab ] [ -m mnttab ] rawdisk ...\n";

	(void) write(STDERR, before, (uint) strlen(before));
	(void) write(STDERR, myname, (uint) strlen(myname));
	(void) write(STDERR, after, (uint) strlen(after));
	exit(1);
}

/*
 * warn()
 *
 * Print an error message. Always returns -1.
 */
static int
warn(what, why)
reg char	*what;
reg char	*why;
{
	static char	between[] = ": ";
	static char	after[] = "\n";

	(void) write(STDERR, myname, (uint) strlen(myname));
	(void) write(STDERR, between, (uint) strlen(between));
	(void) write(STDERR, what, (uint) strlen(what));
	(void) write(STDERR, between, (uint) strlen(between));
	(void) write(STDERR, why, (uint) strlen(why));
	(void) write(STDERR, after, (uint) strlen(after));
	return (-1);
}

/* a version of getmntent() and getvfsent() that doesn't use stdio */
#define	TOOLONG	1	/* entry exceeds LINE_MAX */
#define	TOOMANY	2	/* too many fields in line */
#define	TOOFEW	3	/* too few fields in line */

#define	LINE_MAX	1024

#define	MNT_GETTOK(xx, ll)\
	if ((mp->xx = strtok(ll, sepstr)) == NULL)\
		return	TOOFEW;\
	if (strcmp(mp->xx, dash) == 0)\
		mp->xx = NULL

#define	VFS_GETTOK(xx, ll)\
	if ((vp->xx = strtok(ll, sepstr)) == NULL)\
		return	TOOFEW;\
	if (strcmp(vp->xx, dash) == 0)\
		vp->xx = NULL

static char	line[LINE_MAX];
static char	sepstr[] = " \t\n";
static char	dash[] = "-";

int
getmntent(fd, mp)
	register int	fd;
	register struct mnttab	*mp;
{
	register int	ret;

	/* skip leading spaces and comments */
	if ((ret = getline(line, fd)) != 0)
		return	ret;

	/* split up each field */
	MNT_GETTOK(mnt_special, line);
	MNT_GETTOK(mnt_mountp, NULL);
	MNT_GETTOK(mnt_fstype, NULL);
	MNT_GETTOK(mnt_mntopts, NULL);
	MNT_GETTOK(mnt_time, NULL);

	/* check for too many fields */
	if (strtok(NULL, sepstr) != NULL)
		return	TOOMANY;

	return	0;
}

int
getvfsent(fd, vp)
	register int	fd;
	register struct vfstab	*vp;
{
	register int	ret;

	/* skip leading spaces and comments */
	if ((ret = getline(line, fd)) != 0)
		return	ret;

	/* split up each field */
	VFS_GETTOK(vfs_special, line);
	VFS_GETTOK(vfs_fsckdev, NULL);
	VFS_GETTOK(vfs_mountp, NULL);
	VFS_GETTOK(vfs_fstype, NULL);
	VFS_GETTOK(vfs_fsckpass, NULL);
	VFS_GETTOK(vfs_automnt, NULL);
	VFS_GETTOK(vfs_mntopts, NULL);

	/* check for too many fields */
	if (strtok(NULL, sepstr) != NULL)
		return	TOOMANY;

	return	0;
}

int
getline(lp, fd)
	register char	*lp;
	register int	fd;
{
	register int	ret;
	register char	*cp;

	while ((lp = fgets(lp, LINE_MAX, fd)) != NULL) {
		if (strlen(lp) == LINE_MAX-1 && lp[LINE_MAX-2] != '\n')
			return	TOOLONG;

		for (cp = lp; *cp == ' ' || *cp == '\t'; cp++)
			;

		if (*cp != '#' && *cp != '\n')
			return	0;
	}
	return	-1;
}

/* an fgets that doesn't use stdio */
char	*
fgets(startp, lpsz, fd)
	char	*startp;
	int	lpsz, fd;
{
	char	*lp;

	for (lp = startp; --lpsz && read(fd, lp, 1) == 1 && *lp != '\n'; lp++)
		;

	*lp = '\0';

	if (lp == startp)
		return	NULL;

	return	startp;
}
