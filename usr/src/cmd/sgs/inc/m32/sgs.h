/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xenv:m32/sgs.h	1.67"
/*
 */

#define	SGS	""

/*	The symbol FBOMAGIC is defined in filehdr.h	*/

#define MAGIC	FBOMAGIC
#define TVMAGIC (MAGIC+1)

#define ISMAGIC(x)	(x ==  MAGIC)


#ifdef ARTYPE
#define	ISARCHIVE(x)	( x ==  ARTYPE)
#define BADMAGIC(x)	((((x) >> 8) < 7) && !ISMAGIC(x) && !ISARCHIVE(x))
#endif


/*
 *	When a UNIX aout header is to be built in the optional header,
 *	the following magic numbers can appear in that header:
 *
 *		AOUT1MAGIC : default
 *		PAGEMAGIC  : configured for paging
 */

#define AOUT1MAGIC 0407
#define AOUT2MAGIC 0410
#define PAGEMAGIC  0413
#define LIBMAGIC   0443

/* The first few .got and .plt entries are reserved
 *	PLT[0]	jump to dynamic linker (indirect through GOT[2])
 *
 *	GOT[0]	address of _DYNAMIC
 *	GOT[1]	link map address
 *	GOT[2]	address of rtbinder in rtld
 */
#define PLT_XRTLD	0	/* plt index for jump to rtld */
#define PLT_XNumber	1

#define GOT_XDYNAMIC	0	/* got index for _DYNAMIC */
#define GOT_XLINKMAP	1	/* got index for link map */
#define GOT_XRTLD	2	/* got index for rtbinder */
#define GOT_XNumber	3


#define	SGSNAME	""
#define PLU_PKG "C Development Set "
#define PLU_REL "(CDS) 5.0 09/26/89"
#define CPL_PKG "C Development Set "
#define CPL_REL "(CDS) 5.0 09/26/89"
#define SGU_PKG "C Development Set "
#define SGU_REL "(CDS) 5.0 09/26/89"
#define ACU_PKG "Enhanced Programming Utilities "
#define ACU_REL "(EPU) 5.0 09/26/89"
#define ESG_PKG "Enhanced Programming Utilities "
#define ESG_REL "(EPU) 5.0 09/26/89"
#define CPPT_PKG "Enhanced Programming Utilities "
#define CPPT_REL "(EPU) 5.0 09/26/89"
