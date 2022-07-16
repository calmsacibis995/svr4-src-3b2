/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/p_func.c	1.4"

#include <sys/types.h>
#include <sys/sbd.h>
#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/iobd.h>
#include <sys/diagnostic.h>
#include <sys/dsd.h>
#include <sys/boot.h>
#include <sys/csr.h>
#include <sys/iu.h>
#include <sys/sit.h>
#include <sys/cio_defs.h>

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

#define RST_DLY 0x14		/* count of 10 msec. HWCNTR clock ticks for board reset */
#define LOOP_LIM 200		/* delay loop limit (10msec ticks) for com I/O jobs */
#define FW_QSIZE 0x814		/* space FW requires for Com I/O queues:
				 * the EXPRESS header		0x008
				 * the EXPRESS Appl. field	0x400
				 * the load/unload pointers	0x004
				 * one NORMAL header		0x008
				 * one NORMAL Appl. field	0x400
				 */


#define C_ADDR FW_CQ_ADDR		/* completion queue address ends at BOOTADDR */

#define R_ADDR  FW_RQ_ADDR		/* request queue address */


#include <sys/queue.h>
#include <sys/cio_defs.h>

#define RQST (RQUEUE *)R_ADDR
#define CMPLT (CQUEUE *)C_ADDR

#define FF 0xff						/* non-normal completion code */
#define CXP_OPCD C_EXP.common.codes.bytes.opcode	/* macro for express comp. opcode */
#define CXP_SUBDEV C_EXP.common.codes.bytes.subdev	/* macro for express comp. subdev */
#define CXP_BYTCNT C_EXP.common.codes.bytes.bytcnt	/* macro for express comp byte count field */
#define CXP_ADDR C_EXP.common.addr			/* macro for express address field */
#define RXP_OPCD R_EXP.common.codes.bytes.opcode	/* macro for express request opcode */
#define RXP_SUBDEV R_EXP.common.codes.bytes.subdev	/* macro for express request subdev */
#define RXP_ADDR R_EXP.common.addr			/* macro for express address field */
#define RXP_BYTCNT R_EXP.common.codes.bytes.bytcnt	/* macro for express request byte count field */
#define RXP_SEQ R_EXP.common.codes.bits.seqbit		/* macro for express request seq bit */
#define RXP_CMD R_EXP.common.codes.bits.cmd_stat	/* macro for express request cmd/status bit */

extern void checkpwr();

char clear_pint();

unsigned long old_IPL;		/* storage for previous IPL */
long (*old_int_hand)();		/* storage for provious interrupt handler */
unsigned short fw_cio_flg;	/* flag for clearing peripheral interrupt */
extern long fw_int_hand();



/* routine to find console terminal on peripheral devices */



fw_dcons(board,subdev)
unsigned char board;	/* device slot number */
unsigned char subdev;	/* subdevice number */
{
unsigned long i;	/* utility variable */
unsigned char retval;	/* return value for function*/
struct fl_cons temp_cons;	/* copy of FL_CONS to send peripheral */

retval = FAIL;		/* set initial value to FAIL */

old_int_hand = INT_HAND;		/* save incoming int. handler */
INT_HAND = fw_int_hand;			/* set comio int. handler */

asm("	EXTFW &3,&13,%psw,old_IPL");	/* save incoming IPL */
asm("	INSFW &3,&13,&0xF,%psw");	/* run at level 15 during function */


RXP_OPCD = DCONS;		/* set opcode value */
RXP_SUBDEV = subdev;		/* set subdevice value */
CXP_OPCD = FF;			/* set express completion code to non-NORMAL state */
RXP_ADDR = (long)(&temp_cons);		/* set  FL_CONS structure pointer */
temp_cons.cflags = FL_CONS->cflags;
temp_cons.cons_found = 0;



i = IO_SMART(board)->id1;	/* interrupt the board */

/* now wait for something to happen */
for (i = 0; i <= LOOP_LIM; i++)
	{
	if (CXP_OPCD == NORMAL)
		{
		retval = PASS;
		FL_CONS->port = temp_cons.port;
		FL_CONS->cons_found = ON;
		FL_CONS->cflags = temp_cons.cflags;
/* debug */
/* PRINTF("DCONS passed on iteration %d\n",i); */
		break;		/* determine subdevices completed successfully */
		}

	else if (i == LOOP_LIM)
		{
		/* board timed out; print error message and return */
		/* PRINTF("DCONS failed for %s in slot %d\n",EDTP(board)->dev_name,EDTP(board)->opt_slot); */
		retval = FAIL; /* set retval to failure */

		IO_SMART(board)->reset = ON; 		/* reset device before returning */
		HWCNTR(RST_DLY);
		}
	HWCNTR(1);	/* each pass takes ~10msec */
	}


/* restore IPL */
clear_pint();

return((int) retval);
}


