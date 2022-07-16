/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)devintf:mkdtab/mkdtab.c	1.1.2.1"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<unistd.h>
#include	<devmgmt.h>
#include	<sys/mkdev.h>
#include	<sys/edt.h>
#include	<sys/libxedt.h>
#include	<sys/stat.h>
#include	<sys/vtoc.h>
#include	<sys/vfstab.h>
#include	<sys/fs/s5param.h>
#include	<sys/fs/s5filsys.h>

/*
 * Update device.tab and dgroup.tab to reflect current configuration.
 * Designed so it can be run either once at installation time or after
 * every reboot.  The alias naming scheme used is non-intuitive but
 * is consistent with existing conventions and documentation and with
 * the device numbering scheme used by the disks command.
 * Code borrowed liberally from prtconf, disks and prtvtoc commands.
 */

extern struct mainedt	*getedt();

static struct dpart {
	char	alias[16];
	char	cdevice[20];
	char	bdevice[20];
	long	capacity;
} dparttab[16];

static int		vfsnum;
static char		putdevcmd[512];
static char		cmd[80];
static struct mainedt	*edtp;
static struct vfstab	*vfstab;

static void		ctape(), fdisk(), hdisk(), initialize(), mkdgroups();
static char		*memstr();
static boolean_t	s5part();

main(argc, argv)
int	argc;
char	**argv;
{
	int			i, drivenum;
	struct subdevice	*subdev;
	struct stat		sb;
	major_t			ctcmajor;

	strcpy(cmd, argv[0]);

	initialize();
	subdev = (struct subdevice *)(&edtp->mainedt[edtp->edtsze.esize]);

	/*
	 * Scan the equipped device table looking for configured devices.
	 * We ignore the XDC if we find it because it would confuse the
	 * alias naming scheme and make it difficult to stick to existing
	 * naming conventions while at the same time maintaining a
	 * position sensitive naming scheme.  We ignore any floppy device
	 * associated with the CTC for similar reasons.  If these devices
	 * are configured the administrator can make the entries manually.
	 */
	for (i = 0; i < edtp->edtsze.esize; ++i) {
		if (strcmp(edtp->mainedt[i].dev_name, "SBD") == 0) {
			drivenum = 0;
			while (edtp->mainedt[i].n_subdev--) {
				if (strncmp((const char *)subdev->name,
				  "FD", 2) == 0) {
				    if (stat("/dev/rdiskette", &sb) == 0 &&
					S_ISCHR(sb.st_mode))
					    fdisk();
				} else if (strncmp((const char *)subdev->name,
				    "HD",2) == 0) {
					hdisk(edtp->mainedt[i].opt_slot + 1,
					    drivenum++);
				} else if (drivenum != 2) {
					/*
					 * A mysterious bug sometimes causes
					 * the SBD subdevice names in the
					 * EDT to be corrupted.  If we see
					 * a name we don't recognize and
					 * we haven't found two hard drives
					 * yet, assume it's a hard drive (it
					 * usually is).
					 */
					hdisk(edtp->mainedt[i].opt_slot + 1,
					    drivenum++);
				}
				subdev++;
			}
		} else if (strcmp(edtp->mainedt[i].dev_name, "CTC") == 0) {
			/*
			 * EDT subdevice info for the CTC is unreliable so
			 * we just check for the presence of the appropriate
			 * special dev files to determine whether the ctape
			 * is configured.
			 */
			ctcmajor = (major_t)edtp->mainedt[i].opt_slot;
			if (stat("/dev/rSA/ctape1", &sb) == 0 &&
			    sb.st_rdev == makedev(ctcmajor, (minor_t)3))
				ctape();

			/*
			 * Skip over CTC subdevice entries.
			 */
			while (edtp->mainedt[i].n_subdev--)
				subdev++;

		} else {
			/*
			 * Skip over subdevice entries for unrecognized
			 * devices.
			 */
			while (edtp->mainedt[i].n_subdev--)
				subdev++;
		}
	}

	/*
	 * Update the dgroup.tab file.
	 */
	mkdgroups();
}


/*
 * Add device table entry for the floppy drive.
 */
