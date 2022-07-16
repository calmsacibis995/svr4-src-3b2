/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newboot:newboot.c	1.12"
/*
 * Write the two boot programs (mboot and boot) to the disk:
 *
 *	Block 0    ==> mboot program
 *	Block 1    ==> updated VTOC
 *	Block 2... ==> boot program
 *
 * Usage: newboot boot mboot filsys
 *
 * Before writing the filesys, a confirmation message is issued if the -y
 * flag was not present.  The response must be "y" or "Y", or the program
 * aborts.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <sys/vtoc.h>
#include <fcntl.h>
#include <a.out.h>
#include <libelf.h>
#include <unistd.h>
#include <stdlib.h>

# define USAGE "Usage: %s [-y] boot mboot partition\n"

/*
 * external routines
 */
extern char	*strtok (), *optarg;
extern int	optind;

void check_size();
char *mkboot(), *mkcoffboot();
static char *argv0;

main (argc, argv)
int	argc;
char	*argv[];
{
	char	*block0, *block2, *fsys, *boot, *mboot, *error_message;
	int	c, fso = -1, yes = 0, error = 0;
	long	msize, morigin, lsize, lorigin;
	/*
	 * VTOC contains pointers which determine where boot is to be loaded;
	 * it is updated as one entire block (NBPSCTR long)
	 *
	 * Any changes here must be coordinated with /usr/src/uts/3b5/boot/mboot/mboot.c
	 */
	union {
		struct vtoc vtoc_buffer;
		char	buffer[NBPSCTR];
	} block1;


	argv0 = argv[0];
	while ((c = getopt (argc, argv, "y?")) != EOF)
		switch (c) {
		case 'y':
			yes = 1;
			break;
		case '?':
			error = 1;
			break;
		}
	if (error || argc - optind < 3) {
		fprintf (stderr, USAGE, argv0);
		exit (1);
	}
	boot = argv[optind++];
	mboot = argv[optind++];
	fsys = argv[optind];

	error_message = "boot not written";
	block0 = mkboot (mboot, &morigin, &msize);

	block2 = mkboot (boot, &lorigin, &lsize);
	lsize = ((lsize + NBPSCTR - 1) / NBPSCTR) * NBPSCTR;

	check_size(error_message, mboot, boot, msize, lsize, fsys);
	/*
	 * Write the boot partition after receiving confirmation
	 */
	if (!yes) {
		char	line[81], *p;

		do {
			printf ("%s: confirm request to write boot programs to %s: ", argv0, fsys);
		} while (fgets (line, sizeof(line), stdin) == NULL || (p = strtok (line, " \t\n\r")) == NULL);
		if (*p != 'y' && *p != 'Y')
			error_exit (error_message);
	}

	/*
	 * Open the boot device
	 */
	if ((fso = open (fsys, O_RDWR, S_IRUSR|S_IWUSR)) == -1) {
		perror (fsys);
		error_exit (error_message);
	}
	error_message = "boot partition destroyed!";
	if (mywrite (fso, block0, msize, fsys))
		error_exit (error_message);
	if (mylseek (fso, NBPSCTR, 0, fsys))
		error_exit (error_message);
	if (myread (fso, (char *) &block1, NBPSCTR, fsys))
		error_exit (error_message);
	block1.vtoc_buffer.v_bootinfo[0] = lorigin;
	block1.vtoc_buffer.v_bootinfo[1] = lsize;
	if (mylseek (fso, (long) 0 - NBPSCTR, 1, fsys) ||
	    mywrite (fso, (char *) &block1, NBPSCTR, fsys) ||
	    mywrite (fso, block2, (unsigned) lsize, fsys))
		error_exit (error_message);
	close (fso);
	exit (0);
}


