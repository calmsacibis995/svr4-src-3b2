/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)exec:exec/intp/intp.c	1.8"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/signal.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/buf.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/fstyp.h"
#include "sys/acct.h"
#include "sys/sysinfo.h"
#include "sys/reg.h"
#include "sys/var.h"
#include "sys/immu.h"
#include "sys/proc.h"
#include "sys/tuneable.h"
#include "sys/tty.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/rf_messg.h"
#include "sys/conf.h"
#include "sys/uio.h"
#include "sys/pathname.h"
#include "sys/disp.h"
#include "sys/exec.h"
	

short intpmagic = 0x2321;		/* magic number for cunix */

STATIC int getintphead();

#define	INTPSZ	32
struct intpdata {
	char	line1p[INTPSZ];
	int	intp_ssz;
	char	*intp_name;
	char	*intp_arg;
};


/*
 * Crack open a '#!' line.
 */
STATIC int
getintphead(vp, idatap, ehdp)
	struct vnode *vp;
	register struct intpdata *idatap;
	exhda_t *ehdp;
{
	register int error;
	register char *cp, *linep;
	int resid;
	int rdsz;
	int ssz = 0;

	/*
	 * Read the entire line and confirm that it starts with '#!'.
	 */
	rdsz = INTPSZ;
	if (rdsz > ehdp->vnsize)
		rdsz = ehdp->vnsize;
	if ((error = exhd_getmap(ehdp, (off_t) 0, rdsz,
			EXHD_COPY, idatap->line1p)) != 0)
		return error;
	linep = idatap->line1p;
	if (linep[0] != '#' || linep[1] != '!')
		return ENOEXEC;
	/*
	 * Blank all white space and find the newline.
	 */
	cp = &linep[2];
	linep += rdsz;
	for (; cp < linep && *cp != '\n'; cp++)
		if (*cp == '\t')
			*cp = ' ';
	if (cp >= linep)
		return ENOEXEC;
	ASSERT(*cp == '\n');
	*cp = '\0';

	/*
	 * Locate the beginning and end of the interpreter name.
	 * In addition to the name, one additional argument may
	 * optionally be included here, to be prepended to the
	 * arguments provided on the command line.  Thus, for
	 * example, you can say
	 *
	 * 	#! /usr/bin/awk -f
	 */
	for (cp = &idatap->line1p[2]; *cp == ' '; cp++)
		;
	if (*cp == '\0')
		return ENOEXEC;
	idatap->intp_name = cp;
	while (*cp && *cp != ' ') {
		ssz++;
		cp++;
	}
	ssz++;
	if (*cp == '\0')
		idatap->intp_arg = NULL;
	else {
		*cp++ = '\0';
		while (*cp == ' ')
			cp++;
		if (*cp == '\0')
			idatap->intp_arg = NULL;
		else {
			idatap->intp_arg = cp;
			while (*cp && *cp != ' ') {
				ssz++;
				cp++;
			}
			*cp = '\0';
			ssz++;
		}
	}
	idatap->intp_ssz = ssz;
	return 0;
}


int
intpexec(vp, args, level, execsz, ehdp)
struct vnode *vp;
struct uarg *args;
int level;
long *execsz;
exhda_t *ehdp;
{
	vnode_t *nvp;
	int num,c,i;
	int error=0;
	int *from,*to;
	struct intpdata idata;
	struct pathname intppn;
	extern int userstack[];

	if (level) {		/* Can't recurse */
		error = ENOEXEC;
		goto bad;
	}
	if ((error = getintphead(vp, &idata, ehdp)) != 0)
		goto bad;
	/*
	 * Look the new vnode up.
	 */
	if (error = pn_get(idata.intp_name, UIO_SYSSPACE, &intppn))
		return error;
	if (error = lookuppn(&intppn, FOLLOW, NULLVPP, &nvp)) {
		pn_free(&intppn);
		return error;
	}
	pn_free(&intppn);

	num = 1;
	if (idata.intp_arg)
		num++;
	args->prefixc = num;
	args->prefixp = (&idata.intp_name);
	args->prefixsize = idata.intp_ssz;
	
	error = gexec(nvp, args, ++level, execsz);

	VN_RELE(nvp);
bad:
	return error;

}