static void
fdisk()
{
	(void)system("/usr/bin/putdev -a diskette1 \
cdevice=/dev/rdiskette bdevice=/dev/diskette desc=\"Floppy Drive\" \
mountpt=/install volume=diskette type=diskette removable=true \
capacity=1422 fmtcmd=\"/usr/sbin/fmtflop -v /dev/rdiskette\" \
erasecmd=\"/usr/sbin/fmtflop /dev/rdiskette\" copy=true \
mkfscmd=\"/sbin/mkfs -F s5 /dev/rdiskette 1422 1 18\"");
}

/*
 * hdisk() gets information about the specified hard drive from the vtoc
 * and vfstab and adds the disk and partition entries to device.tab. If
 * we can't access the raw disk we simply assume it isn't properly configured
 * and we add no entries to device.tab.
 */
static void
hdisk(ctl, drive)
unsigned	ctl;
int		drive;
{
	char		cdskpath[20];
	char		bdskpath[20];
	char		*mountpoint;
	int		i, j, dpartcnt, fd;
	struct pdsector	pdsector;
	struct vtoc	vtoc;
	struct io_arg	args;

	sprintf(cdskpath, "/dev/rdsk/c%ud%ds6", ctl, drive);
	if ((fd = open(cdskpath, O_RDONLY)) == -1)
		return;

	/*
	 * Read physical description area.
	 */
	args.sectst = 0;
	args.memaddr = (ulong_t)&pdsector;
	args.datasz = sizeof(struct pdsector);
	if (ioctl(fd, V_PDREAD, &args) < 0 || args.retval == V_BADREAD)
		return;

	/*
	 * Read volume table of contents.
	 */
	args.sectst = pdsector.pdinfo.logicalst + 1;
	args.memaddr = (ulong_t)&vtoc;
	args.datasz = sizeof(struct vtoc);
	if (ioctl(fd, V_PREAD, &args) < 0 || args.retval == V_BADREAD ||
	    vtoc.v_sanity != VTOC_SANE)
		return;

	/*
	 * Begin building the putdev command string that will be
	 * used to make the entry for this disk.
	 */
	sprintf(bdskpath, "/dev/dsk/c%ud%ds6", ctl, drive);
	sprintf(putdevcmd, "/usr/bin/putdev -a disk%d cdevice=%s \
bdevice=%s desc=\"Disk Drive\" type=disk display=true remove=true part=true \
removable=false capacity=%ld dpartlist=", drive + 1, cdskpath, bdskpath,
vtoc.v_part[6].p_size);

	/*
	 * Build a table of disk partitions we are interested in and finish
	 * the putdev command string for the disk by adding the dpartlist.
	 */
	dpartcnt = 0;
	for (i = 0; i < (int)vtoc.v_nparts; ++i) {
		if (vtoc.v_part[i].p_size == 0 || vtoc.v_part[i].p_flag != 0)
			continue;
		sprintf(dparttab[dpartcnt].alias, "dpart%d%02d", drive + 1, i);
		sprintf(dparttab[dpartcnt].cdevice, "/dev/rdsk/c%ud%ds%x",
		    ctl, drive, i);
		sprintf(dparttab[dpartcnt].bdevice, "/dev/dsk/c%ud%ds%x",
		    ctl, drive, i);
		dparttab[dpartcnt].capacity = vtoc.v_part[i].p_size;

		if (dpartcnt != 0)
			strcat(putdevcmd, ",");
		strcat(putdevcmd, dparttab[dpartcnt].alias);
		dpartcnt++;
	}
	(void)system(putdevcmd);

	/*
	 * We assemble the rest of the information about the partitions by
	 * looking in the vfstab and at the disk itself.  If vfstab says the
	 * partition contains a non-s5 file system we believe it, otherwise
	 * we call s5part() which will check for an s5 super block on the disk.
	 */
	for (i = 0; i < dpartcnt; i++) {
		for (j = 0; j < vfsnum; j++) {
			if(strcmp(dparttab[i].bdevice,vfstab[j].vfs_special)==0)
				break;
		}
		if (j < vfsnum) {
			/*
			 * Partition found in vfstab.
			 */
			if (strncmp(vfstab[j].vfs_fstype,"s5",2) == 0) {
				/*
				 * Call s5part() but ignore return value. If
				 * s5part() finds a file system it will create
				 * the device.tab entry.  If not, we have a
				 * conflict with what vfstab says so we leave
				 * this partition out of device.tab.
				 */
				(void)s5part(i, vfstab[j].vfs_mountp);
			} else {
				if (strcmp(vfstab[j].vfs_mountp, "-") == 0)
					mountpoint="/mnt";
				else
					mountpoint=vfstab[j].vfs_mountp;
				sprintf(putdevcmd, "/usr/bin/putdev \
-a %s cdevice=%s bdevice=%s desc=\"Disk Partition\" type=dpart removable=false \
capacity=%ld dparttype=fs fstype=%s mountpt=%s", dparttab[i].alias,
dparttab[i].cdevice, dparttab[i].bdevice, dparttab[i].capacity,
vfstab[j].vfs_fstype, mountpoint);
				(void)system(putdevcmd);
			}
		} else {
			/*
			 * Partition not in vfstab.  See if it's an s5
			 * file system; if not, call it a data partition.
			 */
			if (s5part(i, NULL) == B_FALSE) {
				sprintf(putdevcmd, "/usr/bin/putdev \
-a %s cdevice=%s bdevice=%s desc=\"Disk Partition\" type=dpart removable=false \
capacity=%ld dparttype=dp", dparttab[i].alias, dparttab[i].cdevice,
dparttab[i].bdevice, dparttab[i].capacity);
				(void)system(putdevcmd);
			}
		}
	}
}


