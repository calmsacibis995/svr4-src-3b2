/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libw:_wchar.h	1.2"
#define SS2     0x8e
#define SS3     0x8f
#define EUCMASK 0x8080
#define P11     0x8080          /* Code Set 1 */
#define P01     0x0080          /* Code Set 2 */
#define P10     0x8000          /* Code Set 3 */
#define multibyte       (__ctype[520]>1)
#define eucw1   __ctype[514]
#define eucw2   __ctype[515]
#define eucw3   __ctype[516]
#define scrw1   __ctype[517]
#define scrw2   __ctype[518]
#define scrw3   __ctype[519]
