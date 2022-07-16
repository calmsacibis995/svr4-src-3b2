/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-3b2:wtvtoc.c	1.2"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vtoc.h>

struct vtoc	boot_vtoc;	/* Save current vtoc boot data */

#define	nel(a)	(sizeof(a)/sizeof(*(a)))	/* Number of array elements */
#define ICDBLKONE 0	/* The first logical block for ICD */
#define ICDBLKSZ  512	/* The block size for ICD */

unsigned long logical;

/*
 * External functions.
 */
extern int errno;
void	exit();
char	*strncpy();
int	atoi();

/*
 * Internal functions.
 */
void 	display();
void	initial();
void	insert();
void	load();
int	pread();
void	pwrite();
void	usage();
void	validate();

/*
 * Static variables.
 */
static char	*tsize;		/* total device size */
static char	*delta;			/* Incremental update */
static short	iflag;			/* Prints VTOC w/o updating */

main(argc, argv)
int	argc;
char	**argv;
{
	int		fd;
	int		c;
	char		*dfile, *vname;
	struct vtoc	bufvtoc;
	struct stat	statbuf;
	extern char	*optarg;
	extern int	optind;

	dfile = NULL;
	vname = NULL;
	while ((c = getopt(argc, argv, "t:d:in:s:")) != EOF)
		switch (c) {
		case 't':
		 	tsize=optarg;
			break;
		case 'd':
			delta = optarg;
			break;
		case 'i':
			++iflag;
			break;
		case 'n':
			vname = optarg;
			break;
		case 's':
			dfile = optarg;
			break;
		default:
			usage();
		}

	if (argc - optind != 1)
		usage();
	if (stat(argv[optind], (struct stat *) &statbuf) == -1) {
		(void) fprintf(stderr, "wtvtoc:  Cannot stat device %s\n",argv[optind]);
		exit(1);
	}

	if ((fd = open(argv[optind], O_RDWR)) < 0) {
		(void) fprintf(stderr, "wtvtoc:  Cannot open device %s\n",argv[optind]);
		exit(1);
	}

	logical = ICDBLKONE;

	if (delta) {
		if(pread(fd, logical + 1, sizeof(struct vtoc), (char *) &bufvtoc) == 0)
			exit(1);
		if (bufvtoc.v_sanity != VTOC_SANE) {
			(void) fprintf(stderr, "%s: Invalid VTOC\n", argv[optind]);
			exit(1);
		}
		insert(delta, &bufvtoc);
		validate(&bufvtoc);
		pwrite(fd, logical + 1, sizeof(struct vtoc), (char *) &bufvtoc);
		exit(0);
	}

	initial(vname, &bufvtoc);

	if (dfile)
		load(dfile, &bufvtoc);

	if (iflag) {
		display(&bufvtoc, argv[optind]);
		exit(0);
	}

	/*
	*  Read the current VTOC. If SANE, copy the boot
	*  information from the first three words into
	*  the new VTOC information.
	*/
	if(pread(fd,logical + 1, sizeof(struct vtoc), (char *) &boot_vtoc) != 0 && (boot_vtoc.v_sanity == VTOC_SANE))
	{
		bufvtoc.v_bootinfo[0] = boot_vtoc.v_bootinfo[0];
		bufvtoc.v_bootinfo[1] = boot_vtoc.v_bootinfo[1];
		bufvtoc.v_bootinfo[2] = boot_vtoc.v_bootinfo[2];
	}

	validate(&bufvtoc);
	pwrite(fd, logical + 1, sizeof(struct vtoc), (char *) &bufvtoc);
	/*
		Shut system down after writing a new vtoc to disk
		This is used during installation of core floppies.
	*/

	(void) printf("wtvtoc:  New volume table of contents now in place.\n");
	exit(0);
}

/*
 * display ()	
 *
 * display contents of VTOC without writing it to disk
 */
