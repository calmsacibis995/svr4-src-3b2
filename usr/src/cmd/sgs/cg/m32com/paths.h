/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nifg:cg/mau/paths.h	1.1"
/*
 *	static char *ID_pathsh = "@(#) paths.h: 1.8 9/19/84";
 *
 *
 *	Pathnames for m32
 */

/*
 *	Directory containing libraries and executable files
 *	(e.g. assembler pass 1)
 */
#define LIBDIR	"/sgs/sgs1/cmd/sgs/xenv/m32/lib"
#define LLIBDIR1 "/sgs/sgs1/cmd/sgs/xenv/m32/lib/../usr/lib"
#define NDELDIRS 2

/*
 *	Directory containing executable ("bin") files
 */
#define BINDIR	"/sgs/sgs1/xbin"

/*
 *	Directory containing include ("header") files for building tools
 */
#define INCDIR	"/tmp"

/*
 *	Directory for "temp"  files
 */
#define TMPDIR	"/tmp"

/*
 *	Default name of output object file
 */
#define A_OUT	"m32a.out"

/*
 *	The following pathnames will be used by the "cc" command
 *
 *	m32 cross compiler
 */
#define CPP	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/cpp"
/*
 *	Directory containing include ("header") files for users' use
 */
#define INCLDIR	"-I/tmp"
#define COMP	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/comp"
#define C0	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/front"
#define C1	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/back"
#define OPTIM	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/optim"
/*
 *	m32 cross assembler
 */
#define AS	"/sgs/sgs1/xbin/m32as"
#define AS1	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/m32as1"	/* assembler pass 1 */
#define AS2	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/m32as2"	/* assembler pass 2 */
#define M4	"/usr/bin/m4"			/* macro preprocessor */
#define CM4DEFS	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/cm4defs"	/* C interface macros */
#define CM4TVDEFS "/sgs/sgs1/cmd/sgs/xenv/m32/lib/cm4tvdefs"	/* C macros with 'tv' call */
/*
 *	m32 link editor
 */
#define LD	"/sgs/sgs1/xbin/m32ld"
#define LD2	"/sgs/sgs1/cmd/sgs/xenv/m32/lib/m32ld2"	/* link editor pass 2 */