/*
 * Add device table entry for the cartridge tape drive.
 */
static void
ctape()
{
	(void)system("/usr/bin/putdev -a ctape1 cdevice=/dev/rSA/ctape1 \
bdevice=/dev/SA/ctape1 desc=\"Cartridge Tape Drive\" volume=\"cartridge tape\" \
type=ctape removable=true capacity=45539 bufsize=15872 \
fmtcmd=\"/etc/ctcfmt -v -t /dev/rSA/ctape1\" \
erasecmd=\"/etc/ctcfmt -t /dev/rSA/ctape1\" display=true");
}


static void
initialize()
{
	FILE		*fp;
	int		i;
	struct vfstab	vfsent;
	char		*criteria[5];
	char		**olddevlist;

	/*
	 * Get the equipped device table.
	 */
	if ((edtp = getedt()) == NULL) {
		fprintf(stderr,
		    "%s: can't update device tables:Can't get EDT\n", cmd);
		exit(1);
	}

	/*
	 * Build a copy of vfstab in memory for later use.
	 */
	if ((fp = fopen("/etc/vfstab", "r")) == NULL) {
		fprintf(stderr,
		    "%s: can't update device tables:Can't open /etc/vfstab\n",
		     cmd);
		exit(1);
	}

	/*
	 * Go through the vfstab file once to get the number of entries so
	 * we can allocate the right amount of contiguous memory.
	 */
	vfsnum = 0;
	while (getvfsent(fp, &vfsent) == 0)
		vfsnum++;
	rewind(fp);

	if ((vfstab = (struct vfstab *)malloc(vfsnum * sizeof(struct vfstab)))
	    == NULL) {
		fprintf(stderr,"%s: can't update device tables:Out of memory\n",
		    cmd);
		exit(1);
	}

	/*
	 * Go through the vfstab file one more time to populate our copy in
	 * memory.  We only populate the fields we'll need.
	 */
	i = 0;
	while (getvfsent(fp, &vfsent) == 0 && i < vfsnum) {
		if (vfsent.vfs_special == NULL)
			vfstab[i].vfs_special = NULL;
		else
			vfstab[i].vfs_special = memstr(vfsent.vfs_special);
		if (vfsent.vfs_mountp == NULL)
			vfstab[i].vfs_mountp = NULL;
		else
			vfstab[i].vfs_mountp = memstr(vfsent.vfs_mountp);
		if (vfsent.vfs_fstype == NULL)
			vfstab[i].vfs_fstype = NULL;
		else
			vfstab[i].vfs_fstype = memstr(vfsent.vfs_fstype);
		i++;
	}
	(void)fclose(fp);

	/*
	 * Now remove all current entries of type disk, dpart, ctape
	 * and diskette from the device and device group tables.
	 * Any changes made manually since the last time this command
	 * was run will be lost.  Note that after this we are committed
	 * to try our best to rebuild the tables (i.e. the command
	 * should try not to fail completely after this point).
	 */
	criteria[0] = "type=disk";
	criteria[1] = "type=dpart";
	criteria[2] = "type=ctape";
	criteria[3] = "type=diskette";
	criteria[4] = (char *)NULL;
	olddevlist = getdev((char **)NULL, criteria, 0);
	_enddevtab();	/* getdev() should do this but doesn't */

	for (i = 0; olddevlist[i] != (char *)NULL; i++) {
		(void)sprintf(putdevcmd,"/usr/bin/putdev -d %s", olddevlist[i]);
		(void)system(putdevcmd);
	}

	sprintf(putdevcmd, "/usr/bin/putdgrp -d disk 2>/dev/null");
	(void)system(putdevcmd);
	sprintf(putdevcmd, "/usr/bin/putdgrp -d dpart 2>/dev/null");
	(void)system(putdevcmd);
	sprintf(putdevcmd, "/usr/bin/putdgrp -d ctape 2>/dev/null");
	(void)system(putdevcmd);
	sprintf(putdevcmd, "/usr/bin/putdgrp -d diskette 2>/dev/null");
	(void)system(putdevcmd);




}


