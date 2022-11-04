/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:pkgtrans.c	1.15.1.1"

#include <stdio.h>
#include <varargs.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <pkginfo.h>
#include <pkgstrct.h>
#include <pkgtrans.h>
#include <pkgdev.h>

extern int	errno;
extern char	*pkgdir;
extern FILE	*epopen();
extern char	**gpkglist(), *devattr(), *fpkginst();
extern void	progerr(),
		logerr(),
		rpterr(),
		ds_order();
extern int	getvol(),
		rrmdir(),
		mkdir(),
		chdir(),
		access(),
		ckvolseq(),
		isdir(),
		ds_init(),
		ds_findpkg(),
		ds_getpkg(),
		fpkginfo(),
		esystem(),
		devtype(),
		pkgmount(),
		pkgumount();

#define CMDSIZ	512
#define LSIZE	512
#define TMPSIZ	64

#define PKGINFO	"pkginfo"
#define PKGMAP	"pkgmap"
#define INSTALL	"install"
#define RELOC	"reloc"
#define ROOT	"root"
#define DDPROC		"/bin/dd"
#define CPIOPROC	"/bin/cpio"

#define MSG_TRANSFER	"Transferring <%s> package instance\n"
#define MSG_RENAME 	"\t... instance renamed <%s> on destination\n"
#define MSG_CORRUPT \
	"Volume is corrupt or is not part of the appropriate package."

#define ERR_TRANSFER	"unable to complete package transfer"
#define MSG_SEQUENCE	"- volume is out of sequence"
#define MSG_MEM		"- no memory"
#define MSG_CMDFAIL	"- process <%s> failed, exit code %d"
#define MSG_POPEN	"- popen of <%s> failed, errno=%d"
#define MSG_PCLOSE	"- pclose of <%s> failed, errno=%d"
#define MSG_BADDEV	"- invalid or unknown device <%s>"
#define MSG_GETVOL	"- unable to obtain package volume"
#define MSG_NOSIZE 	"- unable to obtain maximum part size from pkgmap"
#define MSG_CHDIR	"- unable to change directory to <%s>"
#define MSG_FSTYP	"- unable to determine filesystem type for <%s>" 
#define MSG_NOTEMP	"- unable to create or use temporary directory <%s>"
#define MSG_SAMEDEV	"- source and destination represent the same device"
#define MSG_NOTMPFIL	"- unable to create or use temporary file <%s>"
#define MSG_NOPKGMAP	"- unable to open pkgmap for <%s>"
#define MSG_BADPKGINFO	"- unable to determine contents of pkginfo file"
#define MSG_NOPKGS	"- no packages were selected from <%s>"
#define MSG_MKDIR	"- unable to make directory <%s>"
#define MSG_NOEXISTS \
	"- package instance <%s> does not exist on source device"
#define MSG_EXISTS \
	"- no permission to overwrite existing path <%s>"
#define MSG_DUPVERS \
	"- identical version of <%s> already exists on destination device"
#define MSG_TWODSTREAM \
	"- both source and destination devices cannot be a datastream"

static struct pkgdev srcdev, dstdev;
static char	*tmpdir;
static char	*tmppath;
static char	dstinst[16];
static void	(*func)();
static void	cleanup(), sigtrap();
static int	pkgxfer(), wdsheader(), ckoverwrite();
int		pkgtrans();

char	**xpkg;	/* array of transferred packages */
int	nxpkg;

static	char *allpkg[] = {
	"all",
	NULL
};

int
pkghead(device)
char	*device;
{
	static char	*tmppath;
	char	*pt;
	int	n;

	cleanup();
	if(tmppath) {
		/* remove any previous tmppath stuff */
		rrmdir(tmppath);
		free(tmppath);
		tmppath = NULL;
	}

	if(device == NULL)
		return(0);
	else if((device[0] == '/') && !isdir(device)) {
		pkgdir = device;
		return(0);
	} else if((pt = devattr(device, "pathname")) && !isdir(pt)) {
		pkgdir = pt;
		return(0);
	}

	if(devtype(device, &srcdev)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_BADDEV, device);
		return(1);
	}
	srcdev.rdonly++;

	if(srcdev.norewind) {
		/* need to provide a temp directory where we can 
		 * unwrap the datastream
		 */
		tmppath = tmpnam(NULL);
		tmppath = strdup(tmppath);
		if(tmppath == NULL) {
			progerr(ERR_TRANSFER);
			logerr(MSG_MEM);
			return(1);
		}
		if(mkdir(tmppath, 0755)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_MKDIR, tmppath);
			return(1);
		}
		if(n=pkgtrans(device, tmppath, allpkg, PT_SILENT|PT_INFO_ONLY))
			return(n);
		/* pkgtrans has set pkgdir */
	} else if(srcdev.mount) {
		pkgmount(&srcdev, NULL, 1, 0);
		pkgdir = srcdev.mount;
	}
	return(0);
}

