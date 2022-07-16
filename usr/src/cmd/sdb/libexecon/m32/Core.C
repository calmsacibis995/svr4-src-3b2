//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libexecon/m32/Core.C	1.6"
// Core.C -- provides access to core files,
// both old and new (ELF) format
//
// If old format, fake new format data as best we can.

#include "Core.h"
#include <libelf.h>
#include <memory.h>
#include <osfcn.h>
#include "Interface.h"

#define MAXSIG	32

// following are for COFF core files ONLY
#include "SectHdr.h"
#include <sys/fs/s5dir.h>
#include <sys/user.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/reg.h>

struct CoreData {
	int		 is_coff;	// have to fake it
	char		*notes;		// copy of NOTE segment
	Elf32_Phdr	*phdr;		// copy of program header array
	int		 numphdr;	// how many phdrs?
	long		 statseek;	// seek addr of status struct
	long		 fpseek;	// seek addr of fpregs
	prstatus_t	*status;	// points into notes
	fpregset_t	*fpregs;	// ditto, or 0

	CoreData()	{ memset( (char *)this, 0, sizeof(CoreData) ); }
};

static void fake_ELF_core( CoreData *, int corefd );

Core::Core( int corefd )
{
	data = new CoreData;
	long magic = 0;
	if ( corefd < 0 )
		return;			// no core file
	::lseek(corefd, 0, 0);
	::read(corefd, (char*)&magic, sizeof magic);
	::lseek(corefd, 0, 0);
	if ( magic == 0x7f454c46 ) {	// DEL E L F == ELF file
		Elf_Cmd		 cmd;
		Elf 		*elf;
		Elf32_Ehdr 	*ehdrp;
		Elf32_Phdr	*phdrp;
		int		 phnum;
	
		elf_version( EV_CURRENT );
	
		cmd = ELF_C_READ;
		if ( ( elf = elf_begin( corefd, cmd, 0 )) == 0 ) {
			printe("elf_begin() failed!\n");
			return;
		} else if ( ( ehdrp = elf32_getehdr(elf)) == 0 ) {
			printe("elf32_getehdr() failed!\n");
			return;
		}
	
		data->numphdr = phnum = ehdrp->e_phnum;
	
		if ( !phnum ) {
			printe("no program headers!\n");
			printe("phnum = %d\n", phnum );
			return;
		}
	
		data->phdr = phdrp = elf32_getphdr(elf);
		while ( phnum-- > 0 ) {
			if ( phdrp->p_type == PT_NOTE ) {
				int size = phdrp->p_filesz;
				data->notes = new char[ size ];
				::lseek(corefd, phdrp->p_offset, 0);
				if ( ::read(corefd, data->notes, size) !=
								    size ) {
					printe("can't read NOTES segment!\n");
					return;
				}
				int namesz, descsz, type;
				char *p = data->notes;
				while ( size > 0 ) {
					namesz = *(int *)p; p += sizeof(int);
					descsz = *(int *)p; p += sizeof(int);
					type   = *(int *)p; p += sizeof(int);
					size -= 3 * sizeof(int) +
							namesz + descsz;
					p += namesz;
					switch( type ) {
					default:
						printe(
						"unknown type %d, size = %d\n",
							type, descsz);
						break;
					case 1:
						data->status = (prstatus_t *)p;
						data->statseek =
							p - data->notes +
							phdrp->p_offset;
						break;
					case 2:
						data->fpregs = (fpregset_t *)p;
						data->fpseek =
							p - data->notes +
							phdrp->p_offset;
						break;
					case 3:			// psinfo
						break;
					}
					p += descsz;
				}
				if ( !data->status )
					printe("no prstatus struct!\n");
			}
			phdrp++;
		}
	} else {			// old style
		fake_ELF_core( data, corefd );
	}
}

Core::~Core()
{
//	if ( data->phdr )
//		elf_unmap(0)( data->phdr, sizeof(Elf32_Phdr) );
	delete data->notes;
	delete data;
}

int
Core::numsegments()
{
	return data->numphdr;
}

Elf32_Phdr *
Core::segment( int which )
{
	if ( data->phdr && which >= 0 && which < data->numphdr )
		return data->phdr + which;
	else
		return 0;
}

prstatus_t *
Core::getstatus()
{
	return data->status;
}

fpregset_t *
Core::getfpregs()
{
	return data->fpregs;
}

long
Core::statusbase()
{
	return data->statseek;
}

