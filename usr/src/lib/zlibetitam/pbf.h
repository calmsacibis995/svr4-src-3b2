/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:pbf.h	1.1"
/*
 *  pbf.h - V1.1
 *
 *	Paste buffer definitions
 */

struct	s_kwtbl {			/* Keyword table structure */
  char	*keyword;
  int	token;
};

	/*  ADF token definitions */

#define	atk_err		-1		/* Error */
#define	atk_null	 0		/* Empty line */

extern	FILE *pb_open ();
extern	char *pb_name ();
extern	char *pb_gets ();
extern	char *adf_gtwrd ();
extern	char *adf_gtxcd ();
