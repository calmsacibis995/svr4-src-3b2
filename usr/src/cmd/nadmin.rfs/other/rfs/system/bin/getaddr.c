/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nadmin.rfs:other/rfs/system/bin/getaddr.c	1.1"
#include <netconfig.h>
#include <netdir.h>
#include <tiuser.h>
#include <stdio.h>

#define NULL 0
/* arguments:argv[1] =netspec
             argv[2] =hostname
             argv[3] =domain
             argv[4] =type(primary or secondary)
*/

main(argc,argv)
int argc;
char *argv[];

{
struct nd_hostserv 	nd_hostserv;
struct netconfig	*netconfigp;
struct nd_addrlist	*nd_addrlistp;
struct netbuf 		*addr;
char			*netspec;
char			*domain;
char			*type;
char			buf[300];
FILE			*fp,*efp;

int i;

nd_hostserv.h_host=argv[2];
nd_hostserv.h_serv="listen";
netspec=argv[1];
domain=argv[3];
type=argv[4];


/* transport not installed */
if ((netconfigp=getnetconfigent(netspec)) == NULL) {
	exit(1);
}

/* name to address mapping not setup correctly */
if (netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) != 0) {
	exit(2);
}

addr=nd_addrlistp->n_addrs;

sprintf(buf,"/etc/rfs/%s",argv[1]);

if (access(buf,0) == -1 ) {
	mkdir(buf,0755);
}

strcat(buf,"/rfmaster");
if ((fp=fopen(buf, "a")) == NULL) {
	exit(2);
}
fprintf(fp,"%s\t%s\t%s.%s\n%s.%s\tA\t",domain,type,domain,argv[2],domain,argv[2]);
fprintf(fp,"\\x");

for (i=0; i<addr->len; i++) {
	fprintf(fp,"%.2x",addr->buf[i]);
}

fprintf(fp,"\n");
fclose(fp);
exit(0);

}