/* will return 0, 1, 3, or 99 */
int
pkgtrans(device1, device2, pkg, options)
char	*device1, *device2, **pkg;
int	options;
{
	char	*src, *dst;
	int	errflg, i, n;

	func = signal(SIGINT, sigtrap);

	/* transfer spool to appropriate device */
	if(devtype(device1, &srcdev)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_BADDEV, device1);
		return(1);
	}
	srcdev.rdonly++;

	if(devtype(device2, &dstdev)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_BADDEV, device2);
		return(1);
	}

	if(srcdev.norewind && dstdev.norewind) {
		progerr(ERR_TRANSFER);
		logerr(MSG_TWODSTREAM);
		return(1);
	}
	if((srcdev.bdevice && dstdev.bdevice) &&
	!strcmp(srcdev.bdevice, dstdev.bdevice)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_SAMEDEV);
		return(1);
	}
	if((srcdev.dirname && dstdev.dirname) &&
	!strcmp(srcdev.dirname, dstdev.dirname)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_SAMEDEV);
		return(1);
	}

	if(srcdev.norewind) {
		if(n = getvol(srcdev.cdevice, NULL, NULL, NULL)) {
			cleanup();
			if(n == 3)
				return(3);
			progerr(ERR_TRANSFER);
			logerr(MSG_GETVOL);
			return(1);
		}
		if(srcdev.dirname = tmpnam(NULL)) 
			tmpdir = srcdev.dirname = strdup(srcdev.dirname);
		if((srcdev.dirname == NULL) || mkdir(srcdev.dirname) || 
		   chdir(srcdev.dirname)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOTEMP, srcdev.dirname);
			cleanup();
			return(1);
		}
		if(ds_init(srcdev.norewind, pkg)) {
			cleanup();
			return(1);
		}
	} else if(srcdev.mount) {
		if(n = pkgmount(&srcdev, NULL, 1, 0)) {
			cleanup();
			return(n);
		}
	}
	src = srcdev.dirname;

	if(dstdev.mount) {
		if(n = pkgmount(&dstdev, NULL, 0, 0)) {
			cleanup();
			return(n);
		}
	}
	dst = dstdev.dirname;

	if(chdir(src)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_CHDIR, src);
		cleanup();
		return(1);
	}

	xpkg = pkg = gpkglist(src, pkg);
	if(!pkg) {
		progerr(ERR_TRANSFER);
		logerr(MSG_NOPKGS, src);
		cleanup();
		return(1);
	}
	for(nxpkg=0; pkg[nxpkg]; )
		nxpkg++; /* count */

	if(srcdev.norewind)
		ds_order(pkg); /* order requests */

	if(dstdev.norewind) {
		if(n = getvol(dstdev.cdevice, NULL, NULL, NULL)) {
			cleanup();
			if(n == 3)
				return(3);
			progerr(ERR_TRANSFER);
			logerr(MSG_GETVOL);
			return(1);
		}
		if(wdsheader(src, dstdev.norewind, pkg)) {
			cleanup();
			return(1);
		}
	}

	errflg = 0;
	for(i=0; pkg[i]; i++) {
		if(errflg = pkgxfer(pkg[i], options)) {
			pkg[i] = NULL;
			if(dstdev.norewind || (errflg != 2))
				break;
		} else if(strcmp(dstinst, pkg[i]))
			pkg[i] = strdup(dstinst);
	}

	if((dstdev.norewind == NULL) && dst)
		pkgdir = strdup(dst);
	cleanup();
	return(errflg);
}