char *
mkcoffboot (filename, origin, size)
char *filename;
long *origin, *size;
{
	char *error_message, *block;
	int	f = -1;
	register FILHDR *header;
	register SCNHDR *text = NULL, *data = NULL, *sheader;

	error_message = "boot not written";
	if ((header = (FILHDR *) malloc (FILHSZ)) == NULL ||
	    (sheader = (SCNHDR *) malloc (SCNHSZ)) == NULL ||
	    (f = open (filename, O_RDONLY)) == -1) {
		perror (filename);
		error_exit (error_message);
	}
	if (myread (f, (char *)header, FILHSZ, filename))
		error_exit (error_message);
	if (header->f_magic != FBOMAGIC) {
		fprintf (stderr, "%s: bad magic\n", filename);
		error_exit (error_message);
	}
	if (mylseek (f, (long) (header->f_opthdr), 1, filename))
		error_exit (error_message);
	while (header->f_nscns-- != 0) {
		if (myread (f, (char *)sheader, SCNHSZ, filename))
			error_exit (error_message);
		if (0 == strncmp (sheader->s_name, ".text", sizeof(sheader->s_name))) {
			text = sheader;
			if ((sheader = (SCNHDR *) malloc (SCNHSZ)) == NULL) {
				perror (filename);
				error_exit (error_message);
			}
		}
		if (0 == strncmp (sheader->s_name, ".data", sizeof(sheader->s_name))) {
			data = sheader;
			if ((sheader = (SCNHDR *) malloc (SCNHSZ)) == NULL) {
				perror (filename);
				error_exit (error_message);
			}
		}
	}
	if (text == NULL || data == NULL) {
		fprintf (stderr, "%s: can't find text or data section\n", filename);
		error_exit (error_message);
	}
	free (sheader);
	*origin = text->s_paddr;
	*size = data->s_paddr + data->s_size - text->s_paddr;
	if ((block = (char *)malloc (*size)) == NULL) {
		fprintf (stderr, "%s: out of memory\n", filename);
		error_exit (error_message);
	}
	if (mylseek (f, text->s_scnptr, 0, filename) ||
	    myread (f, block, (unsigned)text->s_size, filename) ||
	    mylseek (f, data->s_scnptr, 0, filename) ||
	    myread (f, block + (data->s_paddr - text->s_paddr), (unsigned)data->s_size, filename))
		error_exit (error_message);
	free (text);
	free (data);
	close (f);
	return (block);
}


char *
mkboot (filename, origin, size)
char *filename;
long *origin, *size;
{
	char *block = NULL;
	char *err_str = "boot not written";
	int fd, i;
	unsigned int bsize;
	Elf *elfd;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	short magic;

	if ((fd = open (filename, O_RDONLY)) == -1) {
		perror (filename);
		error_exit (err_str);
	}

	if (read(fd, &magic, sizeof(magic)) != sizeof(magic)){
		perror(filename);
		error_exit (err_str);
	}

	if ((lseek(fd, 0, 0)) == -1L) {
		perror(filename);
		error_exit (err_str);
	}

	if (magic == FBOMAGIC)
		return(mkcoffboot(filename, origin, size));

	if (elf_version (EV_CURRENT) == EV_NONE) {
		fprintf (stderr, "ELF access library out of date\n");
		error_exit (err_str);
	}

	if ((elfd = elf_begin (fd, ELF_C_READ, NULL)) == NULL) {
		fprintf (stderr, "can't elf_begin: %s\n", elf_errmsg (0));
		error_exit (err_str);
	}

	if ((elf_kind(elfd)) != ELF_K_ELF) {
		fprintf(stderr, "%s: Invalid binary\n", filename);
		error_exit(err_str);
	}

	if ((ehdr = elf32_getehdr(elfd)) == NULL) {
		fprintf(stderr, "%s: can't get Elf header\n", filename, elf_errmsg(0));
		error_exit(err_str);
	}

	if ((phdr = elf32_getphdr (elfd)) == NULL) {
		fprintf (stderr, "%s: can't get ELF program header: %s\n",
			filename, elf_errmsg (0));
		error_exit (err_str);
	}

	bsize = 0;
	*origin = 0;

	for (i = 0; i < (int)ehdr->e_phnum; i++) {

		if (phdr[i].p_type != PT_LOAD)
			continue;

		if (phdr[i].p_filesz == 0)
			continue;

		if (*origin == 0)
			*origin = phdr[i].p_paddr;	/* Phdrs in addr order */

		if (bsize == 0) {
			bsize = phdr[i].p_filesz;

			if ((block = malloc(bsize)) == NULL) {
				fprintf(stderr, "Cannot malloc space for PT_LOAD Segment\n", filename);
				error_exit(err_str);
			}

			if ((lseek(fd, (long)phdr[i].p_offset, 0)) == -1) {
				fprintf(stderr, "%s: lseek error\n", filename);
				error_exit(err_str);
			}

			if ((read(fd, block, phdr[i].p_filesz)) != phdr[i].p_filesz) {
				fprintf(stderr, "%s: read error\n", filename);
				error_exit(err_str);
			}
		} else {	/* more PT_LOAD segments */

			if ((block = realloc(block, (bsize + phdr[i].p_filesz))) == NULL) {
				fprintf(stderr, "Cannot realloc space for PT_LOAD Segment\n", filename);
				error_exit(err_str);
			}

			if ((lseek(fd, (long)phdr[i].p_offset, 0)) == -1) {
				fprintf(stderr, "%s: lseek error\n", filename);
				error_exit(err_str);
			}

			if ((read(fd, &block[bsize], phdr[i].p_filesz)) != phdr[i].p_filesz) {
				fprintf(stderr, "%s: read error\n", filename);
				error_exit(err_str);
			}

			bsize += phdr[i].p_filesz;
		}
	}

	*size = bsize;

	elf_end (elfd);

	close (fd);

	return (block);
}

 void
