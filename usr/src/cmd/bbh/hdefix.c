/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bbh:hdefix.c	1.9.2.1"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/mkdev.h"
#include "sys/vtoc.h"
#include "sys/hdelog.h"
#include "sys/hdeioctl.h"
#include "sys/stat.h"
#include "sys/fs/s5ino.h"
#include "sys/fs/s5param.h"
#include "sys/fs/s5dir.h"
#include "sys/fs/s5macros.h"
#include "hdecmds.h"
#include "edio.h"
#include "sys/signal.h"
#include "sys/filsys.h"
#include "sys/extbus.h"
#include <stdio.h>
#include <fcntl.h>
#include <utmp.h>
#include <errno.h>
#include "sys/uadmin.h"

#define	CMDNAME	"hdefix"
#define	DEVNAME	"DEVXXXXXX"
#define BUFSIZE	512
#define MAXSARGV 32

extern char *malloc(), *realloc();
extern char *sys_errlist[];
extern int errno;
void error();
char * cmdname;
char Command[BUFSIZE];
dev_t devarg = -1;
char *Argv[MAXSARGV];
int Argc;
char Major[BUFSIZE];
char Minor[BUFSIZE];
struct eddata blist;
int blistcnt;
#define blistp ((daddr_t *) blist.badr)

struct eddata zerobuf;
struct eddata	inobuf;
struct eddata	dirbuf;

main(argc, argv)
char *argv[];
{

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	if (argc < 1) cmdname = CMDNAME;
	else cmdname = argv[0];
	if (argc < 2) {
		cmdusage();
		exit(INVEXIT);
	}
	if (argv[1][0] != '-') badusage();
	edinit();
	switch(argv[1][1]) {
		case 'a':
			doadd(argc-2, argv+2);
			break;
		case 'F':
			doforce(argc-2, argv+2);
			break;
		case 'p':
			dodmprnt(argc-2, argv+2);
			break;
		case 'r':
			dorestore(argc-2, argv+2);
			break;
		case 's':
			dosave(argc-2, argv+2);
			break;
		default:
			badusage();
	}
	exit(NORMEXIT);
}

cmdusage()
{
	fprintf(stderr,
		"usage:\t%s -a [[-D] maj min [{ -b blockno | -B cyl trk sec } ... ]]\n",
		cmdname);
	fprintf(stderr,
		"\t%s -F [[-D] maj min [{ -b blockno | -B cyl trk sec } ... ]]\n",
		cmdname);
	fprintf(stderr,
		"\t%s -p [[-D] maj min]\n",
		cmdname);
	fprintf(stderr,
		"\t%s -r [-D] maj min filename\n",
		cmdname);
	fprintf(stderr,
		"\t%s -s [-D] maj min filename\n",
		cmdname);
}

dodmprnt(ac, av)
char **av;
{
	register int i;

	Argv[1] = "-p";

	if (ac > 0) {
		Argc = 2;
		getdev(ac, av);
		prnt1dm();
		exit(NORMEXIT);
	}
	for (i = 0; i < edcnt; i++) {
		devarg = edtable[i];
		sprintf(Major, "%d", major(edtable[i]));
		sprintf(Minor, "%d", minor(edtable[i]));
		Argv[2] = Major;
		Argv[3] = Minor;
		Argc = 4;
		prnt1dm();
	}
	exit(NORMEXIT);
}

