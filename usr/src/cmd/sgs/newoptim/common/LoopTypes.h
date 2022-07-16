/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/LoopTypes.h	1.2"

enum LoopTypes_E {Begin,Header,Increment,Condition,End,Missing};

typedef enum LoopTypes_E LoopType;
