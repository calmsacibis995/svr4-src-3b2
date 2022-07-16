/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)face:src/filecab/fileb/listserv.c	1.7"

#include <stdio.h>
#include <string.h>
#include "wish.h"

main(argc,argv)
int argc;
char *argv[];
{
	FILE *fp;
	char *home, *getenv(), *label, *name, *penv, *fname;
	char tpath[PATHSIZ], hpath[PATHSIZ], buf[BUFSIZ], path[PATHSIZ], *opt;
	int flag=0, cond=0;

	penv=argv[argc-1];
	while(--argc > 0 && (*++argv)[0] == '-')
		for(opt=argv[0]+1; *opt != '\0'; opt++)
		switch(*opt) {
			case 'd':
				flag=1;
				break;
			case 'l': /* used to create the rmenu */
				flag=2;
				break;
			case 'm':
				flag=3;
				break;
			default:
				break;
		}
	home=getenv(penv);

	if (strcmp(penv,"HOME") == 0) {
		sprintf(hpath, "%s/pref/services",home);
		sprintf(tpath,"$VMSYS/OBJECTS/programs");
	}
	else {
		sprintf(hpath, "%s/lib/services",home);
		sprintf(tpath,"$OBJ_DIR");
	}

	if ((fp=fopen(hpath,"r")) == NULL) {
		printf("init=`message No Programs Installed`false\n");
		exit(FAIL);
	}

	while(fp && (fgets(buf,BUFSIZ,fp) != NULL)) {
		if (*buf == '\n' || *buf == '#' )
			continue;

		label = strtok(buf,"=");

		if (! strcmp(label,"name")) {
			name=strtok(NULL,"\n");
			sprintf(path,"%s/bin/%s.ins",home,name);
		} else if (! strcmp(label,"`echo 'name")) {
			name=strtok(NULL,"'");
			fname=strtok(NULL,"=");
			fname=strtok(NULL," ");
			if (! strcmp(fname,"OPEN"))
				continue;
			fname=strtok(NULL,"`");
			sprintf(path,"%s%s",home,&fname[strlen(penv)+1]);
		} else
			continue;
		if ( ! access(path,00)) {
			cond=1;
			if (flag == 2)  {
				printf("%s\n",name);
				continue;
			}
			printf("name=%s\n",name);
			printf("lininfo=\"%s\"\n",path);
			if (flag == 1 )
				printf("action=OPEN TEXT %s/Text.conf %s \"$LININFO\" \"%s\" `getfrm`\n",tpath,name,penv);
			else if (flag == 3 )
				printf("action=OPEN FORM %s/Form.mod %s \"$LININFO\" \"%s\" `getfrm`\n",tpath,name,penv);
			else 
				printf("action=`run %s`nop\n",path);
		}
	}
	if (!cond) {
		printf("init=`message No Programs Installed`false\n");
		exit(FAIL);
	}
	exit(SUCCESS);
}
