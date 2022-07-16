/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:filter/postscript/download/download.h	1.2"
/*
 *
 * The font data for a printer is saved in an array of the following type.
 *
 */

typedef struct map {

	char	*font;			/* a request for this PostScript font */
	char	*file;			/* means copy this unix file */
	int	downloaded;		/* TRUE after *file is downloaded */

} Map;

Map	*allocate();
