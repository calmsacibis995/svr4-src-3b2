/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pkginfo.h:pkginfo.h	1.5.1.2"

#define PI_INSTALLED 	0
#define PI_PARTIAL	1
#define PI_PRESVR4	2
#define PI_UNKNOWN	3
#define PI_SPOOLED	4

#define COREPKG	"core"

struct pkginfo {
	char	*pkginst;
	char	*name;
	char	*arch;
	char	*version;
	char	*vendor;
	char	*basedir;
	char	*catg;
	char	status;
};

extern char	*pkgdir;

extern char	*pkgparam();
extern int	pkginfo(),
		pkgnmchk();
