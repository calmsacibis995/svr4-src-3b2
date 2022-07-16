//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libexp/m32/Call.C	1.5"

#include "Expr.h"
#include "Expr_priv.h"
#include "SDBinfo.h"
#include "Type.h"
#include "Tag.h"
#include "Reg.h"
#include "Process.h"
#include "utility.h"
#include "Interface.h"
#include "format.h"
#include "Reg.h"
#include <stdlib.h>
#include <string.h>

overload dump;

extern void dump( Symbol sym, char *label = 0 );

extern void dump( TYPE type, char *label = 0 );

static char _call = 0x2c;
static char _abs  = 0x7f;
#define CALL	&_call
#define ABS	&_abs

#define SAVESIZE	1   +  (1+4)  +  (1+4)
		   /* CALL    ABS sp    ABS addr */

int
Expr::do_call( char *fmt, Process *proc, Iaddr pc, Frame *frm )
{
//DBG	if ( debugflag & DBG_CALL )
//DBG		printe("do_call( '%s' ) fcn = '%s', this = %#x\n",
//DBG				fmt, etree->call.fcn_name, this);

	process = proc ? proc : current_process;
	frame = process ? process->topframe() : 0;

	if ( !process || !frame || process->is_proto() ||
			process->state == es_corefile ) {
		printe("not a live process\n");
		return 0;
	}

// Find address of _start so we can patch in CALL instruction.
// Do this before modifying any registers, so we can avoid having to
// undo our modifications.

	Iaddr start = 0;

	Symbol sym = process->find_global("_start");
	if ( sym.isnull() ) {
		printe("can't locate _start()!\n");
		return 0;
	} else {
		start = sym.pc( an_lopc );
	}


// Save registers we are about to modify.

	Execstate oldstate = process->state;

	Iaddr oldsp = process->getreg( REG_SP );
	Iaddr oldfp = process->getreg( REG_FP );
	Iaddr oldap = process->getreg( REG_AP );
	Iaddr oldpc = process->getreg( REG_PC );
	Iaddr oldr0 = process->getreg( REG_R0 );
	Iaddr oldr1 = process->getreg( REG_R1 );
	Iaddr oldr2 = process->getreg( REG_R2 );

	Iaddr cursp = oldsp;

// Evaluate all the args and save the Rvalues, pushing temporary
// space for strings, arrays, and returned structs.

	Vector rvec;

	int argno = 0;

	rvec.clear();

	SDBinfo *arg = etree->next_arg;
	while ( arg ) {
		argno++;
//DBG		if ( debugflag & DBG_CALL ) {
//DBG			printe("arg %d = ", argno);
//DBG			dump_SDBinfo( arg );
//DBG		}
		Expr e(arg);
		if ( !e.eval(EV_RHS, process, pc, frm) ) {
			printe("can't evaluate argument %d\n",
					argno);
			return 0;
		}
		widen( e.rval, &e.type );
		int need = need_stack(e.rval, e.etree->kind);
		if ( need ) {
			int size = e.rval->size();	// not necessarily == need
			if ( process->write(cursp, e.rval->raw(), size) != size ) {
				printe("can't write stack for temp, addr %#x, size %d\n",					cursp, size);
				return 0;
			}
			// change rval from X to (u_long)&X (address is now on stack)
			delete e.rval;
			e.rval = new Rvalue( cursp );
			cursp += need;
		}
		rvec.add(&e.rval, 4);
//DBG		if ( debugflag & DBG_CALL )
//DBG			e.rval->print(sf("arg %d",argno), 0, "/");
		e.rval = 0;		// so ~Expr won't delete it;
		arg = arg->next_arg;
	}

// Now resolve the function name (no wildcards).

	Symbol func = function_symbol( etree->call.fcn_name, process, pc );
	if ( func.isnull() ) {
		printe("%s not found\n", etree->call.fcn_name );
		return 0;
	}

	Iaddr funcaddr = func.pc( an_lopc );

// Save space on the stack for return value if func returns struct or union.

	TYPE ret_type = return_type( &func, fmt );
	Symbol ut;
	if ( user_type(&ret_type, &ut) ) {
		if ( ut.tag() == t_structuretype || ut.tag() == t_uniontype ) {
			writereg( process, REG_R2, cursp );	// hidden arg
			cursp += ret_type.size();
			cursp = round(cursp, 4);	// should be no-op
		}
	}

// "push" args

	int ret;
	Iaddr argptr = cursp;
	Rvalue **rvalp = (Rvalue **)rvec.ptr();
	Rvalue *rval;
	for ( int narg = argno ; narg ; --narg,++rvalp ) {
		rval = *rvalp;
		ret = process->write( cursp, rval->raw(), (int)rval->size() );
		if ( ret != rval->size() ) {
			printe("can't write arg %d to stack, addr %#x, size %d, ret = %d\n",
				argno - narg + 1, cursp, rval->size(), ret);
		}
		cursp += ret;
		cursp = round(cursp, 4);
	}

// patch _start with CALL instruction

	char savebuf[ SAVESIZE ];

	if ( process->read( start, SAVESIZE, savebuf ) != SAVESIZE ) {
		printe("couldn't read %d bytes at _start (%#x)\n",
				SAVESIZE, start);
		return 0;
	}

	Vector instr;

	instr.clear().add( CALL, 1 );
	instr.add( ABS, 1 ).add( rev(argptr), 4 );
	instr.add( ABS, 1 ).add( rev(funcaddr), 4 );

	if ( process->write( start, instr.ptr(), SAVESIZE ) != SAVESIZE ) {
		printe("couldn't write %d bytes to _start (%#x)\n",
				SAVESIZE, start);
		return 0;
	}

// adjust registers

	writereg( process, REG_SP, cursp );
	writereg( process, REG_PC, start );

// call the function

	runnit( process, start+SAVESIZE, oldstate );

// if requested, fetch and print the result

	if ( fmt ) {
		Rvalue *retval = get_ret ( process, &ret_type );
		if ( retval ) {
			// transmogrify this Expr into the return value
			if ( own_tree ) {
				delete etree;
			}
			etree = SDBinfo_name( 1, 0, func.name() );
			own_tree = 1;
			current_sym = func;
			lab = sf("%s()",func.name());
			returnval = retval;
			type = ret_type;
			mode = sPRINT_RESULT;
			frame = process->topframe();
			decomp = new Decomp( etree, &type, 1);
			base = new Place;
			base->kind = pAddress;
			base->addr = (Iaddr)retval->raw();
			//     ^ this address is in SDB's memory, not in
			//     the subject process! see Expr::get_rvalue()
			while ( evaluate( fmt ) )
				;
		}
	}

// restore saved code and registers

	ret = process->write( start, savebuf, SAVESIZE );
	if ( ret != SAVESIZE )
		printe("couldn't restore code at _start %#x, ret = %d\n", start, ret);

	writereg( process, REG_SP, oldsp );
	writereg( process, REG_AP, oldap );
	writereg( process, REG_FP, oldfp );
	writereg( process, REG_PC, oldpc );
	writereg( process, REG_R0, oldr0 );
	writereg( process, REG_R1, oldr1 );
	writereg( process, REG_R2, oldr2 );

	process->state = oldstate;

	return 1;
}
