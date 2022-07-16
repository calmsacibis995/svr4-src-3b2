//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libexecon/m32/Frame.C	1.10"

// Frame.c -- stack frames and register access, 3B2 version

#include "Reg.h"
#include "Frame.h"
#include "RegAccess.h"
#include "Process.h"
#include "Interface.h"
#include "Symtab.h"
#include "Attribute.h"
#include "oslevel.h"

static int frame_debug = 0;
#define DPR	if ( frame_debug ) printe

struct frameid {
	Iaddr ap;
	Iaddr fp;
};

struct framedata {
	Process	*proc;
	int	 level;		// 0 is top frame;
	unsigned long epoch;	
	Iaddr	 accv[NGREG];	// "access vector"
	Iaddr	 nosavesp;
	int	 nargwds;
	framedata();
};

FrameId::FrameId(Frame *frame)
{
	id = 0;
	if ( frame ) {
		id = new frameid;
		id->ap = frame->getreg( REG_AP );
//		id->ap = ::get_ap( frame->data->proc->key );
		id->fp = frame->getreg( REG_FP );
//		id->fp = frame->data->proc->pc;		// ????
	}
}

FrameId::~FrameId()
{
	delete id;
}

FrameId &
FrameId::operator=( FrameId & other )
{
	if ( other.id == 0 && id == 0 )
		return *this;
	else if ( other.id == 0 )
	{
		delete id;
		id = 0;
		return *this;
	}
	else if ( id == 0 ) id = new frameid;
	*id = *other.id;
	return *this;
}

void
FrameId::print( char * s )
{
	if (s ) printf(s);
	if ( id == 0 )
		printf(" is null.\n");
	else
		printf(" ap is %#x, fp is %#x\n",id->ap,id->fp);
}

int
FrameId::operator==( FrameId& other )
{
	if ( (id == 0) && ( other.id == 0 ) )
		return 1;
	else if ( id == 0 )
		return 0;
	else if ( other.id == 0 )
		return 0;
	else if ( id->ap != other.id->ap )
		return 0;
	else if ( id->fp != other.id->fp )
		return 0;
	else
		return 1;

}

int
FrameId::operator!=( FrameId& other )
{
	return ! (*this == other);
}

framedata::framedata()
{
	proc = 0;
	level = 0;
	epoch = 0;
	for ( register int i = 0; i < NGREG ; i++ )
		accv[i] = 0;
	nosavesp = 0;
	nargwds = -1;
}

Frame::Frame( Process *proc ) : (1)
{
	data = new framedata;
	data->proc = proc;
	data->epoch = proc->epoch;	// data->epoch never changes
//	if ( proc->is_proto() )
//		data->nosavesp = 0;
//	else
//		data->nosavesp = getreg(REG_SP);
	DPR("new topframe() == %#x\n", this);
}

Frame::Frame( Frame *prev ) : (0)
{
	data = new framedata;
	prev->append( this );
	*data = *(prev->data);	// copy framedata struct
	data->level++;		// bump level
	data->nargwds = -1;	// invalidate arg words
	DPR("new next frame(%#x) == %#x\n", prev, this);
}

Frame::~Frame()
{
	DPR("%#x.~Frame()\n", this);
	unlink();
	delete data;
}

int
Frame::valid()
{
	return this && data && data->epoch == data->proc->epoch;
}

int
Frame::isleaf()
{
	return 0;		// for now
}

FrameId
Frame::id()
{
	FrameId *fmid = new FrameId(this);
	return *fmid;
}

#define INSTACK(x)	( (((x) & 0xffff0000) >= (unsigned)0xc0020000) && \
			  (((x) & 0xffff0000) <  (unsigned)0xc1000000) )
#define INTEXT(x)	  (((x) & 0x80000000) == 0x80000000)

