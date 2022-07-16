/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fltboot:fltboot.c	1.5.2.1"

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/psw.h>
#include <sys/edt.h>
#include <sys/sys3b.h>
#include <sys/sbd.h>
#include <sys/csr.h>
#include <sys/firmware.h>
#include <sys/boot.h>
#include <sys/immu.h>
#include <sys/nvram.h>

struct load_dev {		/* declaration of local load device table */
	unsigned short edti;
	unsigned short ibd;
	unsigned short slot;
	char name[E_NAMLEN];		/* E_NAMLEN is defined in edt.h */
};
#define NUMDEVS 19	/* allow for 16 slots + FD5 and 2 hard disks */
struct load_dev ld_dev[NUMDEVS];

#define FALSE 0
#define TRUE 1
#define NEDTP(X) (nedtp + X)

extern int errno;
void wrapup();

struct nvparams	nvparams;
struct fw_nvr fw_nvr;	

main()
{
	char buf[80];
	char *a;
	int indx, subdev;
	int bootdev;
	int num_dev;
	int edtsize, size;
	int i;
	int s;
	int def;
	struct edt *nedtp;

	/* Preliminary read of edt to get size information.
	 * The number of devices is in upper 16 bits, 
	 * the number of subdevices is in lower 16 bits.
	 */

	sys3b(S3BEDT, &edtsize, sizeof(int));

	num_dev = edtsize >> 16;

	size = num_dev * sizeof(struct edt) + 
		(edtsize & 0xffff) * sizeof(struct subdevice) + sizeof(int);

	/* allocate amount of space needed as determined above */

	nedtp = (struct edt *)(malloc(size));

	sys3b(S3BEDT, nedtp, size);

	/* Bump nedtp past size info, to point at EDT info. */
	nedtp = (struct edt *) ((char *)nedtp + sizeof(int));

	/* read nvram to get current autoload program data */

	nvparams.addr = (char *)(ONVRAM+FW_OFSET);
 	nvparams.data = (char *)&fw_nvr;
	nvparams.cnt = sizeof(struct fw_nvr);

	sys3b(RNVR, &nvparams, 0);
	if ( errno != 0 ) {
		printf("ERROR: Not invoked by superuser.\n");
		exit(1);
	}

	printf("\nEnter name of default program for manual load [ %s ]: ",
		fw_nvr.b_name);
	a = gets(buf);

	/* if user enters q or Q or EOF, return back to simple admin */

	if ( (a == NULL) || (*a == 'q') || (*a == 'Q') ) {
		printf("\n");
		exit(0);
	}

	/* check for a carriage return without any data, 
	 * If the user really wants to clear nvram they should 
	 * enter a space char. prior to return.
	 */

	if (*a == NULL) {
		printf("NULL response detected, current value will be retained.\n");
		printf("To clear value, enter space before return.\n");
	} else {
		if ( strcmp(a," ") == 0 ) 
			*a = NULL;
		strcpy(fw_nvr.b_name, a);
	}
	
	/* initialize local load device table */

	for ( i = 0; i < NUMDEVS; i++ ) {
		ld_dev[i].edti = 0;
		ld_dev[i].ibd = 0;
		ld_dev[i].slot = 0;
		ld_dev[i].name[0] = '\0';
	}

	/* The following overwrites the kernel physical addresses
	 * returned by the sys3b call into virtual addresses into
	 * our copy of the table.
	 */

	NEDTP(0)->subdev = (struct subdevice *) 
		((char *)nedtp + num_dev * sizeof(struct edt));
	for ( i = 1; i < num_dev; i++ ) {
		NEDTP(i)->subdev = (struct subdevice *) 
			((char *)NEDTP(i-1)->subdev +
			NEDTP(i-1)->n_subdev * sizeof(struct subdevice));
	}

	indx = 0;

	/* floppy is always presumed present */

	strcpy(ld_dev[indx].name,NEDTP(0)->subdev[0].name);
	ld_dev[indx].ibd = TRUE;
	ld_dev[indx++].edti = FLOPDISK;

	if ( ( NEDTP(0)->n_subdev) > 1 ) {
		strcpy(ld_dev[indx].name,NEDTP(0)->subdev[1].name);
		ld_dev[indx].ibd = TRUE;
		ld_dev[indx++].edti = HARDDISK0;
	}
	if ( ( NEDTP(0)->n_subdev) > 2) {
		strcpy(ld_dev[indx].name,NEDTP(0)->subdev[2].name);
		ld_dev[indx].ibd = TRUE;
		ld_dev[indx++].edti = HARDDISK1;
	}

	/* scan EDT for bootable peripherals */

	for ( i = 1; i < num_dev; i++ ) {
		if ( NEDTP(i)->boot_dev ) {     /* boot device if set */
			strcpy(ld_dev[indx].name,NEDTP(i)->dev_name);
			ld_dev[indx].ibd = FALSE;
			ld_dev[indx].slot = NEDTP(i)->opt_slot;
			ld_dev[indx++].edti = i;
		} else if ( (NEDTP(i)->dev_name[0] == '\0') || 
			  (!strcmp(NEDTP(i)->dev_name,"*VOID*")) ) {

			/* assume boot device if name is null or 
			 * "*VOID*". ld_dev[i].name is left NULL
			 */
			ld_dev[indx].ibd = FALSE;
			ld_dev[indx].slot = NEDTP(i)->opt_slot;
			ld_dev[indx++].edti = i;
		}
	}

	/* print possiblities and prompt for device */

	printf("\tPossible load devices are:\n\n");
	printf("Option Number    Slot     Name\n");
	printf("---------------------------------------\n");
	for ( i = 0; i < indx; i++ ) {
		printf("      %2d         %2d",i,ld_dev[i].slot);
		if ( strcmp(ld_dev[i].name,"*VOID*") )
			printf("       %-10s", ld_dev[i].name);
		printf("\n");
	}
		
	def = 0;
	for ( i = 0; i < indx; i++ ) {
		if ( ld_dev[i].ibd ) {
			if ( fw_nvr.b_dev == ld_dev[i].edti ) {
				def = i;
				break;
			}

		} else if ( ld_dev[i].slot == (fw_nvr.b_dev >> 4) ) {
			def = i;
			break;
		}
	}


retry:
	printf("enter number corresponding to autoload device desired [ %d ]:"
		,def);

	a = gets(buf);
	if ( (a == NULL) || (*a == 'q') || (*a == 'Q') ) {
		printf("\n");
		printf("Quit request received, no load values will be changed \n");
		exit(0);
	}

	/* if the user entered a return here it could be that only the 
	 * program name is to be changed, or a subdevice of the default 
	 * autoload device should be changed.  Print default message 
	 * and continue processing.
	 */

	if ( *a == NULL ) {
		printf("NULL response detected, current value will be maintained. \n\n");
		i = def;
	} else {
		i = satoi(buf);
		if ( (i < 0) || (i >= indx) ) {
			printf("\n%s is not a valid option number.\n", buf);
			goto retry;
		}
	}

	/* If its an integral device, we have all we need. Just 
	 * call wrapup to write nvram and print closing messages.
	 */
	if ( ld_dev[i].ibd ) {
		fw_nvr.b_dev = ld_dev[i].edti;
		wrapup();
		exit(0);
	}

	/* If we get here it's a board with subdevices. 
	 * Let the user choose a subdevice.  We must shift 
	 * the slot number 4 bits to the left to combine it 
	 * with the subdevice info that will be prompted for.
	 */

	bootdev = ld_dev[i].slot << 4;

	/* if the user chose the default device,
	 * set the subdevice from the value in NVRAM.
	 */
	if ( def == i )	
		subdev = fw_nvr.b_dev & 0xF;
	else
		subdev = 0;

	i = ld_dev[i].edti;	/* edt index of chosen device */

	/* re-initialize all used entries in local device table */

	for ( s = 0; s < indx; s++ ) {
		ld_dev[s].edti = 0;
		ld_dev[s].ibd = 0;
		ld_dev[s].slot = s;
		ld_dev[s].name[0] = '\0';
	}

	/* determine possible subdevices */

	if ( (indx = NEDTP(i)->n_subdev) == 0 ) {
		indx = 16;
	} else {
		for ( s = 0; s < indx; s++ )
			strcpy(ld_dev[s].name,NEDTP(i)->subdev[s].name);
	}

	/* print possiblities and prompt for subdevice */

	printf("Possible subdevices are:\n\n");
	printf("Option Number   Subdevice    Name\n");
	printf("--------------------------------------------\n");
	for ( s = 0; s < indx; s++ ) {
		printf("      %2d          %2d", s, ld_dev[s].slot);
		if( strcmp(ld_dev[s].name,"*VOID*") )
			printf("         %-10s", ld_dev[s].name);
		printf("\n");
	}

	/* Determine if the default subdevice from NVRAM 
	 * is currently a valid device in option table.
	 * If not, make option 0 the default.
	 */

	if ( subdev < indx )
		def = subdev;
	else
		def = 0;

tryagain:
	printf("\nEnter Subdevice Option Number [%d",def);
	if ( strcmp(ld_dev[def].name,"*VOID*") )
		printf("(%s)",ld_dev[def].name);
	printf("]: ");

	a = gets(buf);
	if ( (a == NULL) || (*a == 'q') || (*a == 'Q') ) {
		printf("\n");
		printf("Quit request received.  No load values will be changed.\n");
		exit(0);
	}

	if ( *a == NULL ) {
		printf("\nNull response. Current value will be maintained.\n");
		s = def;
	} else {
		s = satoi(buf);
		if ( (s < 0) || (s >= indx) ) {
			printf("\n%s is not a valid option number.\n",buf);
			goto tryagain;
		}
	}

	def = s;

	bootdev |= def;
	fw_nvr.b_dev = bootdev;
	wrapup();
	return(0);
}


/* convert string to int - returns -1 if non decimal character is found */

satoi(s)
char *s;
{
	char c;
	int i, x;

	i = x = 0;
	while( (c = *s++) != '\0' ) {
		if( ('0' <= c) && (c <= '9') )
			x = (c - '0');
		else 
			return(-1);

		i = i * 10 + x;
	}
	return(i);
}

void
wrapup()
{
	sys3b(WNVR, &nvparams, 0);
	printf("\n LOAD PARAMETER UPDATE COMPLETE \n");
}