prnt1dm()
{
	if (gen_cmdname()) {
		register int	done = 0;
		register int	i;

		Argv[0] = Command;
		Argv[Argc] = NULL;

		/* Exec command */
		switch (fork()) {
		case (pid_t)-1 :
			error("fork failed\n");
			break;
		case 0 :
			execv(Command, Argv);
			error("%s exec failed\n", Command);
			break;
		default :
			while (!done) {
				switch (wait((int *) 0)) {
				case (pid_t)-1 :
					if (errno != EINTR)
						error("wait failed\n");
					errno = 0;
					break;
				default :
					done = 1;
					break;
				}
			}
			break;
		}
	} else {
		register struct hddm *mp, *emp;
		int sz;

		if (edopen(devarg)) {
			printf("%s: can't access disk maj=%d min=%d\n",
				cmdname, major(devarg), minor(devarg));
			return(-1);
		}
		if (edpdck()) {
			edclose();
			return(-1);
		}
		if (edgetdm()) {
			edclose();
			return(-1);
		}
		sz = edpdp->pdinfo.defectsz/sizeof(struct hddmap);
		mp = eddmp;
		emp = mp + sz;
		printf("\nBasic physical description of disk maj=%d min=%d:\n",
			major(devarg), minor(devarg));
		printf("sector size=%d, sectors per track=%d tracks per cylinder=%d\n",
			edpdp->pdinfo.bytes, edpdp->pdinfo.sectors, edpdp->pdinfo.tracks);
		printf("number of cylinders=%d, block number range: 0 thru %d\n",
			edpdp->pdinfo.cyls, edenddad-1);
		printf("defect map has %d slots\n", sz);
		printf("its active slots are:\nslot#   from blk#     to blk#\n");
		for (sz = 0; mp < emp; sz++, mp++) {
			if (mp->frmblk == -1) continue;
			printf("%5d %11d %11d\n", sz, mp->frmblk, mp->toblk);
		}
		printf("its surrogate region description is:\n");
		printf("\tstart blk#: %d\n", edpdp->pdinfo.relst);
		printf("\tsize (in blks): %d\n", edpdp->pdinfo.relsz);
		printf("\tnext blk#: %d\n", edpdp->pdinfo.relnext);
		printf("physical description is at blk# %d\n", edpdsno);
		printf("error log is at blk# %d\n", edpdp->pdinfo.errlogst);
		printf("logical start is at blk# %d\n", edpdp->pdinfo.logicalst);
		edclose();
		return(0);
	}
}

char *blkbp;

dosave(ac, av)
char **av;
{
	register int	rval;

	Argv[1] = "-s";
	Argc = 2;
	rval = getdev(ac, av);
	ac -= rval;
	av += rval;
	if (ac < 1) badusage();

	if (gen_cmdname()) {
		register int	i;

		Argv[0] = Command;
		Argv[Argc++] = av[0];	/* Save file name */
		Argv[Argc] = NULL;

		/* Exec command */
		execv(Command, Argv);
		error("%s exec failed\n", Command);
	} else {
		register int	sfdes;

		if (edopen(devarg)) exit(ERREXIT);
		if (edgetel()) {
			edclose();
			exit(ERREXIT);
		}
		if (edhdelp->l_valid != HDEDLVAL) {
			fprintf(stderr,
				"%s: WARNING: disk log does not contain a valid log\n",
				cmdname);
			fprintf(stderr, "\tdisk maj=%d min=%d\n",
				major(devarg), minor(devarg));
		}
		if ((sfdes = creat(av[0], 0644)) < 0) {
			fprintf(stderr, "unable to create save file: \"%s\"\n", av[0]);
			exit(ERREXIT);
		}
		if ((rval = write(sfdes, edhdel.badr, edhdel.csz)) != edhdel.csz) {
			if (rval < 0)
				fprintf(stderr, "write of save file failed\n");
			else fprintf(stderr,
				"wrote %d bytes (instead of %d) in save file\n",
				rval, edhdel.csz);
			edclose();
			close(sfdes);
			exit(ERREXIT);
		}
		fprintf(stderr, "save successful\n");
		edclose();
		close(sfdes);
		exit(NORMEXIT);
	}
}

dorestore(ac, av)
char **av;
{
	register int	rval;

	Argv[1] = "-r";
	Argc = 2;
	rval = getdev(ac, av);
	ac -= rval;
	av += rval;
	if (ac < 1) badusage();

	if (gen_cmdname()) {
		register int	i;

		Argv[0] = Command;
		Argv[Argc++] = av[0];	/* Restore file name */
		Argv[Argc] = NULL;

		/* exec command */
		execv(Command, Argv);
		error("%s exec failed\n", Command);
	} else {
		register int	sfdes;
		register int	sz;
		struct stat	sbuf;

		rval = getdev(ac, av);
		ac -= rval;
		av += rval;
		if (ac < 1) badusage();
		if (edopen(devarg)) exit(ERREXIT);
		if (edpdck()) {
			edclose();
			exit(ERREXIT);
		}
		sz = edallocel();
		if ((sfdes = open(av[0], O_RDONLY)) < 0) {
			fprintf(stderr, "open of save file \"%s\" failed\n", av[0]);
			edclose();
			exit(ERREXIT);
		}
		if (fstat(sfdes, &sbuf) < 0) {
			fprintf(stderr, "fstat of save file \"%s\" failed\n", av[0]);
			edclose();
			close(sfdes);
			exit(ERREXIT);
		}
		if (sbuf.st_size != edhdel.csz) {
			if (sbuf.st_size < edhdel.csz)
				fprintf(stderr, "save file too small (%d < %d)\n",
					sbuf.st_size, edhdel.csz);
			else fprintf(stderr, "save file too large (%d > %d)\n",
					sbuf.st_size, edhdel.csz);
			exit(ERREXIT);
		}
		if ((rval = read(sfdes, edhdel.badr, edhdel.csz)) != edhdel.csz) {
			fprintf(stderr, "bad read of save file\"%s\"\n", av[0]);
			edclose();
			close(sfdes);
			exit(ERREXIT);
		}
		close(sfdes);
		edhdel.valid = 1;
		if (edhdelp->l_valid != HDEDLVAL) {
			fprintf(stderr,
				"WARNING: save file does not contain a valid log\n");
			sleep(10);
		}
		if(edputel()) {
			edclose();
			exit(ERREXIT);
		}
		edclose();
		fprintf(stderr, "disk error log restored from file \"%s\"\n",
			av[0]);
		exit(NORMEXIT);
	}
}

