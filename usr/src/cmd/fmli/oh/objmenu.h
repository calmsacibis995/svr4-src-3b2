/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)vm.oh:src/oh/objmenu.h	1.2"

typedef struct {
	int flags;
	int numactive;		/* number of inactive menu items */
	struct fm_mn fm_mn;
	int *visible;
	int *slks;
	char **args;
} menuinfo;