long
Core::fpregbase()
{
	return data->fpseek;
}

void
Core::update_reg( RegRef ref, register long *word, int size )
{
	RegAttrs *a = regattrs( ref );

	if ( a->ref == REG_UNK )
		return;			// message?

	if ( size != a->size ) {
		printe("bad size %d for reg %s in Core::update_reg()\n",
			size, a->name );
		return;
	}
	if ( a->flags & FPREG ) {
		if ( ref == REG_DR ) {
			data->fpregs->f_dr[0] = *word++;
			data->fpregs->f_dr[1] = *word++;
			data->fpregs->f_dr[2] = *word;
		} else {
			int which;
			switch ( ref ) {
			case REG_F0:	case REG_D0:	case REG_X0:
				which = 0;
				break;
			case REG_F1:	case REG_D1:	case REG_X1:
				which = 1;
				break;
			case REG_F2:	case REG_D2:	case REG_X2:
				which = 2;
				break;
			}
			switch ( size ) {
			case 12: data->fpregs->f_fpregs[which][2] = word[2];
			case 8:  data->fpregs->f_fpregs[which][1] = word[1];
			case 4:  data->fpregs->f_fpregs[which][0] = word[0];
			}
		}
	} else {
		data->status->pr_reg[ ref ] = word[0];
	}
}

static void
fake_ELF_core( register CoreData *d, int corefd )
{
	user_t u;

	if ( ::read(corefd, (char *)&u, sizeof(user_t) ) != sizeof(user_t) ) {
		printe("can't get core\n");
		return;
	}

	d->numphdr = 3;		// TEXT, DATA, STACK, UBLOCK

	register Elf32_Phdr *p;

	p = d->phdr = (Elf32_Phdr *) new char [ 3 * sizeof(Elf32_Phdr) ];
	p->p_type	= PT_LOAD;	// DATA segment
	p->p_offset	= ctob(USIZE);
	p->p_vaddr	= (Elf32_Addr) u.u_exdata.ux_datorg;
	p->p_paddr	= 0;
	p->p_filesz	= ctob(u.u_dsize);
	p->p_memsz	= ctob(u.u_dsize);
	p->p_flags	= (PF_R|PF_W);
	p->p_align	= 0;

	++p;
	p->p_type	= PT_LOAD;	// STACK segment
	p->p_offset	= ctob(USIZE) + ctob(u.u_dsize);
	p->p_vaddr	= 0xc0020000;
	p->p_paddr	= 0;
	p->p_filesz	= ctob(u.u_ssize);
	p->p_memsz	= ctob(u.u_ssize);
	p->p_flags	= (PF_R|PF_W);
	p->p_align	= 0;

	++p;
	p->p_type	= PT_LOAD;	// UBLOCK
	p->p_offset	= 0;
	p->p_vaddr	= 0xc0000000;
	p->p_paddr	= 0;
	p->p_filesz	= ctob(USIZE);
	p->p_memsz	= ctob(USIZE);
	p->p_flags	= (PF_R);
	p->p_align	= 0;

	register prstatus_t *pr;

	pr = d->status = (prstatus_t *) new char [ sizeof(prstatus_t) ];

	memset( (char *)pr, sizeof(prstatus_t), 0 );

	int *ar0 = (int *)((long)&u + (long)u.u_ar0 - 0xc0000000);

	pr->pr_reg[R_R0] = ar0[R0];
	pr->pr_reg[R_R1] = ar0[R1];
	pr->pr_reg[R_R2] = ar0[R2];
	pr->pr_reg[R_R3] = ar0[R3];
	pr->pr_reg[R_R4] = ar0[R4];
	pr->pr_reg[R_R5] = ar0[R5];
	pr->pr_reg[R_R6] = ar0[R6];
	pr->pr_reg[R_R7] = ar0[R7];
	pr->pr_reg[R_R8] = ar0[R8];
	pr->pr_reg[R_FP] = ar0[FP];
	pr->pr_reg[R_AP] = ar0[AP];
	pr->pr_reg[R_PS] = ar0[PS];
	pr->pr_reg[R_SP] = ar0[SP];
	pr->pr_reg[R_PC] = ar0[PC];

	register fpregset *fp;

	fp = d->fpregs = (fpregset *) new char [ sizeof(fpregset) ];
	d->fpseek = (long)&((user *)0)->u_mau;
	memcpy( fp, &u.u_mau, sizeof(fpregset) );
}
