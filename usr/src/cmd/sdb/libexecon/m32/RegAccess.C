//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libexecon/m32/RegAccess.C	1.14"
#define lint	1
#include	"RegAccess.h"
#include	"Interface.h"
#include	"Reg1.h"
#include	"Rvalue.h"
#include	"prioctl.h"
#include	<memory.h>
#include	<osfcn.h>
#include	<sys/reg.h>
#include	<sys/psw.h>
#include	<sys/pcb.h>
#include	<sys/signal.h>
#include	<sys/fs/s5dir.h>
#define MAXSIG	32
#include	<sys/user.h>
#include	<errno.h>
#include	"Core.h"

RegAccess::RegAccess()
{
	key.fd = -1;
	key.pid = -1;
	core = 0;
	corefd = -1;
	fpcurrent = 0;
}

int
RegAccess::setup_core( int cfd, Core *coreptr )
{
	corefd = cfd;
	core = coreptr;
	key.fd = cfd;
	key.pid = -1;
	return 1;
}

extern int errno ;

static long	regsize[] = {			 // indexed by RegRef
				sizeof(long),	 // REG_R0
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),
				sizeof(long),	 // REG_PC
				0,		 // unused
				0,		 // unused
				sizeof(long),	 // REG_ASR
				3 * sizeof(long),// REG_DR
				sizeof(long),	 // REG_F0
				sizeof(long),
				sizeof(long),
				sizeof(long),	 // REG_F3
				2 * sizeof(long),// REG_D0
				2 * sizeof(long),
				2 * sizeof(long),
				2 * sizeof(long),// REG_D3
				3 * sizeof(long),// REG_X0
				3 * sizeof(long),
				3 * sizeof(long),
				3 * sizeof(long),// REG_X3
			};

static int gpmap[16] = {	// indexed by RegRef, IU regs only
	R0,
	R1,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	R8,
	FP,	/* FP   == R9  */
	AP,	/* AP   == R10 */
	PS,	/* PS   == R11 */
	SP,	/* SP   == R12 */
	-4,	/* PCBP == R13 (lies) */
	-3,	/* ISP  == R14 (lies) */
	PC	/* PC   == R15 */
};

#define ar0offset	((long)&(((struct user *)0)->u_ar0))

#define fpbase		((long)&(((struct user *)0)->u_mau))

#define fpmap(x)	((long)&(((struct mau_st *)0)->x))

#if PTRACE

int
RegAccess::setup_live( Key k )
{
	key.pid = k.pid;
	key.fd = -1;
	core = 0;
	::errno = 0;
	gpbase = ::ptrace( 3, key.pid, ar0offset, 0 );
	return ( ::errno == 0 );
}

int
RegAccess::update( prstatus & prstat )
{
	if ( key.pid != -1 )
	{
		::errno = 0;
		gpbase = ::ptrace( 3, key.pid, ar0offset, 0 );
		return ( ::errno == 0 );
	}
	return 0;
}

int
RegAccess::readlive( RegRef regref, long * word )
{
	int	i;
	long	offset;
	long	sz;

	if ( key.pid == -1 )
	{
		return 0;
	}
	switch (regref)
	{
		case REG_ASR:	offset = fpbase + fpmap(asr);		break;

		case REG_DR:	offset = fpbase + fpmap(dr[0]);		break;

		case REG_X0:
		case REG_D0:
		case REG_F0:	offset = fpbase + fpmap(fpregs[0][0]);	break;

		case REG_X1:
		case REG_D1:
		case REG_F1:	offset = fpbase + fpmap(fpregs[1][0]);	break;

		case REG_X2:
		case REG_D2:
		case REG_F2:	offset = fpbase + fpmap(fpregs[2][0]);	break;

		case REG_X3:
		case REG_D3:
		case REG_F3:	offset = fpbase + fpmap(fpregs[3][0]);	break;

		default:
			gpbase = ::ptrace( 3, key.pid, ar0offset, 0 );
			offset = gpbase - 0xc0000000 + gpmap[regref] * sizeof(int);
			break;
	}
	sz = regsize[regref];
	i = 0;
	do {
		::errno = 0;
		word[i] = ::ptrace(3,key.pid,offset,0);
		sz -= sizeof(long);
		offset += sizeof(long);
		++i;
	} while ( sz > 0 );
	return (::errno == 0 );
}

