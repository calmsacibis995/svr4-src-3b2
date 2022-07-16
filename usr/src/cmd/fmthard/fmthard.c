/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fmthard:fmthard.c	1.5.2.27"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/id.h>
#include <sys/open.h>
#include <sys/uadmin.h>
#include <sys/vtoc.h>


struct vtoc	boot_vtoc;	/* Save current vtoc boot data */

#define	nel(a)	(sizeof(a)/sizeof(*(a)))	/* Number of array elements */

/* defines for single 32MB hard disk */
#define SWAP0_32  	6020 
#define ROOT0_32  	10106
#define STAND0_32  	3400
#define USR0_32  	27000
#define VAR0_32   	4000

/* defines for single 72MB hard disk */
#define SWAP0_72 	14000 
#define ROOT0_72  	17000 
#define STAND0_72  	5500
#define USR0_72   	96000
#define VAR0_72   	10000

/* defines for first 32MB disk in two disk configuration */
#define SWAP1_32  	14000 
#define ROOT1_32  	18000 
#define STAND1_32  	5500

/* defines for second 32MB disk in two disk configuration */
#define USR2_32   	55000
#define VAR2_32   	6000

/* defines for first 72MB disk in two disk configuration */
#define SWAP1_72  	14000 
#define ROOT1_72  	18000 
#define STAND1_72  	5500

/* defines for second 72MB disk in two disk configuration */
#define USR2_72   	97000
#define VAR2_72   	10000


/* disk sizes */
#define SIZE_10	 21888
#define SIZE_32	 62550
#define SIZE_72	149526

#define BOOTSIZE   100
#define HOME_MIN  2000		/* minimum home size (used by minval function) */

/* Default Partition Assignments */
#define P_ROOT		0		/* Root partition */
#define P_SWAP		1		/* Swap partition */
#define P_USR 		2		/* Usr partition */
#define P_STAND		3		/* Stand partition */
#define P_BACKUP	6		/* full disk */
#define P_BOOT		7		/* Boot partition */
#define P_VAR 		8		/* Var partition */
#define P_HOME		9		/* Home partition */
#define P_HOME2		10		/* left out space on second disk */

#define PDBLKNO		0x00
#define MFGBLKNO	0x01
#define DEFBLKNO	0x02
#define RELBLKNO	0x03
#define NUMOFREL	0x01

#define BLKSZ		512


union datatype	{
	struct idsector0 o_sect0;
	struct pdsector sect0;
} datastruct;

unsigned long logical;
unsigned long size_newdsk;
unsigned long cyl_blks;
unsigned long swap;
unsigned long boot;
unsigned long root;
unsigned long stand;	
unsigned long usr;
unsigned long var;
unsigned long home;
long lsize, ssize, sroot, lroot, sswap, lswap, part_size; 
long lusr, susr, lvar, svar, lstand, sstand;

/*
 * External functions.
 */
extern int errno;
void	exit();
char	*strchr();
char	*strcpy();
char	*strncpy();

/*
 * Internal functions.
 */
void    coredsk0();		/* to partition a single disk system */
void    coredsk1();		/* to partition first of two disk system */
void    coredsk2();		/* to partition second of two disk system */
void    dftdsk();		/* to partition any disk */
void 	display();
void	initial();
void	insert();
void	load();
void	minval();
void	pt_assgn();		/* partition and tag assignments */
void	mkfs();
void	pread();
void	pwrite();
int	round();
void	tinyvtoc();
void	upgrade();
void	usage();
void	validate();
void	values();

/*
 * Static variables.
 */
static char	*cflag;			/* Core system disk */
static char	*delta;			/* Incremental update */
static short	iflag;			/* Prints VTOC w/o updating */
static short	aflag;			/* Prints default partitions & tags */
static short	pflag;			/* Just print the vtoc */
static short	qflag;			/* Just check for a 1.0-format disk */
static short	rflag;			/* Just print minimum partition sizes */
static short	tflag;			/* Set up minimum partition size in VTOC */
static short	uflag;			/* Exit to firmware after writing  */
					/* new vtoc and reboot. Used during */
					/* installation of core floppies */