badusage()
{
	fprintf(stderr, "bad command usage\n");
	cmdusage();
	exit(INVEXIT);
}


getdev(ac, av)
char *av[];
{
	major_t maj;
	minor_t min;
	int rcnt;
	major_t maxmaj;
	minor_t maxmin;
	dev_t maxdev = -1;

	rcnt = 2;
	if (ac < 2) badusage();
	if (av[0][0] == '-') {
		if (av[0][1] != 'D' || ac < 3) badusage();
		rcnt++;
		av++;
		Argv[Argc++] = "-D";
	}
	if (sscanf(av[0], "%ld", &maj) != 1
		|| sscanf(av[1], "%ld", &min) != 1) badusage();
	maxmaj = major(maxdev);
	maxmin = minor(maxdev);
	if (maj < 0 || maj > maxmaj) {
		fprintf(stderr,
			"major device number %d out of range (0 to %d)\n",
			maj, maxmaj);
		exit(INVEXIT);
	}
	if (min < 0 || min > maxmin) {
		fprintf(stderr,
			"minor device number %d out of range (0 to %d)\n",
			min, maxmin);
		exit(INVEXIT);
	}
	devarg = makedev(maj, min);
	strcpy(Major, av[0]);
	strcpy(Minor, av[1]);
	Argv[Argc++] = Major;
	Argv[Argc++] = Minor;
	return(rcnt);
}

doforce(ac, av)
char **av;
{
	register int	rval;

	chkstate();
	Argv[1] = "-F";
	Argc = 2;
	rval = getdev(ac, av);
	ac -= rval;
	av += rval;

	if (gen_cmdname()) {
		register int	i;

		Argv[0] = Command;
		Argv[Argc] = NULL;

		/* Exec command */
		execv(Command, Argv);
		error("%s exec failed\n", Command);
	} else {
		register int	sz;
		register struct	hddm *mp, *m2p, *emp;

		if (edopen(devarg)) {
			exit(ERREXIT);
		}
		if (edpdck()) {
			exit(ERREXIT);
		}
		if (edgetdm()) {
			exit(ERREXIT);
		}
		sz = edpdp->pdinfo.defectsz/sizeof(struct hddmap);
		mp = eddmp;
		emp = mp + sz;
		for ( ; mp < emp; mp ++)
			if (mp->frmblk == -1) break;
		emp = --mp;
		if (mp < eddmp) {
			fprintf(stderr, "there are no defects to remove\n");
			exit(ERREXIT);
		}
		if (ac < 1) {
			printf("%d\n", mp->frmblk);
			emp->frmblk = -1;
			emp->toblk = -1;
			if (edputdm()) {
				exit(ERREXIT);
			}
			edclose();
			exit(NORMEXIT);
		}
		getablist(ac, av);
		for (sz = 0; sz < blistcnt; sz++) {
			for(mp = eddmp; mp <= emp; mp++) {
				if (blistp[sz] == mp->frmblk) {
					mp->frmblk = -1;
					mp->toblk = -1;
					goto gotit;
				}
			}
			fprintf(stderr, "%s: block# %d not a from block in defect map\n",
				cmdname, blistp[sz]);
			exit(ERREXIT);
gotit:			;
		}
		for (mp = m2p = eddmp; mp <= emp; mp++) {
			if (mp->frmblk == -1) continue;
			if (mp != m2p) *m2p = *mp;
			m2p++;
		}
		while (m2p <= emp) {
			m2p->frmblk = -1;
			m2p->toblk = -1;
			m2p++;
		}
		if (edputdm()) {
			exit(ERREXIT);
		}
		edclose();
		printf("changed defect table\n");
		exit(NORMEXIT);
	}
}

