/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/smtp.h	1.3"
#ident "@(#)smtp.h	1.3 'attmail mail(1) command'"
/* smtp constants and the like */

/* tunable constants */
#define MAXSTR 10240			/* maximum string length */
#define NAMSIZ MAXSTR			/* max file name length */

typedef struct namelist namelist;
struct namelist {
	namelist *next;
	char *name;
};

/* spooling constants */
#ifdef SVR4
#define SMTPQ "/var/spool/smtpq"
#else
#define SMTPQ "/usr/spool/smtpq"
#endif
#define SMTP "/smtp"