static char	*uboot = "boot";
static char	*ufirm = "firm";
static short	vflag;			/* Verbose */

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
	while ((c = getopt(argc, argv, "ac:d:u:in:qrs:tvp")) != EOF)
		switch (c) {
		case 'a':
			++aflag;
			break;
		case 'c':
		 	cflag=optarg;
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
		case 'p':
			++pflag;
			break;
		case 'q':
			++qflag;
			break;
		case 'r':
			++rflag;
			break;
		case 's':
			dfile = optarg;
			break;
		case 't':
			++tflag;
			break;
		case 'v':
			++vflag;
			break;
		case 'u':
			if (strcmp(uboot,optarg) == 0)
				++uflag;
			else if (strcmp(ufirm,optarg) == 0)
				uflag = 2;

			break;
		default:
			usage();
		}


	if (argc - optind != 1)
		usage();

	if (stat(argv[optind], (struct stat *) &statbuf) == -1) {
		(void) fprintf(stderr, "fmthard:  Cannot stat device %s\n",argv[optind]);
		exit(1);
	}

	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		(void) fprintf(stderr, "fmthard:  Must specify a raw device.\n");
		exit(1);
	}	

	if ((fd = open(argv[optind], O_RDWR)) < 0) {
		(void) fprintf(stderr, "fmthard:  Cannot open device %s\n",argv[optind]);
		exit(1);
	}

	/* READ SECTOR 0 */
	pread(fd, PDBLKNO, BLKSZ, (char *) &datastruct);
	/*
	 *  Verify the PD sector sanity.
	 */
	if (datastruct.sect0.pdinfo.sanity != VALID_PD) {
		(void) fprintf(stderr, "%s: Invalid PD Sector\n", argv[optind]);
		exit(1);
	}

	if (qflag)
		exit(datastruct.o_sect0.reserved == 0xfeedbeef ? 0 : 1);

	/*
	 * Upgrade a Release 1.0 disk. The alternate-case pwrite() is
	 * needed to initialize the driver in the case of a "new" disk.
	 */
	if (datastruct.o_sect0.reserved == 0xfeedbeef)
		upgrade(fd, &datastruct);
	else
		pwrite(fd, PDBLKNO, BLKSZ, (char *) &datastruct);

	logical = datastruct.sect0.pdinfo.logicalst;

	cyl_blks = datastruct.sect0.pdinfo.sectors * 
		   datastruct.sect0.pdinfo.tracks;

	size_newdsk = datastruct.sect0.pdinfo.sectors * 
		      datastruct.sect0.pdinfo.tracks * 
		      datastruct.sect0.pdinfo.cyls -
		      logical - cyl_blks ;

	if (rflag) {
		minval();	/* minumum partition sizes */
		exit(0);
	}

	if (aflag) {
		pt_assgn();	/* partition and tag assignments */
		exit(0);
	}

	if (delta) {
		pread(fd, logical + 1, sizeof(struct vtoc), (char *) &bufvtoc);
		if (bufvtoc.v_sanity != VTOC_SANE) {
			(void) fprintf(stderr, "%s: Invalid VTOC\n", argv[optind]);
			exit(1);
		}
		insert(delta, &bufvtoc);
		validate(&datastruct.sect0.pdinfo, &bufvtoc);
		pwrite(fd, logical + 1, sizeof(struct vtoc), (char *) &bufvtoc);
		exit(0);
	} 
	else {
		initial(vname, &datastruct.sect0.pdinfo, &bufvtoc);
	}

	if (tflag) {
		tinyvtoc(size_newdsk, &bufvtoc);

		/*
		*  Read the current VTOC. If SANE, copy the boot
		*  information from the first three words into
		*  the new VTOC information.
		*/
		pread(fd, logical + 1, sizeof(struct vtoc), (char *) &boot_vtoc);
		if (boot_vtoc.v_sanity == VTOC_SANE)
		{
			bufvtoc.v_bootinfo[0] = boot_vtoc.v_bootinfo[0];
			bufvtoc.v_bootinfo[1] = boot_vtoc.v_bootinfo[1];
			bufvtoc.v_bootinfo[2] = boot_vtoc.v_bootinfo[2];
		}

		validate(&datastruct.sect0.pdinfo, &bufvtoc);
		pwrite(fd, logical + 1, sizeof(struct vtoc), (char *) &bufvtoc);
		exit(0);
	}

	if (dfile)
		load(dfile, &datastruct.sect0.pdinfo, &bufvtoc);

	else if(cflag)  

		switch (*cflag)  {

		case '0':			/* single disk */
			coredsk0(size_newdsk, cyl_blks, &bufvtoc);
			break;
		case '1':			/* first of two disks */
			coredsk1(size_newdsk, cyl_blks, &bufvtoc);
			break;
		case '2':			/* second of two disks */
			coredsk2(size_newdsk, cyl_blks, &bufvtoc);
			break;
		default:
			fprintf(stderr,
			     "fmthard: argument for -c must be 0, 1 or2.\n");
			exit(1);
		}
	 else 
		dftdsk (size_newdsk, cyl_blks, &bufvtoc);


	if (iflag) {
		display(&bufvtoc, argv[optind]);
		exit(0);
	}
	if (pflag) {
		values();
		exit(0);
	}

	/*
	*  Read the current VTOC. If SANE, copy the boot
	*  information from the first three words into
	*  the new VTOC information.
	*/
	pread(fd,logical + 1, sizeof(struct vtoc), (char *) &boot_vtoc);
	if (boot_vtoc.v_sanity == VTOC_SANE)
	{
		bufvtoc.v_bootinfo[0] = boot_vtoc.v_bootinfo[0];
		bufvtoc.v_bootinfo[1] = boot_vtoc.v_bootinfo[1];
		bufvtoc.v_bootinfo[2] = boot_vtoc.v_bootinfo[2];
	}

	validate(&datastruct.sect0.pdinfo, &bufvtoc);
	pwrite(fd, logical + 1, sizeof(struct vtoc), (char *) &bufvtoc);
	/*
		Shut system down after writing a new vtoc to disk
		This is used during installation of core floppies.
	*/
	if (uflag == 1)
		uadmin(A_REBOOT,AD_BOOT,0);
	else if (uflag == 2)
		uadmin(A_REBOOT,AD_IBOOT,0);

	(void) printf("fmthard:  New volume table of contents now in place.\n");

	exit(0);
}

