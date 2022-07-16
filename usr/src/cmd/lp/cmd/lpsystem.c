/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpsystem.c	1.3"

#include	<sys/utsname.h>
#include	<stdio.h>
#include	<tiuser.h>
#include	<netconfig.h>
#include	<netdir.h>

#include	"lp.h"
#include	"systems.h"

#define	DEFAULT_TIMEOUT	-1
#define	DEFAULT_RETRY	10

#if	defined(__STDC__)
void usage ( void )
#else
void usage ( )
#endif
{
    printf("Usage:  lpsystem -s system [ options ]\n");
    printf("\nTo add or change and existing system:\n");
    printf("        lpsystem [ -t type ] [ -T timeout ]\n");
    printf("                 [ -R retry ] [ -y comment ] system-name\n");
    printf("\nTo list a system (or all of the systems):\n");
    printf("        lpsystem -l [ system-name ]\n");
    printf("\nTo remove a system:\n");
    printf("        lpsystem -r system-name\n");
    printf("\nTo get the TCP/IP address for the local port-monitor:\n");
    printf("        lpsystem -A\n");
}

#if	defined(__STDC__)
void formatsys ( const SYSTEM * sys )
#else
void formatsys ( sys )
SYSTEM	*sys;
#endif
{
    printf("System:                     %s\n", sys->name);
    printf("Type:                       %s\n",
	   (sys->protocol == S5_PROTO ? NAME_S5PROTO : NAME_BSDPROTO));
    if (sys->timeout == -1)
	printf("Connection timeout:         never\n");
    else
	printf("Connection timeout:         %d minutes\n", sys->timeout);
    if (sys->retry == -1)
	printf("Retry failed connections:   no\n");
    else
	printf("Retry failed connections:   after %d minutes\n", sys->retry);
    printf("Comment:                    %s\n",
	sys->comment == NULL ? "none" : sys->comment);
    printf("\n");
}

