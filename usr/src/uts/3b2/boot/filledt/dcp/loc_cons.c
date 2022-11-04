/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/loc_cons.c	1.2"



/*
 * This function will locate the console device, if possible.
 * It tests for the console that FL_CONS describes, then
 * begins a search of all devices that have the cons_cap bit in
 * their EDT entries set.
 * The search begins with the system board "console" and "link"
 * channels, then moves through the remaining entries in numerical
 * order.
 * If no console is found, the system board "console" is the default
 * device.
 */
#include <sys/types.h>
#include <sys/sbd.h>
#include <sys/iu.h>
#include <sys/firmware.h>
#include <sys/nvram.h>
#include <sys/edt.h>
#include <sys/dsd.h>
#include <sys/iobd.h>
#include <sys/diagnostic.h>
#include <sys/termio.h>

extern unsigned char Dirname[];
extern unsigned char filename[];

extern void dflt_init();
extern fw_pump();

loc_cons()
{
/* skip this if console is already located */
if (FL_CONS->cons_found == ON)
	{
/* debug */
/* PRINTF("console already found\n"); */
	return(PASS);
	}

/*
 * Read putative console slot and port #'s from NVRAM.
 * If the baud rates in nvram are legit, set uart to them, else use defaults.
 * UARTS initialized at least once before the reset so nvram values are current.
 */


if (RNVRAM(&FW_NVR->cons_def,&(FL_CONS->port),sizeof(FW_NVR->cons_def)) == PASS)
	{
	FL_CONS->device = (FL_CONS->port & 0xf0) >> 4;	/* upper nibble is slot # */
	FL_CONS->port &= 0xf;				/* lower nibble is port # */

	/*
	 * Suppress non-default console values for DEMON PROMS
	 * Test based on DEMON requiring 64 kB while normal PROMS
	 * require 32 kB
	 *
	 * May be suppressed if code built with EBUG flag
	 */
#ifndef EBUG
	if (0x8000 < (long)SERNO)
		{
		FL_CONS->port = 0;
		FL_CONS->device = 0;
		}
#endif
	}
else	/* set defaults if NVRAM read failed */
	{
	FL_CONS->device = 0;
	FL_CONS->port = 0;
	}

/*
 * This test on FLOAT is used to suppress the floating console search for
 * all cases until both the 3B2 firmware and software support floating
 * console.  This test and the contained code must be removed at that time.
 */
#ifndef FLOAT			/* FLOAT test */
FL_CONS->device = 0;		/* FLOAT test */
FL_CONS->port = 0;		/* FLOAT test */
#endif				/* FLOAT test */

/* test for console at slot & port values stored in FL_CONS */
if (prev_cons() == PASS)
	return (PASS);

/*
 * Suppress search of non-default console values for DEMON PROMS
 * Test based on DEMON requiring 64 kB while normal PROMS
 * require 32 kB
 *
 * May be suppressed if code built with EBUG flag
 */

#ifndef EBUG
	if (0x8000 < (long)SERNO)
		{
		/* return a FAIL since console not found on system board as expected */
		return(FAIL);
		}
#endif

/*
 * This test on FLOAT is used to suppress the floating console search for
 * all cases until both the 3B2 firmware and software support floating
 * console.  This test and its "else" code must be removed at that time.
 */
#ifdef FLOAT			/* FLOAT test */

/* search for console; return PASS/FAIL report to calling routine */
return(search());

#else				/* FLOAT test */
return(FAIL);			/* FLOAT test */
#endif				/* FLOAT test */
}

/* function to test for console terminal at saved slot & port #'s.	*/
/* console values taken from FL_CONS structure.				*/
/* FL_CONS' cons_found flag set if a terminal is found.			*/

prev_cons()
{
unsigned long temp,ent_num,slot,port;

/* short-hand for console slot and port # */
slot = FL_CONS->device;
port = FL_CONS->port;

/* find EDT entry # for device slot # */
for (ent_num = 0; ent_num < NUM_EDT; ent_num++)
	{
	if (EDTP(ent_num)->opt_slot == slot)
		break;
	}
/* debug */
/* PRINTF("ent_num = %d, slot = %d, port = %d\n",ent_num,slot,port); */

if (slot == EDTSBD)
	/* test for console terminal on system board */
	/* DUART programmed by initchan() in firmware */
	{
	temp = IDUART->ip_opcr;
	if (port == 0 && (~temp & DCDA))
		{
		/* set console found flag if correct DCD present */
		FL_CONS->cons_found = ON;
		CONSOLE = (struct duart *)CONS;
/* debug */
/* PRINTF("console found on SBD port %d\n",port); */
		return(PASS);
		}

	else	if (port == 1 && (~temp & DCDB))
		{
		/* set console found flag if correct DCD present */
		FL_CONS->cons_found = ON;
		CONSOLE = LINK;
/* debug */
/* PRINTF("console found on SBD port %d\n",port); */
		return(PASS);
		}

	else	return(FAIL);	/* console terminal not on SBD as expected */
	}

	/* try specified peripheral device */
if (EDTP(ent_num)->cons_cap == OFF ||
	FW_SYSGEN(slot) == FAIL)
	return(FAIL);	/* board can't support console or SYSGEN failed */

else	{
	/* SYSGEN passed and board supports console, according to EDT */
	if (EDTP(ent_num)->cons_file == ON)
		{
		/* construct pump file path name */
		strcpy(Dirname,"dgn/C.");
		strcpy(filename,EDTP(ent_num)->dev_name);
		strcat(Dirname,filename);

		if (fw_pump(slot,Dirname) == FAIL)
			{
			/* pump failed; reset board */
			SL_RESET(ent_num) = 0;
/* debug */
/* PRINTF("console pump failed for %s at slot %d\n",EDTP(ent_num)->dev_name,slot); */
			return(FAIL);
			}
		}
	if (fw_dcons(slot,port) == PASS)
		{
		FL_CONS->cons_found = ON;	/* set console found flag; device can support console */

		save_cons();			/* save console values in NVRAM */
/* debug */
/* PRINTF("console found on slot %d, port %d\n",slot,port); */
		return(PASS);
		}
	else	{
		/* terminal not found on device */
/* debug */
/* PRINTF("DCONS failed on slot %d, port %d\n",slot,port); */
		SL_RESET(ent_num) = 0;
		return(FAIL);
		}
	}
}

