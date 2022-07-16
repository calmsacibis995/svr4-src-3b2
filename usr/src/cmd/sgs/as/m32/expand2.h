/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)as:m32/expand2.h	1.1"


/*	Span-dependent instruction types
 */

typedef	enum
{
	IT_UBR,		/* unconditional branch */
	IT_CBR,		/* conditional branch */
	IT_JSB,		/* jump subroutine */
	IT_CALL,	/* call */
	IT_LABEL,	/* label definition */
	NITYPE
} ITYPE;


typedef struct
{
	unsigned int	it_binc,	/* byte->half increment */
			it_hinc;	/* half->word increment */
} ITINFO;

extern ITINFO	itinfo[];


/*	The sizes below are for span-dependent optimizations.
 *	Branches and JSB sizes cover the entire instruction;
 *	call sizes include only the span-dependent operand.
 *
 *	Word conditional jumps don't exist; they are composed
 *	of a conditional byte jump and an unconditional word jump.
 */

#define UBR_BSZ		2
#define UBR_HSZ		3
#define UBR_WSZ		6
#define CBR_BSZ		2
#define CBR_HSZ		3
#define CBR_WSZ		(CBR_BSZ + UBR_WSZ)
#define JSB_BSZ		2
#define JSB_HSZ		3
#define JSB_WSZ		6
#define CALL_BSZ	2
#define CALL_HSZ	3
#define CALL_WSZ	5

#define SPAN_BLO	(-128)
#define SPAN_BHI	(127)
#define SPAN_HLO	(-32768L)
#define SPAN_HHI	32767L