/*
 *  coredsk0()    
 *
 * Sets up the VTOC for a single core system disk.  Contains partitions for 
 * boot, swap, root, stand, var, usr, home on a single core disk.) (and stand)
 */
static void
coredsk0(size_newdsk, cyl_blks, vtoc)
unsigned long	size_newdsk;
unsigned long	cyl_blks;
struct 	vtoc	*vtoc;
{
	auto int 	part;
	auto int	tag;
	auto int 	flag;
	auto long 	start;
	auto long 	size;
	auto long 	remainder;
	long		last_used;
	
	if (size_newdsk < SIZE_32) {

		(void) fprintf(stderr, 
  			"Error:  Single disk w/smaller than 32M is not supported.");

		exit(1);
	}

	else if (size_newdsk > SIZE_72) {

		swap = SWAP0_72 * (size_newdsk/SIZE_72);
		root = ROOT0_72 * (size_newdsk/SIZE_72);
		stand = STAND0_72 * (size_newdsk/SIZE_72);
		usr  = USR0_72 * (size_newdsk/SIZE_72);
		var  = VAR0_72 * (size_newdsk/SIZE_72); 
	}

	else {
 		ssize = SIZE_32;
		lsize = SIZE_72;
		sswap = SWAP0_32;
		lswap = SWAP0_72;
		sroot = ROOT0_32;
		lroot = ROOT0_72;
		sstand = STAND0_32;
		lstand = STAND0_72;
		susr = USR0_32;
		lusr = USR0_72;
		svar = VAR0_32;
		lvar = VAR0_72;

		swap =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lswap - sswap)  + sswap;