chkstate()
{
	register struct utmp *up;
	int c;
	extern struct utmp *getutid();
	struct utmp runlvl;

	runlvl.ut_type = RUN_LVL;
	if ((up = getutid(&runlvl)) == NULL) {
		fprintf(stderr, "%s: can't determine run state\n", cmdname);
		exit(ERREXIT);
	}
	c = up->ut_exit.e_termination;
	if (c != 'S' && c != 's') {
		fprintf(stderr, "%s: not in single user state\n", cmdname);
		exit(INVEXIT);
	}
}

getablist(ac, av)
char **av;
{
	register int sz;
	register int rval;

	sz = (ac/2) * sizeof(daddr_t);	/* possible over estimate, so what */
	if (!(blist.badr = malloc(sz))) {
		fprintf(stderr, "%s: malloc of blist failed\n", cmdname);
		exit(ERREXIT);
	}
	blist.bsz = blist.csz = sz;
	blistcnt = 0;
	sz = ac/2;
	while(ac > 0) {
		rval = getbno(ac, av);
		ac -= rval;
		av += rval;
	}
}

getbno(ac, av)
char **av;
{
	daddr_t bno;
	int cyl, trk, sec;

	if (ac < 2) badusage();
	if (av[0][0] != '-')
		badusage();
	if (av[0][1] == 'b') {
		if (sscanf(av[1], "%d", &bno) != 1)
			badusage();
		if (dadcheck(bno)) badusage();
		blistp[blistcnt++] = bno;
		return(2);
	}
	if (av[0][1] == 'B') {
		if (ac < 4) badusage();
		if (sscanf(av[1], "%d", &cyl) != 1) badusage();
		if (sscanf(av[2], "%d", &trk) != 1) badusage();
		if (sscanf(av[3], "%d", &sec) != 1) badusage();
		if ((bno = ctstodad(cyl, trk, sec)) == -1) badusage();
		blistp[blistcnt++] = bno;
		return(4);
	}
	badusage();
}

int rebootflg;
struct hdedata hdequeue[HDEQSIZE];
int hdeqcnt;

gethdeq()
{

	if (hdeqcnt = edgetrct()) {
		if (hdeqcnt < 0) {
			fprintf(stderr,
				"%s: attempt to get error report queue cnt failed\n",
				cmdname);
			hdeqcnt = 0;
		} else {
			if (hdeqcnt > HDEQSIZE) {
			  /* just in case somebody increases HDEQSIZE
			   * and fails to recompile/install hdefix
			  */
				hdeqcnt = HDEQSIZE;
			}
			if ((hdeqcnt = edgetrpts(hdequeue, hdeqcnt)) < 0) {
				fprintf(stderr,
					"%s: attempt to get queued error reports failed\n",
					cmdname);
				hdeqcnt = 0;
			}
			printf("there are %d unlogged error reports\n",
				hdeqcnt);
			edclrrpts(hdequeue, hdeqcnt);
			doclear();
		}
	}
}

doclear()
{
	struct hdedata junkit;

	while (edgetrct() > 0) {
		if (edgetrpts(&junkit, 1) != 1) break;
		if (edclrrpts(&junkit, 1) != 1) break;
	}
}

doadd(ac, av)
char **av;
{
	register int i;

	chkstate();
	sync();
	printf("%s: once fixing bad blocks is started, signals are ignored\n", cmdname);
	sleep(10);
	printf("starting\n");
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	edfixlk();
	gethdeq();

	Argv[1] = "-a";

	if (ac < 1) {
		for (i = 0; i < edcnt; i++) {
			devarg = edtable[i];
			Argv[2] = "-n";
			sprintf(Major, "%d", major(edtable[i]));
			sprintf(Minor, "%d", minor(edtable[i]));
			Argv[3] = Major;
			Argv[4] = Minor;
			Argc = 5;
			do1add(ac, av);
		}
	} else {
		Argv[2] = "-n";
		Argc = 3;
		i = getdev(ac, av);
		ac -= i;
		av += i;
		do1add(ac, av);
	}
	doclear();
	if (rebootflg) {
		printf("%s: causing system reboot\n", cmdname);
		doreboot();
	}
	printf("%s: you may now resume normal operation\n", cmdname);
	exit(NORMEXIT);
}

