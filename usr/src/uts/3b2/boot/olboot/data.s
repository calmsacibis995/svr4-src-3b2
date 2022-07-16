#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

	.ident	"@(#)boot:boot/olboot/data.s	1.2"
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
	.comm	IOBASE,2048,4
	.comm	IND3,2048,4
	.comm	DATA,2048,4
	.comm	AHDR,2048,4
	.comm	IND2,2048,4
	.comm	IND1,2048,4
	.comm	INODE,2048,4
	.comm	DIR,2048,4
	.comm	p_req,16,4
