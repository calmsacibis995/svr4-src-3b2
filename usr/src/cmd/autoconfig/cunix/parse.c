/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/parse.c	1.9"

#include <sys/types.h>
#include <sys/localtypes.h>
#include <stdio.h>
#include <a.out.h>
#include <sys/sys3b.h>
#include <sys/dir.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/dproc.h>
#include <sys/machdep.h>
#include <ctype.h>
#include <sys/error.h>
#include <sys/cunix.h>
#include <vm/bootconf.h>

#ifdef u3b15
#include <sys/cc.h>
#endif

#ifdef u3b2
#include <sys/sbd.h>
#endif

#ifdef unix	/* get rid of special pre-processor name */
#undef unix
#endif


/*
 * Static function declarations for this file
 */
static char    *do_mirror();
static char    *do_boot();
static char    *do_device();
static char    *do_exclude();
static char    *do_include();
static char    *do_swapdevice();
static char    *interpret();
static boolean  numeric();
static int      parse();
static void     prompt();


/*
 * Blankline(line)
 *
 * Return TRUE if the line is entirely blank or null; return FALSE otherwise.
 */
 boolean
blankline(line)
	register char *line;
	{

	return(strspn(line," \t\r\n") == strlen(line));
	}



 void
fsystem(stream)
	register FILE *stream;
	{

	char line[256];
	register char *msg;
	register lineno = 1;

	while ((int) fgets(line,sizeof(line),stream) != NULL)
		{

		if (strlen(line) == sizeof(line)-1)
			{
			/* System: line too long */
			error((stream->_flag&_IOMYBUF)? ER68 : ER69, lineno);
			continue;
			}

		if (ferror(stream))
			{
			error(ER7);
			break;
			}

		if (line[0] != '*' && ! blankline(line))
			if ((msg=interpret(line)) != NULL)
				{
				if (stream->_flag & _IOMYBUF)
					/* System: line <lineno>; <msg> */
					error(ER53, lineno, msg);
				else
					/* System: <msg> */
					error(ER54, msg);
				}

		++lineno;
		}
	}

/*
 * Interpret(line)
 *
 * Interpret the line from the /etc/system file
 */


struct	syntax
	{
	char	*keyword;
	char	*(*process)();
	char	*argument;
	};

static struct syntax syntax[] ={
			{ "BOOT", do_boot, 0 },
			{ "EXCLUDE", do_exclude, 0 },
			{ "INCLUDE", do_include, 0 },
#ifdef u3b15
			{ "DUMPDEV", do_device, (char*) &dumpdev },
#endif
      			{ "MIRRORDEV", do_mirror, 0 },
			{ "ROOTDEV", do_device, (char*) &rootdev },
			{ "SWAPDEV", do_swapdevice, 0 },
				0 };

 static
 char *
interpret(line)
	char *line;
	{

	register int argc;
	char *argv[50];
	register struct syntax *p;

	if ((argc=parse(argv,sizeof(argv)/sizeof(argv[0]),line)) > sizeof(argv)/sizeof(argv[0]))
		return("line too long");

	if (argc == 0)
		return(NULL);

	if (argc == 1 || *argv[1] != ':')
		return("syntax error");

	for (p=syntax; p->keyword; ++p)
		{
		if (0 == strcmp(*argv,p->keyword))
			{
			if (argc == 2)
				return(NULL);
			else
				return((*(p->process))(argc-2, &argv[2], p->argument));
			}
		}

	return("syntax error");
	}

/*
 * Parse(argv, sizeof(argv), line)
 *
 * Parse a line from the /etc/system file; argc is returned, and argv is
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

/*
 * prompt(message)
 */
 static
 void
prompt(message)
	char *message;
	{

	panic("unable to parse the system file, parsing %s\n",message);
	exit(1);
	}

/*
 * Numeric(assign, string)
 *
 * If the string is a valid numeric string, then set *assign to its numeric
 * value and return TRUE; otherwise return FALSE.
 */
 static
 boolean