daddr_t curbno, effbno;
long lgclsz;

do1add(ac, av)
char **av;
{
	register int	i;

	if (gen_cmdname()) {
		register int	done = 0;

		Argv[0] = Command;
		for (i = 0; i < ac; i++)
			Argv[Argc++] = av[i];
		Argv[Argc] = NULL;

		edfixul();

		/* Exec command */
		switch (fork()) {
		case (pid_t)-1 :
			error("fork failed\n");
			break;
		case 0 :
			execv(Command, Argv);
			error("%s exec failed\n", Command);
			break;
		default :
			while (!done) {
				int	status;

				switch (wait(&status)) {
				case (pid_t)-1 :
					if (errno != EINTR)
						error("wait failed\n");
					errno = 0;
					break;
				default :
					if (((status & 0xFF00) >> 8) == REBOOTEXIT)
						rebootflg = 1;
					done = 1;
					break;
				}
			}
			break;
		}

		edfixlk();
	} else {
		register int	sz;
		daddr_t		bigsur;
		int		clrlogflg;
		int		zaplogflg;
		register struct	hddm *mp, *m2p, *emp, newmp;

		if (edopen(devarg)) {
			fprintf(stderr, "%s: unable to access disk maj=%d min=%d\n",
				cmdname, major(devarg), minor(devarg));
			return(-1);
		}
		setzbuf();
		if (edgetdm()) {
			fprintf(stderr,
				"%s: unable to access defect map on disk maj=%d min=%d\n",
				cmdname, major(devarg), minor(devarg));
			edclose();
			return(-1);
		}
		zaplogflg = 0;
		if (ac > 0) {
			if (hdeqcnt > 0) {
				printf("unlogged error reports are being ignored\n");
				hdeqcnt = 0;
			}
			getablist(ac, av);
			clrlogflg = 0;
		} else {
			clrlogflg = 1;
			getloglist();
		}
		if (blistcnt == 0) {
			edclose();
			printf("%s: no bad blocks on disk maj=%d min=%d\n",
				cmdname, major(devarg), minor(devarg));
			return(0);
		}
		printf("%s: processing %d bad blocks on disk maj=%d min=%d\n",
			cmdname, blistcnt, major(devarg), minor(devarg));
		sz = edpdp->pdinfo.defectsz/sizeof(struct hddmap);
		m2p = eddmp;
		emp = m2p + sz;
		for ( ; m2p < emp; m2p++)
			if (m2p->frmblk == -1) break;
		for (bigsur = -1, mp = eddmp; mp < m2p; mp++)
			if (mp->toblk > bigsur) bigsur = mp->toblk;
		if (bigsur >= ((daddr_t)edpdp->pdinfo.relst+edpdp->pdinfo.relsz))
			printf("defect map corrupted, it has a to-block out of surrogate region\n");
		else if (bigsur > edpdp->pdinfo.relnext) {
			edpdp->pdinfo.relnext = bigsur;
			printf("correcting next block value of surrogate region accounting\n");
			edputpd();
		}
		for (i = 0; i < blistcnt; i++) {
			effbno = curbno = blistp[i];
			printf("processing block %d:\n", curbno);
			for (mp = eddmp; mp < m2p; mp++) {
				if (mp->frmblk == curbno) {
					printf("    block already mapped\n");
					if (clrlogflg) {
						printf("    clearing block from log\n");
						clrlog(curbno);
					}
					goto nextone;
				}
			}
			printf("    new bad block (e.g., not already mapped)\n");
			if (edpdp->pdinfo.relnext >= edpdp->pdinfo.relst + edpdp->pdinfo.relsz) {
runout:
				printf("CRITICAL PROBLEM: you have run out of surrogate image space on this disk\n");
abortit:
				printf("unable to map bad block\n");
				printf("processing being aborted for disk maj=%d min=%d\n",
					major(devarg), minor(devarg));
				edclose();
				return(-1);
			}
			if (curbno == edpdsno) {
				printf("CRITICAL PROBLEM: physical description sector is bad\n");
				goto abortit;
			}
			if (curbno >= edpdp->pdinfo.defectst &&
			    curbno < edpdp->pdinfo.defectst + (edpdp->pdinfo.defectsz+edsecsz-1)/edsecsz) {
				printf("CRITICAL PROBLEM: a defect map block is bad\n");
				goto abortit;
			}
			for (mp = eddmp; mp < m2p; mp++) {
				if (mp->toblk == curbno) {
					printf("    block is surrogate image of block %d\n",
						mp->frmblk);
					effbno = mp->frmblk;
					goto classify;
				}
			}
			if (curbno >= edpdp->pdinfo.relst && curbno < edpdp->pdinfo.relst + edpdp->pdinfo.relsz) {
				printf("    block is unused surrogate image block\n");
				printf("    doing nothing about it now\n");
				goto nextone;
			}
classify:
			if (effbno >=  edpdp->pdinfo.errlogst &&
			    effbno < edpdp->pdinfo.errlogst + edpdp->pdinfo.errlogsz) {
				printf("    block in disk error log\n");
				clrlogflg = 0;	/* no touchie, its bad */
				zaplogflg = 1;
				goto fixit;
			}
			if (edpdp->pdinfo.logicalst < edpdsno) {
				lgclsz = edpdsno - edpdp->pdinfo.logicalst;
				if (effbno >= edpdsno) {
					printf("    non-critical block in device specific data\n");
					goto fixit;
				}
			} else {
				lgclsz = edenddad - edpdp->pdinfo.logicalst;
				if (effbno < edpdp->pdinfo.logicalst) {
					printf("    non-critical block in device specific data\n");
					goto fixit;
				}
			}
			rebootflg = 1;		/* the conservative strategy */
			printf("    block in partitioned portion of disk\n");
			partit();
fixit:
			printf("    assigning new surrogate image block for it\n");
			newmp.toblk = edpdp->pdinfo.relnext++;
			newmp.frmblk = effbno;
			if (effbno != curbno) {	/* use same slot */
				for (mp = eddmp; mp < m2p; mp++) {
					if (mp->frmblk == effbno)
						goto gotit;
				}
				/* not possible */
				printf("this program is sick! sick! sick!\n");
			}
			for (mp = m2p-1; mp >= eddmp; --mp) {
				if (mp->frmblk < effbno) break;
				mp[1] = mp[0];
			}
			++mp;
gotit:
			*mp = newmp;
			m2p++;
			edputpd();
			edputdm();
			doclear();
			if (zaplogflg) {
				zaplogflg = 0;
				edvalel(1);
			} else edwrite (effbno, 1, zerobuf.badr);
			if (edgetrct() > 0) {
				doclear();
				printf("got an error while zeroing surrogate\n");
				if (edpdp->pdinfo.relnext >=
				    edpdp->pdinfo.relst + edpdp->pdinfo.relsz) {
					goto runout;
				}
				goto fixit;
			}
nextone:
			if (clrlogflg) clrlog(curbno);
		}
		edclose();
		printf("finished with this disk\n");
		return(0);
	}
}

