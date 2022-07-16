/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:libadmIO.d/ctc_io.c	1.4"

#include	<sys/types.h>
#include	<stdio.h>
#include	<sys/vtoc.h>
#include	<sys/ct.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<sys/stropts.h>
#include	"libadmIO.h"

#define CTC_NAME	"CTC"
#define	CTC_BUFSZ	15872

extern void	*malloc();

static int	pfd;

static int	from_fd;
static int	to_fd;

static int	fd;

static void	local_exit();
static void	ctc_set_up();
static void	ctc_init();
static void	ctc_copy();
static void	ctc_wrap_up();
static void	ctc_open();
static void	ctc_read();
static void	ctc_write();
static void	ctc_close();

#ifdef TRACE
#define	DEBUG_FILE	"/usr/tmp/CTC.log"
FILE			*f;
#endif

main(argc, argv)
int	argc;
char	*argv[];
{
	int	cmd;
	int	p[8];

#ifdef TRACE
	if ((f = fopen(DEBUG_FILE, "a+")) == NULL) {
		exit(1);
	}
	fprintf(f, "*** CTC OUTPUT FILE ***\n");
	fflush(f);
#endif
	switch (atoi(argv[1])) {
	case DS_SET_UP:
		if (argc != 5) local_exit(1);
		pfd = atoi(argv[2]);
		ctc_set_up(
			argv[3],	/* device name */
			atoi(argv[4])	/* flags */
			);
		local_exit(0);
		break;
	case DS_INIT:
		if (argc != 7) local_exit(1);
		pfd = atoi(argv[2]);
		ctc_init(
			argv[3],	/* from device name */
			atoi(argv[4]),	/* from flags */
			argv[5],	/* to device name */
			atoi(argv[6])	/* to flags */
			);
		break;
	case DS_OPEN:
		if (argc != 5) local_exit(1);
		pfd = atoi(argv[2]);
		ctc_open(
			argv[3],	/* device name */
			atoi(argv[4])	/* flags */
			);
		break;
	default:
		local_exit(1);
		break;
	}
	while (1) {
#ifdef TRACE
		fflush(f);
#endif
		READ(pfd, &cmd, sizeof(int));

		switch (cmd) {
		case DS_COPY:
			READ(pfd, p, 4*sizeof(int));
			ctc_copy(
				p[0],	/* starting block for from device */
				p[1],	/* starting block for to device */
				p[2],	/* block size */
				p[3]	/* job size */
				);
			break;
		case DS_WRAP_UP:
			ctc_wrap_up();
			local_exit(0);
			break;
		case DS_READ:
			READ(pfd, p, 2*sizeof(int));
			ctc_read(
				p[0],	/* return from read call */
				p[1]	/* errno from read call */
				);
			break;
		case DS_WRITE:
			READ(pfd, p, 2*sizeof(int));
			ctc_write(
				p[0],	/* return from write call */
				p[1]	/* errno from write call */
				);
			break;
		case DS_CLOSE:
			ctc_close();
			local_exit(0);
			break;
		default:
			local_exit(1);
			break;
		}
	}
} /* main() */

static void
local_exit(i)
int	i;
{
#ifdef TRACE
	fprintf(f, "local_exit(%d)\n", i);
	fclose(f);
#endif
	exit(i);
} /* local_exit() */

static void
ctc_set_up(name, flags)
char	*name;
int	flags;
{
	int	ret = 0;

#ifdef TRACE
	fprintf(f, "ctc_set_up(%s, %d)\n", name, flags);
#endif
	WRITE(pfd, &ret, sizeof(int));
} /* ctc_set_up() */

static void
ctc_init(f_name, f_flags, t_name, t_flags)
char	*f_name;
int	f_flags;
char	*t_name;
int	t_flags;
{
	int	ret;

#ifdef TRACE
	fprintf(f, "ctc_init(%s, %d, %s, %d)\n",
				f_name, f_flags, t_name, t_flags);
#endif
	errno = 0;

	if ((from_fd = open(f_name, O_RDWR|O_CTSPECIAL)) < 0) {
		ret = -errno;
		WRITE(pfd, &ret, sizeof(int));
		local_exit(1);
	}
	if (ioctl(from_fd, STREAMON) < 0) {
		(void)close(from_fd);
		ret = -errno;
		WRITE(pfd, &ret, sizeof(int));
		local_exit(1);
	}
	(void)close(from_fd);

	if ((from_fd = open(f_name, f_flags)) < 0) {
		ret = -errno;
		WRITE(pfd, &ret, sizeof(int));
		local_exit(1);
	}
	ret = CTC_BUFSZ;

	WRITE(pfd, &ret, sizeof(int));

	sleep(2);
	if (ioctl(pfd, I_SENDFD, from_fd) < 0) {
		(void)close(from_fd);
		local_exit(1);
	}
	if ((to_fd = open(t_name, O_RDWR|O_CTSPECIAL)) < 0) {
		(void)close(from_fd);
		local_exit(1);
	}
	if (ioctl(to_fd, STREAMON) < 0) {
		(void)close(from_fd);
		(void)close(to_fd);
		local_exit(1);
	}
	(void)close(to_fd);

	if ((to_fd = open(t_name, t_flags)) < 0) {
		(void)close(from_fd);
		local_exit(1);
	}
	sleep(2);
	if (ioctl(pfd, I_SENDFD, to_fd) < 0) {
		(void)close(from_fd);
		(void)close(to_fd);
		local_exit(1);
	}
} /* ctc_init() */

