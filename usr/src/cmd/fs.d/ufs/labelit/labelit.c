/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ufs.cmds:ufs/labelit/labelit.c	1.5"

/*
 * Label a file system volume.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mntent.h>
#include <sys/vnode.h>
#include <fcntl.h>
#include <sys/fs/ufs_inode.h>
#include <sys/fs/ufs_fs.h>

union sbtag {
	char		dummy[SBSIZE];
	struct fs	sblk;
} sb_un, altsb_un;

#define sblock sb_un.sblk
#define altsblock altsb_un.sblk

extern int	optind;
extern char	*optarg;

int	status;

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int		opt;
	char		*special = NULL;
	char		*fsname = NULL;
	char		*volume = NULL;

	while ((opt = getopt (argc, argv, "o:")) != EOF) {
		switch (opt) {

		case 'o':		/* specific options (none defined yet) */
			break;

#if 0
		case 'V':		/* Print command line */
			{
				char		*opt_text;
				int		opt_count;

				(void) fprintf (stdout, "labelit -F ufs ");
				for (opt_count = 1; opt_count < argc ; opt_count++) {
					opt_text = argv[opt_count];
					if (opt_text)
						(void) fprintf (stdout, " %s ", opt_text);
				}
				(void) fprintf (stdout, "\n");
			}
			break;
#endif
		case '?':
			usage();
		}
	}
	if (optind > (argc - 1)) {
		usage ();
	}
	argc -= optind;
	argv = &argv[optind];
	special = argv[0];
	if (argc > 0) {
		fsname = argv[1];
		if (strlen(fsname) > 6) usage();
	}
	if (argc > 1) {
		volume = argv[2];
		if (strlen(volume) > 6) usage();
	}
	label (argv[0], fsname, volume);
}

usage ()
{

	(void) fprintf (stderr, "ufs usage: labelit [-F ufs] [generic options] special [fsname volume]\n");
	exit (31+1);
}

label (special, fsname, volume)
	char		*special;
	char		*fsname;
	char		*volume;
{
	int	f;
	int	blk;
	int	i;
	int	cylno;
	int	rpos;
	char	*p;
	int	offset;

	if (fsname == NULL) {
		f = open(special, O_RDONLY);
	} else {
		f = open(special, O_RDWR);
	}
	if (f < 0) {
		(void) fprintf (stderr, "labelit: ");
		perror ("open");
		exit (31+1);
	}
	if (lseek(f, SBLOCK * DEV_BSIZE, 0) < 0) {
		(void) fprintf (stderr, "labelit: ");
		perror ("lseek");
		exit (31+1);
	}
	if (read(f, &sblock, SBSIZE) != SBSIZE) {
		(void) fprintf (stderr, "labelit: ");
		perror ("read");
		exit (31+1);
	}
	if (sblock.fs_magic != FS_MAGIC) {
		(void) fprintf (stderr, "labelit: ");
		(void) fprintf (stderr, "bad super block magic number\n");
		exit (31+1);
	}
	/*
	 * calculate the available blocks for each rotational position
	 */
	blk = sblock.fs_spc * sblock.fs_cpc / NSPF(&sblock);
	for (i = 0; i < blk; i += sblock.fs_frag)
		/* void */;
	i -= sblock.fs_frag;
	cylno = cbtocylno(&sblock, i);
	rpos = cbtorpos(&sblock, i);
	blk = i / sblock.fs_frag;
	p = (char *)&(sblock.fs_rotbl[blk]);
	if (fsname != NULL) {
		for (i = 0; i < 14; i++)
			p[i] = '\0';		
		for (i = 0; (i < 6) && (fsname[i]); i++, p++)
			*p = fsname[i];
		p++;	
	}
	if (volume != NULL) {
		for (i = 0; (i < 6) && (volume[i]); i++, p++)
			*p = volume[i];
	}
	if (fsname != NULL) {
		if (lseek(f, SBLOCK * DEV_BSIZE, 0) < 0) {
			(void) fprintf (stderr, "labelit: ");
			perror ("lseek");
			exit (31+1);
		}
		if (write(f, &sblock, SBSIZE) != SBSIZE) {
			(void) fprintf (stderr, "labelit: ");
			perror ("write");
			exit (31+1);
		}
		for (i = 0; i < sblock.fs_ncg; i++) {
		offset = cgsblock(&sblock, i) * sblock.fs_fsize;
		if (lseek(f, offset, 0) < 0) {
			(void) fprintf (stderr, "labelit: ");
			perror ("lseek");
			exit (31+1);
		}
		if (read(f, &altsblock, SBSIZE) != SBSIZE) {
			(void) fprintf (stderr, "labelit: ");
			perror ("read");
			exit (31+1);
		}
		if (altsblock.fs_magic != FS_MAGIC) {
			(void) fprintf (stderr, "labelit: ");
			(void) fprintf (stderr, "bad alternate super block(%i) magic number\n", i);
			exit (31+1);
		}
		bcopy((char *)&(sblock.fs_rotbl[blk]),
			(char *)&(altsblock.fs_rotbl[blk]), 14);
		
		if (lseek(f, offset, 0) < 0) {
			(void) fprintf (stderr, "labelit: ");
			perror ("lseek");
			exit (31+1);
		}
		if (write(f, &altsblock, SBSIZE) != SBSIZE) {
			(void) fprintf (stderr, "labelit: ");
			perror ("write");
			exit (31+1);
		}
		}
	}
	p = (char *)&(sblock.fs_rotbl[blk]);
	fprintf (stderr, "fsname: ");
	for (i = 0; (i < 6) && (*p); i++, p++) {
		fprintf (stderr, "%c", *p);
	}
	fprintf (stderr, "\n");
	fprintf (stderr, "volume: ");
	p++;
	for (i = 0; (i < 6); i++, p++) {
		fprintf (stderr, "%c", *p);
	}
	fprintf (stderr, "\n");
}
