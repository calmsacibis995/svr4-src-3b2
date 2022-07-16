/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/boot/hdacs.c	1.2"
/**********************************************************************/
/*                                                                    */
/*   Function Name: hd_acs                                            */
/*                                                                    */
/*         Authors: G. E. Laggis, M. H. Halt, R. S. Schnell           */
/*                                                                    */
/*Original Purpose: read or write a hard disk sector to or from mem   */
/*         Purpose: Read MULTIPLE SECTORS to memory                   */
/* Input Parameters: disk sector number, memory addr, and number of   */
/*                                                         sectors    */
/*    Return Value: FW_PASS or FW_FAIL                                */
/*                                                                    */
/* Globals Affected: none                                             */
/**********************************************************************/

#include <sys/firmware.h>
#include <sys/dma.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/id.h>

#define RETRY 16        /* number of disk operation retries */

extern char hdparms[];
extern char crcerr;

hd_read_sectors(select,sector,addr,nsectors)
unsigned char select;
unsigned long sector,addr,nsectors;
{
extern unsigned short hd_home();
extern struct pdinfo idpdt[];

unsigned long offset;           /* sector physical offset into cylinder */
unsigned long temp;             /* used to check for valid cylinder */
unsigned short retries;         /* number of retries attempted */
union iddskadr dsksect;         /* physical disk address */
char dmamode;                   /* dma setup word */
unsigned char disk;             /* disk accessed based on select and head */


/* figure sector's physical location, insuring an existing cylinder */

if ((temp = (sector / (idpdt[select].tracks * idpdt[select].sectors))) >= idpdt[select].cyls) {
	PRINTF("NON EXIST\n");
        return(FW_FAIL);
}

dsksect.part.pcnh = (temp >> 8) & 0xff;
dsksect.part.pcnl = temp & 0xff;

offset = sector % (idpdt[select].tracks * idpdt[select].sectors);

dsksect.part.phn = offset / (idpdt[select].sectors);

dsksect.part.psn = offset % (idpdt[select].sectors);


if (dsksect.part.phn > 7) {     /* use select + 2 for additional head select */
        disk = select + 2;
} else {
        disk = select;
}

for(retries = 0;retries < RETRY;++retries) {

        if (retries) {                          /* retry from home */
                hd_home(select);
        }

        if(!hd_seek(select,&dsksect)) {         /* Seek to cylinder */
                continue;
        }
        /* Setup disk command parameters */

        hdparms[0] = dsksect.part.phn;                  /* phn */
        hdparms[1] = ~dsksect.part.pcnh;                /* lcnh */
        hdparms[2] = dsksect.part.pcnl;                 /* lcnl */
        hdparms[3] = dsksect.part.phn;                  /* lhn */
        hdparms[4] = dsksect.part.psn;                  /* lsn */
        hdparms[5] = (char)nsectors;                    /* scnt */

        /* Init the DMAC for data xfer */
        dmamode = DMNDMOD | CH0IHD |  WDMA;
        if (!ddmainit(addr,dmamode,nsectors*512)) {
		PRINTF("addr is %x, count is %x\n", addr, nsectors*512);
		PRINTF("DMA SETUP FAILED.\n");
                return(FW_FAIL);
        }

        crcerr = OFF;
        /* Load the disk command */
        if(hdld_cmd(IDREAD | disk,hdparms,6)) {
                return(FW_PASS);
        }
}
if (crcerr) {
        PRINTF("id%d CRC error at disk address %08x (%d retries)\n",select,dsksect.all,RETRY);
        crcerr = OFF;
}

return(FW_FAIL);
}

/**********************************************************************/
/*                                                                    */
/*   Function Name: hd_seek                                           */
/*                                                                    */
/*         Authors: G. E. Laggis, M. H. Halt                          */
/*                                                                    */
/*         Purpose: move to a specified cylinder on the hard disk     */
/*                                                                    */
/* Input Parameters: cylinder                                         */
/*                                                                    */
/*    Return Value: 1 (pass) or 0 (fail)                              */
/*                                                                    */
/* Globals Affected: none                                             */
/**********************************************************************/

hd_seek(select,cylinder)
unsigned char select;
union iddskadr *cylinder;
{
        if (!hd_ready(select)) {
                return(FW_FAIL);
        }

        hdparms[0] = cylinder->part.pcnh;
        hdparms[1] = cylinder->part.pcnl;

        /* Load the disk command */
        return(hdld_cmd(IDSEEK | IDBUFFERED | select,hdparms,2));
}
