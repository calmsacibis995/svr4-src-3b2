/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/getdgn.c	1.2"

/*	routine to  download standard and auxiliary files for diagnostics */

#include <sys/types.h>
#include <sys/sbd.h>
#include <sys/boot.h>
#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/diagnostic.h>
#include <sys/inode.h>
#include <sys/lboot.h>

extern void dgnerror(), sysreset();

extern struct edt edt[];        /* equipped device table structure */
extern char option[];		/* option (device) name for diag req */
extern char *strcpy(), *strcat();

/*    File Name variables    */

extern char filename[];		/*  name of phase file  */
extern char Dirname[];		/*  path name of diagnostic directory  */

/* function to load auxiliary diagnostic file from disk */

static int
ld_aux()
{
int i;		/* storage for return value from loadprog */

/* construct path name for auxiliary diagnostic file */

(void) strcat(strcpy(Dirname,"dgn/X."),filename);

/* find the inode for the auxiliary diagnostic file */

if( findfile(Dirname) == NOTFOUND)
	{
	/* file not found; print error message */
	dgnerror(2);
	return(FAIL);
	}

/* load the auxiliary diagnostic file */

if((i = loadprog(Fndinode)) == LDFAIL)
	{
	/* load failed; print error message */
	dgnerror(3);
	return(FAIL);
	}
else	if (i == BADMAGIC)	/* file has a bad magic number */
		return(BADMAGIC);

/* the file load was successful if the code has reached this point */
return(PASS);
}

/*
 * Routine to download an auxilliary diagnostic file as data,
 * using the function address of the END "phase" in the phase table
 * as the destination
 */

static int
ld_data() 
{
struct phtab *phptr;	/* pointer to phase table */
unsigned char i;
long fileaddr;		/* address for data download */

(void) strcat(strcpy(Dirname,"dgn/X."),filename);

if ( findfile(Dirname) == NOTFOUND)
	{
	/* file not found; print error message */
	dgnerror(2);
	return(FAIL);
	}

/* Scan phase table for last phase */

phptr = (struct phtab *)DOWNADDR;
fileaddr = 0;

for ( i = 1; i <= MAXPHSZ; i++)
	{
	if ((phptr+i-1)->type == END )
		{
		fileaddr = (long)((phptr+i-1)->phase);
		break;
		}
	}

/* test for absurd download addresses */

if (fileaddr < DOWNADDR || fileaddr > (MAINSTORE + SIZOFMEM))
	return(FAIL);

	if ( loadfile(Fndinode,fileaddr) == LDFAIL)
		{
		/* data down load failed; print error message */
		dgnerror(3);
		return(FAIL);
		}
return(PASS);
}

getdgn(board)
	unsigned long board;
{
int i;

	/* Return without attempting to download null or error-named files */

	if (!STRCMP(EDTP(board)->dev_name,"\0") || !STRCMP(EDTP(board)->dev_name,"*VOID*"))
		{
		dgnerror(1);	/* print error message */

		if (!STRCMP(option,"*VOID*"))	/* fail any request asking for *VOID* */
			return(FAIL);
		else
			return(NTR);		/* return NTR for slot-based requests */
		}

	else		/* At least there is a device name; try the download. */
		{
		if (setup() != PASS) {
			dgnerror(0);	/* print error message */
			sysreset();	/* file sys setup failed; reset system */
		}
		/* get name for diagnostic file */
		(void) strcpy(filename,EDTP(board)->diag_file);

		/* return a failure if either auxiliary or sbd file loads fail */
		if ((i = ld_aux()) == FAIL || ld_sbd() == FAIL)
			return(FAIL);

		/*
		 * if auxilliary file didn't have magic # for executable program
		 * try to download it as data
		 */
		else	if (i== BADMAGIC && ld_data() == FAIL)
			return(FAIL);
		}
return(PASS);
}

/* function to load sbd resident diagnostic code from disk */

ld_sbd()
{ 
/* construct path name for diagnostic file in dgn directory */

(void) strcat(strcpy(Dirname,"dgn/"),filename);

/* find the inode for the diagnostic file */

if( findfile(Dirname) == NOTFOUND)
	{
	/* file not found; print error message */
	dgnerror(2);
	return(FAIL);
	}

/* load the diagnostic file into main memory from disk */

if( loadprog(Fndinode) == LDFAIL)
	{
	/* load failed; print error message */
	dgnerror(3);
	return(FAIL);
	}

/* the file load was successful if the code has reached this point */
return(PASS);
}