static int
wdsheader(src, device, pkg)
char	*src, *device, **pkg;
{
	FILE	*pp, *fp;
	char	path[PATH_MAX], cmd[CMDSIZ];
	int	i, nparts, maxpsize;

	/* 
	 * open pipe to dd() so that we can write the 
	 * datastream header to the device
	 */
	(void) sprintf(cmd, "%s of=%s", DDPROC, device);
	if((pp = epopen(cmd, "w")) == NULL) {
		rpterr();
		progerr(ERR_TRANSFER);
		logerr(MSG_POPEN, cmd, errno);
		return(1);
	}

	nparts = maxpsize = 0;
	(void) fprintf(pp, "# OA&M DATASTREAM FORMAT 1.0\n");
	for(i=0; pkg[i]; i++) {
		(void) sprintf(path, "%s/%s/%s", src, pkg[i], PKGMAP);
		if((fp = fopen(path, "r")) == NULL) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOPKGMAP, pkg[i]);
			sighold(SIGINT);
			(void) pclose(pp);
			sigrelse(SIGINT);
			return(1);
		}
		if(fscanf(fp, ":%d%d", &nparts, &maxpsize) != 2) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOSIZE);
			(void) fclose(fp);
			return(1);
		}
		(void) fprintf(pp, "%s %d %d\n", pkg[i], nparts, maxpsize);
		(void) fclose(fp);
	}
	sighold(SIGINT);
	if(pclose(pp)) {
		sigrelse(SIGINT);
		rpterr();
		progerr(ERR_TRANSFER);
		logerr(MSG_PCLOSE, cmd, errno);
		return(1);
	}
	sigrelse(SIGINT);

	/*
	 * write the first cpio() archive to the datastream
	 * which should contain the pkginfo & pkgmap files
	 * for all packages
	 */
	(void) sprintf(cmd, "%s -oc >%s", CPIOPROC, device);
	if((pp = epopen(cmd, "w")) == NULL) {
		rpterr();
		progerr(ERR_TRANSFER);
		logerr(MSG_POPEN, cmd, errno);
		cleanup();
		return(1);
	}
	for(i=0; pkg[i]; i++) {
		(void) fprintf(pp, "%s/%s\n", pkg[i], PKGINFO);
		(void) fprintf(pp, "%s/%s\n", pkg[i], PKGMAP);
	}
	sighold(SIGINT);
	if(pclose(pp)) {
		sigrelse(SIGINT);
		rpterr();
		progerr(ERR_TRANSFER);
		logerr(MSG_PCLOSE, cmd, errno);
		return(1);
	}
	sigrelse(SIGINT);
	return(0);
}

static int
ckoverwrite(dir, inst, options)
char	*dir;
char	*inst;
int	options;
{
	char	path[PATH_MAX];

	(void) sprintf(path, "%s/%s", dir, inst);
	if(access(path, 0) == 0) {
		if(options & PT_OVERWRITE)
			return(rrmdir(path));
		progerr(ERR_TRANSFER);
		logerr(MSG_EXISTS, path);
		return(1);
	}
	return(0);
}

