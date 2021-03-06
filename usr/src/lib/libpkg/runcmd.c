/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:runcmd.c	1.8.2.2"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>

static char	*errfile = NULL;

extern int	errno;
extern void	*calloc(),
		progerr(),
		logerr(),
		free(),
		exit();
extern pid_t	fork(),
		wait();
extern int	execl(),
		unlink();

void
rpterr()
{
	FILE	*fp;
	int	c;

	if(errfile) {
		if(fp = fopen(errfile, "r")) {
			while((c = getc(fp)) != EOF)
				putc(c, stderr);
			(void) fclose(fp);
		}
		(void) unlink(errfile);
		errfile = NULL;
	}
}

void
ecleanup()
{
	if(errfile) {
		(void) unlink(errfile);
		errfile = NULL;
	}
}

int
esystem(cmd, ifd, ofd)
char *cmd;
int ifd, ofd;
{
	char	*errfile;
	int	status;
	pid_t	pid;

	errfile = tmpnam(NULL);
	if(errfile == NULL) {
		progerr("unable to create temp error file, errno=%d", errno);
		return(-1);
	}
	pid = fork();
	if(pid == 0) {
		/* child */
		if(ifd > 0) {
			(void)close(0);
			(void)fcntl(ifd, F_DUPFD, 0);
			(void)close(ifd);
		}
		if(ofd >= 0 && ofd != 1) {
			(void)close(1);
			(void)fcntl(ofd, F_DUPFD, 1);
			(void)close(ofd);
		}
		freopen(errfile, "w", stderr);
		execl("/sbin/sh", "/sbin/sh", "-c", cmd, NULL);
		progerr("exec of <%s> failed, errno=%d", cmd, errno);
		exit(99);
	} else if(pid < 0) {
		logerr("bad fork(), errno=%d", errno);
		return(-1);
	} 

	/* parent process */
	sighold(SIGINT);
	pid = waitpid(pid, &status, 0);
	sigrelse(SIGINT);

	if(pid < 0)
		return(-1); /* probably interrupted */

	switch(status & 0177) {
	  case 0:
	  case 0177:
		status = status >> 8;

	  default:
		/* terminated by a signal */
		status = status & 0177;
	}
	if(status == 0)
		ecleanup();
	return(status);
}

FILE *
epopen(cmd, mode)
char	*cmd, *mode;
{
	char	*buffer;
	FILE	*pp;

	if(errfile) {
		/* cleanup previous errfile */
		unlink(errfile);
	}

	errfile = tmpnam(NULL);
	if(errfile == NULL) {
		progerr("unable to create temp error file, errno=%d", errno);
		return((FILE *) 0);
	}

	buffer = (char *) calloc(strlen(cmd)+6+strlen(errfile), sizeof(char));
	if(buffer == NULL) {
		progerr("no memory in epopen(), errno=%d", errno);
		return((FILE *) 0);
	}

	if(strchr(cmd, '|'))
		(void) sprintf(buffer, "(%s) 2>%s", cmd, errfile);
	else
		(void) sprintf(buffer, "%s 2>%s", cmd, errfile);

	pp = popen(buffer, mode);

	free(buffer);
	return(pp);
}

int
epclose(pp)
FILE *pp;
{
	int n;

	n = pclose(pp);
	if(n == 0)
		ecleanup();
	return(n);
}
