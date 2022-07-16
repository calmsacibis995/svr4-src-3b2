/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vmkfs:vmkfs.c	1.11.1.2"

/*
 * vmkfs.c
 *
 * Make a filesystem within a hard disk partition. Avoids the
 * standard I/O library to remain within first-floppy space
 * constraints.
 */

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vtoc.h>
#include <sys/sysmacros.h>
#include <sys/mkdev.h>
#include <sys/id.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 * Definitions.
 */
#define	reg	register		/* Convenience */
#define	uint	unsigned int		/* Convenience */
#define	ulong	unsigned long		/* Convenience */
#define	ushort	unsigned short		/* Convenience */
#define	STDERR	2			/* Standard error file descriptor */
#define	STDOUT	1			/* Standard output file descriptor */
#define	ONE_K 	1024
#define	TWO_K 	2048

#define DEF_FS		"s5"			/* Default file system type. */
#define S5_PATH		"/etc/fs/s5/mkfs"	/* path for s5 mkfs */
#define BFS_PATH	"/etc/fs/bfs/mkfs"	/* path for bfs mkfs */

/*
 * Internal functions.
 */
void	child();
void	fatal();
char	*itoa();
int	readpd();
int	readvtoc();
char	*syserr();
void	usage();
int	vmkfs();
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
static short	nflag;			/* Don't execute mkfs(1M) */
static short	xflag;			/* Trace generated mkfs(1M) commands */
static char	*myname;		/* Last qualifier of arg0 */

main(ac, av)
int		ac;
reg char	**av;
{
	/* var decl */
	extern	char	*optarg;	/* used by getopt-gives option arg */
	extern	int	optind;		/* by getopt - index of next arg */
	reg 	int	ret=0;		/* return value from this program */
		int	option=0;	/* option from parsing argv vector */
		int	blksiz=0;	/* logical block size */
		int 	gapsiz=0;	/* gap size */
		char	*fstype;	/* file system type */


	if (myname = strrchr(av[0], '/'))
		++myname;
	else
		myname = av[0];


	fstype = DEF_FS;

	while( ( option = getopt(ac, av, "xnb:f:g:" ) ) != -1 )
		switch(option)
		{
		case 'x':	++xflag;		
				break;

		case 'n':	++nflag;	
				break;
		
		case 'b':	if ( (blksiz = atoi(optarg)) < 0)
					fatal( av[optind-1],
					"Block size can be 512, 1024 or 2048 only");
				
				/* check if block siz is accepatable */
				if( !( blksiz == 512 ||
				       blksiz == ONE_K  ||
				       blksiz == TWO_K    )   )
					fatal( av[optind-1],
					"Block size can be 512, 1024 or 2048 only");
				break;
		case 'f':				
				fstype = optarg;
				break;

		case 'g':
				if ( (gapsiz = atoi(optarg)) < 0)
					fatal( av[optind-1], "Invalid Gapsize");

				if ( gapsiz < 4 || gapsiz > 12 )
					fatal( av[optind-1],
					"Gap size can be between 4 and 12 only");
				break;

		case '?':
				fatal(av[optind-1],
				"Unknown option or option has no required arg");
				break;
		}



	if (*av == 0)
		usage();

	for (ret = 0; *(av + optind); optind++ )
		ret |= vmkfs( *(av + optind), fstype, blksiz, gapsiz );

	exit(ret);
}

/*
 * s5_child()
 *
 * This is the forked child. Execute mkfs(1M) for s5 filesystem type with
 * appropriate parameters. NEVER returns.
 */