setzbuf()
{
	if (zerobuf.csz >= edsecsz) {
		zerobuf.csz = edsecsz;
		return(0);
	}
	if (zerobuf.bsz >= edsecsz) {
		memset(zerobuf.badr, 0, edsecsz);
		zerobuf.csz = edsecsz;
		return(0);
	}
	doalloc(&zerobuf, edsecsz, "zerobuf");
	memset(zerobuf.badr, 0, edsecsz);
	return(0);
}

struct eddata sblkbuf;
#define sbp	((struct filsys *) sblkbuf.badr)

partit()
{
	daddr_t lbno, vtocst, vtocend, maxp, pend, pbno, sbbno, inobno;
	register int sz, i, inpart, fspart, j, result;
	char *cp1, *cp2;
	register struct partition *pp;
	long fssize, rootino;

	lbno = effbno - edpdp->pdinfo.logicalst;
	sz = edsecsz;
	if (sz < 512) sz = ((512 + edsecsz - 1)/edsecsz) * edsecsz;
	vtocst = sz/edsecsz;
	vtocend = 2 * vtocst;
	if (lbno < vtocst) {
		printf("CRITICAL PROBLEM: mboot sector is bad\n");
		return(0);
	}
	if (lbno >= vtocst && lbno < vtocend) {
		printf("CRITICAL PROBLEM: vtoc sector is bad\n");
		return(0);
	}
	if (edgetvt()) {
		fprintf(stderr, "%s: no vtoc, cannot classify bad block\n",
			cmdname);
		return(-1);
	}
	if (edvtocp->v_sanity != VTOC_SANE) {
		fprintf (stderr,
			"%s: invalid vtoc, cannot classify bad block\n",
			cmdname);
		return(-1);
	}
	pp = edvtocp->v_part;
	maxp = 0;
	for (inpart = fspart = i = 0; i < V_NUMPAR; i++, pp++) {
		if (pp->p_size <= 0) continue;
		pend = pp->p_start + pp->p_size;
		if (pend > maxp) maxp = pend;
		if (pp->p_start > lbno || pend <= lbno)
			continue;
		inpart |= 1 << i;
		printf("    block in partition %d\n", i);
		pbno = lbno - pp->p_start;
		printf("\tit is block %d in the partition\n", pbno);
		if (pp->p_tag == V_SWAP) {
			printf("\tblock is in a swap partition\n");
		}
		if (pp->p_flag & V_UNMNT) continue;
		if ( pbno == sz/edsecsz) {
			printf("\tif partition %d contained a file system,\n", i);
			printf("\tthat file system's superblock was lost\n");
			continue;
		}
		if (pbno == 0) continue;
		doalloc(&sblkbuf, sz, "sblkbuf");
		sbbno = edpdp->pdinfo.logicalst + pp->p_start + sz/edsecsz;
		if (edread(sbbno, sz/edsecsz, sblkbuf.badr)) {
			printf("\tcannot read potential superblock sector of partition\n");
			continue;
		}
		if (sbp->s_magic != FsMAGIC) continue;
		fssize = sbp->s_fsize *(sz/edsecsz);
		rootino = 2*(sz/edsecsz);
		if(sbp->s_type != Fs1b && sbp->s_type != Fs2b && sbp->s_type != Fs4b)
			printf("\tcan't calculate end of this type of file system\n");
		else {
			if (sbp->s_type == Fs2b) {
			    result = chkbsize(pp, sz);
			    if (result == Fs2b) {
				fssize *= 2;
				rootino *= 2;
			    }
			    else if (result == Fs4b) {
				fssize *= 4;
				rootino *= 4;
			    }
			    else
				printf("\tcan't determine block size to calculate end of this file system,\n\t\troot inode or root directory may be corrupted\n");
			}
			else if (sbp->s_type == Fs4b) {
				fssize *= 4;
				rootino *= 4;
			}
			if (pbno >= fssize) {
				printf("\tblock was past end of file system\n");
				continue;
			}
			printf ("\tpartition has a file system containing the bad block\n");
		}
		sbp->s_state = FsBADBLK;
		fspart++;
		printf("\tmarking file system dirtied by bad block handling\n");
		if (edwrite(sbbno, sz/edsecsz, sblkbuf.badr)) {
			printf("WARNING: write of superblock failed\n");
			continue;
		}
		if(sbp->s_type != Fs1b && sbp->s_type != Fs2b && sbp->s_type != Fs4b){
			printf("\tfile system is type %d and I ",
				sbp->s_type);
			printf("don't know how to do any more\n");
			continue;
		}
		if (sz != 512) {
			printf("\tstrange block size of %d and I ",
				edsecsz);
			printf("don't know how to do any more\n");
			continue;
		}
		fssize = (sbp->s_isize)*(sz/edsecsz);
		if (sbp->s_type == Fs2b) {
			if (result == Fs2b)
				fssize *= 2;
			else if (result == Fs4b)
				fssize *= 4;
			else
				continue;
		}
		else if (sbp->s_type == Fs4b)
			fssize *= 4;
		if (pbno < fssize) {
			printf("\tthe bad block was an inode block\n");
			if (pbno == rootino) {
				printf("\tthe root inode of the file system was lost\n");
			}
		} else printf("\tthe bad block was a file block\n");
	}
	if (fspart > 1) {
		printf("WARNING: %d file systems claimed to contain block\n");
		printf("all but one claim should be false, but I marked all\n");
	}
	return(0);
}

