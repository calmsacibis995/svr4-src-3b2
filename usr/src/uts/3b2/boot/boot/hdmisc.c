/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/boot/hdmisc.c	1.2"
/**********************************************************************/
/*                                                                    */
/*   Function Name: hd_init,hd_ready,hd_home                          */
/*                                                                    */
/*         Authors: G. E. Laggis, M. H. Halt                          */
/*                                                                    */
/*         Purpose: support routines for hd_seek & hd_acs             */
/*                                                                    */
/* Input Parameters: various                                          */
/*                                                                    */
/*    Return Value: FW_PASS or FW_FAIL                                */
/*                                                                    */
/* Globals Affected:                                                  */
/*                                                                    */
/**********************************************************************/

#include <sys/sbd.h>
#include <sys/firmware.h>
#include <sys/types.h>
#include <sys/id.h>
#include <sys/vtoc.h>

char    hdparms[8];     /* Disk Parameter Buffer */
unsigned long hdc_info; /* hard disk controller state
                        -------------------------------------------------
                        | command | unit status | error status | status |
                        ------------------------------------------------- */

extern struct pdinfo idpdt[];

/* save old psw and go to level 15 on the current psw */
#define NEWPSW asm("    MOVW %psw,oldpsw"); \
               asm("    ORW2 &0x1e100,%psw")

/* restore the old psw */
#define RESTORPSW asm(" MOVW oldpsw,%psw")

/*   SPECIFY Command Parameters */
#ifdef ECC
        /* ECC parameters */
        char    specparm[] = {0x58,0xb2,0x00,0x03,0x11,0x0d,0x00,0x80};
#else
        /* CRC parameters */
        char    specparm[] = {0x18,0xf2,0x00,0x03,0x11,0x0d,0x00,0x80};
#endif

char    ramspec[8];

/*  Initialize the disk controller  */

hd_init(select)
char select;
{
	register int i;

	for (i=0; i < i; ++i)
		ramspec[i] = specparm[i];

	ramspec[1] &=0xf0;
        ramspec[1] |= (char)(idpdt[select].bytes >> 8);
        ramspec[2] = (char)idpdt[select].bytes;
        ramspec[3] = (char)(idpdt[select].tracks - 1);
        ramspec[4] = (char)(idpdt[select].sectors - 1);
        /* r/w compensation at half-way point */
        ramspec[6] = (char)(idpdt[select].cyls >> 9);
        ramspec[7] = (char)(idpdt[select].cyls >> 1);

        IDISK->statcmd = IDRESET;       /* Reset the disk controller */

        HWCNTR(1);

        /* Setup the mode of the controller */
        if (hdld_cmd(IDSPECIFY,ramspec,8)) {
                if (hd_home(select)) {
                        return(FW_PASS);
                } else {
                        return(FW_FAIL);
                }
        }

        return(FW_FAIL);
}

/*  Home Disk Drive - recalibrate the drive to track 0  */

hd_home(select)
char select;
{
        if (!hd_ready(select)) {                        /* Is drive ready ? */
                return(FW_FAIL);
        } else {
                /* recal the drive */
                hdld_cmd(IDRECAL | IDBUFFERED | select,0,0);
                if (hdld_cmd(IDRECAL | IDBUFFERED | select,0,0)) {
                        return(FW_PASS);
                } else {
                        return(FW_FAIL);
                }
        }
}

/*  Load a Command into the Disk Controller  */

unsigned long oldpsw;   /* storage for psw before routine      */
                        /* Ldcmd causes an interrupt           */
                        /* but clears it again on exit.  The   */
                        /* psw is raised to level fifteen      */
                        /* during this routine to mask the     */
                        /* interrupt, and restored at the end  */
                        /* once the interrupt has been cleared */

char crcerr;            /* flag for CRC errors that occur      */

#define CMD_TIME 200    /* 2 sec max for command completion */
#define NAUX_TIME 5     /* 50 msec for non-auxilliary command end */