static void
s5_child(devname, pdsector, part, blksiz, gapsiz)
reg char		*devname;	/* device file name */
reg struct pdsector	*pdsector;	/* for device info- size etc */
reg struct partition	*part;		/* for partition info */
reg int   		blksiz;		/* logical block size */
reg int			gapsiz;		/* gap size */
{
	auto char	partnsiz[20];	/* size of partition */
	auto char	cylsize[64]; 	/* cylinder size */
	auto char	mkfs[25];	/* path name for fs specific mkfs */
	auto char	inodes[30];	/*  used for TWO_K to pass on inodes */
	auto int	ax=0;		/* index into args list to mkfs */
	auto char	*args[12];	/* give ROOM for 12 ARGUMENTS */
	auto char	blksizstr[10];	/* block size as string */
	auto char	gapsizstr[10];	/* gap size as string */
	reg  char	**dbugv;	/* used when debug - xflag */

	
	args[ax++] = strcpy(mkfs, S5_PATH);
	
	/* if block size given then pass on to mkfs */
	if ( blksiz != 0 )
	{
		args[ax++] = "-b";
		sprintf(blksizstr, "%d", blksiz);
		args[ax++] = blksizstr;
	}

	/* get device file for partition */
	args[ax++] = devname;

	/* get size of partition in blocks */
	args[ax++] = strcpy(partnsiz, itoa((ulong) part->p_size));

	/* for TWO_K, mkfs makes too  few inodes, specify double (same as in ONE_K) */
	if( blksiz == TWO_K )
	{
		sprintf(inodes,":%d", part->p_size/8);
		strcat(partnsiz, inodes);
	}

	/* if gapsize is not specified, set according to logical blocksize, if any */
	if (gapsiz == 0)
	{
		switch( blksiz)  {
		case 512:
			args[ax++] = "8";
			break;

		case ONE_K:
			args[ax++] = "10";
			break;

		case TWO_K:	
			args[ax++] = "12";
			break;

		default:
			args[ax++] = "10";
			break;
		}

	}
	else {
		sprintf( gapsizstr, "%d", gapsiz );
		args[ax++] = gapsizstr;
	}


	args[ax++] = strcpy(cylsize, 
			  itoa(pdsector->pdinfo.tracks * pdsector->pdinfo.sectors));
	args[ax++] = 0;

	if (xflag) {

		(void) write(STDOUT, "++", 2);

		for (dbugv = args; *dbugv; ++dbugv) {
			(void) write(STDOUT, " ", 1);
			(void) write(STDOUT, *dbugv, (uint) strlen(*dbugv));
		}
		(void) write(STDOUT, "\n", 1);
	}

	if (nflag)
		exit(0);

	(void) execv(args[0], args);

	fatal(devname, syserr());
}

/*
 * bfs_child()
 *
 * This is the forked child. Execute mkfs(1M) for bfs filesystem type with
 * appropriate parameters. Any blocksize passed is not used. NEVER returns.
 */