Frame *
Frame::caller()
{
	DPR("%#x.caller()\n", this);
	Frame *p = (Frame *) next();
	if ( !p ) {	// try to construct it
//		DPR("no next, building it\n");
		Iaddr pc, ap, fp;
		Iaddr prevpc1, prevpc2;
		Itype itype;
		pc = getreg(REG_PC);
//		DPR("%#x.caller() pc == %#x\n", this, pc);
		if ( pc != 0 && !INSTACK(pc) ) {
			ap = getreg(REG_AP);
//			DPR("%#x.caller() ap == %#x\n", this, ap);
			fp = getreg(REG_FP);
//			DPR("%#x.caller() fp == %#x\n", this, fp);
			if ( fp ) {
				data->proc->read(fp-36, Saddr, itype);
				prevpc1 = itype.iaddr;
				data->proc->read(fp-40, Saddr, itype);
				prevpc2 = itype.iaddr;
			} else {
				prevpc1 = prevpc2 = 0;
			}
//			DPR("%#x.caller() prevpc1 = %#x, prevpc2 = %#x\n", this, prevpc1, prevpc2);
			if ( !INTEXT(prevpc1) && !INTEXT(prevpc2) ) {
				DPR("%#x.caller() no prev frame, returns 0\n", this);
				return 0;	// no previous frame
			}
			p = new Frame(this);
			if ( fp <= ap ) {	// call without save
				if ( !data->nosavesp )
					data->nosavesp = getreg(REG_SP);
				p->data->accv[REG_PC] = data->nosavesp - 8;
				p->data->accv[REG_AP] = data->nosavesp - 4;
				p->data->nosavesp     = ap;
				DPR("%#x.caller() no save\n", this);
			} else if ( !INTEXT(prevpc1) ) {	// savr
				DPR("%#x.caller() savr\n", this);
				p->data->accv[REG_AP] = fp + 12;
				p->data->accv[2]      = fp + 8;
				p->data->accv[1]      = fp + 4;
				p->data->accv[0]      = fp;
				p->data->accv[8]      = fp - 4;
				p->data->accv[7]      = fp - 8;
				p->data->accv[6]      = fp - 12;
				p->data->accv[5]      = fp - 16;
				p->data->accv[4]      = fp - 20;
				p->data->accv[3]      = fp - 24;
				p->data->accv[REG_FP] = fp - 28;
				p->data->accv[REG_PC] = fp - 40;
				p->data->nosavesp     = fp - 40;
			} else {	// normal call/save sequence
				DPR("%#x.caller() call + save\n", this);
				Iaddr r, fn;
				int numsaved = 0;
				Symbol entry;
//				Symtab *symtab = data->proc->find_symtab( pc );
//				entry = symtab->find_entry( pc );
				entry = data->proc->find_entry( pc );
				if ( entry.isnull() ) {
					printe("can't find entry for PC = 0x%x\n",
							pc );
					delete p;
					return 0;
				} else {
					Attribute *a = entry.attribute(an_lopc);
					if ( a ) {
						fn = entry.pc(an_lopc); //addr of save
						DPR("entry lopc = %#x\n", fn);
						if ( pc > fn ) {
							// read second byte of
							// save instruction
							data->proc->read(fn+1,
								Suchar, itype);
							numsaved = 9 -
							    (itype.iuchar & 0xf);
						}
					} 
				}
				DPR("numsave = %d\n", numsaved);
				r = fp - 24;
				switch ( numsaved ) {
				case 6: p->data->accv[3] = r; r += 4;
				case 5: p->data->accv[4] = r; r += 4;
				case 4: p->data->accv[5] = r; r += 4;
				case 3: p->data->accv[6] = r; r += 4;
				case 2: p->data->accv[7] = r; r += 4;
				case 1: p->data->accv[8] = r; r += 4;
				}
				p->data->accv[REG_FP] = fp - 28;
				p->data->accv[REG_AP] = fp - 32;
				p->data->accv[REG_PC] = fp - 36;
				p->data->nosavesp     = ap;
			}
		}
	}
//	DPR("%#x.caller() accv[PC] = %#x, [AP] = %#x, [FP] = %#x\n", this,
//		p->data->accv[REG_PC], p->data->accv[REG_AP],
//		p->data->accv[REG_FP]);
	return p;
}

Frame *
Frame::callee()
{
	return is_head() ? (Frame*) 0 : (Frame*) Prev();
}

