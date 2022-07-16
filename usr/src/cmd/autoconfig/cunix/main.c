/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/main.c	1.16"

#include <sys/types.h>
#include <sys/localtypes.h>
#include <stdio.h>
#include <sys/signal.h>
#include <sys/sys3b.h>
#include <sys/stat.h>
#include <sys/boothdr.h>
#include <sys/error.h>
#include <libelf.h>


/*
 * path name variables. initialized to default values.
 *
 */
char *larg=NULL; 			/* arguments to loader  */
char *slash_boot="/boot";	  	/* /boot directory name */
char *etcsystem="/stand/system";	/* /stand/system path name */
char cofile[]="/conf.o";
char *conf_file;
char *conf_dir="/config";
char *raw_disk="/dev/rSA/disk1";	/* default root device	-r option */
char *unix_abs="/unix_test";		/* default output -o option */
char *loader= NULL;			/* loader to use -l option */
char *ifile=NULL;			/* default ifile set in check_ifile -i*/
int escapehatch = 0;			/* undocumented -x option, see off.c  */
char cwd[80];				/* current working directorey */
unsigned char loader_type;	/* assume linking COFF files */

boolean DebugMode = FALSE; 
boolean QuietMode = TRUE; 
boolean confdebug = FALSE;

time_t umtime = 0;			/* time of unix overwriting */
void catcher();
void buildconfig();

struct s3bboot	 bootprogram;		/* kernel path name */


/*
 * main()
 * sort out arguments and call loadunix
 */

main(argc,argv)
char **argv;
{
	FILE *ptr;
	struct stat sb;
	int c;
	int errflg=0;
	char *unix_sym;
	extern char *optarg;
	extern int optind;

	setbuf(stdout,0);
	setbuf(stderr,0);


	if ((ptr=popen("/usr/bin/pwd","r")) != NULL){
		fgets(cwd,sizeof(cwd),ptr);
		c = strlen(cwd);
		cwd[c - 1]= '\0';
		pclose(ptr);
	}
	else {
		panic("unable to establish current working directory\n");
		exit(1);
	     }
	
	(void)ulimit(2, 8192); /* if super user then o.k */
	
	while ((c= getopt(argc,argv,"f:o:dvr:l:b:i:a:gxc:")) != -1)
		switch(c){
		case 'i':
			ifile = optarg;
			break;
		case 'b':
			slash_boot = optarg;
			break;
		case 'f':
			etcsystem = optarg;
			break;
		case 'c':
			conf_dir = optarg;
			break;
		case 'o':
			unix_abs = optarg;
			break;
		case 'd':
			DebugMode = TRUE;
			break;
		case 'v':
			QuietMode = FALSE;
			break;
		case 'r':
			raw_disk = optarg;
			break;
		case 'l':
			loader = optarg;
			break;
		case 'g':
			confdebug = TRUE;
			break;
		case 'a':
			larg = optarg;
			break;
		case 'x':
			escapehatch=1;
			break;
		case '?':
			errflg++;
			break;
		}
	if (errflg || (optind != argc)){
		fprintf(stderr,"Usage: cunix [-i ifile] [-f system] [-c config_dir] [-o outfile] [-r raw_disk] [-l link_ed] [-b boot_dir] [-a \"ld_args\"] [-d] [-v] [-g]\n");
		exit(1);
	}

	/*
	**	init libelf.  if fail, must exit cause
	**	cause patch_addr() depends on libelf for
	**	both COFF and ELF
	*/
	if ((elf_version(EV_CURRENT)) == EV_NONE) {
		error(ER105);
		exit (1);
	}

	buildconfig(conf_dir);
	/*
	 * Protect the user being confused if ld interrupted and output 
	 * is a symbolic link. Symbolic links are ugly UHH.
	 */

	if (lstat(unix_abs,&sb) == 0){
		if ((sb.st_mode & S_IFLNK) == S_IFLNK){
			if ((unix_sym = (char *)malloc(sb.st_size + 1)) == NULL){
				panic("Can't malloc space to resolve symbolic link %s\n", unix_abs);
				exit(1);
			}
			readlink(unix_abs, unix_sym, sb.st_size);
			unix_abs = unix_sym;
			unix_abs[sb.st_size]='\0';
		}
		if (stat(unix_abs,&sb) == 0){
			if (!(sb.st_mode & S_IFREG)){
				error(ER6,unix_abs);
				exit(1);
			}	
		umtime = sb.st_mtime;
		}
	}

	if (loader != NULL && stat(loader,&sb) == -1){
		error(ER80,loader);
		exit(1);
	}
		
	
	/*
	 * May not be working with a job control shell.
 	 */
	if ((void(*)())sigset(SIGINT,SIG_IGN) != SIG_IGN)
			sigset(SIGINT, catcher);
	loadunix();

	exit(0);
}

 void
buildconfig(cdir)
char *cdir;
{
	struct stat sb;
	
	conf_file = (char *) malloc(strlen(cdir) + strlen(cofile) + 1);

	if (conf_file == NULL){
		panic("Can't malloc space for configuration file\n");
		exit(1);
	}

	strcpy(conf_file,cdir);
	strcat(conf_file,cofile);
}

  void
catcher(sig)
int sig;
{
	struct stat sb;

	chdir(cwd);
	if (!confdebug)
		unlink(conf_file);

	if (stat(unix_abs,&sb) == 0){
		fprintf(stderr,"\n");
		if (sb.st_mtime > umtime){
			fprintf(stderr,"%s not complete, removing\n",unix_abs);
			unlink(unix_abs);
		} else 
			fprintf(stderr,"original %s not modified\n",unix_abs);
			
	}
	exit(1);
}

