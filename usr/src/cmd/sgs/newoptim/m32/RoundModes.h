/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/RoundModes.h	1.2"

enum RoundMode_E {Default,ToZero,ToNearest,ToPlusI,ToMinusI};

typedef enum RoundMode_E RoundMode;
