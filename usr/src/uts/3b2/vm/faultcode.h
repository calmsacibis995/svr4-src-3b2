/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_FAULTCODE_H
#define _VM_FAULTCODE_H

#ident	"@(#)kernel:vm/faultcode.h	1.2"

/*
 * This file describes the "code" that is delivered during
 * SIGBUS and SIGSEGV exceptions.  It also describes the data
 * type returned by vm routines which handle faults.
 *
 * If FC_CODE(fc) == FC_OBJERR, then FC_ERRNO(fc) contains the errno value
 * returned by the underlying object mapped at the fault address.
 */
#define	FC_HWERR	0x1	/* misc hardware error (e.g. bus timeout) */
#define	FC_ALIGN	0x2	/* hardware alignment error */
#define	FC_NOMAP	0x3	/* no mapping at the fault address */
#define	FC_PROT		0x4	/* access exceeded current protections */
#define	FC_OBJERR	0x5	/* underlying object returned errno value */

#define	FC_MAKE_ERR(e)	(((e) << 8) | FC_OBJERR)

#define	FC_CODE(fc)	((fc) & 0xff)
#define	FC_ERRNO(fc)	((unsigned)(fc) >> 8)

#ifndef LOCORE
typedef	int	faultcode_t;	/* type returned by vm fault routines */
#endif /* LOCORE */

#endif	/* _VM_FAULTCODE_H */