int
Frame::readreg( RegRef which, Stype what, Itype& dest )
{
	if ( data->level == 0 ) {
		return !data->proc->readreg( which, what, dest );
	} else {			// on stack, possibly
//		DPR("%#x.readreg(%d, %d, %#x) level = %d\n", this, which, what,
//			&dest, data->level);
		if ( which <= REG_PC ) {	// IU regs
			if ( data->accv[which] ) {
//				DPR("readreg() does read(%#x)\n", data->accv[which]);
				return !data->proc->read(data->accv[which],
								what, dest);
			} else {
//				DPR("readreg() gets real register\n");
				return !data->proc->readreg( which, what, dest );
			}
		} else {			// FP regs (how?)
			printe("can't readreg() FP regs from stack\n");
			return -1;
		}
	}
#if 0
	long x, y, z;
	int yvalid = 0, zvalid = 0;

	if ( !valid() ) {
		printe("frame is not valid\n");
		return -1;			// frame is no longer valid
	}

	if ( data->level == 0 ) {
//		DPR("%#x.readreg(%d, %d, %#x) level = 0\n", this, which, what, &dest);
		if ( which <= REG_PC ) {	// IU reg
			// get bytes from register;
			x = data->proc->haltstatus.pr_reg[which];
		} else {			// FP reg
			register fpregset_t *fp = &data->proc->fpregs;

			switch (which) {

			case REG_ASR:
				x = fp->f_asr;
				break;

			case REG_DR:
				x = fp->f_dr[0];
				y = fp->f_dr[1];
				z = fp->f_dr[2];
				yvalid = zvalid = 1;
				break;

			case REG_X0:
				z = fp->f_fpregs[0][2];	zvalid = 1;
			case REG_D0:
				y = fp->f_fpregs[0][1];	yvalid = 1;
			case REG_F0:
				x = fp->f_fpregs[0][0];
				break;

			case REG_X1:
				z = fp->f_fpregs[1][2];	zvalid = 1;
			case REG_D1:
				y = fp->f_fpregs[1][1];	yvalid = 1;
			case REG_F1:
				x = fp->f_fpregs[1][0];
				break;

			case REG_X2:
				z = fp->f_fpregs[2][2];	zvalid = 1;
			case REG_D2:
				y = fp->f_fpregs[2][1];	yvalid = 1;
			case REG_F2:
				x = fp->f_fpregs[2][0];
				break;

			case REG_X3:
				z = fp->f_fpregs[3][2];	zvalid = 1;
			case REG_D3:
				y = fp->f_fpregs[3][1];	yvalid = 1;
			case REG_F3:
				x = fp->f_fpregs[3][0];
				break;

			default:
				printe("Frame::readreg(): bad RegRef (%d)\n",
					which);
			}
		}
	} else {			// on stack, possibly
//		DPR("%#x.readreg(%d, %d, %#x) level = %d\n", this, which, what,
//			&dest, data->level);
		if ( which <= REG_PC ) {	// IU regs
			if ( data->accv[which] ) {
//				DPR("readreg() does read(%#x)\n", data->accv[which]);
				data->proc->read(data->accv[which], Suint4, dest);
				x = dest.iuint4;
			} else {
//				DPR("readreg() gets real register\n");
				x = data->proc->haltstatus.pr_reg[which];
			}
		} else {			// FP regs (how?)
			printe("can't readreg() FP regs from stack\n");
			return -1;
		}
	}

	// convert to proper type

	switch (what) {
	case SINVALID:
		printe("Frame:readreg(): what == SINVALID\n");
		abort();
		break;
	case Schar:
	case Suchar:
		dest.ichar = x;
		break;
	case Sint1:
	case Suint1:
		dest.iint1 = x;
		break;
	case Sint2:
	case Suint2:
		dest.iint2 = x;
		break;
	case Sint4:
	case Suint4:
	case Ssfloat:
	case Saddr:
	case Sbase:
	case Soffset:
		dest.iint4 = x;
		break;
	case Sdfloat:
		if ( yvalid ) {
			dest.iint4 = x;
			dest.rawwords[1] = y;
		} else {
			printe("Frame::readreg(): bad size for double\n");
			return -1;
		}
		break;
	case Sxfloat:
		if ( yvalid && zvalid ) {
			dest.iint4 = x;
			dest.rawwords[1] = y;
			dest.rawwords[2] = z;
		} else {
			printe("Frame::readreg(): bad size for extended\n");
			return -1;
		}
		break;
	}
	return 0;
#endif
}

int
Frame::writereg( RegRef which, Stype what, Itype& dest )
{
	return !data->proc->writereg(which, what, dest);
}

Iaddr
Frame::getreg( RegRef which )
{
	Itype itype;
	if ( readreg( which, Saddr, itype ) ) {
		if(!data->proc->is_proto())
			printe("can't getreg(%d)\n", which);
		return 0;
	}
	return itype.iaddr;
}

Iint4
Frame::argword(int n)
{
	Itype itype;
	Iaddr ap = getreg( REG_AP );
	data->proc->read( ap + (4*n), Sint4, itype );
	return itype.iint4;
}

int
Frame::nargwds()
{
	if ( data->nargwds < 0 ) {
		Iaddr ap, fp, prevpc;
		Itype itype;
		ap = getreg(REG_AP);
		fp = getreg(REG_FP);
		if( fp == 0 )
			data->nargwds = 0;
		else {
			if ( !data->nosavesp )
				data->nosavesp = getreg(REG_SP);
			data->proc->read(fp-36, Saddr, itype);
			prevpc = itype.iaddr;
			if ( fp <= ap )
				data->nargwds = data->nosavesp - 8 - ap;
			else if ( INTEXT(prevpc) )
				data->nargwds = fp - 36 - ap;
			else
				data->nargwds = 0;
			data->nargwds /= 4;	// convert from bytes to words
		}
	}
	DPR("%#x.nargwds() level = %d, returns %d\n", this, 
		data->level, data->nargwds);
	return data->nargwds;
}

Iaddr
Frame::pc_value()
{
	if ( data->accv[REG_PC] == 0 )
	{
		return getreg( REG_PC );
	}
	else
	{
		return getreg( REG_PC ) - 1;
	}
}
