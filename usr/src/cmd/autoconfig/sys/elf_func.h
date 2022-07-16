/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:sys/elf_func.h	1.2"

#define DEBUG_NAME ".debug"


#define dwarf2(x)       (((short)(x)[0]<<8)+(x)[1])
#define dwarf4(x)       (((((((int)(x)[0]<<8)+(x)[1])<<8)+(x)[2])<<8)+(x)[3])

extern void elf_debug_info();
