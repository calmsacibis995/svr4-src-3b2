/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/syg.c	1.1"



#include <sys/sbd.h>
#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/iobd.h>
#include <sys/diagnostic.h>
#include <edt_def.h>
#include <sys/dsd.h>
#include <sys/boot.h>


typedef struct rapplic{
	long data1;	/* filler for application area of queue */
	}RAPP;

typedef struct capplic{
	long data1;	/* filler for application area of queue */
	}CAPP;

#define NUM_QUEUES 1	/* request queue number */

/* number of entries for request & completion queues */

#define RQSIZE 2
#define CQSIZE 2

#define Q_SPACE 0x8000			/* provide space between queue entries */
#define SYSGEN_LIM 100			/* pass limit for SYSGEN delay loop */
#define DSD_LIM 1500			/* pass limit for DSD delay loop */

#define C_ADDR (DOWNADDR + Q_SPACE)		/* completion queue address */

#define R_ADDR  (C_ADDR + Q_SPACE)		/* request queue address */


#include <sys/queue.h>
#include <sys/cio_defs.h>

SG_DBLK sgdblk;
#define RQST (RQUEUE *)R_ADDR
#define CMPLT (CQUEUE *)C_ADDR

#define CXP_OPCD C_EXP.common.codes.bytes.opcode	/* macro for express request opcode */
#define RXP_OPCD R_EXP.common.codes.bytes.opcode	/* macro for express comp. opcode */
#define RXP_ADDR R_EXP.common.addr			/* macro for express address field */


extern unsigned long dsd_addr;		/* address of structure to store subdevice
					 * information */

extern fw_pump();		/* routine to pump peripheral console */


/* routine to SYSGEN then determine subdevices (DSD) for a device */

syg(board)
unsigned long board;	/* EDT entry number */
{
unsigned long i;	/* utility variable */
unsigned char retval;		/* return value for function*/

retval = FAIL;		/* set initial value to FAIL */

/* run at interrupt level 15 while sysgening boards */
asm("	INSFW &3,&13,&0xF,%psw");

/*
 * reset running peripheral console or boot device
 * to re-SYSGEN with this available sysgen data block
 */

if ((FL_CONS->device == EDTP(board)->opt_slot && FL_CONS->cons_found == ON) ||
	((P_CMDQ->b_dev >> 4) == EDTP(board)->opt_slot))
	SL_RESET(board) = 0;

/* define sysgen data block parameters */

*((long *)DBLK_PTR) = (long)&sgdblk;
sgdblk.request = R_ADDR;
sgdblk.complt = C_ADDR;
sgdblk.req_size = RQSIZE;
sgdblk.comp_size = CQSIZE;
sgdblk.no_rque = NUM_QUEUES;


/*  clear both queues */

BZERO(C_ADDR,2*Q_SPACE);

i = IO_SMART(EDTP(board)->opt_slot)->id1;	/* interrupt the board */

HWCNTR(20);		/* wait 20 10msec ticks between interrupts */

i = S_GEN(board);	/* SYSGEN the board */

for (i=0; i <= SYSGEN_LIM; i++)	/* now wait for something to happen */
	{
	HWCNTR(1);		/* wait 10msec between passes */
	if (CXP_OPCD == SYSGEN)
		{
		retval = PASS;
		break;		/* sysgen completed successfully */
		}

	else if (i == SYSGEN_LIM)
		 {
		/* board has timed out; print error message and return */
		fillerror(6);

		SL_RESET(board) = ON;	/* reset the board */

                /* wait for the reset to finish */
		HWCNTR(20);

		/* lower interupt level */
		asm("	INSFW &3,&13,&0x0,%psw");
		return(retval);		/* return the default failure value */
		}
	}

/* device SYSGEN'd OK; retval = PASS */

HWCNTR(20);		/* wait 20 10msec ticks before DSD */

/* now board is running; give it DSD command */

RXP_OPCD = DSD;			/* set opcode value */
RXP_ADDR = dsd_addr;		/* set DSD structure pointer */
CXP_OPCD = 0xff;		/* set express completion code to non-NORMAL state */


i = IO_SMART(EDTP(board)->opt_slot)->id1;	/* interrupt the board */

for (i=0; i <= DSD_LIM; i++)	/* now wait for something to happen */
	{
	HWCNTR(1);		/* wait 10msec between passes */
	if (CXP_OPCD == NORMAL)
		{
		break;		/* determine subdevices completed successfully */
		}

	else if (i == DSD_LIM)
		{
		/* board timed out; print error message and return */
		fillerror(7);

		retval = FAIL; /* set retval to failure */
		}
	}

SL_RESET(board) = ON; 		/* reset device before returning */

/* wait for the reset to finish */
HWCNTR(20);

/* lower interrupt level */
asm("	INSFW &3,&13,&0x0,%psw");

/* re-establish peripheral console and/or boot device, if necessary */

rstrcons();

if (rstrbdev(P_CMDQ->b_dev) == FAIL)
	{
	retval = FAIL;
	}

return(retval);
}
