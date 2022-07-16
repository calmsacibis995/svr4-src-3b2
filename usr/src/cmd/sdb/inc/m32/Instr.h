/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/m32/Instr.h	1.5"
#ifndef	Instr_h
#define	Instr_h

#include	"Itype.h"

class Process;

struct Instrdata;
enum Opndtype;
struct Modedata;

class Instr {
	Process *	process;
	Iaddr		addr;
	char		opcode;
	char		byte[25];
	int		i;
	char		get_byte();
	short		get_short();
	long		get_long();
	int		get_text( Iaddr );	// replaces decode()
	Instrdata *	get_opcode( char * );
	char *		sym_deasm( Iaddr );
	int		parse_opnd( Opndtype );
	int		make_moded_opnd( Modedata &, char, int );
	int		make_opnd( Opndtype, int, int );
	int		make_mnemonic( char *, int );
public:

			Instr( Process * );

	char *		deasm( Iaddr, int = 0 );
    					// the equivalent assembly language
					// strings.

	Iaddr		retaddr( Iaddr );	// return address if call instr

	Iaddr		next_instr( Iaddr );	// address of next instr

	Iaddr		brtbl2fcn( Iaddr );
					// branch table to functiton

	Iaddr		fcn2brtbl( Iaddr, int );
					// function to branch table

	int		is_bkpt( Iaddr );
					// is this a breakpoint instruction?
	Iaddr		adjust_pc();	// adjust pc after bkpt

	Iaddr		fcn_prolog( Iaddr, short );

	Iaddr		jmp_target( Iaddr );	// target addr if JMP
};

#endif