comioxmt(board, mmaddr, peraddr, bytecnt)
unsigned long board;	/*  Board slot  */
unsigned long mmaddr;	/*  Address in main memory to get code from  */
unsigned long peraddr;	/*  Address to load code on peripheral  */
unsigned long bytecnt;	/*  Number of bytes to send  */
{
    long i;
    short retval;

#ifdef	DEBUG
PRINTF("Comioxmt called, board=%d  mmaddr=0x%x  peraddr=0x%x  bytecnt=%d\n",
board, mmaddr, peraddr, bytecnt);
#endif
i = peraddr & 0xff;  /*  Offset from 256 byte boundary  */
mmaddr -= i;
bytecnt += i;
peraddr &= 0xff00;
#ifdef	DEBUG
PRINTF("Parameters used: board=%d  mmaddr=0x%x  peraddr=0x%x  bytecnt=%d\n",
board, mmaddr, peraddr, bytecnt);
#endif
/* now board is running; give it DLM command */

RXP_OPCD = DLM;			/* set opcode value */
RXP_ADDR = mmaddr;		/* set memory address */
RXP_CMD  = CS_COMMAND;
if (peraddr > 0x3fff) {
    peraddr -= 0x4000;
    RXP_SEQ = 1;
}
else  RXP_SEQ = 0;

RXP_SUBDEV = (peraddr>>8);	/*  Set peripheral address  */

retval = FAIL;

/* run at interrupt level 15 while pumping boards */
asm("	INSFW &3,&13,&0xF,%psw");


    RXP_BYTCNT = bytecnt - 1;
    CXP_BYTCNT = 0xffff;		/*  Set comp. bytecount to illegal value  */
    CXP_OPCD = 0xff;		/* set express completion code to non-NORMAL state */
#ifdef	DEBUG
PRINTF("bytecnt=0x%x\n",bytecnt);
PRINTF("--- OPCD=0x%x, ADDR=0x%x, SUBD=0x%x, BYTCNT=%d\n",
RXP_OPCD,RXP_ADDR,RXP_SUBDEV,RXP_BYTCNT);
#endif
    i = IO_SMART(board)->id1;	/* interrupt the board */

    for (i=0; i <= LOOP_LIM; i++)	/* now wait for something to happen */
	{
	HWCNTR(1);  /*  Sleep 10 ms  */

	if ((CXP_OPCD == NORMAL) && (CXP_BYTCNT == (bytecnt-1)))
		{
#ifdef	DEBUG
PRINTF("DLM sent %d bytes successfully\n",bytecnt);
#endif
		retval = PASS;
		break;		/* completed successfully */
		}

	else if (i == LOOP_LIM)
		{
		/* board timed out; print error message and return */
		PRINTF("DLM failed for %s in slot %d\n",EDTP(board)->dev_name,EDTP(board)->opt_slot);
		PRINTF("CXP_OPCD=0x%x, CXP_BYTCNT=%d\n",CXP_OPCD,CXP_BYTCNT);
		retval = FAIL; /* set retval to failure */
		}
	}

/* lower interupt level */
asm("	INSFW &3,&13,&0x0,%psw");

return(retval);
}


comioxeq(board, peraddr)
short board;
unsigned long peraddr;
{
    long i;

#ifdef	DEBUG
PRINTF("Comioxeq called, board=%d  peraddr=0x%x\n",board,peraddr);
#endif
    RXP_OPCD = FCF;		/*  XEQ the code  */
    RXP_ADDR = peraddr;
    RXP_SUBDEV = 0;
    RXP_BYTCNT = 0;

#ifdef	DEBUG
PRINTF("+++ OPCD=0x%x, ADDR=0x%x\n",
RXP_OPCD,RXP_ADDR);
#endif

/* run at interrupt level 15 while talking to board */
asm("	INSFW &3,&13,&0xF,%psw");

i = IO_SMART(board)->id1;	/* interrupt the board */

/* lower interupt level */
asm("	INSFW &3,&13,&0x0,%psw");

return(PASS);
}

/* interrupt handler for common I/O functions */

long
fw_int_hand()
{
asm("	INSFW &3,&13,&0xF,%psw");	/* raise IPL */

checkpwr();	/* test for power-down request */

INT_HAND = old_int_hand;		/* restore old handler */
if (SBDRCSR & (CSRCLK | CSRPIR8 | CSRPIR9 | CSRUART | CSRDISK | CSRDMA))
	/* EMPTY */ ;
	/* some other interrupt was detected -- do nothing */
else
	/* peripheral interrupt so clear flag for clear_pint() */
	fw_cio_flg = 0;

asm("	INSFW &3,&13,old_IPL,%psw");	/* re-restore IPL */
}

/* routine to clear peripheral interrupts for fw_comio functions */
/* returns 1 if interrupt was cleared, 0 if not			 */

char
clear_pint()
{

fw_cio_flg = 1;

int_jail:	/* label for goto below */

asm("	INSFW &3,&13,&0x0,%psw");	/* drop IPL */

asm(" NOP ");				/* give the interrupt a little time */
asm(" NOP ");				/* to occur			    */
asm(" NOP ");
asm(" NOP ");

asm("	INSFW &3,&13,&0xF,%psw");	/* raise IPL */

if( fw_cio_flg == 0 )
	return(1);
else if( fw_cio_flg > 0xff ){		/* 255 interrupts - we must be stuck */
	return(0);
} else {
	INT_HAND = fw_int_hand;
	++fw_cio_flg;
	goto int_jail;
}
}