numeric(assign, string)
	register int *assign;
	char *string;
	{

	register long value;
	char *next;

	value = strtol(string, &next, 0);

	if (*next)
		/*
		 * bad number
		 */
		return(FALSE);

	*assign = value;

	return(TRUE);
	}

/*
 * Do_xxxx(argc, argv, optional)
 *
 * Handle the processing for each type of line in /etc/system
 */


/*
 * BOOT: program
 */
 static
 char *
do_boot(argc, argv)
	int argc;
	register char **argv;
	{

	char *def_name;
	struct stat statbuf;
	register type;

	if (argc > 1)
		return("syntax error");

	if (*argv[0] == '?')
		{
		prompt("BOOT:");
		return(NULL);
		}

	if (strcmp(*argv,"DEFAULT") == 0){
		def_name = (char *) malloc(strlen(slash_boot) + strlen("/KERNEL") + 2);
		if (def_name == NULL){
			panic("can't malloc space for default boot name\n");
			exit(1);
		}
		strcpy(def_name,slash_boot);
		strcat(def_name,"/");
		strcat(def_name,"KERNEL");
		*argv = def_name;
	}
		
	if (stat(*argv,&statbuf) == -1)
		return("no such file");

	if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
		return("cannot boot directory");

	if ((type=statbuf.st_mode&S_IFMT) == S_IFCHR || type == S_IFBLK)
		return("cannot boot special device");

	if (type == S_IFIFO)
		return("cannot boot special file");

	if (strlen(*argv) >= sizeof(bootprogram.path))
		return("path too long");

	strcpy(bootprogram.path, *argv);

	return(NULL);
	}


/*
 * EXCLUDE: driver ...
 */
 static
 char *
do_exclude(argc, argv)
	register int argc;
	register char **argv;
	{

	while (argc-- > 0)
		{
		if (*argv[0] != ',')
			/* not comma */
			if (*argv[0] == '?')
				prompt("EXCLUDE:");
			else
				exclude(*argv);

		++argv;
		}

	return(NULL);
	}


/*
 * INCLUDE: driver(optional-number) ...
 */
 static
 char *
do_include(argc, argv)
	register int argc;
	register char **argv;
	{

	register char *p;
	int n;

	while (argc > 0)
		{

		if (*argv[0] == ',')
			{
			--argc;
			++argv;
			continue;
			}

		if (*argv[0] == '?')
			{
			prompt("INCLUDE:");
			--argc;
			++argv;
			continue;
			}

		p = *argv;
		n = 1;

		if (argc >= 4 && *argv[1] == '(')
			{
			if (*argv[3] != ')')
				return("syntax error");

			if (! numeric(&n, argv[2]))
				return("count must be numeric");

			argc -= 3;
			argv += 3;
			}
		else
			if (*p == '(' || *p == ')')
				return("syntax error");

		include(p, n);

		--argc;
		++argv;
		}

	return(NULL);
	}


/*
 * DUMPDEV: { path | DEV(number,number) }
 * ROOTDEV: { path | DEV(number,number) }
 * PIPEDEV: { path | DEV(number,number) }
 */
 static
 char *
do_device(argc, argv, device)
	register int argc;
	register char **argv;
	register dev_t *device;
	{

	struct stat statbuf;
	dev_t ndev;
	register type;
	int M, m;

	if (argc == 1)
		/*
		 * path
		 */
		{
		if (*argv[0] == '?')
			{
			char what[10];

			/*
			 * we cheat here; we know that interpret() calls this
			 * routine with argv+2; therefore, argv[-2] is the original
			 * statement type
			 */
			strcat(strcpy(what,argv[-2]), ":");

			prompt(what);
			return(NULL);
			}

		if (stat(*argv,&statbuf) == -1)
			return("no such file");
		else
			if ((type=statbuf.st_mode&S_IFMT) != S_IFCHR && type != S_IFBLK)
				return("file not BLOCK or CHAR special");
		ndev =	statbuf.st_rdev;
		*device = ndev;
		return(NULL);
		}

	if (argc != 6 || 0 != strcmp("DEV",argv[0]) || *argv[1] != '(' || *argv[3] != ',' || *argv[5] != ')')
		return("syntax error");
	
	/*
	 * DEV(number,number)
	 */

	if (! numeric(&M,argv[2]) || ! numeric(&m,argv[4]))
		return("major/minor must be numeric");

	*device = makedevice(M, m);

	return(NULL);
	}


