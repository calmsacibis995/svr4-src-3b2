/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:tam.h	1.1"
/* user-includeable tam definition file		*/
#ifndef tam_h

#define tam_h

#include	"sys/window.h"
#include	"kcodes.h"

#include	"chartam.h"

#define NWINDOW 20	/* max # of windows in a single process	*/
			/* must be >= NOFILE in <sys/param.h>	*/

struct wstat { short begy,begx,height,width; unsigned short uflags; };
typedef struct wstat WSTAT;

extern short wncur;		/* current window */
extern int LINES, COLS;

#define A_STANDOUT A_REVERSE

#endif
