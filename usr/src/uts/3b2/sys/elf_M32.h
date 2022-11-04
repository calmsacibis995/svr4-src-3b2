/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_ELF_M32_H
#define _SYS_ELF_M32_H

#ident	"@(#)head.sys:sys/elf_M32.h	1.2"

#define EF_M32_MAU		1	/* e_flags */

#define R_M32_NONE		0	/* relocation type */
#define R_M32_32		1
#define R_M32_32_S		2
#define R_M32_PC32_S		3
#define R_M32_GOT32_S		4
#define R_M32_PLT32_S		5
#define R_M32_COPY		6
#define R_M32_GLOB_DAT		7
#define R_M32_JMP_SLOT		8
#define R_M32_RELATIVE		9
#define R_M32_RELATIVE_S	10
#define R_M32_NUM		11	/* must be >last */

#endif
