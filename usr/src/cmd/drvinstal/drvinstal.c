/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)drvinstal:drvinstal.c	1.14.3.1"
#include	<stdio.h>
#include	<ctype.h>
#include	<filehdr.h>
#include	<fcntl.h>

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/mkdev.h>
#include	<dirent.h>

#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>


static char VERSION[] = "1.0";			/* Version of drvinstall */

void		error();
void		exit_msg();
char		*getcwd();			/* to save current working dir */

extern int	errno;
extern char *sys_errlist[];
extern int sys_nerr;


int removeflg;			/* Indicates an uninstall */
int cntlflag;			/* Indicates specified # controllers */

char	*ncntrl;		/* Pointer to number of controllers */
char	*cmdname;		/* Pointer to arg[0]; ie. command name */
char	*masbuf;		/* Pointer to tmp buffer space */



/*
 * Paths to the appropiate files
 */


char	*mod_name = NULL;	/* Path to module object file */
char	*master_name = NULL;	/* Path for input master file */
char	*boot_name = NULL;	/* Path for mkboot output */
char	*system_name = NULL;	/* Path to system file */
char	*omas = NULL;		
char	*version = NULL;

/*
 *	Default path names
 */

static char master_def[] = "/etc/master.d";
static char sys_def[] = "/etc";


char modname[MAXPATHLEN];
char masname[MAXPATHLEN];
char bootname[MAXPATHLEN];
char systemname[MAXPATHLEN];
char tempfile[MAXPATHLEN];
char tmpdir[MAXPATHLEN];		/* current working directory */

/*
 * defines for master file field sizes
 * IF THE MASTER FILE FORMAT CHANGES
 * THEN THESE NUMBERS MUST CHANGE.
 *
 */

#define MODNSIZE 10		/* Max module name size */
#define FLAGFLDSZ 12		/* Max number of flags +1 */
#define VECSZ	4		/* Max size of vector field + 1 */
#define PRFSZ	5		/* Max prefix size + 1 */
#define MAJSZ	4		/* Max major number size + 1 */
#define MYBSIZE 2048		/* Temp buffer size */

#define EQ(x,y)		(strcmp(x,y)==0)

char capital_name[MODNSIZE];	/* Storage for module name in CAPS */
char lower_name[MODNSIZE];	/* Storage for module name in lower case */
char flags[FLAGFLDSZ];		/* Storage for flags from master file */
char vecs[VECSZ];		/* Storage for #vecs from master file */
char prefx[PRFSZ];		/* Storage for prefix from master file */
char major_number[MAJSZ];	/* Storage for major number from master file */
char tmpbuf[256];		/* Storage for remainder of line */





FILE	*masterfile = NULL;
FILE	*systemfile = NULL;
FILE	*tmpmast = NULL;

int	rtnv;			/* Exit return value set to 0 for success */
int	rtnval;			/* Storage for return value */

				/* An array of valid major
				 * numbers for software
				 * drivers. A zero entry
				 * indicates legal number.
				 * A non-zero entry indicates
				 * that number reserved for
				 * hardware drivers.
				 */
