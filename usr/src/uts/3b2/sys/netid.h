/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_NETID_H
#define _SYS_NETID_H

#ident	"@(#)head.sys:sys/netid.h	11.2"

#define NIDCPY(a,b)	(a[0]=b[0],\
			 a[1]=b[1],\
			 a[2]=b[2],\
			 a[3]=b[3],\
			 a[4]=b[4],\
			 a[5]=b[5])

#define NIDCLR(a)	(a[0]=0,\
			 a[1]=0,\
			 a[2]=0,\
			 a[3]=0,\
			 a[4]=0,\
			 a[5]=0)

#define NIDCMP(a,b)	(a[0]==b[0]&&\
			 a[1]==b[1]&&\
			 a[2]==b[2]&&\
			 a[3]==b[3]&&\
			 a[4]==b[4]&&\
			 a[5]==b[5])


#endif	/* _SYS_NETID_H */
