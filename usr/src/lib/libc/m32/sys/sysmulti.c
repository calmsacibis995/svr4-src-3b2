/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:sys/sysmulti.c	1.3"
#include	"synonyms.h"
#include	"signal.h"
#include	"sys/sys3b.h"
#include	"errno.h"
#include	"sys/sysmulti.h"

/*
 * C library -- sysmulti
 * 
 * Sysmulti.c implements the setup and GATE into the kernel
 * for multi-processor system call sysmulti(2). Sysmulti may not
 * be supported on all 3B systems and an attempt to gate into
 * the kernel with the sysmulti gate vector index of 140*8
 * might result in a core dump on some systems. Consequently
 * sysmulti.c must only enter the kernel on multi-processor systems
 * as determined by first executing a multi-processor-specific sys3b
 * call. Because the sys3b call may generate SIGSYS on some
 * 3B2 systems, the handling of SIGSYS is first changed to 
 * SIG_IGN and then changed back to the previous setting
 * after the sys3b call. The call to sys3b is performed
 * only the first time sysmulti() is entered. A static variable
 * is set so subsequent sysmulti calls can determine the
 * system type without incurring the cost of the sys3b and
 * signal code.
 *
 * Note that syscall() is used to setup and perform the
 * actual GATE so that the argument pointer is correctly setup.
 * If sysmulti() has been called on a multi-processor it simply
 * returns the response from syscall(). If not called
 * on a multi-processor system, errno is set to EINVAL and -1 is returned.
 */

#define SYSMULTI 140	/* define gate vector index */


sysmulti(cmd, res_type, pe, rsrc)
int	cmd;
int	res_type;
PE_t *	pe;
long	rsrc;
{
	static int	ap_sys = 0;
	int		(*sigsave)();

	if (ap_sys > 0)
	{
		return(syscall(SYSMULTI, cmd, res_type, pe, rsrc));
	}
	else if (ap_sys < 0)
	{
		errno = EINVAL;
		return(-1);
	}
	else			/* ap_sys == 0 */

	/*
	 * The following code which sets the value of ap_sys on
	 * the first call to sysmulti() is intentionally placed
	 * and intentionally duplicates the syscall() and errno
	 * code to optimize the variable check for subsequent calls.
	 */
	
	{
		sigsave = signal(SIGSYS, SIG_IGN);
		if ((ap_sys = sys3b(S3BPEID, PEID_HOST)) != -1)
		{
			ap_sys = 1;
		}
		(void)signal(SIGSYS, sigsave);

		if (ap_sys < 0)
		{
			errno = EINVAL;
			return(-1);
		}
		else
		{
			return(syscall(SYSMULTI, cmd, res_type, pe, rsrc));
		}
	}
}

/*
 * useful functions
 *
 * turn on the bit in the bitmap corresponding to pe
 */
sm_turnon(pe, bmp)
int	pe;
PE_t	*bmp;
{
	if ((unsigned) (pe/NBPL) < MAPSIZE) {
		bmp->bmap[pe/NBPL] |= (((unsigned) 1) << (pe%NBPL));
		return(0);
	}
	return(-1);
}

/*
 *  turn off the bit in the bitmap corresponding to pe
 */
sm_turnoff(pe, bmp)
int	pe;
PE_t	*bmp;
{
	if ((unsigned) (pe/NBPL) < MAPSIZE) {
		bmp->bmap[pe/NBPL] &= ~(((unsigned) 1) << (pe%NBPL));
		return(0);
	}
	return(-1);
}

/*
 * check if a bit in the bitmap is on
 */
sm_ison(pe,bmp)
int	pe;
PE_t	*bmp;
{
	if ((unsigned) (pe/NBPL) < MAPSIZE)
		if (bmp->bmap[pe/NBPL] & (((unsigned) 1) << (pe%NBPL)) )
			return(1);
		else
			return(0);
	return(-1);
}

/*
 * check if any bit in the bitmap is on
 */
sm_isanyon(bmp)
PE_t	*bmp;
{
	/* if MAPSIZE changes, change this function */
	return (bmp->bmap[0] | bmp->bmap[1] | bmp->bmap[2] | bmp->bmap[3]);
}

/*
 * clear the bitmap
 */
void
sm_clrbmap(bmp)
PE_t	*bmp;
{
	/* if MAPSIZE changes, change this function */
	bmp->bmap[0] = bmp->bmap[1] = bmp->bmap[2] = bmp->bmap[3] = 0;
}

/*
 * return the number of the pe bit on in the bitmap
 */
sm_whichpe(bmp)
PE_t *bmp;
{
	register int	i;
	register int	penum = 0;
	PE_t		locPE;

	locPE = *bmp; /* structure copy */

	for (i = 0; i < MAPSIZE; i++) {
		if (locPE.bmap[i] != 0) {
			while ((locPE.bmap[i] & 0x1) == 0) {
				locPE.bmap[i] >>= 1;
				penum++;
			}
			return(penum);
		} else
			penum += NBPL;
	}

	return(-1);
}

/*
 * bitwise-or two bitmaps
 */
void
sm_orbmap(bmp1, bmp2, resbmp)
PE_t	*bmp1, *bmp2, *resbmp;
{
	/* if MAPSIZE changes, change this function */
	resbmp->bmap[0] = bmp1->bmap[0] | bmp2->bmap[0];
	resbmp->bmap[1] = bmp1->bmap[1] | bmp2->bmap[1];
	resbmp->bmap[2] = bmp1->bmap[2] | bmp2->bmap[2];
	resbmp->bmap[3] = bmp1->bmap[3] | bmp2->bmap[3];
}

/*
 * bitwise-and two bitmaps
 */
void
sm_andbmap(bmp1, bmp2, resbmp)
PE_t	*bmp1, *bmp2, *resbmp;
{
	/* if MAPSIZE changes, change this function */
	resbmp->bmap[0] = bmp1->bmap[0] & bmp2->bmap[0];
	resbmp->bmap[1] = bmp1->bmap[1] & bmp2->bmap[1];
	resbmp->bmap[2] = bmp1->bmap[2] & bmp2->bmap[2];
	resbmp->bmap[3] = bmp1->bmap[3] & bmp2->bmap[3];
}

/*
 * invert (not) bitmap
 */
void
sm_notbmap(bmp, resbmp)
PE_t	*bmp, *resbmp;
{
	/* if MAPSIZE changes, change this function */
	resbmp->bmap[0] = ~bmp->bmap[0];
	resbmp->bmap[1] = ~bmp->bmap[1];
	resbmp->bmap[2] = ~bmp->bmap[2];
	resbmp->bmap[3] = ~bmp->bmap[3];
}

/*
 * get the next pe in the bitmap
 *
 * typical usage:
 *	nextpe = 0;
 *	while ((penum = sm_getnextpe(&bitmap, &nextpe)) != -1) {
 *		...
 *	}
 */
int
sm_getnextpe(bmp, peptr)
PE_t *	bmp;
int *	peptr;
{
	register int penum;

	for (penum = *peptr; penum < (MAPSIZE * NBPL); penum++)
		if (sm_ison(penum, bmp) == 1) {
			*peptr = penum + 1;
			return(penum);
		}

	return(-1);
}

/*
 * clear pesel structure
 */
void
sm_clrpesel(peselp)
struct pesel *peselp;
{
	char *	memset();

	(void) memset(peselp, 0, sizeof(struct pesel));
}