/*
 * If this is a path name for a device,
 * then fill in *device.  Always fill
 * in the pathname.
 */
 char *
do_swapfile(argc, argv, device)
	register int argc;
	register char **argv;
	register dev_t *device;
{

	struct stat statbuf;
	dev_t ndev;
	register type;

	if (stat(*argv,&statbuf) == -1)
		return("no such file");

        strcpy(swapfile.bo_name, *argv);

        swapdone = 1;

	if ((type=statbuf.st_mode&S_IFMT) != S_IFCHR && type != S_IFBLK)
		return(NULL);

	ndev =	statbuf.st_rdev;
	*device = ndev;


	return(NULL);
}


/*
 * SWAPDEV: path swplo	nswap
 * formerly:
 * SWAPDEV: { path | DEV(number,number) }  swplo  nswap
 */
static
char *
do_swapdevice(argc, argv)
	register int argc;
	register char **argv;
{

	register char *p;

	if (argc == 3) {
		/*
		 * path swplo nswap
		 *
		 * swapdev = path
		 * swplo = number
		 * nswap = number
		 */
		if (*argv[1] == '=') {
			/*
                         * internal prompt response
                         */
			if (0 == strcmp("swapdev",argv[0]))
                                return(do_swapfile(1,&argv[2],&swapdev));

			if (0 == strcmp("swplo",argv[0]))
                                if (! numeric(&swapfile.bo_offset,argv[2]))
                                        return("must be numeric");

			if (0 == strcmp("nswap",argv[0]))
                                if (! numeric(&swapfile.bo_size,argv[2]))
                                        return("must be numeric");

			return(NULL);
		}

		if (*argv[0] == '?')
			prompt("SWAPDEV: swapdev=");
		else
			if ((p=do_swapfile(1,argv,&swapdev)) != NULL)
                                return(p);

		--argc;
		++argv;
	}
	/*
	 * If we ever need to specify swap by major/minor,
	 * then we need to add a bo_dev field to the
	 * bootobj structure and have swapconf make
	 * a vp from it.
	 *
	 else
		if (argc == 8) {
			/*
                         * DEV(number,number) swplo nswap
                         *
                         * swapdev = DEV(number,number)
                         *
			if (*argv[1] == '=') {
                                if (0 == strcmp("swapdev",argv[0]))
                                        return(do_device(6,&argv[2],&swapdev));
			}

			if ((p=do_device(6,argv,&swapdev)) != NULL)
                                return(p);

			argc -= 6;
			argv += 6;
		}
	*/
	else
		return("syntax error");

	if (*argv[0] == '?')
		prompt("SWAPDEV: swplo=");
	else
		if (! numeric(&swapfile.bo_offset,argv[0]))
			return("must be numeric");

	if (*argv[1] == '?')
		prompt("SWAPDEV: nswap=");
	else
		if (! numeric(&swapfile.bo_size,argv[1]))
			return("must be numeric");

	return(NULL);
}

/*
 * MIRRORDEV: path [ path ]
 */

static char *
do_mirror( argc, argv )
register int argc;
register char **argv;
{
 	char *rtn;



        if (argc >= 3)
		return("syntax error");

	if ((rtn = do_device(1, &argv[0], &mirrordev[0])) != NULL)
		return(rtn);

	if (argc > 1) {
		if ((rtn = do_device(1, &argv[1], &mirrordev[1])) != NULL)
			return(rtn);
	}
 	
	return(NULL);
}