		root =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lroot - sroot)  + sroot;

		stand =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lstand - sstand)  + sstand;

		usr =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lusr - susr)  + susr;

		var =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lvar - svar)  + svar;
    	}


	boot = BOOTSIZE;
	last_used = boot;
	swap = round (swap, last_used, cyl_blks);
	last_used = boot + swap;
	root = round (root, last_used, cyl_blks);
	last_used += root;
	stand = round (stand, last_used, cyl_blks);
	last_used += stand;
	usr = round (usr, last_used, cyl_blks);
	last_used += usr;
	var = round (var, last_used, cyl_blks);
	last_used += var;

	remainder = size_newdsk - last_used;


	for (part=0; part < 16; ++part) {

		/* initialize values */
		tag = 0;
		flag = V_UNMNT;
		start = 0L;
		size = 0L;

		switch (part) {
		case P_ROOT:
			tag = V_ROOT;
			flag = 00;
			start = boot + swap;
			size = root;
			break;

		case P_SWAP:
			tag = V_SWAP;
			start = boot;
			size = swap;
			break;

		case P_USR:
			tag = V_USR;
			flag = 00;
			start = boot+swap+root+stand; 
			size = usr;
			break;
			
		case P_STAND:
			tag = V_STAND;
			flag = 00;
			start = boot+swap+root;
			size = stand;
			break;

		case P_BACKUP:
			tag=V_BACKUP;
			size = size_newdsk;
			break;

		case P_BOOT:
			tag=V_BOOT;
			size = boot;
			break;

		case P_VAR:
			tag = V_VAR;
			flag = 00;
			start = boot+swap+root+stand+usr; 
			size = var;
			break;
		case P_HOME:
			tag = V_HOME;
			flag = 00;
			start = last_used; 
			home = remainder;	/* for global var home */
			size = home;
			break;
		}
		vtoc ->v_part[part].p_tag = tag;
		vtoc ->v_part[part].p_flag = flag;
		vtoc ->v_part[part].p_start = start;
		vtoc ->v_part[part].p_size = size;
	}
}
 


/*
 *  coredsk1()    
 *
 * Sets up VTOC for first disk of dual disk system.  
 * Contains partitions for boot, swap, root, stand and home. 
 */
static void
coredsk1(size_newdsk, cyl_blks, vtoc)
unsigned long	size_newdsk;
unsigned long	cyl_blks;
struct vtoc	*vtoc;
{
	auto int 	part;
	auto int	tag;
	auto int 	flag;
	auto long 	start;
	auto long 	size;
	auto long 	remainder;
	long		last_used;

	if (size_newdsk < SIZE_32) {

		(void) fprintf(stderr, 
  			"Error:  Single disk w/smaller than 32M is not supported.");

		exit(1);
	}

	else if (size_newdsk > SIZE_72) {

		swap = SWAP1_72 * (size_newdsk/SIZE_72);
		root = ROOT1_72 * (size_newdsk/SIZE_72);
		stand = STAND1_72 * (size_newdsk/SIZE_72);
	}

	else {
 		ssize = SIZE_32;
		lsize = SIZE_72;
		sswap = SWAP1_32;
		lswap = SWAP1_72;
		sroot = ROOT1_32;
		lroot = ROOT1_72;
		sstand = STAND1_32;
		lstand = STAND1_72;

		swap =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lswap - sswap)  + sswap;

		root =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lroot - sroot)  + sroot;

		stand =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lstand - sstand)  + sstand;
    	}

	boot = BOOTSIZE;
	last_used = boot;
	swap = round (swap, last_used, cyl_blks);
	last_used = boot + swap;
	root = round (root, last_used, cyl_blks);
	last_used += root;
	stand = round (stand, last_used, cyl_blks);
	last_used += stand;

	remainder = size_newdsk - last_used;

	for (part=0; part < 16; ++part) {

		/* initialize values */
		tag = 0;
		flag = V_UNMNT;
		start = 0L;
		size = 0L;

		switch (part) {
		case P_ROOT:
			tag = V_ROOT;
			flag = 00;
			start = boot + swap;
			size = root;
			break;
		case P_SWAP:
			tag = V_SWAP;
			start = boot;
			size = swap;
			break;
		case P_STAND:
			tag = V_STAND;
			flag = 00;
			start = boot+swap+root;
			size = stand;
			break;

		case P_BACKUP:
			tag=V_BACKUP;
			size = size_newdsk;
			break;

		case P_BOOT:
			tag=V_BOOT;
			size = boot;
			break;

		case P_HOME:
			tag = V_HOME;
			flag = 00;
			start = last_used; 
			home = remainder;	/* for global var home */
			size = home;
			break;
		}
		vtoc ->v_part[part].p_tag = tag;
		vtoc ->v_part[part].p_flag = flag;
		vtoc ->v_part[part].p_start = start;
		vtoc ->v_part[part].p_size = size;
	}
}

