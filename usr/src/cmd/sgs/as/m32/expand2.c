/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/expand2.c	1.1"


#include "expand2.h"


/*	Information for span-dependent instruction types
 */


ITINFO	itinfo[NITYPE] =
{
	/* binc			hinc */
	{UBR_HSZ - UBR_BSZ,	UBR_WSZ - UBR_HSZ,},	/* UBR */
	{CBR_HSZ - CBR_BSZ,	CBR_WSZ - CBR_HSZ,},	/* CBR */
	{JSB_HSZ - JSB_BSZ,	JSB_WSZ - JSB_HSZ,},	/* JSB */
	{CALL_HSZ - CALL_BSZ,	CALL_WSZ - CALL_HSZ,},	/* CALL */
};
