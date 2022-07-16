/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-3b2:absunix.c	1.2.3.2"
/*
 * absunix.c
 *
 * Zap the <EDT> section of a mkunix'ed kernel. This allows
 * lboot to load it regardless of hardware configuration.
 */

#include <fcntl.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>

#include <libelf.h>
#include <sys/elf_M32.h>


/*
 * Definitions.
 */
#define reg	register		/* Convenience */
#define	uint	unsigned int		/* Convenience */
#define	ulong	unsigned long		/* Convenience */
#define	ushort	unsigned short		/* Convenience */
#define	EDTNAME	"<EDT>"			/* Section to be zapped */
#define	EDTZAP	"<...>"			/* New name to hide "zapped" section */
#define	STDERR	2			/* fileno(stderr) */

/*
 * External functions.
 */
void	exit();
long	lseek();
char	*strrchr();
char	*strncpy();

/*
 * Internal functions.
 */
void	fatal();
char	*syserr();
void	usage();

/*
 * External variables.
 */
extern int	errno;			/* System error number */
extern char	*sys_errlist[];		/* System error messages */
extern int	sys_nerr;		/* Number of sys_errlist entries */

/*
 * Static variables.
 */
static char	*myname;		/* Last qualifier of arg0 */
static char	*target;		/* Target file (argv[1]) */

main(ac, av)
char	**av;
int	ac;
{
	reg int		idx;
	reg int		fd;
	struct filehdr	file;
	struct scnhdr	sec;

	if (myname = strrchr(av[0], '/'))
		++myname;
	else
		myname = av[0];

	if (ac != 2)
		usage();

	target = av[1];

	if ((fd = open(target, O_RDWR)) == -1)
		fatal(syserr(), target);

	if ((read(fd, (char *) &file, sizeof(file))) != sizeof(file))
		fatal(syserr(), target);

	if (file.f_magic == FBOMAGIC) {

		if ((lseek(fd, (long) file.f_opthdr, 1)) == -1L)
			fatal(syserr(), target);

		for (idx = 0; idx < (int)file.f_nscns; ++idx) {
			if (read(fd, (char *) &sec, sizeof(sec)) != sizeof(sec))
				fatal(syserr(), target);
			if (strncmp(sec.s_name, EDTNAME, sizeof(sec.s_name)) != 0)
				continue;
			(void) strncpy(sec.s_name, EDTZAP, sizeof(sec.s_name));
			if (lseek(fd, (long) -sizeof(sec), 1) < 0
			    || write(fd, (char *) &sec, sizeof(sec)) != sizeof(sec))
				fatal(syserr(), target);
		}

	} else {	/* ELF */

		if ((rd_elf(fd)) != 0)
			fatal("Not an executable file\n", target);
	}

	(void) close(fd);
	exit(0);
}

rd_elf(fd)
int fd;
{
	Elf *elfd;
	Elf32_Ehdr *ehdr;
	Elf_Scn *scn;
	Elf32_Shdr *eshdr;
	Elf_Data *strtab;
	char *name;
	int namefnd = 0;

	if (elf_version (EV_CURRENT) == EV_NONE) {
		fatal("ELF access library out of date\n", myname);
	}

	if ((lseek(fd, 0L, 0)) == -1L) {
		fatal(syserr(), target);
	}

	if ((elfd = elf_begin (fd, ELF_C_READ, (Elf *)0)) == (Elf *)0) {
		close(fd);
		fatal("Unable to elf begin\n", target);
	}

	if ((elf_kind(elfd)) != ELF_K_ELF) {
		elf_end(elfd);
		close(fd);
		return (-1);
	}

	if ((ehdr = elf32_getehdr(elfd)) == (Elf32_Ehdr *)0) {
		elf_end(elfd);
		close(fd);
		fatal("Error reading Elf Header\n", target);
	}

	if ((scn = elf_getscn(elfd, ehdr->e_shstrndx)) == (Elf_Scn *)0) {
		elf_end(elfd);
		close(fd);
		fatal("Error reading shdr string table\n", target);
	}

	eshdr = elf32_getshdr(scn);
	if ((!eshdr) || (eshdr->sh_type != SHT_STRTAB)) {
		elf_end(elfd);
		close(fd);
		fatal("Error string table not SHT_STRTAB\n", target);
	}

	if (((strtab = elf_getdata(scn, (Elf_Data *)0)) == (Elf_Data *)0) ||
		strtab->d_size == 0) {
			elf_end(elfd);
			close(fd);
			fatal("Error reading string table\n", target);
	}

	scn = (Elf_Scn *)0;
	while ((scn = elf_nextscn(elfd, scn)) != (Elf_Scn *)0) {

		if ((eshdr = elf32_getshdr(scn)) == (Elf32_Shdr *)0) {
			elf_end(elfd);
			close(fd);
			fatal("Error reading Shdr\n", target);
		}

		name = (char *)strtab->d_buf + eshdr->sh_name;
		if ((name) && ((strcmp(name, EDTNAME)) == 0)) {
			strcpy(name, EDTZAP);
			namefnd = 1;
			break;
		}
	}

	if (!namefnd) {
		printf("%s: Section <EDT>, not found\n", target);
	} else {
		elf_flagdata(strtab, ELF_C_SET, ELF_F_DIRTY);
		if ((elf_update(elfd, ELF_C_WRITE)) == (off_t)-1) {
			elf_end(elfd);
			close(fd);
			fatal("Error updating target\n", target);
		}
	}

	elf_end(elfd);

	return (0);
}

/*
 * fatal()
 *
 * Print an error message and exit.
 */
static void
fatal(why, what)
reg char	*why;
reg char	*what;
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
 * syserr()
 *
 * Return pointer to system error message.
 */
static char *
syserr()
{
	return (errno < 0 ? "Unknown error (?)"
	    : errno == 0 ? "EOF"
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
	static char	before[] = "Usage: ";
	static char	after[] = " mkunix'ed_kernel\n";

	(void) write(STDERR, before, (uint) strlen(before));
	(void) write(STDERR, myname, (uint) strlen(myname));
	(void) write(STDERR, after, (uint) strlen(after));
	exit(1);
}
