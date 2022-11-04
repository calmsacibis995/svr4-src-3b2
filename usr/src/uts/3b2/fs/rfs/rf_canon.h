/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_RFS_RF_CANON_H
#define _FS_RFS_RF_CANON_H

#ident	"@(#)fs:fs/rfs/rf_canon.h	1.2"

/*
 * Macros describing the format of various structures.
 */
#define COMMV1_FMT	"llllllllll"		/* version 1 request/response */
#define MSGCOM_FMT	"llllllllllllllllll"	/* message and common headers */
#define RESPV2_FMT	"llllllllllllll"
#define REQV2_FMT	"llllllllllllllllllllllllllllllllllllllllllllll"
#define DIRENT_FMT	"llsc0"
#define FLOCK_FMT	"ssllllllll"
#define O_FLOCK_FMT	"ssllss"
#define STAT_FMT	"sssssssllll"
#define STATFS_FMT	"sllllllc6c6"
#define USTAT_FMT	"lsc6c6"
#define ATTR_FMT	"llllllllllllllllllllllllll"		/* rf_attr */
#define RFLKC_FMT	"lllllllllllllllllllllllllllll"		/* rflkc_info */
#define SYMLNK_FMT	"llllllllllllllllllllllllllc256c0"	/* symlnkarg */
#define STATVFS_FMT	"lllllllllc16llc32llllllllllllllll"
#define MKDENT_FMT	"llllllllllllllllllllllllllc0"

/*
 * character, short, integer type expansion
 */
#define CXPAND		(2 * sizeof(long))
#define SXPAND		(sizeof(long) - sizeof(short))
#define IXPAND		(sizeof(int))

/*
 * Macros for heterogeneity buffer expansion.
 */
#define MINXPAND	(2 * sizeof(long))		/* for misalignment */
#define DIRENT_XP	(SXPAND + CXPAND)		/* struct dirent */
#define FLOCK_XP	(MINXPAND + 2*SXPAND)		/* struct flock  */
#define OFLOCK_XP	(MINXPAND + 4*SXPAND)		/* struct oflock  */
#define STAT_XP		(MINXPAND + 7*SXPAND)		/* struct stat   */
#define STATFS_XP	(MINXPAND + SXPAND + 2*CXPAND)	/* struct statfs */
#define USTAT_XP	(MINXPAND + SXPAND + 2*CXPAND)	/* struct ustat */
#define ATTR_XP		MINXPAND 			/* struct vattr */
#define RFLKC_XP	MINXPAND 			/* rflkc_info_t */
#define SYMLNK_XP	(MINXPAND + 2*CXPAND)		/* symlnkarg */
#define STATVFS_XP	(MINXPAND + 2*CXPAND)		/* struct statvfs */
#define MKDENT_XP	(MINXPAND + CXPAND)		/* struct mkdent */

/*
 * Generic data canonization
 */
extern int	rf_fcanon();
extern int	rf_tcanon();

/*
 * RFS header-specific data canonization
 */
extern int	rf_mcfcanon();
extern void	rf_hdrtcanon();
extern int	rf_rhfcanon();

/*
 * Directory entry canonization.
 */
extern int	rf_dentcanon();
extern int	rf_denfcanon();

#endif /* _FS_RFS_RF_CANON_H */