static void
display(buf, device)
struct vtoc	*buf;
char 		*device;
{
	register int	i;

	/* PRINT OUT THE VTOC */
	printf("* %s default partition map\n",device);
	if(*buf->v_volume)
		printf("* Volume Name:  %s\n",buf->v_volume);
	printf("*\n");
	printf("* Flags:\n");
	printf("*   1:  unmountable\n");
	printf("*  10:  read-only\n");
	printf("*\n");
	printf("\n* Partition    Tag     Flag	    First Sector    Sector Count\n");
	for(i=0;i<16;i++)	{
		if(buf->v_part[i].p_size > 0) 
			printf("    %d		%d	0%x		%d		%d\n",i,buf->v_part[i].p_tag, buf->v_part[i].p_flag, buf->v_part[i].p_start, buf->v_part[i].p_size);
	}
	exit(0);
}

/*
 * initial()
 *
 * Initialize a new VTOC.
 */
static void
initial(volume, vtoc)
char			*volume;
register struct vtoc	*vtoc;
{
	register int	i;

	for(i = 0; i < nel(vtoc->v_bootinfo); ++i)
		vtoc->v_bootinfo[i] = 0;
	vtoc->v_sanity = VTOC_SANE;
	vtoc->v_version = V_VERSION;
	for(i = 0; i < nel(vtoc->v_volume); ++i)
		vtoc->v_volume[i] = '\0';
	if (volume)
		(void) strncpy(vtoc->v_volume, volume, nel(vtoc->v_volume));
	vtoc->v_sectorsz = ICDBLKSZ;
	vtoc->v_nparts = V_NUMPAR;
	for(i = 0; i < nel(vtoc->v_reserved); ++i)
		vtoc->v_reserved[i] = 0;
	for(i = 0; i < nel(vtoc->v_part); ++i) {
		vtoc->v_part[i].p_tag = 0;
		vtoc->v_part[i].p_flag = V_UNMNT;
		vtoc->v_part[i].p_start = 0;
		vtoc->v_part[i].p_size = 0;
	}
}

/*
 * insert()
 *
 * Insert a change into the VTOC.
 */
static void
insert(data, vtoc)
char		*data;
struct vtoc	*vtoc;
{
	auto int	part;
	auto int	tag;
	auto int	flag;
	auto int	start;
	auto int	size;

	if (sscanf(data, "%x:%d:%x:%d:%d",
	    &part, &tag, &flag, &start, &size) != 5) {
		(void) fprintf(stderr, "Delta syntax error on \"%s\"\n", data);
		exit(1);
	}
	if (part >= V_NUMPAR) {  /* Decimal 10 - 15 look like 16 - 21 */
		part = part - 6;
		if (part >= V_NUMPAR) {
			(void) fprintf(stderr,
			    "Error in data %s: No such partition %x\n",
			    data, part+6);
			exit(1);
		}
	}
	vtoc->v_part[part].p_tag = tag;
	vtoc->v_part[part].p_flag = flag;
	vtoc->v_part[part].p_start = start;
	vtoc->v_part[part].p_size = size;
}

/*
 * load()
 *
 * Load VTOC information from a datafile.
 */
static void
load(dfile, vtoc)
char		*dfile;
struct vtoc	*vtoc;
{
	FILE		*dstream;
	auto int	part;
	auto int	tag;
	auto int	flag;
	auto int	start;
	auto int	size;
	char		line[256];

	if (strcmp(dfile, "-") == 0)
		dstream = stdin;
	else if ((dstream = fopen(dfile, "r")) == NULL) {
		(void) fprintf(stderr, "Cannot open file %s\n", dfile);
		exit(1);
	}
	while (fgets(line, sizeof(line) - 1, dstream)) {
		if (line[0] == '\0' || line[0] == '\n' || line[0] == '*' || line[0] == '#')
			continue;
		line[strlen(line) - 1] = '\0';
		if (sscanf(line, "%x %d %x %d %d",
		    &part, &tag, &flag, &start, &size) != 5) {
			(void) fprintf(stderr, "%s: Syntax error on \"%s\"\n",
			    dfile, line);
			exit(1);
		}
		if (part >= V_NUMPAR) {  /* Decimal 10 - 15 look like 16 - 21 */
			part = part - 6;
			if (part >= V_NUMPAR) {
				(void) fprintf(stderr,
				    "Error in datafile %s: No such partition %x\n",
				    dfile, part+6);
				exit(1);
			}
		}
		vtoc->v_part[part].p_tag = tag;
		vtoc->v_part[part].p_flag = flag;
		vtoc->v_part[part].p_start = start;
		vtoc->v_part[part].p_size = size;
	}
	if (dstream != stdin && fclose(dstream) == EOF) {
		(void) fprintf(stderr, "I/O error reading datafile %s\n", dfile);
		exit(1);
	}
	for (part=0; part < V_NUMPAR; part++)
		vtoc->timestamp[part] = (time_t)0;
}

