/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:sys/ledt.h	1.2"


extern int edt_count;
extern struct s3bconf	*sys3bconfig;


/* extended s3bconf structure for two (HA/TC) level devices */

struct xs3bconf{
	int	haslot;			/* Controlling HA */
	long	maj;			/* external major number of TC */
	char	tcname[DIRSIZ];		/* TC driver name */
	struct xs3bconf *next;		/* pointer to next entry */
};

#define HTCMAJ 127		/* Starting TC major number */
#define LTCMAJ 71		/* Last TC major number,
                                 * major numbers 71-127 are
                                 * reserved for two tri-devices
                                 */

#define S3BC_TCDRV	0x10	/* EDT name[] is a TC driver; board code is the
                                 * major number of this TC.
                                 */

#define IMEG (1024 * 1024)
#define BSIZE 512

#define CHKMAJ(x,y) (((x-y) >= LTCMAJ) ? 1:0)	/* x is starting major number,
                                                 * y is number of majors to
                                                 * be allocated.
                                                 */

extern void mark();
extern void fndxbus();
extern void catch();
