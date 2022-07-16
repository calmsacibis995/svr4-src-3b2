/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-3b2:pdinfo.c	1.2.4.1"
/*
 * pdinfo.c
 *
 * Print shell assignments for hard disk physical information.
 *
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <sys/vtoc.h>
#include <sys/id.h>
#include <errno.h>

/*
 * Definitions.
 */
#define	reg	register		/* Convenience */
#define	uint	unsigned int		/* Convenience */
#define	ulong	unsigned long		/* Convenience */
#define	ushort	unsigned short		/* Convenience */
#define	DECIMAL	10			/* Numeric base 10 */
#define	STDERR	2			/* Standard error file descriptor */
#define	STDOUT	1			/* Standard output file descriptor */

/*
 * External functions.
 */
void	exit();
char	*strchr();
char	*strrchr();
char	*strcmp();

/*
 * Internal functions.
 */
int	findswap();
int	findstand();
int	pdinfo();
int	pread();
void	prn();
void	prs();
int	readpd();
void	result();
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
static char	*myname;		/* Last qualifier of arg0 */

main(ac, av)
int		ac;
reg char	**av;
{
	reg ulong	errors = 0;

	if (myname = strrchr(av[0], '/'))
		++myname;
	else
		myname = av[0];
	if (ac < 2)
		usage();
	while (*++av)
		if (pdinfo(*av) < 0)
			++errors;
	exit(errors ? 1 : 0);
}

/*
 * pdinfo()
 *
 * Print physical information for one device.
 */
static int
pdinfo(name)
char		*name;
{
	reg ulong	cylsize;
	reg ulong	nsector;
	reg int		fd;
	reg int		drive;
	reg int		ok;
	reg int		partno;
	struct stat	sb;
	struct pdsector	pdsector;
	struct 	vtoc	vtoc;
	char	*rtd = "/dev/rdsk/c1d0s0";

	if (stat(name, &sb) < 0)
		return (warn(name, syserr()));
	if ((sb.st_mode & S_IFMT) != S_IFCHR)
		return (warn(name, "Not a raw device"));
	if ((fd = open(name, O_RDONLY)) < 0)
		if (errno == ENXIO)
			return (0);
		else
			return (warn(name, syserr()));
	ok = (readpd(fd, name, &pdsector) == 0);
	(void) close(fd);
	if (!ok)
		return (-1);
	drive = iddn(minor(sb.st_rdev));
	cylsize = pdsector.pdinfo.tracks * pdsector.pdinfo.sectors;
	nsector = pdsector.pdinfo.cyls - 1
	    - (pdsector.pdinfo.logicalst + cylsize - 1) / cylsize;
	result("DRIVEID", drive, pdsector.pdinfo.driveid);
	result("CYLSIZE", drive, cylsize);
	result("NSECTOR", drive, nsector);
	if (strcmp(name,rtd) == 0) {
		if ((fd = open(name, O_RDONLY)) < 0
	  	|| readpd(fd, name, &pdsector) < 0
	  	|| pread(fd, (daddr_t) pdsector.pdinfo.logicalst + 1,
	      	(caddr_t) &vtoc, (daddr_t) sizeof(struct vtoc)) < 0) {
			result("SWP_SZ",0,-1);
			return(0);
	}

		if ((partno = findswap(vtoc.v_part, vtoc.v_nparts)) < 0){
			result("SWP_SZ",0,-1);
			return(0);
		}
		result("SWP_PART",0,partno);
		result("SWP_STRT",0,vtoc.v_part[partno].p_start);
		result("SWP_SZ",0,vtoc.v_part[partno].p_size);
		if ((partno = findstand(vtoc.v_part, vtoc.v_nparts)) < 0){
			result("EXIST_STAND",0,0);
			return(0);
		} 
		result("EXIST_STAND",0,1);
	}
	return (0);
}


/*
 * findswap()
 *
 * Look for a valid swap partition.
 */
static int
findswap(ptab, nparts)
struct partition	*ptab;
ushort			nparts;
{
	reg struct partition	*pidx;
	reg struct partition	*pend = ptab + nparts;

	for (pidx = ptab; pidx < pend; ++pidx)
		if (pidx->p_size
		    && pidx->p_tag == V_SWAP
		    && (pidx->p_flag & (V_UNMNT | V_RONLY)) == V_UNMNT)
			return (pidx - ptab);
	return (-1);
}
/*
 * findstand()
 *
 * Look for the stand partition.
 */
static int
findstand(ptab, nparts)
struct partition	*ptab;
ushort			nparts;
{
	reg struct partition	*pidx;
	reg struct partition	*pend = ptab + nparts;

	for (pidx = ptab; pidx < pend; ++pidx)
		if (pidx->p_size
		    && pidx->p_tag == V_STAND)
			return (pidx - ptab);
	return (-1);
}

/*
 * pread()
 *
 * Physical read.
 */
static int
pread(fd, sectst, memaddr, datasz)
int	fd;
daddr_t	sectst;
caddr_t	memaddr;
daddr_t	datasz;
{
	struct io_arg	args;

	args.sectst = (ulong) sectst;
	args.memaddr = (ulong) memaddr;
	args.datasz = (ulong) datasz;
	if (ioctl(fd, V_PREAD, &args) < 0)
		return (-1);
	if (args.retval == V_BADREAD) {
		errno = EIO;
		return (-1);
	}
	return (0);
}

/*
 * prn()
 *
 * Print a number.
 */
static void
prn(number, base)
reg ulong	number;
reg int		base;
{
	reg char	*idx;
	auto char	buf[64];

	idx = buf + sizeof(buf);
	*--idx = '\0';
	do {
		*--idx = "0123456789abcdef"[number % base];
		number /= base;
	} while (number);
	prs(idx);
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

	args.retval = 0;
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
 * result()
 *
 * Print a single result.
 */
static void
result(name, drive, value)
char	*name;
int	drive;
ulong	value;
{
	prs(name);
	prs("_");
	prn((ulong) drive, DECIMAL);
	prs("=");
	prn(value, DECIMAL);
	prs("\n");
}

/*
 * syserr()
 *
 * Return a pointer to a system error message.
 */
static char *
syserr()
{
	return (errno <= 0 ? "No error (?)"
	    : errno < sys_nerr ? sys_errlist[errno]
	    : "Unknown error (!)");
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
	static char	after[] = " raw_disk_device ...\n";

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

