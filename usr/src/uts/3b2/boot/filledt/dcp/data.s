#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

	.ident	"@(#)boot:boot/filledt/dcp/data.s	1.1"
	.file	"data.s"
#
#	The following variables were taken out of C source programs.
#	The reason is, some were defined as char arrays.  However,
#	they were not always used as char arrays but as just
#	allocated memory.
#	Since some were char, ELF would not guarantee alignment
#	other than byte.  So, to insure word boundary alignment,
#	we'll define them here, and indicate alignment.
#
	.data
	.comm	Dinode,108,4
	.comm	Fndinode,108,4
	.comm	muinode,108,4
	.comm	Linode,108,4
	.comm	Fso,4,4
	.comm	fstype,4,4
	.comm	p_req,16,4

	.section .buffers,"aw",@nobits
	.globl IOBASE
	.globl IND3
	.globl DATA
	.globl AHDR
	.globl IND2
	.globl IND1
	.globl INODE
	.globl DIR
	.align	4
IOBASE:
	.set	.,.+2048
IND3:
	.set	.,.+2048
DATA:
	.set	.,.+2048
AHDR:
	.set	.,.+2048
IND2:
	.set	.,.+2048
IND1:
	.set	.,.+2048
INODE:
	.set	.,.+2048
DIR:
	.set	.,.+2048