static
pread(fd, block, len, buf)
int		fd;
unsigned long	block;
unsigned long	len;
char		*buf;
{
	lseek(fd, block * ICDBLKSZ, 0);
	if (read(fd, buf, len) < 0) {
		(void) fprintf(stderr, "wtvtoc: Cannot read block %lu\n", block);
		exit(1);
	}
}

static void
pwrite(fd, block, len, buf)
int		fd;
unsigned long	block;
unsigned long	len;
char		*buf;
{
	lseek(fd, block * ICDBLKSZ, 0);
	if(write(fd, buf, len) < 0) {
		if (errno == EPERM)  {
			(void) fprintf(stderr, "wtvtoc: Must have super-user privileges \n");
		}else  {
			(void) fprintf(stderr, "wtvtoc: Cannot write block %lu  errno = %d\n", block, errno);
		}
		exit(1);
	}
}

static void
usage()
{
	(void) fprintf(stderr, "\
Usage:	wtvtoc [ -i ] [ -n volumename ] [ -d insert_data ]\n\
	[ -s datafile ] -t total_size fs_file\n");
	exit(2);
}

/*
 * validate()
 *
 * Validate the new VTOC.
 */
static void
validate(vtoc)
struct vtoc	*vtoc;
{
	register int	i, j;
	int	fullsz, total, endsect;
	int	isize, istart, jsize, jstart;

	fullsz = atoi(tsize);
	for(i=0, total=0; i<V_NUMPAR; i++) {
		if(i==6) {
			if(vtoc->v_part[6].p_size <= 0) {
				(void) fprintf(stderr, "\
wtvtoc: Partition 6 specifies the full disk and cannot have a size of 0.\n\
\tThe full disk capacity is %lu sectors.\n", fullsz);
			exit(1);
			}
		}
		if(vtoc->v_part[i].p_size == 0)
			continue;	/* Undefined partition */
		if (vtoc->v_part[i].p_start > fullsz
		  || vtoc->v_part[i].p_start
		    + vtoc->v_part[i].p_size > fullsz) {
			(void) fprintf(stderr, "\
wtvtoc: Partition %d specified as %lu sectors starting at %lu\n\
\tdoes not fit. The full disk contains %lu sectors.\n",
			    i, vtoc->v_part[i].p_size,
			    vtoc->v_part[i].p_start, fullsz);
			exit(1);
		}

		if (i!=6)	{
			total += vtoc->v_part[i].p_size;
			if(total > fullsz)	{
				(void) fprintf(stderr, "\
wtvtoc: Total of %lu sectors specified within partitions\n\
\texceeds the disk capacity of %lu sectors.\n",
				    fullsz);
				exit(1);
			}
			for(j=0; j<V_NUMPAR; j++)	{
				isize = vtoc->v_part[i].p_size;
				jsize = vtoc->v_part[j].p_size;
				istart = vtoc->v_part[i].p_start;
				jstart = vtoc->v_part[j].p_start;
				if((i!=j) && (j!=6) && (isize != 0) && (jsize != 0))	{
					endsect = jstart + jsize -1;
					if((jstart <= istart) && (istart <= endsect))	{
						(void) fprintf(stderr,"\
wtvtoc: Partition %d overlaps partition %d. Overlap is allowed\n\
\tonly on partition 6 (the full disk).\n",
						    i, j);
						exit(1);
					}
				}
			}
		}
	}
}