int
RegAccess::writelive( RegRef regref, long * word )
{
	int	i;
	long	offset;
	long	sz;

	if ( key.pid == -1 )
	{
		return 0;
	}
	switch (regref)
	{
		case REG_ASR:	offset = fpbase + fpmap(asr);		break;

		case REG_DR:	offset = fpbase + fpmap(dr[0]);		break;

		case REG_X0:
		case REG_D0:
		case REG_F0:	offset = fpbase + fpmap(fpregs[0][0]);	break;

		case REG_X1:
		case REG_D1:
		case REG_F1:	offset = fpbase + fpmap(fpregs[1][0]);	break;

		case REG_X2:
		case REG_D2:
		case REG_F2:	offset = fpbase + fpmap(fpregs[2][0]);	break;

		case REG_X3:
		case REG_D3:
		case REG_F3:	offset = fpbase + fpmap(fpregs[3][0]);	break;

		default:	offset = gpbase - 0xc0000000 + gpmap[regref] * sizeof(int);	break;
	}
	sz = regsize[regref];
	i = 0;
	do {
		::errno = 0;
		::ptrace(6,key.pid,offset,word[i]);
		sz -= sizeof(long);
		offset += sizeof(long);
		++i;
	} while ( sz > 0 );
	return (::errno == 0 );
}

#else

int
RegAccess::setup_live( Key k )
{
	key.pid = k.pid;
	key.fd = k.fd;
	core = 0;
	return 1;
}

int
RegAccess::update( prstatus & prstat )
{
	if ( key.pid != -1 )
	{
		::memcpy( (char*)gpreg, (char*)prstat.pr_reg, sizeof(gpreg) );
		return 1;
	}
	return 0;
}

int
RegAccess::readlive( RegRef regref, long * word )
{
	if ( regref <= REG_PC )
	{
		word[0] = gpreg[regref];
		return 1;
	}
	else if ( (!fpcurrent) && (::getfpset( key, fpreg ) == 0) )
	{
		return 0;
	}
	else
	{
		fpcurrent = 1;
		switch (regref)
		{
			case REG_ASR:	word[0] = fpreg.f_asr;		break;

			case REG_DR:	word[0] = fpreg.f_dr[0];
					word[1] = fpreg.f_dr[1];
					word[2] = fpreg.f_dr[2];	break;

			case REG_X0:	word[2] = fpreg.f_fpregs[0][2];
			case REG_D0:	word[1] = fpreg.f_fpregs[0][1];
			case REG_F0:	word[0] = fpreg.f_fpregs[0][0];	break;

			case REG_X1:	word[2] = fpreg.f_fpregs[1][2];
			case REG_D1:	word[1] = fpreg.f_fpregs[1][1];
			case REG_F1:	word[0] = fpreg.f_fpregs[1][0];	break;

			case REG_X2:	word[2] = fpreg.f_fpregs[2][2];
			case REG_D2:	word[1] = fpreg.f_fpregs[2][1];
			case REG_F2:	word[0] = fpreg.f_fpregs[2][0];	break;

			case REG_X3:	word[2] = fpreg.f_fpregs[3][2];
			case REG_D3:	word[1] = fpreg.f_fpregs[3][1];
			case REG_F3:	word[0] = fpreg.f_fpregs[3][0];	break;

			default:	return 0;
		}
		return 1;
	}
}