doreboot()
{
	printf("system being rebooted\n");
	sleep (10);
	uadmin(A_REBOOT, AD_BOOT, 0);
	printf("REBOOT FAILED\n");
	exit(BADBEXIT);
}

getloglist()
{
	register int i, j;
	register struct hderecord *rp;

	if (blist.bsz == 0) {
		i = blist.csz = blist.bsz = (NUMRCRDS+HDEQSIZE) * sizeof(daddr_t);
		if (!(blist.badr = malloc(i))) {
			fprintf(stderr, "%s: malloc of blist failed\n", cmdname);
			exit(ERREXIT);
		}
	}
	blistcnt = 0;
	if (edgetel()) return(-1);
	for (i = 0, rp = edhdelp->errlist; i < edhdelp->logentct; i++, rp++) {
		for (j = 0; j < blistcnt; j++) {
			if (blistp[j] == rp->blkaddr) goto contin;
		}
		blistp[blistcnt++] = rp->blkaddr;
contin:		;
	}
	for (i = 0; i < hdeqcnt; i++) {
		if (hdequeue[i].diskdev != devarg) continue;
		for (j = 0; j < blistcnt; j++) {
			if (blistp[j] == hdequeue[i].blkaddr) goto contin2;
		}
		blistp[blistcnt++] = hdequeue[i].blkaddr;
contin2:	;
	}
	return(0);
}