static void
bfs_child(devname, pdsector, part)
reg char		*devname;	/* device file name */
reg struct pdsector	*pdsector;	/* for device info- size etc */
reg struct partition	*part;		/* for partition info */
{
	auto char	partnsiz[20];	/* size of partition */
	auto char	mkfs[25];	/* path name for fs specific mkfs */
	auto char	*args[12];	/* give ROOM for 12 ARGUMENTS */
	auto int	ax=0;		/* index into args list */
	reg  char	**dbugv;	/* used when debug - xflag */

	/*  get the mkfspath */
	args[ax++] = strcpy(mkfs, BFS_PATH);
	
	/* get device file for partition */
	args[ax++] = devname;

	/* get size of partition in blocks */
	args[ax++] = strcpy(partnsiz, itoa((ulong) part->p_size));

	args[ax++] = 0;

	if (xflag) {

		(void) write(STDOUT, "++", 2);

		for (dbugv = args; *dbugv; ++dbugv) {
			(void) write(STDOUT, " ", 1);
			(void) write(STDOUT, *dbugv, (uint) strlen(*dbugv));
		}
		(void) write(STDOUT, "\n", 1);
	}

	if (nflag)
		exit(0);

	(void) execv(args[0], args);

	fatal(devname, syserr());
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
 * itoa()
 *
 * Convert an integer to a null-terminated ASCII
 * string. Returns a pointer to the static result.
 */
static char *
itoa(number)
reg ulong	number;
{
	reg char	*idx;
	static char	buf[64];

	idx = buf + sizeof(buf);
	*--idx = '\0';
	do {
		*--idx = '0' + number % 10;
		number /= 10;
	} while (number);
	return (idx);
}

/*
 * readpd()
 *
 * Read physical device information.
 */
static int
readpd(fd, devname, pdsector)
int		fd;
reg char	*devname;
struct pdsector	*pdsector;
{
	struct io_arg	args;

	args.sectst = 0;
	args.memaddr = (unsigned long) pdsector;
	args.datasz = sizeof(struct pdsector);
	if (ioctl(fd, V_PDREAD, &args) < 0)
		return (warn(devname, syserr()));
	if (args.retval == V_BADREAD)
		return (warn(devname, "Unable to read device information sector"));
	return (0);
}

/*
 * readvtoc()
 *
 * Read a partition map.
 */
static int
readvtoc(fd, devname, pdinfo, vtoc)
int		fd;
char		*devname;
struct pdinfo	*pdinfo;
struct vtoc	*vtoc;
{
	struct io_arg	args;

	args.sectst = pdinfo->logicalst + 1;
	args.memaddr = (unsigned long) vtoc;
	args.datasz = sizeof(struct vtoc);
	if (ioctl(fd, V_PREAD, &args) < 0)
		return (warn(devname, syserr()));
	if (args.retval == V_BADREAD)
		return (warn(devname, "Unable to read VTOC"));
	if (vtoc->v_sanity != VTOC_SANE)
		return (warn(devname, "No valid VTOC on device"));
	return (0);
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
	static char	after[] = " [ -v ] raw_disk_device ...\n";

	(void) write(STDERR, before, (uint) strlen(before));
	(void) write(STDERR, myname, (uint) strlen(myname));
	(void) write(STDERR, after, (uint) strlen(after));
	exit(1);
}

/*
 * vmkfs()
 *
 * Get device information and make a filesystem. Quietly skips
 * unmountable and zero-length partitions.
 */
static int
vmkfs(devname, fstype, blksiz, gapsiz)
char		*devname;
char		*fstype;
int		blksiz;
int		gapsiz;
{
	reg struct partition	*part;
	reg int		fd;
	reg pid_t	pid;
	reg int		ok;
	auto int	status;
	struct stat	sb;
	struct vtoc	vtoc;
	struct pdsector	pdsector;

	if (stat(devname, &sb) < 0)
		return (warn(devname, syserr()));

	if ((sb.st_mode & S_IFMT) != S_IFCHR)
		return (warn(devname, "Not a raw device"));

	if ((fd = open(devname, O_RDONLY)) < 0)
		return (warn(devname, syserr()));

	ok = (readpd(fd, devname, &pdsector) == 0
	    && readvtoc(fd, devname, &pdsector.pdinfo, &vtoc) == 0);

	(void) close(fd);

	if (!ok)
		return(-1);

	part = &vtoc.v_part[idslice(minor(sb.st_rdev))];

	if (part->p_size == 0) 
		return( warn(devname, "Size 0: Filesystem not made") );	

	if (part->p_flag & V_UNMNT) 
		return( warn(devname, "Unmountable: Filesystem not made") );

	if ((pid = fork()) < (pid_t)0)
		return (warn(devname, syserr()));

	if (pid == (pid_t)0) {

		if (strcmp(fstype, "s5") == 0)
			s5_child(devname, &pdsector, part, blksiz, gapsiz);

		else if (strcmp(fstype, "bfs") == 0)
			bfs_child(devname, &pdsector, part);

		else
			fatal(fstype, "Unknown filesystem type passed");
	}

	while (wait(&status) != pid)
		;

	return (status ? warn(devname, "mkfs failed") : 0);
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