/*
 *  coredsk2()    
 *
 * Partitions a noncore disk or the second disk of dual core system. 
 * Contains partitions for boot and usr.
 */
static void
coredsk2 (size_newdsk, cyl_blks, vtoc)
unsigned long	size_newdsk;
unsigned long	cyl_blks;
struct vtoc	*vtoc;
{
	auto int	part;
	auto int	tag;
	auto int 	flag;
	auto long 	start;
	auto long 	size;
	auto long 	remainder;
	long		last_used;

	if (size_newdsk < SIZE_32) {

		(void) fprintf(stderr, 
  			"Error:  Second disk w/smaller than 32M is not supported.");

		exit(1);
	}

	else if (size_newdsk > SIZE_72) {

		usr  = USR2_72 * (size_newdsk/SIZE_72);
		var  = VAR2_72 * (size_newdsk/SIZE_72); 
	}

	else {
 		ssize = SIZE_32;
		lsize = SIZE_72;
		susr = USR2_32;
		lusr = USR2_72;
		svar = VAR2_32;
		lvar = VAR2_72;

		usr =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lusr - susr)  + susr;

		var =  ( (size_newdsk - ssize) / (lsize -ssize) ) * 
				(lvar - svar)  + svar;
    	}

	boot = cyl_blks;
	last_used = boot;
	usr = round (usr, last_used, cyl_blks);
	last_used += usr;
	var = round (var, last_used, cyl_blks);
	last_used += var;

	remainder = size_newdsk - last_used;

	for (part=0; part<16; ++part) {

		tag = 0;
		flag = V_UNMNT;
		start = 0L;
		size = 0L;

		switch (part) {

		case P_USR:
			tag = V_USR;
			flag = 00;
			start = boot;
			size = usr;
			break;

		case P_BACKUP:
			tag=V_BACKUP;
			size = size_newdsk;
			break;

		case P_BOOT:
			tag=V_BOOT;
			size = boot;
			break;

		case P_VAR:
			tag = V_VAR;
			flag = 00;
			start = boot+usr; 
			size = var;
			break;

		case P_HOME2:
			flag = 00;
			start = last_used;
			size = remainder;

			break;
		}
		vtoc ->v_part[part].p_tag = tag;
		vtoc ->v_part[part].p_flag = flag;
		vtoc ->v_part[part].p_start = start;
		vtoc ->v_part[part].p_size = size;
	}
}


/*
 *  dftdsk()    
 *
 * Partitions a noncore disk or the second disk of dual core system. 
 * Contains partitions for boot and usr.
 */
static void
dftdsk (size_newdsk, cyl_blks, vtoc)
unsigned long	size_newdsk;
unsigned long	cyl_blks;
struct vtoc	*vtoc;
{
	auto int	part;
	auto int	tag;
	auto int 	flag;
	auto long 	start;
	auto long 	size;

