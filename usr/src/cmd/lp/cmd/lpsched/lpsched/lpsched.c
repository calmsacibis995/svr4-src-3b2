/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpsched/lpsched.c	1.15.1.2"

#include "limits.h"
#include "sys/utsname.h"

#include "lpsched.h"

#include "sys/stat.h"

#if	defined(MALLOC_3X)
#include "malloc.h"
#endif

FILE			*LockFp		= 0;

int			Starting	= 0;
int			Shutdown	= 0;
int			DoneChildren	= 0;
int			Sig_Alrm	= 0;
int			OpenMax		= OPEN_MAX;
int			Reserve_Fds	= 0;

char			*Local_System	= 0;
char			*SHELL		= 0;

gid_t			Lp_Gid;
uid_t			Lp_Uid;

#if	defined(DEBUG)
int			debug = 0;
int			signals = 0;
#endif

extern int		errno;
extern void		shutdown_messages();

int			am_in_background	= 0;
int			leave_stdout_open	= 0;

static void		disable_signals();
static void		startup();
static void		process();
static void		ticktock();
static void		background();
static void		usage();
static void		Exit();
static void		disable_signals();

/**
 ** main()
 **/

#if	defined(__STDC__)
main (
	int			argc,
	char *			argv[]
)
#else
main (argc, argv)
    int		argc;
    char	*argv[];
#endif
{
    int		c;
    extern int	optind;
    extern char	*optarg;
    extern int	optopt;
    extern int	opterr;
    char	*getenv();

    TRACE_ON ("main");

    if (!(SHELL = getenv("SHELL")))
	SHELL = DEFAULT_SHELL;

#if	defined(MDL)
# ident "lpsched has MDL"
    /*
    **	Set the output of MDL to be MDL-LOG
    */
    mdl(SET_FILE_STREAM, 0, 0, 0, "MDL-LOG");
    /*
    **	Set the toggle Flag to cause the file to be opened
    **	and closed as needed rather than opened once and kept.
    **	(ie, it saves a file descriptor at the cost or performance).
    */
    mdl(TOGGLE_OPEN, 0, 0, 0, 0);
#endif

#if	defined(MALLOC_3X)

# if	defined(DEF_MXFAST)
    mallopt (M_MXFAST, DEF_MXFAST);
# endif
# if	defined(DEF_NLBLKS)
    mallopt (M_NLBLKS, DEF_NLBLKS);
# endif
# if	defined(DEF_GRAIN)
    mallopt (M_GRAIN, DEF_GRAIN);
# endif
    mallopt (M_KEEP, 0);

#endif

    opterr = 0;
    while((c = getopt(argc, (const char **)argv, "dsf:n:r:OM:")) != EOF)
        switch(c)
        {
# if defined (DEBUG)
	    case 'd':
		debug++;
		break;

	    case 's':
		signals++;
		break;
# endif /* DEBUG */

	    case 'f':
		if ((ET_SlowSize = atoi(optarg)) < 1)
		    ET_SlowSize = 1;
		break;

	    case 'n':
		if ((ET_NotifySize = atoi(optarg)) < 1)
		    ET_NotifySize = 1;
		break;

	    case 'r':
		if ((Reserve_Fds = atoi(optarg)) < 1)
		    Reserve_Fds = 0;
		break;

	    case 'O':
		leave_stdout_open = 1;
		break;

#if	defined(MALLOC_3X)
	    case 'M':
		{
			int			value;

			value = atoi(optarg);
			printf ("M_MXFAST set to %d\n", value);
			mallopt (M_MXFAST, value);

			value = atoi(argv[optind++]);
			printf ("M_NLBLKS set to %d\n", value);
			mallopt (M_NLBLKS, value);

			value = atoi(argv[optind++]);
			printf ("M_GRAIN set to %d\n", value);
			mallopt (M_GRAIN, value);
		}
		break;
#endif

	    case '?':
		if (optopt == '?') {
		    usage ();
		    exit (0);
		} else
		    fail ("%s: illegal option -- %c\n", argv[0], optopt);
	}
    
    TRACE_OFF ("main");

    TRACE_ON ("startup");
    startup();
    TRACE_OFF ("startup");

    process();

    lpshut(1);	/* one last time to clean up */
    /*NOTREACHED*/
}

static void
startup()
{
    struct passwd		*p;
    struct utsname		utsbuf;

    
    Starting = 1;
    getpaths();

    /*
     * There must be a user named "lp".
     */
    if ((p = lp_getpwnam(LPUSER)) == NULL)
	fail ("Can't find the user \"lp\" on this system!\n");
    lp_endpwent();
    
    Lp_Uid = p->pw_uid;
    Lp_Gid = p->pw_gid;

    /*
     * Only "root" and "lp" are allowed to run us.
     */
    if (getuid() && getuid() != Lp_Uid)
	fail ("You must be \"lp\" or \"root\" to run this program.\n");

    setuid (0);

    uname(&utsbuf);
    Local_System = strdup(utsbuf.nodename);

    /*
     * Make sure that all critical directories are present and that 
     * symbolic links are correct.
     */
    lpfsck();
    
    /*
     * Try setting the lock file to see if another Spooler is running.
     * We'll release it immediately; this allows us to fork the child
     * that will run in the background. The child will relock the file.
     */
    if (!(LockFp = open_lpfile(Lp_Schedlock, "a", 0664)))
	if (errno == EAGAIN)
	    fail ("Print services already active.\n");
	else
	    fail ("Can't open file \"%s\" (%s).\n", NB(Lp_Schedlock), PERROR);
    close_lpfile(LockFp);

    background();
    /*
     * We are the child process now.
#if	defined(DEBUG)
     * (That is, unless the debug flag is set.)
#endif
     */

    if (!(LockFp = open_lpfile(Lp_Schedlock, "w", 0664)))
	fail ("Failed to lock the file \"%s\" (%s).\n", NB(Lp_Schedlock), PERROR);

    Close (0);
    Close (2);
    if (am_in_background && !leave_stdout_open)
    {
	Close (1);
    }

    if ((OpenMax = ulimit(4, 0L)) == -1)
	OpenMax = OPEN_MAX;

    disable_signals();

    init_messages();

    init_network();

    init_memory();

    note ("Print services started.\n");
    Starting = 0;
}

