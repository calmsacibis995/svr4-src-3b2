//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/common/pc_curr.C	1.4"
#include	"Interface.h"
#include	"Process.h"
#include	"Source.h"
#include	"Symtab.h"
#include	"Tag.h"
#include	"utility.h"

int
pc_current_src( Process * process )
{
	Symbol		symbol;
	Iaddr		pc;
	Symtab *	symtab;
	Source		source;
	long		line;

	if ( process == 0 )
	{
		printe("internal error: ");
		printe("null pointer to pc_current_src()\n");
		return 0;
	}
	else if ( process->is_proto() )
	{
		printe("no process\n");
		return 0;
	}
	pc = process->pc_value();
	if ( (symtab = process->find_symtab( pc )) == 0 )
	{
		return 0;
	}
	else if ( symtab->find_source( pc, symbol ) == 0 )
	{
		printe("no source file\n");
		return 0;
	}
	else if ( symbol.source( source ) == 0 )
	{
		return 0;
	}
	else
	{
		source.pc_to_stmt( pc, line );
		return process->set_current_stmt( symbol.name(), line );
	}
}