struct hderecord zder;

clrlog(bno)
daddr_t bno;
{
	register struct hderecord *rp, *r2p, *erp;
	if (edgetel()) return(-1);
	for (rp = r2p = edhdelp->errlist, erp = rp + edhdelp->logentct;
	     rp < erp; rp++) {
		if (rp->blkaddr == bno) continue;
		if (rp != r2p) *r2p = *rp;
		r2p++;
	}
	edhdelp->logentct = r2p - edhdelp->errlist;
	while (r2p < erp) *r2p++ = zder;
	if (edputel()) return(-1);
	return(0);
}

#define	ADDON_PREFIX	"/usr/lib"

gen_cmdname()
{
	char		devfile[BUFSIZE];
	int		fd;
	struct bus_type	bus_info;

	strcpy(devfile, DEVNAME);

	mktemp(devfile);

	if (mknod(devfile, (S_IFCHR | S_IREAD | S_IWRITE), devarg) < 0)
		error("%s mknod failed\n", devfile);

	if ((fd = open(devfile, O_RDONLY)) < 0) {
		(void) unlink(devfile);
		error("%s open failed\n", devfile);
	}

	(void) unlink(devfile);

	if (ioctl(fd, B_GETTYPE, &bus_info) < 0) {
		errno = 0;
		return(0);
	} else
		sprintf(Command, "%s/%s/%s", ADDON_PREFIX, bus_info.bus_name, CMDNAME);

	close(fd);
	return(1);
}

void
error(message, data)
char	*message;	/* Message to be reported */
long	data;
{
	(void) fprintf(stderr, "%s: ", cmdname);
	(void) fprintf(stderr, message, data);
	if (errno)
		(void) fprintf(stderr, "%s: %s\n", cmdname, sys_errlist[errno]);
	exit(ERREXIT);
}	/* error() */

/* heuristic function to determine logical block size of file system */
chkbsize(pptr, size)
struct partition *pptr;
int	size;
{
	int results[3];
	int count;
	long address;
	struct dinode *inodes;
	struct direct *dirs;
	char * p1;
	char * p2;
	daddr_t	inobno, dirbno;
	
	results[1] = 0;
	results[2] = 0;

	doalloc(&inobuf, size, "inobuf");
	doalloc(&dirbuf, size, "dirbuf");

	for(count = 1; count < 3; count++) {

		inobno = edpdp->pdinfo.logicalst + pptr->p_start 
			+ 4 * count * (size/edsecsz);
		if (edread(inobno, size/edsecsz, inobuf.badr))
			continue;
		inodes = (struct dinode *)inobuf.badr;
		if ((inodes[1].di_mode & S_IFMT) != S_IFDIR)
			continue;
		if (inodes[1].di_nlink < 2)
			continue;
		if ((inodes[1].di_size % sizeof(struct direct)) != 0)
			continue;
	
		p1 = (char *) &address;
		p2 = (char *) inodes[1].di_addr;
		*p1++ = 0;
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1   = *p2;

		dirbno = edpdp->pdinfo.logicalst + pptr->p_start
			+ 2 * address * count * (size/edsecsz);
		if (edread(dirbno, size/edsecsz, dirbuf.badr))
			continue;
		dirs = (struct direct *)dirbuf.badr;
	
		if(dirs[0].d_ino != 2 || dirs[1].d_ino != 2 )
			continue;
		if(strcmp(dirs[0].d_name,".") || strcmp(dirs[1].d_name,".."))
			continue;
		results[count] = 1;
		}
	
	if(results[1])
		return(Fs2b);
	if(results[2])
		return(Fs4b);
	return(-1);
}
