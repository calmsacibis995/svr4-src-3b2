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
#define LIBPATH	"/usr/ccs/lib:/usr/lib"
#define YLDIR 1
#define YUDIR 2
/*	Directory containing library files and executables
 *	not accessed directly by users ("lib") files
 */
#define LIBDIR	"/usr/ccs/lib"

/*
 *	Directory containing executable ("bin") files
 */
#define BINDIR	"/usr/ccs/bin"

/*	Absolute pathnames for dynamic shared library targets
 */
#define	LDSO_NAME "/usr/lib/ld.so.1"
#define	LIBCSO_NAME "/usr/lib/libc.so.1"

/*	Directory containing minimal abi library
 */
#define ABILIBDIR  "/usr/ccs/lib/minabi"
/*
 *	Directory containing include ("header") files for building tools
 */

#define INCDIR	"/tmp"

/*
 *	Directory for "temp"  files
 */
#define TMPDIR	"/var/tmp"

/*
 *	Default name of output object file
 */
#define A_OUT	"a.out"

/*
 *	The following pathnames will be used by the "cc" command
 *
 *	m32/m32mau cross compiler
 */
#define CPP	"/cpp"
/*
 *	Directory containing include ("header") files for users' use
 */
#define INCLDIR	"-I/tmp"
#define COMP	"/usr/ccs/lib/comp"
#define C0	"/usr/ccs/lib/front"
#define C1	"/usr/ccs/lib/back"
#define OPTIM	"/usr/ccs/lib/optim"
/*
 *	m32/m32mau cross assembler
 */
#define AS	"/usr/ccs/bin/as"
#define AS1	"/usr/ccs/lib/as1"	/* assembler pass 1 */
#define AS2	"/usr/ccs/lib/as2"	/* assembler pass 2 */
#define M4	"/usr/ccs/bin/m4"	/* macro preprocessor */
#define CM4DEFS	"/usr/ccs/lib/cm4defs"	/* C interface macros */
#define CM4TVDEFS "/usr/ccs/lib/cm4tvdefs"	/* C macros with 'tv' call */
/*
 *	m32/m32mau link editor
 */
#define LD	"/usr/ccs/bin/ld"
#define LD2	"/usr/ccs/lib/ld2"	/* link editor pass 2 */