check_size(message, mboot, boot, msize, lsize, rdisk)
char *message, *mboot, *boot;
long msize, lsize;
char *rdisk;
{

	struct vtoc vtoc;
	struct io_arg args;
	struct stat sb;
	struct pdsector pdsector;
	int fd, slice;

	if (stat(rdisk, &sb) != 0){
		perror(rdisk);
		error_exit(message);
	}
	
	if ((sb.st_mode & S_IFMT) == S_IFREG)
		return;

	if ((sb.st_mode & S_IFMT) != S_IFCHR){
		fprintf(stderr,"%s must be a regular file or a raw device\n", rdisk);
		error_exit(message);
	}

	if ((fd= open(rdisk, O_RDONLY)) < 0){
		perror(rdisk);
		error_exit(message);
	}
		
        args.sectst = 0;
	args.memaddr = (unsigned long) &pdsector;
	args.datasz = sizeof(struct pdsector);
	if (ioctl(fd, V_PDREAD, &args) < 0){
		fprintf(stderr, "Unable to read physical information of %s\n", rdisk);
		error_exit(message);
	}
	if (args.retval == V_BADREAD){
		fprintf(stderr, "Unable to read physical information of %s\n", rdisk );
		error_exit(message);
	}

        args.sectst = pdsector.pdinfo.logicalst + 1;
	args.memaddr = (unsigned long) &vtoc;
	args.datasz = sizeof(struct vtoc);
	if (ioctl(fd, V_PREAD, &args) < 0){
		fprintf(stderr, "Unable to read the vtoc of %s\n", rdisk);
		error_exit(message);
	}
	if (args.retval == V_BADREAD){
		fprintf(stderr, "Unable to read the vtoc of %s\n", rdisk);
		error_exit(message);
	}
	if (vtoc.v_sanity != VTOC_SANE){
		fprintf(stderr, "VTOC is not sane on %s\n", rdisk);
		error_exit(message);
	}

	slice = minor(sb.st_rdev) % 16;

	if (vtoc.v_part[slice].p_tag != V_BOOT){
		fprintf(stderr,"%s is not tagged as a boot partition\n", rdisk);
		error_exit(message);
	}

	if ( msize > NBPSCTR ){
		fprintf (stderr, "The total size of %s (0x%x) exceeds 1 disk block\n", mboot, msize);
		error_exit(message);
	}

	if (lsize > (vtoc.v_part[slice].p_size - 1) * NBPSCTR){
		fprintf (stderr, "The total size of %s (0x%x) exceeds the size of partition %d\n", boot, lsize, slice);
		error_exit(message);
	}
	close(fd);
}



error_exit (message)
char *message;
{
	fprintf (stderr, "%s: %s\n", argv0, message);
	fprintf (stderr, USAGE, argv0);
	exit (1);
}


/*
 * Read with error checking
 */
int
myread (fildes, buf, nbyte, message)
int	fildes;
char	*buf, *message;
register unsigned	nbyte;
{
	register int	rbyte;

	if ((rbyte = read (fildes, buf, nbyte)) == -1) {
		perror (message);
		return (1);
	} else if (rbyte != nbyte) {
		fprintf (stderr, "%s: truncated read\n", message);
		return (1);
	} else
		return (0);
}


/*
 * Write with error checking
 */
int
mywrite (fildes, buf, nbyte, message)
int	fildes;
char	*buf, *message;
register unsigned	nbyte;
{
	register int	wbyte;

	if ((wbyte = write (fildes, buf, nbyte)) == -1) {
		perror (message);
		return (1);
	} else if (wbyte != nbyte) {
		fprintf (stderr, "%s: truncated write\n", message);
		return (1);
	} else
		return (0);
}


/*
 * Lseek with error checking
 */
int
mylseek (fildes, offset, whence, message)
int	fildes, whence;
long	offset;
char	*message;
{

	if (lseek (fildes, offset, whence) == -1L) {
		perror (message);
		return (1);
	} else
		return (0);
}