void
#if	defined(__STDC__)
lpshut (
	int			immediate
)
#else
lpshut (immediate)
	int			immediate;
#endif
{
	int			i;

	extern MESG *		Net_md;


	/*
	 * If this is the first time here, stop all running
	 * child processes, and shut off the alarm clock so
	 * it doesn't bug us.
	 */
	if (!Shutdown) {
		mputm (Net_md, S_SHUTDOWN, 1);
		for (i = 0; i < ET_Size; i++)
			terminate (&Exec_Table[i]);
		alarm (0);
		Shutdown = (immediate? 2 : 1);
	}

	/*
	 * If this is an express shutdown, or if all the
	 * child processes have been cleaned up, clean up
	 * and get out.
	 */
	if (Shutdown == 2) {

		/*
		 * We don't shut down the message queues until
		 * now, to give the children a chance to answer.
		 * This means an LP command may have been snuck
		 * in while we were waiting for the children to
		 * finish, but that's OK because we'll have
		 * stored the jobs on disk (that's part of the
		 * normal operation, not just during shutdown phase).
		 */
		shutdown_messages();
    
		(void) close_lpfile(LockFp);
		(void) Unlink(Lp_Schedlock);

		note ("Print services stopped.\n");
		exit (0);
		/*NOTREACHED*/
	}
}

static void
process()
{
    register FSTATUS	*pfs;
    register PWSTATUS	*ppws;


    for (pfs = walk_ftable(1); pfs; pfs = walk_ftable(0))
	check_form_alert (pfs, (_FORM *)0);
    for (ppws = walk_pwtable(1); ppws; ppws = walk_pwtable(0))
	check_pwheel_alert (ppws, (PWHEEL *)0);
    
    /*
     * Clear the alarm, then schedule an EV_ALARM. This will clear
     * all events that had been scheduled for later without waiting
     * for the next tick.
     */
    alarm (0);
    schedule (EV_ALARM);

    /*
     * Start the ball rolling.
     */
    schedule (EV_INTERF, (PSTATUS *)0);
    schedule (EV_NOTIFY, (RSTATUS *)0);
    schedule (EV_SLOWF, (RSTATUS *)0);

#if	defined(CHECK_CHILDREN)
    schedule (EV_CHECKCHILD);
#endif

    for (EVER)
    {
	TRACE_ON ("take_message");
	take_message ();
	TRACE_OFF ("take_message");

	if (Sig_Alrm) {
		TRACE_ON ("schedule ALARM");
		schedule (EV_ALARM);
		TRACE_OFF ("schedule ALARM");
	}

	if (DoneChildren) {
		TRACE_ON ("dowait");
		dowait ();
		TRACE_OFF ("dowait");
	}

	if (Shutdown)
	    check_children();
	if (Shutdown == 2)
	    break;
    }
}

static void
#if	defined(__STDC__)
ticktock (
	int			sig
)
#else
ticktock(sig)
	int			sig;
#endif
{
	Sig_Alrm = 1;
	(void)signal (SIGALRM, ticktock);
	return;
}
			    
static void
background()
{
#if	defined(DEBUG)
    if (debug)
	return;
#endif
    
    switch(fork())
    {
	case -1:
	    fail ("Failed to fork child process (%s).\n", PERROR);
	    /*NOTREACHED*/

	case 0:
	    (void) setpgrp();
	    am_in_background = 1;
	    return;
	    
	default:
	    note ("Print services started.\n");
	    exit(0);
	    /* NOTREACHED */
    }
}

static void
usage()
{
    note (
#if	defined(DEBUG)
	"usage: lpsched [ options ]\n\
	[ -d ]			(debug mode, log execs, log messages)\n\
	[ -s ]			(don't trap most signals)\n\
	[ -f #filter-slots ]	(increase no. concurrent slow filters)\n\
	[ -n #notify-slots ]	(increase no. concurrent notifications)\n\
	[ -r #reserved-fds ]	(increase margin of file descriptors)\n\
\n\
WARNING: all these options are currently unsupported\n"
#else
	"usage: lpsched [ options ]\n\
	[ -f #filter-slots ]	(increase no. concurrent slow filters)\n\
	[ -n #notify-slots ]	(increase no. concurrent notifications)\n\
	[ -r #reserved-fds ]	(increase no. spare file descriptors)\n\
\n\
WARNING: all these options are currently unsupported\n"
#endif
	);
}

static void
Exit(n)
    int		n;
{
    fail ("Received unexpected signal %d; terminating.\n", n);
}

static void
disable_signals()
{
    int		i;

# if defined(DEBUG)
    if (!signals)
# endif
    for (i = 0; i < NSIG; i++)
	(void) signal(i, Exit);
    
    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGHUP, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) signal(SIGTERM, lpshut);	/* needs arg, but sig# OK */
    (void) signal(SIGCLD, SIG_IGN);
    (void) signal(SIGALRM, ticktock);
}