	for (part=0; part<16; ++part) {

		tag = 0;
		flag = V_UNMNT;
		start = 0L;
		size = 0L;

		switch (part) {

		case P_BACKUP:
			tag=V_BACKUP;
			size = size_newdsk;
			break;

		case P_BOOT:
			tag=V_BOOT;
			size = cyl_blks;
			break;

		case P_HOME2:
			flag = 00;
			start = cyl_blks;
			size = size_newdsk - cyl_blks;
			break;
		}
		vtoc ->v_part[part].p_tag = tag;
		vtoc ->v_part[part].p_flag = flag;
		vtoc ->v_part[part].p_start = start;
		vtoc ->v_part[part].p_size = size;
	}
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
	printf("* Dimensions:\n");
	printf("*     %d bytes/sector\n",datastruct.sect0.pdinfo.bytes);
	printf("*      %d sectors/track\n",datastruct.sect0.pdinfo.sectors);
	printf("*       %d tracks/cylinder\n",datastruct.sect0.pdinfo.tracks);
	printf("*     %d cylinders\n",datastruct.sect0.pdinfo.cyls);
	printf("*     %d accessible cylinders\n",(datastruct.sect0.pdinfo.cyls - 1 - (logical/cyl_blks) ) );
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
initial(volume, pdinfo, vtoc)
char			*volume;
struct pdinfo		*pdinfo;
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
	vtoc->v_sectorsz = pdinfo->bytes;
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
load(dfile, pdinfo, vtoc)
char		*dfile;
struct pdinfo	*pdinfo;
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
		if (line[0] == '\0' || line[0] == '\n' || line[0] == '*')
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

/*
 * minval()
 *
 * Prints the minimum partition sizes (for use by the instf command).
 */
static void
minval()
{
	printf("bootmin=%ld diskmin=%ld rootmin=%ld swapmin=%ld swapbase=%ld\n",
	    BOOTSIZE, SIZE_32, ROOT0_32, SWAP0_32, BOOTSIZE);

	printf("stndmin=%ld usrmin=%ld varmin=%ld homemin=%ld\n", 
		STAND0_32, USR0_32, VAR0_32, HOME_MIN);

}

/*
 * pt_assgn()
 *
 * Prints default partition and tags (for use by cmd-3b2 component)
 */
static void
pt_assgn()
{
	char	*partns="0123456789abcdef";

	printf(" p_boot=%c p_root=%c p_swap=%c p_backup=%c p_stand=%c\n",
		 partns[P_BOOT], partns[P_ROOT], partns[P_SWAP], 
		 partns[P_BACKUP], partns[P_STAND] );

	printf(" p_usr=%c p_var=%c p_home=%c p_home2=%c\n",
		 partns[P_USR], partns[P_VAR], partns[P_HOME], partns[P_HOME2]);

	printf(" v_boot=%d v_root=%d v_swap=%d v_backup=%d v_stand=%d\n",
		V_BOOT, V_ROOT, V_SWAP, V_BACKUP, V_STAND );

	printf(" v_usr=%d v_var=%d v_home=%d\n",
		V_USR, V_VAR, V_HOME);

}


static void
pread(fd, block, len, buf)
int		fd;
unsigned long	block;
unsigned long	len;
char		*buf;
{
	struct io_arg	args;

	args.sectst = block;
	args.datasz = len;
	args.memaddr = (unsigned long) buf;
	args.retval = 0;
	if (ioctl(fd, V_PREAD, &args) == -1 || args.retval == V_BADREAD) {
		(void) fprintf(stderr, "fmthard: Cannot read block %lu\n", block);
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
	struct io_arg	args;

	args.sectst = block;
	args.datasz = len;
	args.memaddr = (unsigned long) buf;
	args.retval = 0;
	if(ioctl(fd,V_PWRITE,&args) == -1 || args.retval == V_BADWRITE) {
		if (errno == EPERM)  {
			(void) fprintf(stderr, "fmthard: Must have super-user privileges \n");
		}else  {
			(void) fprintf(stderr, "fmthard: Cannot write block %lu  errno = %d\n", block, errno);
		}
		exit(1);
	}
}

/*
 *  round ()
 *
 *  Rounds the root and swap partitions up to the next higher 
 *  cylinder boundary.
 */
int
round (part_size, last_used, cyl_blks)
int 	part_size;
int 	last_used;
int	cyl_blks;
{
	int 	end_sect;	
	int 	nrst_blk_bnd;

	end_sect = last_used + part_size;
	if (end_sect % cyl_blks) {
		nrst_blk_bnd = end_sect / cyl_blks * cyl_blks;
		part_size = nrst_blk_bnd - last_used + cyl_blks;
	}	
	return(part_size);
}

/*
 * tinyvtoc()	
 *
 * Sets up the minimum partition sizes (for use by the partition command).
 */

static void
tinyvtoc (size_newdsk, vtoc) 
unsigned long	size_newdsk;
struct vtoc	*vtoc;
{
	auto int	part;
	auto int	tag;
	auto int	flag;
	auto int	start;
	auto int	size;

	for (part=0; part < 16; ++part) {

		tag = 0;
		flag = V_UNMNT;
		start = 0;
		size = 0;

		switch (part) {

		case P_ROOT:
			tag = V_ROOT;
			flag = 00;
			start = BOOTSIZE + SWAP0_32;
			size = ROOT0_32;
			break;
		case P_SWAP:
			tag = V_SWAP;
			start = BOOTSIZE;
			size = SWAP0_32;
			break;
		case P_USR:
			tag = V_USR;
			flag = 00;
			start = BOOTSIZE+SWAP0_32+ROOT0_32+STAND0_32; 
			size = USR0_32;
			
			break;
		case P_STAND:
			tag = V_STAND;
			start = BOOTSIZE+SWAP0_32+ROOT0_32;
			size = STAND0_32;
			break;

		case P_BACKUP:
			tag=V_BACKUP;
			size = size_newdsk;
			break;

		case P_BOOT:
			tag=V_BOOT;
			size = BOOTSIZE;
			break;

		case P_VAR:
			tag = V_VAR;
			flag = 00;
			start = BOOTSIZE+SWAP0_32+ROOT0_32+STAND0_32+USR0_32; 
			size =  VAR0_32;
			break;

		case P_HOME:
			tag = V_HOME;
			flag = 00;
			start=BOOTSIZE+SWAP0_32+ROOT0_32+STAND0_32+USR0_32+VAR0_32; 
			size = size_newdsk - (BOOTSIZE+SWAP0_32+ROOT0_32+
					      STAND0_32+USR0_32+VAR0_32);
			break;
		}

		vtoc->v_part[part].p_tag = tag;
		vtoc->v_part[part].p_flag = flag;
		vtoc->v_part[part].p_start = start;
		vtoc->v_part[part].p_size = size;
	}
}

/*
 * upgrade()
 *
 * Upgrade a 1.0 defect table.
 */
static void
upgrade(fd, olddata)
int		fd;
union datatype	*olddata;
{

	struct pdsector bufsect;
	struct defstruct bufdef;
	unsigned int bufrel[128];
	int i, j;
	struct defect *new, *old;
	register unsigned long sectno, cyl;

	/* SETUP NEW SECTOR 0 */

	bufsect.pdinfo.driveid = olddata->o_sect0.driveid;
	bufsect.pdinfo.sanity = VALID_PD;
	bufsect.pdinfo.version = V_VERSION;
	for (i=0;i<12;i++)
		bufsect.pdinfo.serial[i] = 0x00;
	bufsect.pdinfo.cyls = olddata->o_sect0.cyls;
	bufsect.pdinfo.tracks = olddata->o_sect0.tracks;
	bufsect.pdinfo.sectors = olddata->o_sect0.sectors;
	bufsect.pdinfo.bytes = olddata->o_sect0.bytes;
	logical = bufsect.pdinfo.tracks * bufsect.pdinfo.sectors;
	bufsect.pdinfo.logicalst = logical;
	bufsect.pdinfo.errlogst = (bufsect.pdinfo.tracks * bufsect.pdinfo.sectors) - 1;
	bufsect.pdinfo.errlogsz = bufsect.pdinfo.bytes;
	bufsect.pdinfo.mfgst = MFGBLKNO;
	bufsect.pdinfo.mfgsz = bufsect.pdinfo.bytes;
	bufsect.pdinfo.defectst = DEFBLKNO;
	bufsect.pdinfo.defectsz = bufsect.pdinfo.bytes;
	bufsect.pdinfo.relno = NUMOFREL;
	bufsect.pdinfo.relst = RELBLKNO;
	bufsect.pdinfo.relsz = (bufsect.pdinfo.tracks * bufsect.pdinfo.sectors) - 4;
	for (i=0;i<10;i++)
		bufsect.reserved[i] = 0x00;
	for (i=0;i<97;i++)
		bufsect.devsp[i] = 0x00;

	/* READ 1ST RELOCATION SECTOR */
	pread(fd, DEFBLKNO, bufsect.pdinfo.bytes, (char *) bufrel);

	/*CREATE NEW DEFECT MAP */
	j = 0;
	old = (struct defect *)&olddata->o_sect0.iddeftab[0];
	new = &bufdef.map[0];
	while (old->bad.full != 0xffffffff)	{
		new->bad.full = old->bad.full;
		new->good.full = old->good.full;
		new++;
		old++;
		j++;
	}

	bufsect.pdinfo.relnext = (bufsect.pdinfo.relst + j-1); 
	for(;j<64;j++)	{
		bufdef.map[j].bad.full = 0xffffffff;
		bufdef.map[j].good.full = 0xffffffff;
	}

	/* WRITE 1ST RELOCATION SECTOR TO END OF LIST - UPDATE DEFECT MAP */
	if (bufdef.map[0].bad.full != 0xffffffff)	{
		cyl = bufsect.pdinfo.relnext / (bufsect.pdinfo.tracks * bufsect.pdinfo.sectors);
		bufdef.map[0].good.part.pcnh = ((cyl >> 8) & 0xff);
		bufdef.map[0].good.part.pcnl = cyl & 0xff;
		sectno = bufsect.pdinfo.relnext % (bufsect.pdinfo.tracks * bufsect.pdinfo.sectors);
		bufdef.map[0].good.part.phn = sectno / bufsect.pdinfo.sectors;
		bufdef.map[0].good.part.psn = sectno % bufsect.pdinfo.sectors;

		pwrite(fd, bufsect.pdinfo.relnext, bufsect.pdinfo.bytes, (char *) bufrel);
		bufsect.pdinfo.relnext++;
	}

	/* SEND SECTOR 0 TO IOCTL FOR A WRITE */
	pwrite(fd, PDBLKNO, bufsect.pdinfo.bytes, (char *) &bufsect);

	/* SEND DEFECT MAP TO IOCTL FOR A WRITE */
	pwrite(fd, DEFBLKNO, bufsect.pdinfo.bytes, (char *) &bufdef);

	/* READ IN NEW SECTOR O */
	pread(fd, PDBLKNO, BLKSZ, (char *) &datastruct);
}

static void
usage()
{
	(void) fprintf(stderr, "\
Usage:	fmthard [ -i ] [ -n volumename ]\n\
	[ -c coredisktype ] [ -s datafile ] [ -d arguments] raw-device\n");
	exit(2);
}

/*
 * validate()
 *
 * Validate the new VTOC.
 */
static void
validate(pdinfo, vtoc)
struct pdinfo	*pdinfo;
struct vtoc	*vtoc;
{
	register int	i, j;
	int	fullsz, total, endsect;
	int	isize, istart, jsize, jstart;

	fullsz = (
	    (pdinfo->cyls - 1)
	    * pdinfo->tracks
	    * pdinfo->sectors
	) - logical;
	for(i=0, total=0; i<V_NUMPAR; i++) {
		if(i==6) {
			if(vtoc->v_part[6].p_size <= 0) {
				(void) fprintf(stderr, "\
fmthard: Partition 6 specifies the full disk and cannot have a size of 0.\n\
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
fmthard: Partition %d specified as %lu sectors starting at %lu\n\
\tdoes not fit. The full disk contains %lu sectors.\n",
			    i, vtoc->v_part[i].p_size,
			    vtoc->v_part[i].p_start, fullsz);
			exit(1);
		}

		if (i!=6)	{
			total += vtoc->v_part[i].p_size;
			if(total > fullsz)	{
				(void) fprintf(stderr, "\
fmthard: Total of %lu sectors specified within partitions\n\
\texceeds the disk capacity of %lu sectors.\n",
					total, fullsz);
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
fmthard: Partition %d overlaps partition %d. Overlap is allowed\n\
\tonly on partition 6 (the full disk).\n",
						    i, j);
						exit(1);
					}
				}
			}
		}
	}
}

/*
 * values()
 *
 * Print the VTOC values.
 */
static void
values ()
{
	char drive='0';

	if (cflag) {
	
		switch(*cflag)  {
		
		case '0':
			drive = '0';
			printf(" rootsize=%ld stndsize=%ld ", root, stand);
			printf(" swapsize=%ld homesize=%ld ", swap, home);
			printf(" usrsize_0=%ld varsize_0=%ld ", usr, var);
			break;

		case '1':
			drive = '0';
			printf(" rootsize=%ld stndsize=%ld ", root, stand);
			printf(" swapsize=%ld homesize=%ld ", swap, home);
			break;

		case '2':
			drive = '1';
			printf(" usrsize_1=%ld varsize_1=%ld ",usr, var);
			break;

		default: 
			fprintf(stderr,
			     "fmthard: argument for -c must be 0, 1 or2.\n");
			exit(1);
		}
	}

	printf(" bootsize_%c=%ld  disksize_%c=%ld\n ",drive,boot,drive,size_newdsk);

}