int	used_maj[MAXMAJ] =
#ifdef u3b2
			 {1,1,2,3,4,5,6,7,
#else
			 {0,0,0,3,4,5,6,7,
#endif
			 8,9,10,11,12,13,14,15,
#ifdef u3b2
			 16,17,18,19,20,21,22,23,
			 24,25,26,27,28,29,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
#else
			 0,17,18,19,20,21,22,23,
			 24,25,26,27,28,29,30,31,
			 0,33,34,35,36,37,38,39,
			 40,41,42,43,44,45,46,47,
#endif
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
#ifdef u3b2
			 0,0,0,0,0,0,0,0,
			 72,73,74,75,76,77,78,79,
			 80,81,82,83,84,85,86,87,
			 88,89,90,91,92,93,94,95,
			 96,97,98,99,100,101,102,103,
			 104,105,106,107,108,109,110,111,
			 112,113,114,115,116,117,118,119,
			 120,121,122,123,124,125,126,127,
#else
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
#endif
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,

			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0};	/* Currently MAXMAJ is 255 */
int	debug;

main(argc,argv)
int	argc;
char	*argv[];
{
	extern	char	*optarg;
	extern	int	optind;

	int	i, c;
	int 	sysed=0;
	int 	mkboot=0;		/* flag for mkboot call */
	int 	frcrmv=0;		/* flag for force removal */
	char	cwd[100];		/* to save current working directory */

	debug = rtnv = rtnval = 0;
	removeflg = 0;
	cntlflag = 0;
	masbuf = (char *)NULL;

	cmdname = argv[0];

	while ((c = (int)getopt(argc, argv, "xufnbd:m:v:s:o:c:")) != EOF)
		switch (c) {

		case 'x':		/* debug flag */
			debug = 1;
			break;

		case 'n':		/* Don't edit system file */
			sysed = 1;
			break;

		case 'b':		/* Don't run mkboot */
			mkboot = 1;
			break;

		case 'd':		/* 
					 * Path to object module 
					 * If not specified then assume /boot
					 *
					 */
			mod_name = optarg;
			if (mod_name == NULL)
				exit_msg("ERROR:option -d No argument specified");
			break;

		case 'o':		/*
					 * Path of mkboot'd files 
					 * If not specified then assume /boot
					 *
					 */
			boot_name = optarg;
			if (boot_name == NULL)
				exit_msg("ERROR:option -o No argument specified");
			break;

		case 'm':		/*
					 * Path to master file 
					 * If not specified then assume
					 * /etc/master.d
					 *
					 */
			master_name = optarg;
			if (master_name == NULL)
				exit_msg("ERROR:option -m No argument specified");
			break;

		case 's':		/*
					 * Path to system file 
					 * If not specified the assume
					 * /stand/system
					 *
					 */
			system_name = optarg;
			if (system_name == NULL)
				exit_msg("ERROR:option -s No argument specified");
			break;

		case 'c':		/* number of controllers */
			ncntrl = optarg;
			cntlflag++;
			break;
		
		case 'v':		/* version number */
			version = optarg;
			break;
		
		case 'u':		/* Remove named module */
			removeflg = 1;
			break;
		
		case 'f':		/* Force remove module */
			frcrmv = 1;
			break;

		case '?':
		default:		/* illegal option letter */
			fprintf(stderr,"Usage: %s [-d name] [-m name] [-o name] [-s name] [-v ver] [-c number] [-nbux]\n",cmdname);
			exit(1);
		}

/*
 * Check the version number 
 *
 */
	if(0 != strcmp(VERSION,version))
		exit_msg("ERROR:Version number required");

/*
 * Save current working direcotory, restore it before calling mkboot
*/
 	if( getcwd( cwd, 99 ) == NULL)
		exit_msg("ERROR:Cannot get current working directory");
		

/*
 * If neither module nor master names given
 * then cannot determine name of module
 * to be loaded.
 *
 */

	if((mod_name == NULL) && (master_name == NULL))
		exit_msg("ERROR:Unable to locate module name");


	/*
	 *	this must be filenames, not directories.
	 */

	ckdir(mod_name);
	ckdir(master_name);

		/*
		 * If module name found, but not master name
		 * then assume master file in master.d directory.
		 *
		 */

	if(mod_name && (master_name == NULL))
		{
		master_name = masname;
		strcpy(master_name, master_def);
		strcat(master_name, "/");
		finish_name(master_name, mod_name, 0);
	}
		/*
		 * If master name found, but not module name
		 * then assume name in boot directory.
		 * Remember, name in boot directory is in
		 * capital letters.
		 *
		 */

	else if(master_name && (mod_name == NULL))
		{
		mod_name = modname;
		strcpy(mod_name, "/boot/");
		finish_name(mod_name, master_name, 1);
	}
	else if((master_name != NULL) && (mod_name != NULL))
		finish_name((char *)NULL, master_name, 0);


	if(debug)
		{
		printf("Using master file from %s\n",master_name);
		printf("Using module from %s\n",mod_name);
		printf("lower_name = %s\n",lower_name);
		printf("Capital_name = %s\n",capital_name);
	}

		/*
		 * If output path for mkboot not specified
		 * then set up default to /boot.
		 *
		 */

	if(boot_name == NULL)
		{
		boot_name = bootname;
		strcpy(boot_name, "/boot");
	}

		/*
		 * If path to system file not specified
		 * then set up default to /stand/system.
		 *
		 */

	if(system_name == NULL)
		{
		system_name = systemname;
		strcpy(system_name, "/stand/system");
	}

	if(debug)
		{
		if(sysed == 0)
			printf("Using %s as system file\n", system_name);
		if(mkboot == 0)
			printf("Using %s as mkboot output\n",boot_name);
	}

	/*
	 *	open master file(s)
	 */

	if((masterfile = fopen(master_name, "r")) == NULL)
		exit_msg("ERROR:Cannot open %s",master_name);

	if(debug)
		printf("Master file %s opened\n",master_name);

	/*
	 *	Get buffer for master file
	 */

	if((masbuf = (char *)malloc(MYBSIZE)) == NULL)
		exit_msg("ERROR:Cannot alloct buffer for master file");


	/*
	 * Skip comments if any
	 */
	
	if ( skip_comments(masterfile) == 0 )
		exit_msg("ERROR:bad master file %s", master_name);

	/*
	 *	Locate flag and major number fields
	 *
	 */

	vecs[0] = prefx[0] = major_number[0] = '\0';
	tmpbuf[0] = '\0';

	fseek(masterfile, -1, 1);
	fscanf(masterfile, "%s", flags);
	fscanf(masterfile, "%s", vecs);
	fscanf(masterfile, "%s", prefx);
	fscanf(masterfile, "%s", major_number);
	fgets(tmpbuf, sizeof(tmpbuf), masterfile);

	if(debug)
		{
		printf("vecs = %s prefx = %s soft = %s\n",vecs,prefx,major_number);
		printf("tmpbuf = %s\n",tmpbuf);

	}

	/*
	 * Check if this is an uninstall
	 *
	 */
	
	if(removeflg != 0)
		{
		if(frcrmv == 0)			/* no force,
						 * Check for dependcies 
						 */
			check_dep();		 

		strcat(boot_name, "/");		
		strcat(boot_name, capital_name);
						/* Remove mkbooted file */
		if(unlink(boot_name) == -1)
			{
			if(debug)
				error("WARNING:unable to unlink %s",boot_name);
		}


	mkboot = 1;				/* Do not run mkboot */
	
	}


	/*
	 *	Now look for software flag in flags field
	 *	if not found, then must be either a
	 *	hardware driver or a loadable module
	 */
	
	if((char *)strchr(flags, 's') != NULL)
		{
		if(debug)
			printf("Working on software driver\n");

	/*
	 *	Check if major number already assigned
	 *	if so, get it for return value. If not,
	 *	then generate one for this module.
	 *
	 *	But first check if this is an
	 *	uninstall of software driver.
	 */

		if(removeflg == 0)
			{
			if(major_number[0] != '-')
				rtnval = atoi(major_number);
			else
				{
				get_majors();

				for(i = 0, rtnval = -1; i < MAXMAJ; ++i)
					{
					if(used_maj[i] != 0)
						continue;
				
					rtnval = i;
					sprintf(major_number, "%d", rtnval);
					break;
				}
				if(rtnval == -1)
					{
					rtnval = 0;
					exit_msg("ERROR:No major number available");
				}
			}
		}
		else				/* 
						 * Removing driver, get major 
						 * number and reset to -
						 */
			{
			rtnval = atoi(major_number);
			strcpy(major_number, "-");
		}
		
		/*
		 * Now write master file to temp file
		 * with modified major number field.
		 *
		 */

		fnamedir(master_name, tmpdir);
		sprintf(tempfile, "%s/<t%d>", tmpdir, getpid());
		tmpmast = (FILE *)fopen(tempfile, "w");

		rewind(masterfile);		/*
						 * Go back to begining of
						 * input master file
						 *
						 */
		for(;;)			/* Write comments, etc */
			{
			fgets(masbuf, MYBSIZE, masterfile);
			if(*masbuf == '*') {
				if(fputs(masbuf, tmpmast) == EOF)
					exit_msg("ERROR:bad write to tempfile");
			} else
				break;
		}			
					/* Write major number */
		if(fprintf(tmpmast, "%s\t%s\t%s\t%s%s",
			   flags, vecs, prefx, major_number, tmpbuf) < 0)
		{
			exit_msg("ERROR:bad write to tempfile");
		}

		/* Write rest of file */
		while(fgets(masbuf, MYBSIZE, masterfile) != NULL)
			if(fputs(masbuf, tmpmast) == EOF)
				exit_msg("ERROR:bad write to tempfile");

		fflush(tmpmast);
		fclose(tmpmast);
	/* 
	 *  rename temp file as new master_name file
	 */
		fclose(masterfile);
	/*
	 * copy rename will rename the file if it is not a symbolic link
	 * otherwise a copy is done. Note that if a copy is done there is
	 * a window where things could get trashed. copyrename minimizes
	 * this window.
	 */
		if (copyrename(tempfile, master_name) == -1) {
			unlink(tempfile);
			exit_msg("failed to create new %s file: %s",master_name, sys_errlist[errno]);
		}
	
	}

	else				/*
			 		 * if not loadable object then must be
			 		 * hardware driver. Therefore, do not
			 		 * edit system file. But, update
					 * time of system file to ensure 
					 * driver gets loaded in next time.
			 		 */

		if((char *)strpbrk(flags, "xm") == NULL)
		{
			system("TZ=GMT0 touch 0101000070 /unix");
			sysed = 1;
		}


/*	Now do edits of system file if required */

	if(sysed == 0)
		{
		do_system();
		if(debug)
			printf("System file %s updated \n", system_name);
	}

/*
 *	Now have everything necessary to do mkboot
 *	on this module. But first check if mkboot
 *	was requested, ie. was drvinstall called
 *	without the -b option or with -u option?
 *
 */

	if(mkboot == 0)
		{

		if(chdir(cwd) == -1)
			exit_msg("ERROR:Unable to change to %s",cwd);

		fnamedir(master_name, tmpdir);		/* find dir that contains
							   master file to build bootable object file 
							 */
		sprintf(masbuf,"/usr/sbin/mkboot -m %s -d %s %s", tmpdir, boot_name,mod_name);

		if (debug)
			printf("Calling mkboot using -> %s \n", masbuf);

			

		if(system(masbuf) != 0){
			error("WARNING:Exec of mkboot failed");
			rtnv=1;
		}
	}

	endjob();	/* Close files and exit with rtnv */

}

/*
 *	This routine finds module name from from_name
 *	and appends it to to_name. It also sets up
 *	lower_name and capital_name arrays.
 *	If to_name is NULL, then only the lower_name
 *	and capital_name arrays are built using
 *	the from_name as the source.
 */


finish_name(to_name,from_name,conv)
char *to_name;
char *from_name;
int conv;
	{
	char *slash, *dotplace, *from, *tmp;
	int len, i;

	if((slash = (char *)strrchr(from_name, '/')))
		{
		dotplace = (char *)strchr(slash, '.');
		len = (int)(dotplace?((dotplace - slash) - 1):strlen(slash));
		from = ++slash;
	}
	else
		{
		dotplace = (char *)strchr(from_name, '.');
		from = from_name;
		len = (int)(dotplace?(dotplace - from_name):strlen(from_name));
	}

	if(isupper(*from))
		{
		for(i = 0, tmp = from; i < len; ++i, tmp++)
			{
			capital_name[i] = *tmp;
			lower_name[i] = tolower(*tmp);
		}
	lower_name[i] = '\0';
	}
	else
		{
		for(i = 0, tmp = from; i < len; ++i, tmp++)
			{
			lower_name[i] = *tmp;
			capital_name[i] = toupper(*tmp);
		}
	capital_name[i] = '\0';
	}

	if(to_name != NULL)
		{
		if(conv)
			from = capital_name;
		else
			from = lower_name;
	
		strncat(to_name,from,len);
	}
}


/*  
 * FNAMEDIR(fname, fdir)
 *	it searchs the specified filename and returns its parent directory
 *
 */
fnamedir(fname, fdir)
char 	*fname, *fdir;
{
	char	*slash;

  	if (slash = strrchr(fname, '/')) {
		strncpy(fdir, fname, slash-fname);
		fdir[slash-fname]='\0';		/* null terminated */
	} else 
		fdir="/";
	
	return;
}

 /*VARARGS1*/

void
error(string,arg1,arg2,arg3)
char	*string;
int	arg1, arg2, arg3;
	{

	char buffer[256];

	sprintf( buffer, string, arg1, arg2, arg3 );

	fprintf(stderr, "%s: %s\n", cmdname, buffer);
}


/*
 *	exit_msg(string,arg1,arg2,arg3) - print the exit string and exit(1)
 */

 /*VARARGS1*/
void
exit_msg(string,arg1,arg2,arg3)
char	*string;
	{

	error(string,arg1,arg2,arg3);
	rtnv = 1;
	endjob();
}


/*
 *	endjob() - closes the source and target files
 *		before exitting with rtnv as exit code
 */

endjob()
	{
	if(masterfile != NULL)
		fclose(masterfile);

	if(masbuf != (char *)NULL)
		free(masbuf);

	if(rtnval > 0)
		printf("%d\n",rtnval);

	exit(rtnv);
}

/*
 *	Skip comment and blank lines if any
 */

skip_comments(fileptr)
FILE *fileptr;
{
	int c;

	for(;;) {
		c = getc(fileptr);
		if(c  == '*' || c == '#' || c == ' ' || c == '\t' || c == '\n'){
			if(c == '\n')
				continue;
			while((c = getc(fileptr)) != '\n') {
				if(c == EOF)  {
					return(0);
				}
			}
		} else break;
	}
	if ( c < 040 || c > 0176 )  {
		return(0);
	}
	return(1);
}

/*
 * GET_MAJORS()
 *	This function will search all files in the /etc/master.d
 *	directory for software drivers. If a software driver is
 *	found, the major number is extracted and placed into
 *	the used_maj number array.
 *
 */
get_majors()
{
	register struct dirent *dp;
	DIR 	*dirp;
	FILE *masptr;
	int index;
	short num;
	char myflags[12];
	char majnum[4];

	myflags[0] = majnum[0] = '\0';

	if(chdir(master_def) == -1)
		exit_msg("ERROR:Unable to change to %s",master_def);

	if ((dirp = opendir(master_def)) == NULL)
		exit_msg("WARNING: cannot open directory %s", master_def);

	while (dp = readdir(dirp)) {
		
		if ( EQ(dp->d_name,".") || EQ(dp->d_name,"..") )
			continue;

		if((masptr = fopen(dp->d_name, "r")) == NULL)  {
			error("WARNING:Unable to open %s",dp->d_name);
		} else if ( skip_comments(masptr) )  {
			fseek(masptr, -1, 1);
			fgets(masbuf, MYBSIZE, masptr);

			num = sscanf(masbuf, "%s%*s%*s %[0-9]", myflags,majnum);

			if((char *)strchr(myflags, 's') != NULL)  {
				if(num == 2)  {
					index = atoi(majnum);
					used_maj[index] = 1;
				}
			}
		}
		fclose(masptr);
		
	}  /* end while (dp = readdir(dirp)).. */

	closedir(dirp);		

}

/*
 * CHECK_DEP()
 *	This function will check all files in the /etc/master.d
 *	directory for dependencies on driver being removed. 
 *
 */
check_dep()
{
	DIR	*dirp;
	FILE *masptr;
	register struct dirent *dp;
	char *dep_name;

	if(chdir(master_def) == -1)
		exit_msg("ERROR:Unable to change to %s",master_def);

	if ( (dirp = opendir(master_def)) == NULL)
		exit_msg("WARNING: cannot open directory %s", master_def);

	while (dp = readdir(dirp)) {

		if ( EQ(dp->d_name,".") || EQ(dp->d_name,"..") )
			continue;

		if((masptr = fopen(dp->d_name, "r")) == NULL)  {
			error("WARNING:Unable to open %s", dp->d_name);
		} else if(rd_file(masptr)) {
			dep_name = (char *)strtok(masbuf, ", \t\n");
			do	{
				if(strcmp(capital_name, dep_name) == 0 || 
					strcmp(lower_name, dep_name) == 0)
					exit_msg("ERROR: driver %s depends on driver %s ", dp->d_name, lower_name);
			}while((dep_name = (char *)strtok((char *)NULL, ", \t\n")) != NULL);
		}
		fclose(masptr);
		
	}  /* end while (dp = readdir(dirp)) */

	closedir(dirp);
}
/*
 * Read first non comment line of file
 * and ignore first 6 fields of line,
 * then read dependencies into buffer.
 * if no dependencies then return 0,
 * else return 1.
 */

rd_file(masptr)
FILE *masptr;
{
	char myflags[12];
	int i, scrtn;

	if (skip_comments(masptr) == 0) {
		return(0);
	}
	fseek(masptr, -1, 1);
	for(i = 0; i < 6; ++i)  {
		scrtn = fscanf(masptr, "%s", myflags);
		if(scrtn == 0 || scrtn == EOF)
			return(0);
	}
	if(fgets(masbuf, MYBSIZE, masptr) != NULL)
		return(1);
	else
		return(0);
	
}

/*
 * This routine removes any inclusion of the driver
 * specified from the system file. 
 * If driver is being installed, adds an include
 * statement.
 *
 */

do_system()
	{

	char tmpbuf[256];
	int first;


	if(chdir(sys_def) == -1)
		exit_msg("ERROR:unable to change to %s", sys_def);

	if((systemfile = (FILE *)fopen(system_name, "r")) == NULL)
		{
		rtnval = 0;
		exit_msg("ERROR:unable to open %s",system_name);
	}

	
	fnamedir(system_name, tmpdir);
	sprintf(tempfile,"%s/<t%d>", tmpdir, getpid());
	tmpmast = (FILE *)fopen(tempfile, "w");

	while(fgets(masbuf, (int)MYBSIZE, systemfile) != NULL) {
		if(masbuf[0] == 'I') {
			if(strncmp("INCLUDE", (char *)masbuf, 7) == 0) {
				register int argc;
				register char **argv;
				char *args[50];
				argv = &args[0];
				if ((argc=parse(args,50,masbuf)) > 50)
					myerror(argc,argv);
				if ( (argc==1) || *argv[1] != ':')
					myerror(argc,argv);
				strcpy(tmpbuf, "\0");
				first = 1;
				argc -=2;
				argv +=2;

				while( argc > 0 )  {
					if( 0 == (strcmp(capital_name, argv[0])) ) {
						if ( argc >= 4 && *argv[1] == '(' )  {
							if ( *argv[3] != ')' )  
								myerror(argc,argv);
							argc -=4;
							argv +=4;
						}else {
							argc -=1;
							argv +=1;
						}
						continue;
					}
					if(first == 0)
						strcat(tmpbuf, " ");

					first = 0;
					if ( argc >= 4 && *argv[1] == '(' )  {
						if ( *argv[3] != ')' )  
							myerror(argc,argv);
						strcat(tmpbuf, argv[0]);
						strcat(tmpbuf, argv[1]);
						strcat(tmpbuf, argv[2]);
						strcat(tmpbuf, argv[3]);
						argc -=4;
						argv +=4;
					}else {
						strcat(tmpbuf, argv[0]);
						argc -=1;
						argv +=1;
					}
				}
				if(first == 0) {
					strcpy(masbuf, "\0");
					strcat(masbuf, "INCLUDE:");
					strcat(masbuf, tmpbuf);
					strcat(masbuf, "\n");
					strcat(masbuf, "\0");
				} else
					continue;
			}
		}
		if (fwrite(masbuf, (int)strlen(masbuf), 1, tmpmast) != 1)
			exit_msg("ERROR:bad write to tempfile");
	}

/*
 *	If this is not an uninstall, then add 
 *	an INCLUDE line for this driver.
 *
 */
	if(removeflg == 0)
		{
		strcpy(masbuf, "INCLUDE:");
		strcat(masbuf, capital_name);
		if ( cntlflag )  {
			strcat(masbuf, "(");
			strcat(masbuf, ncntrl);
			strcat(masbuf, ")");
		}
		strcat(masbuf, "\n");
		if (fwrite(masbuf, (int)strlen(masbuf), 1, tmpmast) != 1)
			exit_msg("ERROR:bad write to tempfile");
	}


	fclose(tmpmast);
	fclose(systemfile);

	if (copyrename(tempfile, system_name) == -1) {
		unlink(tempfile);
		exit_msg("failed to create new %s file: %s", system_name, sys_errlist[errno]);
	}
}

myerror(argc,argv)
int	argc;
char	**argv;
{
	exit_msg("ERROR:syntax INCLUDE:%s...", argv[0]);

}

 /*
 * Parse(argv, sizeof(argv), line)
 *
 * Parse a line from the /stand/system file; argc is returned, and argv is
 * initialized with pointers to the elements of the line.  The contents
 * of the line are destroyed.
 */
 static
 int
parse(argv, sizeof_argv, line)
char **argv;
unsigned sizeof_argv;
register char *line;
{

	register char **argvp = argv;
	register char **maxvp = argv + sizeof_argv;
	register c;

	while (c = *line)
		{
		switch (c)
			{
		/*
		 * white space
		 */
		case ' ':
		case '\t':
		case '\r':
			*line++ = '\0';
			line += strspn(line," \t\r");
			continue;
		/*
		 * special characters
		 */
		case ':':
			*line = '\0';
			*argvp++ = ":";
			++line;
			break;
		case ',':
			*line = '\0';
			*argvp++ = ",";
			++line;
			break;
		case '(':
			*line = '\0';
			*argvp++ = "(";
			++line;
			break;
		case ')':
			*line = '\0';
			*argvp++ = ")";
			++line;
			break;
		case '?':
			*line = '\0';
			*argvp++ = "?";
			++line;
			break;
		case '=':
			*line = '\0';
			*argvp++ = "=";
			++line;
			break;
		/*
		 * end of line
		 */
		case '\n':
			*line = '\0';
			*argvp = NULL;
			return(argvp - argv);
		/*
		 * words and numbers
		 */
		default:
			*argvp++ = line;
			line += strcspn(line,":,()?= \t\r\n");
			break;
			}

		/*
		 * don't overflow the argv array
		 */
		if (argvp >= maxvp)
			return(sizeof_argv + 1);
		}

	*argvp = NULL;
	return(argvp - argv);
}

ckdir(name)
char	*name;
{
	struct stat	statb;
	if ( name == NULL || stat(name, &statb) == -1 )
		return;
	if ( ! (((statb.st_mode) & S_IFMT) == S_IFREG) )
		exit_msg("ERROR:%s is not a file", name);

}

/*
 * copyrename - rename without destroying symbolic link.
 */

copyrename(from,to)
char *from,*to;
{
	struct stat sb;
	register int fdfrom,fdto;
	char *buf;


	if (lstat(to,&sb) == -1)
		return(-1);
	if ((sb.st_mode & S_IFLNK) == S_IFLNK){
		if (stat(from,&sb) == -1)
			return(-1);

		if ((buf=(char *)malloc(sb.st_size)) == NULL)
			return(-1);

		if ((fdfrom= open(from,O_RDONLY,0)) == -1)
			return(-1);

		if (read(fdfrom,buf,sb.st_size) != sb.st_size)
			return(-1);

		if ((fdto= open(to,O_WRONLY | O_TRUNC,0)) == -1)
			return(-1);

		if (write(fdto,buf,sb.st_size) != sb.st_size)
			return(-1);
		
		close(fdto);
		close(fdfrom);
		unlink(from);
	} else {
		if (rename(from, to) == -1) 
			return(-1);
	}
	return(0);
}
