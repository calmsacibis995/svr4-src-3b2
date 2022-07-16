/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)oampkg:libinst/setadmin.c	1.6"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <pkglocs.h>
#include "install.h"

#define DEFMAIL	"root"

extern struct admin 
	adm;
extern int	warnflag;
extern char	*fpkgparam();
extern void	progerr(),
		logerr(),
		quit(),
		free();

static struct {
	char	**memloc;
	char	*tag;
} admlist[] = {
	&adm.mail,	"mail",
	&adm.instance,	"instance",
	&adm.partial,	"partial",
	&adm.runlevel,	"runlevel",
	&adm.idepend,	"idepend",
	&adm.rdepend,	"rdepend",
	&adm.space,	"space",
	&adm.setuid,	"setuid",
	&adm.conflict,	"conflict",
	&adm.action,	"action",
	&adm.basedir,	"basedir",
	NULL
};
	
void
setadmin(file)
char	*file;
{
	FILE	*fp;
	int	i;
	char	param[64];
	char	*value;
	char	path[PATH_MAX];

	if(file == NULL)
		file = "default";
	else if(!strcmp(file, "none"))
		return;

	if(file[0] != '/')
		(void) sprintf(path, "%s/admin/%s", PKGADM, file);
	else
		(void) strcpy(path, file);

	if((fp = fopen(path, "r")) == NULL) {
		progerr("unable to open admin file <%s>", file);
		quit(99);
	}

	param[0] = '\0';
	adm.mail = DEFMAIL; /* if we don't assign anything to it */
	while(value = fpkgparam(fp, param)) {
		if(value[0] == '\0')
			continue; /* same as not being set at all */
		for(i=0; admlist[i].memloc; i++) {
			if(!strcmp(param, admlist[i].tag)) {
				*admlist[i].memloc = value;
				break;
			}
		}
		if(admlist[i].memloc == NULL) {
			logerr("WARNING: unknown admin parameter <%s>", param);
			warnflag++;
			free(value);
		}
		param[0] = '\0';
	}
}
