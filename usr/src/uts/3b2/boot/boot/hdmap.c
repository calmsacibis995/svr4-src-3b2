/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/boot/hdmap.c	1.3"
/**********************************************************************/
/*                                                                    */
/*    Function Name: hd_mapinit                                       */
/*                                                                    */
/*          Authors: G. E. Laggis, M. H. Halt, R. S. Schnell          */
/*                                                                    */
/*          Purpose: Map hard disk sectors that have been determined  */
/*                   to have hard errors to their alternate sector.   */
/*                                                                    */
/*                   Boot program version which keeps each bad sector */
/*                   in a special array.  Boot version only keeps     */
/*                   bad sectors which are contained in file.         */
/*                                                                    */
/* Input Parameters: sector number to be mapped                       */
/*                                                                    */
/*     Return Value: FW_FAIL if maps invalid, FW_PASS otherwise with  */
/*                   input sector changed if it is mapped.            */
/*                                                                    */
/* Globals Affected:                                                  */
/*                                                                    */
/**********************************************************************/

#include <sys/sbd.h>
#include <sys/firmware.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/id.h>
#include "sys/ertyp.h"
#include <sys/fsiboot.h>

#define FIXBYTES 512

#define hd_acs HD_ACS

struct pdinfo idpdt[2];                 /* phys info from each disk */
struct iddeftab sectmap[64];            /* a sector of one disk's defect map */
struct badblock baddies[IDDEFCNT];

char mapsrc;                            /* disk with one map sector in memory */
unsigned short mapsize[2];              /* size of defect map (in sectors) */
unsigned short mapsect[2];              /* current relative map sect in mem */
char idmap;                             /* flag hard disk access w/wo mapping */
unsigned long disk_err;                 /* save disk error type for nvram */

hd_mapinit(sel, startsect, nsects)
long startsect, nsects;
unsigned char sel;
{

register char *ptr,*tptr;               /* temporary ptrs for moving data */
register unsigned short i,j,defsper;
short sectpercyl;
int k = 0;

idmap = OFF;
idpdt[sel].bytes = FIXBYTES;            /* start with minimum defaults */
idpdt[sel].sectors = 18;
idpdt[sel].tracks = 4;
idpdt[sel].cyls = 306;

disk_err = 0;
if (!hd_acs(sel,0,sectmap,DISKRD)) {    /* temp read phys info into defmap */
        disk_err = DSK_PHYS;
        return(FW_FAIL);
}

if (((struct pdinfo *)sectmap)->sanity != VALID_PD) { /* phys info insane */
        disk_err = DSK_WORD;
        return(FW_FAIL);
}

ptr=(char *)(&(idpdt[sel]));            /* copy phys info to real phys info */
tptr=(char *)(sectmap);
for(i=0;i<sizeof(struct pdinfo);i++)
        *ptr++ = *tptr++;

mapsize[sel] = idpdt[sel].defectsz/idpdt[sel].bytes;/* mapsize in sects */


defsper = idpdt[sel].bytes / sizeof(struct iddeftab);   /* defs per map sect */

mapsrc = 0xff;  /* invalidate sectmap while reading defect map sectors */

sectpercyl = idpdt[sel].tracks * idpdt[sel].sectors;

for (mapsect[sel] = 0;mapsect[sel] < mapsize[sel];++mapsect[sel]) {
        if (!hd_acs(sel,idpdt[sel].defectst + mapsect[sel],sectmap,DISKRD)) {
                disk_err = ((DSK_MAP + mapsect[sel]) << DSK_ERLOC);
                return(FW_FAIL);
        }
        for (j = 0;j < defsper;++j) {
                if (sectmap[j].bad.part.pcnh == 0xff) {
                        break;
                }

                i = (sectmap[j].bad.part.pcnh << 8) + sectmap[j].bad.part.pcnl;
		baddies[k].bad = i*sectpercyl + (sectmap[j].bad.part.phn * 
			idpdt[sel].sectors) + sectmap[j].bad.part.psn;

		if ((baddies[k].bad < startsect) ||
			(baddies[k].bad > (startsect+nsects)))
			continue;

                i = (sectmap[j].good.part.pcnh << 8) + sectmap[j].good.part.pcnl;

		baddies[k++].good = i*sectpercyl + (sectmap[j].good.part.phn * 
			idpdt[sel].sectors) + sectmap[j].good.part.psn;

        }
        if (j < defsper) {
                break;
        }
}

baddies[k].bad = 0;
mapsect[sel] = mapsize[sel] - 1;        /* last sector of map in mem */
mapsrc = sel;                           /* sectmap contains map for this disk */

idmap = ON;
return(FW_PASS);
}

