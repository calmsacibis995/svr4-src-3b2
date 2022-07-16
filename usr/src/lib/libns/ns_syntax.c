/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:ns_syntax.c	1.1.1.1"
#include <nserve.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "nslog.h"

#define INV_RES "/. "
#define RES_MESG "resource name cannot contain '/', '.', ' ', or non-printable characters"

#define INV_MACH ":,.\n"
#define MACH_MESG "name cannot contain ':', ',', '\\n', '.', or non-printable characters"

#define VAL_DOM "-_"
#define DOM_MESG "domain name must contain only alphanumerics, '_', or '-'"

#define INV_MESG "contains invalid characters"

pv_resname(cmd,name,flag)
char	*cmd;
char	*name;
int	flag;
{

	LOG2(L_TRACE, "(%5d) enter: pv_resname\n", Logstamp);
	if (name == NULL || *name == '\0') {
		fprintf(stderr,"%s: resource name is null\n",cmd);
		LOG2(L_TRACE, "(%5d) leave: pv_resname\n", Logstamp);
		return(1);
	}

	if (v_resname(name)) {
		fprintf(stderr,"%s: resource name <%s> %s\n",cmd,name,INV_MESG);
		if (!flag)
			fprintf(stderr,"%s: %s\n",cmd,RES_MESG);
		LOG2(L_TRACE, "(%5d) leave: pv_resname\n", Logstamp);
		return(1);
	}

	if (strlen(name) > SZ_RES) {
		fprintf(stderr,"%s: resource name <%s> exceeds <%d> characters\n",cmd,name,SZ_RES);
		LOG2(L_TRACE, "(%5d) leave: pv_resname\n", Logstamp);
		return(2);
	}
	LOG2(L_TRACE, "(%5d) leave: pv_resname\n", Logstamp);
	return(0);
}

pv_uname(cmd,name,flag,title)
char	*cmd;
char	*name;
int	flag;
char	*title;
{

	LOG2(L_TRACE, "(%5d) enter: pv_uname\n", Logstamp);
	if (name == NULL || *name == '\0') {
		fprintf(stderr,"%s: %sname is null\n",cmd, title);
		LOG2(L_TRACE, "(%5d) leave: pv_uname\n", Logstamp);
		return(1);
	}

	if (v_uname(name)) {
		fprintf(stderr,"%s: %sname <%s> %s\n",cmd,title,name,INV_MESG);
		if (!flag)
			fprintf(stderr,"%s: %s%s\n",cmd,title,MACH_MESG);
		LOG2(L_TRACE, "(%5d) leave: pv_uname\n", Logstamp);
		return(1);
	}

	if (strlen(name) > SZ_MACH) {
		fprintf(stderr,"%s: %sname <%s> exceeds <%d> characters\n",cmd,title,name,SZ_MACH);
		LOG2(L_TRACE, "(%5d) leave: pv_uname\n", Logstamp);
		return(2);
	}
	LOG2(L_TRACE, "(%5d) leave: pv_uname\n", Logstamp);
	return(0);
}

pv_dname(cmd,name,flag)
char	*cmd;
char	*name;
int	flag;
{

	LOG2(L_TRACE, "(%5d) enter: pv_dname\n", Logstamp);
	if (name == NULL || *name == '\0') {
		fprintf(stderr,"%s: domain name is null\n",cmd);
		LOG2(L_TRACE, "(%5d) leave: pv_dname\n", Logstamp);
		return(1);
	}

	if (v_dname(name)) {
		fprintf(stderr,"%s: domain name <%s> %s\n",cmd,name,INV_MESG);
		if (!flag)
			fprintf(stderr,"%s: %s\n",cmd,DOM_MESG);
		LOG2(L_TRACE, "(%5d) leave: pv_dname\n", Logstamp);
		return(1);
	}

	if (strlen(name) > SZ_DELEMENT) {
		fprintf(stderr,"%s: domain name <%s> exceeds <%d> characters\n",cmd,name,SZ_DELEMENT);
		LOG2(L_TRACE, "(%5d) leave: pv_dname\n", Logstamp);
		return(2);
	}
	LOG2(L_TRACE, "(%5d) leave: pv_dname\n", Logstamp);
	return(0);
}

/*
 *	Function to check the resource name for '/', '.', ' ',
 *	or non-printable characters.
 */

v_resname(name)
register char *name;
{

	LOG2(L_TRACE, "(%5d) enter: v_resname\n", Logstamp);
	while (*name != '\0') {
		if (!isprint(*name)) {
			LOG2(L_TRACE, "(%5d) leave: v_resname\n", Logstamp);
			return(1);
		} else {
			if (strchr(INV_RES,*name) != NULL) {
				LOG2(L_TRACE, "(%5d) leave: v_resname\n", Logstamp);
				return(1);
			}
		}
		name++;
	}
	LOG2(L_TRACE, "(%5d) leave: v_resname\n", Logstamp);
	return(0);
}

/*
 *	Function to check the nodename for ':', '.', ',', '\\n'
 *	or non-printable characters.
 */

v_uname(name)
register char *name;
{

	LOG2(L_TRACE, "(%5d) enter: v_uname\n", Logstamp);
	while (*name != '\0') {
		if (!isprint(*name)) {
			LOG2(L_TRACE, "(%5d) leave: v_uname\n", Logstamp);
			return(1);
		} else {
			if (strchr(INV_MACH,*name) != NULL) {
				LOG2(L_TRACE, "(%5d) leave: v_uname\n", Logstamp);
				return(1);
			}
		}
		name++;
	}
	LOG2(L_TRACE, "(%5d) leave: v_uname\n", Logstamp);
	return(0);
}

/*
 *	Function to check the domain name for any character
 *	other than '-', '_' or alphanumerics.
 */

v_dname(name)
register char *name;
{

	LOG2(L_TRACE, "(%5d) enter: v_dname\n", Logstamp);
	while (*name != '\0') {
		if (!isalnum(*name) && strchr(VAL_DOM,*name) == NULL) {
			LOG2(L_TRACE, "(%5d) leave: v_dname\n", Logstamp);
			return(1);
		}
		name++;
	}
	LOG2(L_TRACE, "(%5d) leave: v_dname\n", Logstamp);
	return(0);
}