/* function to search for console terminal.				*/
/* default console values defined in code.				*/
/* FL_CONS' cons_found flag set if a terminal is found.			*/

search()
{
unsigned long temp,ent_num,slot,port;

	FL_CONS->cflags = CS8 | HUPCL | B9600 | CREAD;	/* define default cflags */
	/* test for console terminal on system board */
	FL_CONS->device = 0;

/* debug */
/* PRINTF("console search routine\n"); */

	temp = IDUART->ip_opcr;
	if (~temp & DCDA)
		{
		/* set console found flag if "console" DCD present */
		FL_CONS->cons_found = ON;
		FL_CONS->port = 0;
		CONSOLE = (struct duart *)CONS;

		/* program "console" channel with default console cflags */
		dflt_init((struct duart *)CONS);

		save_cons();			/* save console values in NVRAM */

/* debug */
/* PRINTF("console found on SBD port %d\n",FL_CONS->port); */
		return(PASS);
		}

	if (~temp & DCDB)
		{
		/* set console found flag if "link" DCD present */
		FL_CONS->cons_found = ON;
		FL_CONS->port = 1;
		CONSOLE = LINK;

		/* program "link" channel with default console cflags */
		dflt_init(LINK);

		save_cons();			/* save console values in NVRAM */
/* debug */
/* PRINTF("console found on SBD port %d\n",FL_CONS->port); */
		return(PASS);
		}

/* search peripheral devices */

for (ent_num = 1; ent_num < NUM_EDT ; ent_num++)
	{
	slot = EDTP(ent_num)->opt_slot;
	port = 0x3f;		/* assign the search code */

	if (EDTP(ent_num)->cons_cap == OFF ||
		FW_SYSGEN(slot) == FAIL)
		continue;	/* board can't support console or SYSGEN failed */

	else	{
		/* SYSGEN passed and board supports console, according to EDT */
		if (EDTP(ent_num)->cons_file == ON)
			{
			/* construct pump file path name */
			strcpy(Dirname,"dgn/C.");
			strcpy(filename,EDTP(ent_num)->dev_name);
			strcat(Dirname,filename);

			if (fw_pump(slot,Dirname) == FAIL)
				{
				/* pump failed; reset board */
				SL_RESET(ent_num) = 0;
/* debug */
/* PRINTF("pump failed at slot %d\n",slot); */
				continue;
				}
			}
		if (fw_dcons(slot,port) == PASS)
			{
			/* set console found flag; device can support console */
			FL_CONS->cons_found = ON;
			FL_CONS->device = slot;

			save_cons();		/* save console values in NVRAM */

/* debug */
/* PRINTF("console found on slot %d, port %d\n",slot,port); */
			return(PASS);
			}
		else	{
			/* terminal not found on device */
/* debug */
/* PRINTF("DCONS failed on slot %d, port %d\n",slot,port); */
			SL_RESET(ent_num) = 0;
			continue;
			}
		}
	}

/* console terminal not found; use default values */

FL_CONS->device = 0;	/* system board */
FL_CONS->port = 0;	/* "console" port */
CONSOLE = (struct duart *)CONS;
return(FAIL);
}

/*
 *	dflt_init() - reset the channel and initialize the baud rate,
 *	character size, parity, and stop bit(s), according to default
 *	cflags 
 */


void	
dflt_init(channel)
struct	duart	*channel;
{
	register unsigned short i;

	channel->a_cmnd = RESET_MR | DIS_TX | DIS_RX;
	channel->a_cmnd = RESET_RECV;
	channel->a_cmnd = RESET_TRANS;
	channel->a_cmnd = RESET_ERR;
	channel->a_cmnd = STOP_BRK;

	channel->mr1_2a = CHAR_ERR | NO_PAR | BITS8;

	channel->mr1_2a = NRML_MOD | ONESB;

	channel->a_sr_csr = B9600BPS;
	channel->ipc_acr = B9600ACR;

	channel->a_cmnd = RESET_MR | ENB_TX | ENB_RX;

	/* turn on DTR */
	IDUART->scc_sopbc = DTRA | DTRB;

	HWCNTR(1);	/* delay 10 msec before sending space */
	channel->a_data = ' ';
}

/*
 * encode slot, port data; write it and cflags in nvram in case peripheral device
 * changed cflags value or port #
 */

save_cons()
{
char i;

i = ((FL_CONS->device << 4) & 0xf0) | (FL_CONS->port & 0xf);

WNVRAM(&i,&FW_NVR->cons_def,sizeof(FW_NVR->cons_def));
WNVRAM(&FL_CONS->cflags,&UNX_NVR->consflg,sizeof(UNX_NVR->consflg));
}