static int
pkgxfer(srcinst, options)
char	*srcinst;
int	options;
{
	struct pkginfo info;
	FILE	*fp, *pp;
	char	*pt, *src, *dst;
	char	dstdir[PATH_MAX],
		temp[PATH_MAX], 
		srcdir[PATH_MAX],
		cmd[CMDSIZ];
	int	i, n, part, nparts, maxpartsiz;

	info.pkginst = NULL; /* required initialization */

	/*
	 * when this routine is entered, the first part of
	 * the package to transfer is already available in
	 * the directory indicated by 'src' --- unless the
	 * source device is a datstream, in which case only
	 * the pkginfo and pkgmap files are available in 'src'
	 */
	src = srcdev.dirname;
	dst = dstdev.dirname;

	if(!(options & PT_SILENT))
		(void) fprintf(stderr, MSG_TRANSFER, srcinst);
	(void) strcpy(dstinst, srcinst);

	if(dstdev.norewind == NULL) {
		/* destination is a (possibly mounted) directory */
		(void) sprintf(dstdir, "%s/%s", dst, dstinst);

		/*
		 * need to check destination directory to assure
		 * that we will not be duplicating a package which
		 * already resides there (though we are allowed to
		 * overwrite the same version)
		 */
		pkgdir = src;
		if(fpkginfo(&info, srcinst)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOEXISTS, srcinst);
			(void) fpkginfo(&info, NULL);
			return(1);
		}
		pkgdir = dst;

		(void) strcpy(temp, srcinst);
		if(pt = strchr(temp, '.'))
			*pt = '\0';
		(void) strcat(temp, ".*");

		if(pt = fpkginst(temp, info.arch, info.version)) {
			/* the same instance already exists, although
			 * its pkgid might be different
			 */
			if(options & PT_OVERWRITE) {
				(void) strcpy(dstinst, pt);
				(void) sprintf(dstdir, "%s/%s", dst, dstinst);
			} else {
				progerr(ERR_TRANSFER);
				logerr(MSG_DUPVERS, temp);
				(void) fpkginfo(&info, NULL);
				(void) fpkginst(NULL);
				return(2);
			}
		} else if(options & PT_OVERWRITE) {
			/* we're allowed to overwrite, but there seems
			 * to be no valid package to overwrite, so act
			 * as if we weren't given permission to overwrite
			 * --- this keeps us from removing a destination
			 * instance which is named the same as the source
			 * instance, but really reflects a different pkg!
			 */
			options &= (~PT_OVERWRITE);
		}
		(void) fpkginfo(&info, NULL);
		(void) fpkginst(NULL);

		if(options & PT_RENAME) { 
			/* 
			 * find next available instance by appending numbers
			 * to the package abbreviation until the instance
			 * does not exist in the destination directory
			 */
			for(i=2; (access(dstdir, 0) == 0); i++) {
				(void) sprintf(dstinst, "%s.%d", temp, i);
				(void) sprintf(dstdir, "%s/%s", dst, dstinst);
			}
		} else if(ckoverwrite(dst, dstinst, options))
			return(2);

		if(isdir(dstdir) && mkdir(dstdir, 0755)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_MKDIR, dstdir);
			return(1);
		}
	}

	if(!(options & PT_SILENT) && strcmp(dstinst, srcinst))
		(void) fprintf(stderr, MSG_RENAME, dstinst);

	(void) sprintf(srcdir, "%s/%s", src, srcinst);
	if(chdir(srcdir)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_CHDIR, srcdir);
		return(1);
	}

	if(srcdev.norewind) {	/* unpack the datatstream into a directory */
		/*
		 * transfer pkginfo & pkgmap first
		 */
		(void) sprintf(cmd, "%s -pudm %s", CPIOPROC, dstdir);
		if((pp = epopen(cmd, "w")) == NULL) {
			rpterr();
			progerr(ERR_TRANSFER);
			logerr(MSG_POPEN, cmd, errno);
			return(1);
		}
		(void) (void) fprintf(pp, "%s\n%s\n", PKGINFO, PKGMAP);
		sighold(SIGINT);
		if(pclose(pp)) {
			sigrelse(SIGINT);
			rpterr();
			progerr(ERR_TRANSFER);
			logerr(MSG_PCLOSE, cmd, errno);
			return(1);
		}
		sigrelse(SIGINT);

		if(options & PT_INFO_ONLY)
			return(0); /* don't transfer objects */

		if(chdir(dstdir)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_CHDIR, dstdir);
			return(1);
		}

		/*
		 * for each part of the package, use cpio() to
		 * unpack the archive into the destination directory
		 */
		nparts = ds_findpkg(srcdev.norewind, srcinst);
		if(nparts < 0) {
			progerr(ERR_TRANSFER);
			return(1);
		}
		for(part=1; part <= nparts;) {
			if(ds_getpkg(srcdev.norewind, srcinst, part)) {
				progerr(ERR_TRANSFER);
				return(1);
			}
			part++;
			if(dstdev.mount) {
				(void) chdir("/");
				if(pkgumount(&dstdev))
					return(1);
				if(part <= nparts) {
					if(n = pkgmount(&dstdev, NULL, part+1, 
					  nparts))
						return(n);
					if(ckoverwrite(dst, dstinst, options))
						return(1);
					if(isdir(dstdir) && mkdir(dstdir, 0755)) {
						progerr(ERR_TRANSFER);
						logerr(MSG_MKDIR, dstdir);
						return(1);
					}
					/* 
					 * since volume is removable, each part
					 * must contain a duplicate of the 
					 * pkginfo file to properly identify the
					 * volume
					 */
					if(chdir(srcdir)) {
						progerr(ERR_TRANSFER);
						logerr(MSG_CHDIR, srcdir);
						return(1);
					}
					if((pp = epopen(cmd, "w")) == NULL) {
						rpterr();
						progerr(ERR_TRANSFER);
						logerr(MSG_POPEN, cmd, errno);
						return(1);
					}
					(void) fprintf(pp, "pkginfo");
					if(pclose(pp)) {
						rpterr();
						progerr(ERR_TRANSFER);
						logerr(MSG_PCLOSE, cmd, errno);
						return(1);
					}
					if(chdir(dstdir)) {
						progerr(ERR_TRANSFER);
						logerr(MSG_CHDIR, dstdir);
						return(1);
					}
				}
			}
		}
		return(0);
	}

	/* 
	 * read nparts and maxpartsiz from pkgmap
	 */
	if((fp = fopen(PKGMAP, "r")) == NULL) {
		progerr(ERR_TRANSFER);
		logerr(MSG_NOPKGMAP, srcinst);
		return(1);
	}
	nparts = 1;
	if(fscanf(fp, ":%d%d", &nparts, &maxpartsiz) != 2) {
		progerr(ERR_TRANSFER);
		logerr(MSG_NOSIZE);
		(void) fclose(fp);
		return(1);
	}
	(void) fclose(fp);

	if(srcdev.mount) {
		if(ckvolseq(srcdir, 1, nparts)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_SEQUENCE);
			return(1);
		}
	}

	for(part=1; part <= nparts; ) {
		if(options & PT_INFO_ONLY)
			nparts = 0;

		if(part == 1) {
			(void) sprintf(cmd, "find %s %s", PKGINFO, PKGMAP);
			if(nparts && (isdir(INSTALL) == 0)) {
				(void) strcat(cmd, " ");
				(void) strcat(cmd, INSTALL);
			}
		} else
			(void) sprintf(cmd, "find %s", PKGINFO);

		if(nparts > 1) {
			(void) sprintf(temp, "%s.%d", RELOC, part);
			if(isdir(temp) == 0) {
				(void) strcat(cmd, " ");
				(void) strcat(cmd, temp);
			}
			(void) sprintf(temp, "%s.%d", ROOT, part);
			if(isdir(temp) == 0) {
				(void) strcat(cmd, " ");
				(void) strcat(cmd, temp);
			}
		} else if(nparts) {
			if(isdir(RELOC) == 0) {
				(void) strcat(cmd, " ");
				(void) strcat(cmd, RELOC);
			}
			if(isdir(ROOT) == 0) {
				(void) strcat(cmd, " ");
				(void) strcat(cmd, ROOT);
			}
		}
		if(dstdev.norewind) {
			(void) sprintf(cmd+strlen(cmd), 
				" -print | %s -oc >%s",
				CPIOPROC, dstdev.norewind);
		} else {
			(void) sprintf(cmd+strlen(cmd), 
				" -print | %s -pdum %s",
				CPIOPROC, dstdir);
		}

		if(n = esystem(cmd)) {
			rpterr();
			progerr(ERR_TRANSFER);
			logerr(MSG_CMDFAIL, cmd, n);
			return(1);
		}

		part++;
		if(srcdev.mount) {
			(void) chdir("/");
			if(pkgumount(&srcdev))
				return(1);
			while(part <= nparts) {
				/* read only */
				if(n = pkgmount(&srcdev, NULL, part, nparts))
					return(n);
				if(chdir(srcdir)) {
					progerr(ERR_TRANSFER);
					logerr(MSG_CORRUPT, srcdir);
					(void) chdir("/");
					pkgumount(&srcdev);
					continue;
				}
				if(ckvolseq(srcdir, part, nparts)) {
					(void) chdir("/");
					pkgumount(&srcdev);
					continue;
				}
				break;
			}
		}
		if(dstdev.mount) {
			if(pkgumount(&dstdev))
				return(1);
			if(part <= nparts) {
				/* writable */
				pkgmount(&dstdev, NULL, part, nparts);
				if(ckoverwrite(dst, dstinst, options))
					return(1);
			}
		}
	}
	return(0);
}

static void
sigtrap(signo)
int	signo;
{

	cleanup();

 	if(tmppath) { 
		rrmdir(tmppath);
		free(tmppath);
		tmppath = NULL;
	}
	if(func && (func != SIG_DFL) && (func != SIG_IGN))
		/* must have been an interrupt handler */
		(*func)(signo);
}

static void
cleanup()
{
	chdir("/");
 	if(tmpdir) { 
		rrmdir(tmpdir);
		free(tmpdir);
		tmpdir = NULL;
	}
	if(srcdev.mount)
		pkgumount(&srcdev);
	if(dstdev.mount)
		pkgumount(&dstdev);
}