int
RegAccess::writelive( RegRef regref, long * word )
{
	if ( regref <= REG_PC )
	{
		gpreg[regref] = word[0];
		do {
			::errno = 0;
			::ioctl( key.fd, PIOCSREG, &gpreg );
		} while ( ::errno == EINTR );
		return (::errno == 0);
	}
	else
	{
		switch (regref)
		{
			case REG_ASR:	fpreg.f_asr = word[0];		break;

			case REG_DR:	fpreg.f_dr[0] = word[0];
					fpreg.f_dr[1] = word[1];
					fpreg.f_dr[2] = word[2];	break;

			case REG_X0:	fpreg.f_fpregs[0][2] = word[2];
			case REG_D0:	fpreg.f_fpregs[0][1] = word[1];
			case REG_F0:	fpreg.f_fpregs[0][0] = word[0];	break;

			case REG_X1:	fpreg.f_fpregs[1][2] = word[2];
			case REG_D1:	fpreg.f_fpregs[1][1] = word[1];
			case REG_F1:	fpreg.f_fpregs[1][0] = word[0];	break;

			case REG_X2:	fpreg.f_fpregs[2][2] = word[2];
			case REG_D2:	fpreg.f_fpregs[2][1] = word[1];
			case REG_F2:	fpreg.f_fpregs[2][0] = word[0];	break;

			case REG_X3:	fpreg.f_fpregs[3][2] = word[2];
			case REG_D3:	fpreg.f_fpregs[3][1] = word[1];
			case REG_F3:	fpreg.f_fpregs[3][0] = word[0];	break;

			default:	return 0;
		}
		do {
			::errno = 0;
			::ioctl( key.fd, PIOCSFPREG, &fpreg );
		} while ( ::errno == EINTR );
		return (::errno == 0);
	}
}

#endif

Iaddr
RegAccess::top_a_r()
{
	return getreg( REG_AP );
}

Iaddr
RegAccess::getreg( RegRef regref )
{
	Iaddr	addr;
	long	word[3];

	if ( readcore( regref, word ) || readlive( regref, word ) )
	{
		addr = word[0];
	}
	else
	{
		addr = 0;
	}
	return addr;
}

int
RegAccess::readreg( RegRef regref, Stype stype, Itype & itype )
{
	long	word[3];

	if ( readcore( regref, word ) || readlive( regref, word ) )
	{
		switch (stype)
		{
			case SINVALID:	return 0;
			case Schar:	itype.ichar = word[0];		break;
			case Suchar:	itype.iuchar = word[0];		break;
			case Sint1:	itype.iint1 = word[0];		break;
			case Suint1:	itype.iuint1 = word[0];		break;
			case Sint2:	itype.iint2 = word[0];		break;
			case Suint2:	itype.iuint2 = word[0];		break;
			case Sint4:	itype.iint4 = word[0];		break;
			case Suint4:	itype.iuint4 = word[0];		break;
			case Saddr:	itype.iaddr = word[0];		break;
			case Sbase:	itype.ibase = word[0];		break;
			case Soffset:	itype.ioffset = word[0];	break;
			case Sxfloat:	itype.rawwords[2] = word[2];
			case Sdfloat:	itype.rawwords[1] = word[1];
			case Ssfloat:	itype.rawwords[0] = word[0];	break;
			default:	return 0;
		}
		return 1;
	}
	return 0;
}

int
RegAccess::readcore( RegRef regref, register long * word )
{
	register greg_t     *greg;
	register fpregset_t *fp;

	if ( core == 0 )
	{
		return 0;
	}
	greg = core->getstatus()->pr_reg;
	fp = core->getfpregs();
	switch (regref)
	{
		case REG_ASR:	*word = fp->f_asr;		break;

		case REG_DR:	word[0] = fp->f_dr[0];
				word[1] = fp->f_dr[1];
				word[2] = fp->f_dr[2];		break;

		case REG_X0:	word[2] = fp->f_fpregs[0][2];
		case REG_D0:	word[1] = fp->f_fpregs[0][1];
		case REG_F0:	word[0] = fp->f_fpregs[0][0];	break;

		case REG_X1:	word[2] = fp->f_fpregs[1][2];
		case REG_D1:	word[1] = fp->f_fpregs[1][1];
		case REG_F1:	word[0] = fp->f_fpregs[1][0];	break;

		case REG_X2:	word[2] = fp->f_fpregs[2][2];
		case REG_D2:	word[1] = fp->f_fpregs[2][1];
		case REG_F2:	word[0] = fp->f_fpregs[2][0];	break;

		case REG_X3:	word[2] = fp->f_fpregs[3][2];
		case REG_D3:	word[1] = fp->f_fpregs[3][1];
		case REG_F3:	word[0] = fp->f_fpregs[3][0];	break;

		default:	*word   = greg[regref];		break;
	}
	return 1;
}

