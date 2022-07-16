/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:intf_include/menu_io.h	1.1"

#define P_NONE		0	/* no placeholder */
#define P_INACTIVE	1	/* placeholder - inactive */
#define P_ACTIVE	2	/* placeholder - active */

struct item_def {
	char mname[17];		/* menu item name */
	char mdescr[59];	/* menu item description */
	char help[15];		/* menu item help file */
	int pholder;		/* placeholder status */
	char maction[128];	/* menu item action */
	char pkginsts[100];	/* pkg instance identifiers */
	char orig_name[17];	/* original name if rename */
	struct item_def *next;	/* next menu item */
};

struct menu_file {
	struct menu_line *head;		/* menu file header lines */
	struct item_def *entries;	/* menu file entries */
};