static void
ctc_copy(f_start, t_start, block_size, job_size)
int	f_start;
int	t_start;
int	block_size;
int	job_size;
{
	int	cnt;
	int	ret[2];
	char	*p;

#ifdef TRACE
	fprintf(f, "ctc_copy(%d, %d, %d, %d, %d, %d)\n",
			from_fd, to_fd, f_start, t_start, block_size, job_size);
#endif
	ret[0] = 0;
	errno = 0;

	if ((job_size != CTC_BUFSZ) || ((p = malloc(job_size)) == NULL)) {
		ret[0] = -1;
		ret[1] = errno;
		WRITE(pfd, ret, 2*sizeof(int));
		return;
	}
	if ((cnt = read(from_fd, p, job_size)) != job_size) {
		if (cnt > 0) {
			job_size = cnt;
		}
		else {
			job_size = 0;
		}
	}
	ret[0] += cnt;

	if (job_size) {
		ret[0] = write(to_fd, p, job_size);
	}
	ret[1] = errno;
	WRITE(pfd, ret, 2*sizeof(int));
} /* ctc_copy() */

static void
ctc_wrap_up()
{
	int	ret[2];

#ifdef TRACE
	fprintf(f, "ctc_wrap_up(%d, %d)\n", from_fd, to_fd);
#endif
	ret[0] = 0;
	ret[1] = 0;

	if (close(from_fd))
		ret[0] = errno;

	if (close(to_fd))
		ret[1] = errno;

	WRITE(pfd, ret, 2*sizeof(int));
} /* ctc_wrap_up() */

/*
  * open: Determine the device being accessed, set the buffer size,
 * and perform any device specific initialization.
 * For the special case of the 3B2 CTC
 * several unusual things must be done.
 * To enable streaming mode on the CTC,
 * the file descriptor must be closed, re-opened
 * (with O_RDWR and O_CTSPECIAL flags set),
 * the STREAMON ioctl(2) command issued,
 * and the file descriptor re-re-opened either read-only or write_only.
 */

static void
ctc_open(name, flags)
char	*name;
int	flags;
{
	int	ret;
	
#ifdef TRACE
	fprintf(f, "ctc_open(%s, %d)\n", name, flags);
#endif
	errno = 0;

	if ((fd = open(name, O_RDWR|O_CTSPECIAL)) < 0) {
		ret = -(errno + 1000);
		WRITE(pfd, &ret, sizeof(int));
		local_exit(1);
	}
	if (ioctl(fd, STREAMON) < 0) {
		(void)close(fd);
		ret = -(errno + 2000);
		WRITE(pfd, &ret, sizeof(int));
		local_exit(1);
	}
	(void)close(fd);

	if ((fd = open(name, flags)) < 0) {
		ret = -(errno + 3000);
		WRITE(pfd, &ret, sizeof(int));
		local_exit(1);
	}
	ret = CTC_BUFSZ;

	WRITE(pfd, &ret, sizeof(int));

	sleep(2);
	if (ioctl(pfd, I_SENDFD, fd) < 0) {
		(void)close(fd);
		local_exit(1);
	}
	ret = FIXED;

	WRITE(pfd, &ret, sizeof(int));
} /* ctc_open() */

static void
ctc_read(ret, errno)
int	ret;
int	errno;
{
	int	r[2];

#ifdef TRACE
	fprintf(f, "ctc_read(%d, %ld)\n", ret, errno);
#endif
	r[0] = ret;
	r[1] = errno;

	if (ret == 0 && errno == 0) {
		r[0] = -1;
		r[1] = ENOSPC;
	}
	WRITE(pfd, r, 2*sizeof(int));
} /* ctc_read() */

static void
ctc_write(ret, errno)
int	ret;
int	errno;
{
	int	r[2];

#ifdef TRACE
	fprintf(f, "ctc_write(%d, %ld)\n", ret, errno);
#endif
	r[0] = ret;
	r[1] = errno;

	if (ret == -1 && errno == ENXIO) {
		r[1] = ENOSPC;
	}
	WRITE(pfd, r, 2*sizeof(int));
} /* ctc_write() */

static void
ctc_close()
{
	int	r = 0;

#ifdef TRACE
	fprintf(f, "ctc_close(%d)\n", fd);
#endif
	if (close(fd))
		r = errno;

	WRITE(pfd, &r, sizeof(int));
} /* ctc_close() */