hdld_cmd(command,addparm,parmsize)
char command,parmsize;
char *addparm;
{
        unsigned short count;
        unsigned long sav_info;

        NEWPSW;      /* go to IPL 15 */

        /* return fail if never non busy */

        if ((hdc_info = IDISK->statcmd) & IDCBUSY) {
                hdc_info |= (command << 24);    /* command to its loc */
                /* Clear command end bits and any interrupts */
                IDISK->statcmd = IDCLCMNDEND;  
                RESTORPSW;
		PRINTF("Fail 0\n");
                return(FW_FAIL);        
        }

        IDISK->statcmd = IDCLFIFO;      /* Clear parameter buffer pointer */
        IDISK->statcmd = IDCLCMNDEND;     /* Clear command end bits         */


        while(parmsize > 0) {                  /* Load any command parameters */
                IDISK->fifo = *addparm++;
                parmsize--;
        }

        IDISK->statcmd = command;       /* Load the command code */

        /* Wait for idle state */
        for(count = 0;((count < CMD_TIME) && (IDISK->statcmd & IDCBUSY));count++) {
                HWCNTR(1);
        }

        if ((hdc_info = IDISK->statcmd) & IDCBUSY) {
                hdc_info |= (command << 24);    /* command to its loc */
                /* Clear command end bits and any interrupts */
                IDISK->statcmd = IDCLCMNDEND;  
                RESTORPSW;
		PRINTF("Fail 1\n");
                return(FW_FAIL);        
        }

        if (command & 0xF0) {
                /* Non auxiliary command, wait for completion bits */
                for(count = 0;((count < NAUX_TIME) && ((IDISK->statcmd & IDENDMASK) == IDCMDINP));count++) {
                        HWCNTR(1);
                }
        } else {                /* auxiliary command */
                hdc_info = 0;
                /* Clear command end bits and any interrupts */
                IDISK->statcmd = IDCLCMNDEND;  
                RESTORPSW;
                return(FW_PASS);
        }

        /* Normal Termination ? */
        if (((hdc_info = IDISK->statcmd) & IDENDMASK) != IDCMDNRT) {
		short est;

		est = IDISK->fifo;
		PRINTF("Error status is %x\n", est);

                /* command to its loc, error status to its loc */
                hdc_info |= ((command << 24) | (est << 8));
                if ((hdc_info & (IDDATAERR << 8)) && (command & IDREAD)) {
                        crcerr = ON;            /* Report CRC error on read */
                }

                /* check for a reset request */
                if (hdc_info & IDRESETRQ) {
                        sav_info = hdc_info;
                        if (hd_init(command & 1)) {
                                hd_home((command & 1) ^ 1);
                        }
                        hdc_info = sav_info;
                }
                IDISK->statcmd = IDCLCMNDEND;  
                RESTORPSW;
		PRINTF("Fail 4\n");
                return(FW_FAIL);
        }

        /* check for a reset request */
        if (hdc_info & IDRESETRQ) {
                hdc_info |= (command << 24);    /* command to its loc */
                sav_info = hdc_info;
                if (hd_init(command & 1)) {
                        hd_home((command & 1) ^ 1);
                }
                hdc_info = sav_info;
                RESTORPSW;
		PRINTF("Fail 2\n");
                return(FW_FAIL);
        } else {
                /* Clear command end bits and any interrupts */
                IDISK->statcmd = IDCLCMNDEND;  
                hdc_info = 0;
                RESTORPSW;
                return(FW_PASS);
        }
}

/*  Test for Drive Ready  */

hd_ready(select)
char select;
{
        /* Sense the drive status */
        if (!hdld_cmd(IDSENSEUS | select,0,0)) {
                return(FW_FAIL);
        }

        if ((hdc_info = IDISK->fifo) & IDREADY) {  /* Check for drive ready */
                hdc_info = 0;
                return(FW_PASS);
        } else {
                hdc_info <<= 16;                /* unit status to its loc */
                hdc_info |= (IDSENSEUS << 24);  /* command to its loc */
                return(FW_FAIL);
        }
}
