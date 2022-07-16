/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:hdrs/install.h	1.3"

#define MAILCMD	"/usr/bin/mail"
#define DATSTRM	"datastream"
#define SHELL	"/sbin/sh"
#define PKGINFO	"pkginfo"
#define PKGMAP	"pkgmap"

struct mergstat {
	char	*setuid;
	char	*setgid;
	char	contchg;
	char	attrchg;
	char	shared;
};

struct admin {
	char	*mail;
	char	*instance;
	char	*partial;
	char	*runlevel;
	char	*idepend;
	char	*rdepend;
	char	*space;
	char	*setuid;
	char	*conflict;
	char	*action;
	char	*basedir;
};

#define ADM(x, y)	!strcmp(adm.x, y)