int
RegAccess::writereg( RegRef regref, Stype stype, Itype & itype )
{
	long	word[3];

	switch (stype)
	{
		case SINVALID:	return 0;
		case Schar:	word[0] = itype.ichar;		break;
		case Suchar:	word[0] = itype.iuchar;		break;
		case Sint1:	word[0] = itype.iint1;		break;
		case Suint1:	word[0] = itype.iuint1;		break;
		case Sint2:	word[0] = itype.iint2;		break;
		case Suint2:	word[0] = itype.iuint2;		break;
		case Sint4:	word[0] = itype.iint4;		break;
		case Suint4:	word[0] = itype.iuint4;		break;
		case Saddr:	word[0] = itype.iaddr;		break;
		case Sbase:	word[0] = itype.ibase;		break;
		case Soffset:	word[0] = itype.ioffset;	break;
		case Sxfloat:	word[2] = itype.rawwords[2];
		case Sdfloat:	word[1] = itype.rawwords[1];
		case Ssfloat:	word[0] = itype.rawwords[0];	break;
		default:	return 0;
	}
	return ( writecore( regref, word ) || writelive( regref, word ) );
}

int
RegAccess::writecore( RegRef regref, long * word )
{
	long	offset;
	long	sz;
	long	fp;
	long	greg;

	if ( core == 0 )
	{
		return 0;
	}
	fp = core->fpregbase();
	greg = (long) &((prstatus_t *)core->statusbase())->pr_reg;
	switch (regref)
	{
		case REG_ASR:	offset = fp + fpmap(asr);		break;

		case REG_DR:	offset = fp + fpmap(dr[0]);		break;

		case REG_X0:
		case REG_D0:
		case REG_F0:	offset = fp + fpmap(fpregs[0][0]);	break;

		case REG_X1:
		case REG_D1:
		case REG_F1:	offset = fp + fpmap(fpregs[1][0]);	break;

		case REG_X2:
		case REG_D2:
		case REG_F2:	offset = fp + fpmap(fpregs[2][0]);	break;

		case REG_X3:
		case REG_D3:
		case REG_F3:	offset = fp + fpmap(fpregs[3][0]);	break;

		default:	offset = greg + regref * sizeof(int);	break;
	}
	sz = regsize[regref];
	if (::put_bytes(key,offset,word,sz) == sz)
	{
		core->update_reg( regref, word, sz );
		return 1;
	}
	else
	{
		return 0;
	}
}

int
RegAccess::display_regs( int num_per_line )
{
	RegAttrs *p;
	RegRef prev = REG_UNK;
	Itype x;
	int	i;

	i = 1;
	for( p = regs;  p->ref != REG_UNK;  p++ ) {
		if ( p->flags & FPREG )
			break;		// do MAU regs separately
		if ( !p->name || !*p->name )
			continue;	// skip unused slots
		readreg( p->ref, Suint4, x );
		if ( i >= num_per_line )
		{
			printx( "%s	%#10x\n", p->name, x.iuint4 );
			i = 1;
		}
		else
		{
			printx( "%s	%#10x\t", p->name, x.iuint4 );
			i++;
		}
		prev = p->ref;
	}
	if ( i != 1 )
		printx("\n");
	readreg( REG_DR, Sxfloat, x );
	Rvalue *rval = new Rvalue( Sxfloat, x );
	rval->print("%dr","x","     ");
	rval->print("","g"," = ");
	delete rval;
	readreg( REG_X0, Sxfloat, x );
	rval = new Rvalue( Sxfloat, x );
	rval->print("%x0","x","     ");
	rval->print("","g"," = ");
	delete rval;
	readreg( REG_X1, Sxfloat, x );
	rval = new Rvalue( Sxfloat, x );
	rval->print("%x1","x","     ");
	rval->print("","g"," = ");
	delete rval;
	readreg( REG_X2, Sxfloat, x );
	rval = new Rvalue( Sxfloat, x );
	rval->print("%x2","x","     ");
	rval->print("","g"," = ");
	delete rval;
	readreg( REG_X3, Sxfloat, x );
	rval = new Rvalue( Sxfloat, x );
	rval->print("%x3","x","     ");
	rval->print("","g"," = ");
	delete rval;
	return 1;
}
