/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xenv:m32/paths.h	1.14"
/*
 *
 *
 *	Pathnames for m32/m32mau
 */

/*
 *	Default directory search path for link-editor
 *	YLDIR says which directory in LIBPATH is replaced
 * 	by the -YL option to cc and ld.  YUDIR says
 *	which directory is replaced by -YU.
 */
#define LIBPATH	"M32LIBPATH"
#define YLDIR 1
#define YUDIR 2
/*	Directory containing library files and executables
 *	not accessed directly by users ("lib") files
 */
#define LIBDIR	"M32LIBDIR"

/*
 *	Directory containing executable ("bin") files
 */
#define BINDIR	"M32BINDIR"

/*	Absolute pathnames for dynamic shared library targets
 */
#define	LDSO_NAME "/usr/lib/ld.so.1"
#define	LIBCSO_NAME "/usr/lib/libc.so.1"

/*	Directory containing minimal abi library
 */
#define ABILIBDIR  "M32ABILIBDIR"
/*
 *	Directory containing include ("header") files for building tools
 */

#define INCDIR	"M32INCDIR"

/*
 *	Directory for "temp"  files
 */
#define TMPDIR	"M32TMPDIR"

/*
 *	Default name of output object file
 */
#define A_OUT	"SGSa.out"

/*
 *	The following pathnames will be used by the "cc" command
 *
 *	m32/m32mau cross compiler
 */
#define CPP	"M32CPP"
/*
 *	Directory containing include ("header") files for users' use
 */
#define INCLDIR	"-IM32INCDIR"
#define COMP	"M32LIBDIR/comp"
#define C0	"M32LIBDIR/front"
#define C1	"M32LIBDIR/back"
#define OPTIM	"M32LIBDIR/optim"
/*
 *	m32/m32mau cross assembler
 */
#define AS	"M32BINDIR/SGSas"
#define AS1	"M32LIBDIR/SGSas1"	/* assembler pass 1 */
#define AS2	"M32LIBDIR/SGSas2"	/* assembler pass 2 */
#define M4	"M32BINDIR/SGSm4"	/* macro preprocessor */
#define CM4DEFS	"M32LIBDIR/cm4defs"	/* C interface macros */
#define CM4TVDEFS "M32LIBDIR/cm4tvdefs"	/* C macros with 'tv' call */
/*
 *	m32/m32mau link editor
 */
#define LD	"M32BINDIR/SGSld"
#define LD2	"M32LIBDIR/SGSld2"	/* link editor pass 2 */
