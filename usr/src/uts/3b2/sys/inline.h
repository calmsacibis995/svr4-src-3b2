/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_INLINE_H
#define _SYS_INLINE_H

#ident	"@(#)head.sys:sys/inline.h	11.9"

#if !defined(CXREF)  &&  !defined(lint)

asm	char *
strend(cp)
{
%	mem	cp;

	MOVW	cp,%r0
	STREND
}

asm	int
strlen(cp)
{
%	mem	cp;

	MOVW	cp,%r0
	STREND
	SUBW2	cp,%r0
}

asm	char *
strcpy(dp, sp)
{
%	mem	dp, sp;

	MOVW	sp,%r0
	MOVW	dp,%r1
	STRCPY
}

asm	char *
strcat(dp, sp)
{
%	mem	dp, sp;

	MOVW	dp,%r0
	STREND
	MOVW	%r0,%r1
	MOVW	sp,%r0
	STRCPY
}

asm	int
struct_zero(addr, len)
{
%	mem	addr, len;	lab loop;
	MOVW	addr, %r1
	LRSW3	&0x2,len,%r0
loop:
	CLRW	0(%r1)
	MOVAW	4(%r1),%r1
	DECW	%r0
	BNEB	loop
}

asm	int
spl1()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&8,%psw
}

asm	int
spl4()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&10,%psw
}

asm	int
spl5()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&10,%psw	
}

asm	int
spl6()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&12,%psw
}

asm	int
splpp()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&10,%psw
}

asm	int
splni()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&12,%psw
}

asm	int
splvm()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&12,%psw
}

asm	int
splimp()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&13,%psw
}

asm	int
spltty()
{
	MOVW	%psw,%r0
	INSFW	&3,&13,&13,%psw
}

asm	int
spl7()
{
	MOVW	%psw,%r0
	ORW2	&0x1e000,%psw
}

asm	int
splhi()
{
	MOVW	%psw,%r0
	ORW2	&0x1e000,%psw
}

asm	int
spl0()
{
	MOVW	%psw,%r0
	ANDW2	&0xfffe1fff,%psw
}

asm	int
splx(opsw)
{
%	mem	opsw;
	MOVW	%psw,%r0
	MOVW	opsw,%psw
}

asm	void
movpsw(dest)
{
%	mem	dest;
	MOVW	%psw,dest
}

asm paddr_t
kvtophys(va)
{
%	mem	va;
	ANDW3	&0xfffffffc,va,%r0
	MOVTRW	0(%r0),%r1
	ANDW3	&0x3,va,%r0
	ORW2	%r1,%r0
}

#else

#if defined(__STDC__)

extern char *strend(char *);
extern int strlen(char *);
extern char *strcpy(char *, char *);
extern char *strcat(char *, char *);
extern int struct_zero(caddr_t, int);

extern int splx(int);
extern int spl0(void);
extern int spl1(void);
extern int spl4(void);
extern int spl5(void);
extern int spl6(void);
extern int spl7(void);
extern int splhi(void);
extern int splpp(void);
extern int splvm(void);
extern int splni(void);
extern int splimp(void);
extern int spltty(void);

extern void movpsw(int *);
extern paddr_t kvtophys(caddr_t);

#else

extern char	*strend();
extern int	strlen();
extern char 	*strcpy();
extern char	*strcat();
extern int	spl0();
extern int	spl1();
extern int	spl4();
extern int	spl5();
extern int	spl6();
extern int	spl7();
extern int	splx();
extern int	splpp();
extern int	splni();
extern int	splvm();
extern int	splimp();
extern int	spltty();
extern int	splhi();
extern void	movpsw();
extern paddr_t	kvtophys();
extern int struct_zero();

#endif	/* __STDC__ */

#endif	/* !defined(CXREF)  &&  !defined(lint)	*/

#endif	/* _SYS_INLINE_H */

#ifdef	KPERF  /* This is for kernel performance tool */
asm	int
get_spl()
{
	EXTFW	&3,&13,%psw,%r0
}
#endif	/* KPERF */