#if	defined(__STDC__)
main ( int argc, char * argv [] )
#else
main ( argc, argv )
int	argc;
char	*argv[];
#endif
{
    extern int	opterr;
    extern int	optind;
    extern char	*optarg;
    int		c;
    int		lflag = 0;
    int		rflag = 0;
    int		Aflag = 0;
    int		error = 0;
    int		exitCode = 0;
    SYSTEM	*ssp;
    SYSTEM	sysbuf = {
		NULL, NULL, NULL, S5_PROTO, NULL,
		DEFAULT_TIMEOUT, DEFAULT_RETRY, NULL, NULL, NULL
		}; 
    char	*system = NULL;
    char	*protocol = NULL;
    char	*timeout = NULL;
    char	*retry = NULL;
    char	*comment = NULL;


 
    if (geteuid () != 0) {
	(void)	fprintf (stderr,
		"You must be root to execute this command.\n");
	exit (1);
    }
    while ((c = getopt(argc, argv, "t:T:R:y:lrA?")) != EOF)
	switch (c & 0xFF)
	{
	  case 't':
	    if (protocol) {
		printf("Too many -t options.\n");
		exit(1);
	    }
	    protocol = optarg;
	    break;
	    
	  case 'T':
	    if (timeout) {
		printf("Too many -T options.\n");
		exit(1);
	    }
	    timeout = optarg;
	    break;
	    
	  case 'R':
	    if (retry) {
		printf("Too many -R options.\n");
		exit(1);
	    }
	    retry = optarg;
	    break;
	    
	  case 'y':
		if (comment) {
			printf("Too many -y options.\n");
			exit(1);
	    	}
		comment = optarg;
		break;

	  case 'l':
	    if (lflag) {
		printf("Too many -l options.\n");
		exit(1);
	    }
	    lflag++;
	    break;
	    
	  case 'r':
	    if (rflag) {
		printf("Too many -r options.\n");
		exit(1);
	    }
	    rflag++;
	    break;

	  case 'A':
	    if (Aflag) {
		printf("Too many -A options.\n");
		exit(1);
	    }
	    Aflag++;
	    break;

	  default:
	    printf("Unrecognized option \"-%c\".\n", c & 0xFF);
	    
	  case '?':
	    usage();
	    break;
	}

    if (lflag && (protocol || timeout || retry || comment || rflag || Aflag)) {
	(void)	fprintf (stderr, "Improper usage.\n\n");
	usage ();
	exit (1);
    }
    if (rflag && (protocol || timeout || retry || comment || Aflag)) {
	(void)	fprintf (stderr, "Improper usage.\n\n");
	usage ();
	exit (1);
    }
    if (Aflag && (protocol || timeout || retry || comment)) {
	(void)	fprintf (stderr, "Improper usage.\n\n");
	usage ();
	exit (1);
    }
	/*
	**	Lets do some processing.
	**	We'll start with the flags.
	*/
	if (lflag) {
		if (! argv [optind]) {
	    		while ((ssp = getsystem(NAME_ALL)) != NULL)
				formatsys(ssp);
	    		exit(0);
		}
		for (system = argv [optind]; system; system = argv[++optind]) {
    			if (STREQU(NAME_ANY, system)  ||
			    STREQU(NAME_NONE, system) ||
			    STREQU(NAME_ALL, system)) {
				(void)	fprintf(stderr,
					"\"%s\" is a reserved word and may "
					"not be used for a system name.\n",
					system);
				exitCode = 1;
				continue;
			}
			if ((ssp = getsystem (system)) == NULL) {
	    			(void)	fprintf (stderr,
					"\"%s\" is not a known system.\n",
					system);
				exitCode = 1;
	    			continue;
			}
			formatsys(ssp);
    		}
		exit (exitCode);
    	}
    	if (rflag) {
		if (! argv [optind]) {
			(void)	fprintf (stderr, "Improper usage.\n\n");
			usage ();
			exit (1);
		}
		for (system = argv [optind]; system; system = argv[++optind]) {
    			if (STREQU(NAME_ANY, system)  ||
			    STREQU(NAME_NONE, system) ||
			    STREQU(NAME_ALL, system)) {
				(void)	fprintf(stderr,
					"\"%s\" is a reserved word and may "
					"not be used for a system name.\n",
					system);
				exitCode = 1;
				continue;
			}
			if (delsystem (system)) {
	    			(void)	fprintf (stderr,
					"\"%s\" is not a known system.\n",
					system);
				exitCode = 1;
			}
			else
				printf("\"%s\" has been removed.\n", system);
    		}
		exit (exitCode);
    	}
	if (Aflag) {
		int	i;
		struct	utsname		utsbuf;
		struct	netconfig	*configp;
		struct	nd_hostserv	hostserv;
		struct	nd_addrlist	*addrsp;

		struct	netconfig	*getnetconfigent ();

		(void)	uname (&utsbuf);
		configp = getnetconfigent ("tcp");
		if (configp == NULL) {
			perror ("getnetconfigent");
			exit (1);
		}
		hostserv.h_host = utsbuf.nodename;
		hostserv.h_serv = "printer";
		if (netdir_getbyname (configp, &hostserv, &addrsp)) {
			perror ("netdir_getbyname");
			exit (1);
		}
		for (i=0; i < addrsp->n_addrs->len; i++)
			printf ("%02x", addrsp->n_addrs->buf [i]);
		printf("\n");
		exit (0);
	}
	for (system = argv [optind]; system; system = argv [++optind]) {
		if ((ssp  = getsystem(system)) != NULL)
			sysbuf = *ssp;

		sysbuf.name = system;
	
		if (protocol)
		{
			if (STREQU(NAME_S5PROTO, protocol))
	    			sysbuf.protocol = S5_PROTO;
			else
	    		if (STREQU(NAME_BSDPROTO, protocol))
				sysbuf.protocol = BSD_PROTO;
	    		else
			{
				(void)	fprintf (stderr,
					"The only supported protocols are "
					"\"%s\" and \"%s\".\n",
		       			NAME_S5PROTO, NAME_BSDPROTO);
				exit(1);
	    		}
		}
		if (timeout)
		{
			if (*timeout == 'n')
	    			sysbuf.timeout = -1;
			else
			if (sscanf (timeout, "%d", &sysbuf.timeout) != 1 ||
				sysbuf.timeout < 0)
			{
				(void)	fprintf (stderr,
					"Bad timeout argument: %s\n", timeout);
				exit (1);
			}
		}
		if (retry)
		{
			if (*retry == 'n')
	    			sysbuf.retry = -1;
			else 
			if (sscanf (retry, "%d", &sysbuf.retry) != 1 ||
				sysbuf.retry < 0)
			{
				(void)	fprintf (stderr,
					"Bad retry argument: %s\n", retry);
				exit (1);
			}
		}
		if (comment)
		{
			sysbuf.comment = comment;
		}
		if (putsystem(system, &sysbuf) != 0) {
		(void)	fprintf (stderr,
			"Failed to add \"%s\" to the Systems table.\n",
			system);
			exit(1);
		}
		printf("\"%s\" has been added to the Systems table.\n",
			system);
	}
	exit(0);
}
