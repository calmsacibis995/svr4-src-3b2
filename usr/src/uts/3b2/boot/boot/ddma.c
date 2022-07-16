/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/boot/ddma.c	1.5"
/**********************************************************************/
/*                                                                    */
/*    Function Name: ddmainit                                         */
/*                                                                    */
/*          Authors: G. E. Laggis, M. H. Halt                         */
/*                                                                    */
/*          Purpose: set up the dma controller for disk access        */
/*                                                                    */
/* Input Parameters: memory address, mode, and byte count             */
/*                                                                    */
/*     Return Value: none                                             */
/*                                                                    */
/* Globals Affected: none                                             */
/*                                                                    */
/**********************************************************************/

#include <sys/sbd.h>
#include <sys/firmware.h>
#include <sys/dma.h>

union dmaadda {
        long    lngadd;
        char    addr[4];        /* address of dma transfer */
};

/* Dma controller setup for transfer from hard or floppy disk ONLY! */

ddmainit(dmaadd,dmamode,count)
union dmaadda  dmaadd;
unsigned char dmamode;          /* mode of dma transfer (including channel) */
unsigned short count;           /* number of bytes to xfer */
{

/* insure buffer does not cross 64k byte boundary */

if ((long)(dmaadd.lngadd / BND64K) < (long)((dmaadd.lngadd + count - 1) / BND64K)) {
        return(FW_FAIL);
}

/* Setup DMAC command register - DACK active low, DREQ active high,Late write,
    Fixed priority, Normal timing,Channel 0 address hold disable, Memory to
    memory disable */

/* Clear the DMAC */

IDMAC->RTR_WMC = 0;
IDMAC->RSR_CMD = 0;
IDMAC->CBPFF = 0;                       /* clear DMAC pointer flip-flop */

if (dmamode & CH1IFL) {                 /* load address of specified channel */
        IDMAC->C1CA = dmaadd.addr[3];   /* load byte 3 */
        IDMAC->C1CA = dmaadd.addr[2];   /* load byte 2 */
        if(dmamode & RDMA)              /* load byte 1 */
                IDMAPFD = RPAGE | dmaadd.addr[1];
        else
                IDMAPFD = WPAGE | dmaadd.addr[1];
} else {
        IDMAC->C0CA = dmaadd.addr[3];   /* load byte 3 */
        IDMAC->C0CA = dmaadd.addr[2];   /* load byte 2 */
        if(dmamode & RDMA)              /* load byte 1 */
                IDMAPHD = RPAGE | dmaadd.addr[1];
        else
                IDMAPHD = WPAGE | dmaadd.addr[1];
}

IDMAC->CBPFF = 0;

if (dmamode & CH1IFL) {                 /* load count of specified channel */
        IDMAC->C1WC = (count - 1) & 0xff;               /* load byte 0 */
        IDMAC->C1WC = ((unsigned short)(count - 1) >> 8) & 0xff;  /* load byte 1 */
} else {
        IDMAC->C0WC = (count - 1) & 0xff;               /* load byte 0 */
        IDMAC->C0WC = ((unsigned short)(count - 1) >> 8) & 0xff;  /* load byte 1 */
}

IDMAC->WMODR = dmamode;                 /* Setup DMAC mode register */

/* Enable desired DMA Channel and Inhibit other 3 DMA Channels */
IDMAC->WAMKR = (~((dmamode & 3) + 1)) & 0xf;

return(FW_PASS);
}
