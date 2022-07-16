/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lxprof:bblk/u3b/macglb.c	1.1"
/*
* The common global variable (cmd_tbl[]) is initialize with 3B
*              dependent jump and branch opcodes.
*/

/* To add instructions to this table, must make sure CAinstr() in mac.c
   contains check to first letter of instruction */

	/* command table of all possible branch and jump instruction for the 3B */
	/* The table will be searched by the CAinstr() function */
char *cmd_tbl[] = {
	"acjl",
	"acjle",
	"acjleu",
	"acjlu",
	"atjnzb",
	"atjnzh",
	"atjnzw",
	"call",
	"jbc",
	"jbs",
	"jcc",
	"jcs",
	"je",
	"jg",
	"jge",
	"jgeu",
	"jgu",
	"jioe",
	"jiom",
	"jion",
	"jiot",
	"jl",
	"jle",
	"jleu",
	"jlu",
	"jmp",
	"jne",
	"jneg",
	"jnneg",
	"jnpos",
	"jnz",
	"jpos",
	"jsb",
	"jvc",
	"jvs",
	"jz",
	"rsb",
	"switch",
	"switcht"
};

char *tblptr;				/* table entry character pointer */

int tblsize = sizeof(cmd_tbl) / sizeof(tblptr);	/* command table size */