/*
 * s5part() reads the raw partition looking for an s5 file system.  If
 * it finds one it adds a partition entry to device.tab using the
 * information passed as arguments and additional info read from the
 * super-block.  Returns B_TRUE if an s5 file system is found, otherwise
 * returns B_FALSE.
 */
static boolean_t
s5part(dpindex, mountpt)
int	dpindex;
char	*mountpt;
{
	int		fd;
	long		lbsize, ninodes;
	struct filsys	s5super;
	char		*mountpoint;
	struct stat	psb, rsb;


	if ((fd = open(dparttab[dpindex].cdevice, O_RDONLY)) == -1)
		return(B_FALSE);

	if (lseek(fd, SUPERBOFF, SEEK_SET) == -1) {
		(void)close(fd);
		return(B_FALSE);
	}

	if (read(fd, &s5super, sizeof(struct filsys)) < sizeof(struct filsys)) {
		(void)close(fd);
		return(B_FALSE);
	}

	(void)close(fd);

	if (s5super.s_magic != FsMAGIC)
		return(B_FALSE);

	switch(s5super.s_type) {

	case Fs1b:
		lbsize = 512;
		ninodes = (s5super.s_isize - 2) * 8;
		break;

	case Fs2b:
		lbsize = 1024;	/* may be wrong for 3b15 */
		ninodes = (s5super.s_isize - 2) * 16; /* may be wrong for 3b15*/
		break;

	case Fs4b:
		lbsize = 2048;
		ninodes = (s5super.s_isize - 2) * 32;
		break;

	default:
		return(B_FALSE);
	}

	if (mountpt != NULL) {
		mountpoint = mountpt;	/* Use mount point passed as arg */
	} else {
		if (strcmp(s5super.s_fname, "root") == 0 &&
		    stat(dparttab[dpindex].bdevice, &psb) == 0 &&
		    stat("/dev/root", &rsb) == 0 &&
		    psb.st_rdev ==  rsb.st_rdev)
			mountpoint = "/";
		else
			mountpoint = "/mnt";
	}
	sprintf(putdevcmd,"/usr/bin/putdev -a %s cdevice=%s bdevice=%s \
desc=\"Disk Partition\" type=dpart removable=false capacity=%ld dparttype=fs \
fstype=s5 mountpt=%s fsname=%s volname=%s lbsize=%ld nlblocks=%ld ninodes=%ld",
dparttab[dpindex].alias, dparttab[dpindex].cdevice, dparttab[dpindex].bdevice,
dparttab[dpindex].capacity, mountpoint, s5super.s_fname, s5super.s_fpack,
lbsize, s5super.s_fsize, ninodes);

	(void)system(putdevcmd);
	return(B_TRUE);
}


/*
 * Update the dgroup.tab file with information from the updated device.tab.
 */
static void
mkdgroups()
{
	int	i;
	char	*criteria[2];
	char	**devlist;

	criteria[1] = (char *)NULL;

	criteria[0] = "type=disk";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp disk");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);

	criteria[0] = "type=dpart";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp dpart");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);

	criteria[0] = "type=ctape";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp ctape");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);

	criteria[0] = "type=diskette";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp diskette");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);
}


static char *
memstr(str)
register char	*str;
{
	register char	*mem;

	if ((mem = (char *)malloc((uint_t)strlen(str) + 1)) == NULL) {
		fprintf(stderr,"%s: can't update device tables:Out of memory\n",
		    cmd);
		exit(1);
	}
	return(strcpy(mem, str));
}
