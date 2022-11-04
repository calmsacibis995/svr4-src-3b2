/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/edt_fill.c	1.5"



#include <sys/types.h>
#include <sys/sbd.h>
#include <sys/diagnostic.h>
#include <sys/edt.h>
#include <sys/firmware.h>
#include <edt_def.h>
#include <sys/extbus.h>
#include <sys/boot.h>
#include <sys/inode.h>
#include <sys/lboot.h>
#include <sys/dsd.h>
#include <sys/vtoc.h>



DEV_TAB *p_devt;	/* lookup table for edt entries */

SBDEV_TAB *p_sbdevt;	/* subdevice look up table */

unsigned long dsd_addr;		/* address for dsd_poll structure */



extern struct inode Fndinode;	/* inode structure for file to load from disk */
extern syg();		/* routine to sysgen peripheral devices and
			 * collect sub device data for the EDT */


unsigned char filename[E_NAMLEN];	/* name of file */
unsigned char Dirname[DIRLEN];		/* name of directory */


#define BSIZE 512
#define IMEG		(1024*1024)

long fillscanf();



edt_fill()
{
unsigned long i;
long conv, rtn_val;

DSD_POLL *dsd_poll;		/* structure for DSD command */
unsigned long board;		/* temporary value for device slot value */
unsigned long tmp_start;	/* temporary value for STRTOFMEM */

char *ptr;			/* string pointer for device renaming table */

	/* structure for boards to be renamed */
struct	{
	long slot;		/* slot number for board */
	char swname[E_NAMLEN];	/* software name of device */
	char hwname[E_NAMLEN];	/* hardware name of device */
	} rename[MAX_IO + 1];


	OPTPTR = (long)(&board);	/* set global pointer to entry */
	rtn_val = PASS;		/* assign initial return value */

	/* set value of pointers for device, subdevice tables
	 * and dsd structure
	 */

	p_devt = (DEV_TAB *)DOWNADDR;
	p_sbdevt = (SBDEV_TAB *)(DOWNADDR + sizeof(DEV_TAB));

	dsd_addr = DOWNADDR + sizeof(DEV_TAB) + sizeof(SBDEV_TAB);
	dsd_poll = (DSD_POLL *)dsd_addr;

	/* set name of "phase" to lookup table 
	 * and find directory inode */
	strcpy(Dirname,"dgn/edt_data");

	if ( findfile(Dirname) == NOTFOUND)
		/* EDT cannot be completed.  Only SBD entries made by MCP
		 * are available
		 */
		{
		/* print error message */
		fillerror(2);
		return(FAIL);
		}
	/* Load lookup table */
	if ( loadfile(Fndinode,DOWNADDR) == LDFAIL)
		/* EDT cannot be completed.  Only SBD entries made by MCP
		 * are available
		 */
		{
		/* print error message */
		fillerror(3);
		return(FAIL);
		}




	/* Work through the equipped peripheral slots to complete the EDT.
	 * System board entry partially completed by self-configure firmware,
	 * will be redone completely from lookup table.
	 */

	STRTOFMEM = (unsigned long)(P_EDT) + NUM_EDT * sizeof(struct edt)
		+ EDTP(0)->n_subdev * sizeof(struct subdevice);


	for (board = 0; board < NUM_EDT; board++)
		{
	/* Complete device entries by comparing device codes */
		if (!cmp_dev(board))
			{
			/* A device code failed to match.
	 		* Complete EDT as possible.
	 		*/
			fillerror(8);
			}


	/* Determine option number for board by checking earlier EDT
	 * entry opt_codes
	 */

		EDTP(board)->opt_num = 0;
		for (i = 0; i < board; i++)
			if (EDTP(board)->opt_code == EDTP(i)->opt_code)
				EDTP(board)->opt_num++;


	/* gather subdevice codes for peripheral units only */

		if (board != 0)
			{
			dsd_poll->num_sbdev = 0;
			EDTP(board)->n_subdev = 0;

			/* sysgen only smart peripheral boards */

			if (EDTP(board)->smrt_brd == E_SMART && !syg(board))
				{
				/* Subdevice gathering request failed; set return value.
			 	* Complete EDT as possible.
			 	*/

				rtn_val = FAIL;
				}

			/* copy device's subdevice count to EDT */

			EDTP(board)->n_subdev = dsd_poll->num_sbdev;

			if ( EDTP(board)->n_subdev != 0)
				{
				/* set subdevice structure pointer to current
				 * end of bss
				 * STRTOFMEM will be increased by the size of
				 * subdevice entries to the EDT */

			 	(EDTP(board)->subdev) = (struct subdevice *)STRTOFMEM;

				tmp_start = STRTOFMEM;
				tmp_start += EDTP(board)->n_subdev * sizeof(struct subdevice);

				/* Will STRTOFMEM move into usr ISP range? */

				if (tmp_start > BOOTADDR - 0x1000) /* user ISP macro needed */
					{
					/* Entering subdevice data will corrupt user ISP
					 * Return with failure
					 */
					fillerror(10);
					return(FAIL);
					}

				STRTOFMEM = tmp_start;

				/* copy subdev codes into EDT */
				for (i = 0; i < EDTP(board)->n_subdev; i++)
					{
					EDTP(board)->subdev[i].opt_code = dsd_poll->sbdev_code[i];
					}

				}
			else

			/* No subdevices.  Set pointer to zero. */

				EDTP(board)->subdev = (struct subdevice *)0;
		}

		/* match subdevice codes to complete edt */

		for (i=0; i< EDTP(board)->n_subdev; i++)
			{
			/* scan subdevice lookup table for each subdevice */

			if (!cmp_sbdev(board,i))
				{
			/* Code failed to match.  Print message & proceed */
				fillerror(9);
				}
			}
	}

	/*
	 * EDT now complete with hardware names for devices;
	 * next rename these devices, if necessary
	 */

	/* set name to device renaming table and find directory inode */
	strcpy(Dirname,"dgn/.edt_swapp");

	if ( findfile(Dirname) == NOTFOUND)
		/* EDT cannot be renamed; only hardware entries are available */
		{
		/* set "name" to "software application file" to avoid printing true path name */
		strcpy(Dirname,"software application file");
		/* print error message */
		fillerror(2);
		return(rtn_val);
		}

	/* Load device renaming table if it isn't zero length */

	if (Fndinode.i_size == 0)
		return(rtn_val);

	if ( loadfile(Fndinode,DOWNADDR) == LDFAIL)
		/* EDT cannot be renamed; only hardware entries are available */
		{
		/* set "name" to "software application file" to avoid printing true path name */
		strcpy(Dirname,"software application file");
		/* print error message */
		fillerror(3);
		return(rtn_val);
		}

	/* scan the table for devices to rename */

	i = 0;
	ptr = (char *)DOWNADDR;
	*(ptr + Fndinode.i_size) = '\0';	/*
						 * end string with a null using
						 * the inode's file size datum
						 */

	/* scan input lines until the conversion count is 0 */

	while((conv = fillscanf(ptr, "%D %s %s",
		&rename[i].slot,rename[i].swname,rename[i].hwname)) != 0)
		{
		/* were there 3 entries on the line? */

		if (conv != 3)
			{
			/* we have a bad line; print message & go to next line */
			board = rename[i].slot;
			fillerror(13);
			}
		else	{
			/* scan EDT entries for slots that match rename table entries */

			for (board = 0; board < NUM_EDT; board++)
				{
				if (rename[i].slot == EDTP(board)->opt_slot)
					{
					if (!STRCMP(rename[i].hwname,EDTP(board)->dev_name))
						{
						/*
						 * Truncate swnames that exceed defined length.
						 * Edittbl takes care of hwnames.
						 */
						rename[i].swname[E_NAMLEN - 1] = '\0';
						strcpy (EDTP(board)->dev_name,rename[i].swname);
						}

					else	{
						/* EDT device name doesn't match hw name in table */
						fillerror(11);
						}

					/* stop scanning the EDT for this table entry */
					break;
					}
				}

			if (board >= NUM_EDT)
				{
				/*
				 * EDT scan failed to match this slot # in the table.
				 * Set EDT entry # to slot # from table for error message.
				 */
				board = rename[i].slot;
				fillerror(12);
				}
			}

		/*
		 * Increment table index and leave loop if file has more
		 * than MAX_IO + 1 lines
		 */
		if (++i > MAX_IO)
			break;

		nextline(&ptr);		/* move to the next "line" of the table */
		}

	/*
	 * Recalculate option numbers for boards based on software names
	 * instead of ID codes by checking EDT device names.
	 */

	for (board = 0; board < NUM_EDT; board++)
		{
		/* skip EDT device entry if device name is "*VOID*" */
		if (!STRCMP(EDTP(board)->dev_name,"*VOID*"))
			continue;

		EDTP(board)->opt_num = 0;
		for (i = 0; i < board; i++)
			/* increment option counter if device names match */
			if (!STRCMP(EDTP(board)->dev_name, EDTP(i)->dev_name))
				EDTP(board)->opt_num++;
		}

	return(rtn_val);
}

#define BOOTPDINFO ((struct pdinfo *) (BOOTADDR + 2*BSIZE))
#define BOOTVTOC ((struct vtoc *) (BOOTADDR + BSIZE))


/*
 * fill extended edt and write to boot block
 */

int bootstartaddr;

xbusgen()
{

	static long XMAJNUM = 127;
	long startsector;
	int	(*uedtgen)();
	char path[100];
	register char *p;
	static char edtname[] = {"/edt/"}, gename[] = {"edtgen"};
	int i,xcount;
	long *xhead,*sanity;
	B_EDT *xedtp;



	xcount = 0;
	
	sanity = (long *) XEDTSTART;
	xhead = (long *) XEDTSTART + 1;
	xedtp = (B_EDT *)(char *)(XEDTSTART + sizeof(long) * 15);

	startsector = BOOTVTOC->v_part[7].p_start + BOOTVTOC->v_part[7].p_size - XEDTSIZE - 1;

	for(i=0; i < NUM_EDT; i++){

		if (EDTP(i)->indir_dev ){
			if (SIZOFMEM < IMEG){ 
                                PRINTF("Need 1 meg minimum for host adaptor\n");                                        /* need a minimum of 1 meg to configure
                                        * HA device, TC under this HA will be
                                        * ignored.
                                        */
                                return;
			}

		p = path;		/* edtgen file */
		strcat(strcat(strcpy(p, edtname), EDTP(i)->dev_name), "/");
		strcat(p, gename);

		if ( findfile(p) == NOTFOUND){
			fillerror(2);
			return(FAIL);
		}

		if (loadprog(Fndinode) == LDFAIL){
			fillerror(3);
			return(FAIL);
		}

		uedtgen = (int (*)())bootstartaddr;

		(*uedtgen)(i,XMAJNUM,xedtp);

		XMAJNUM -= xedtp->max_tc;
		xhead[xcount++]= (long)xedtp - XEDTSTART;
		xedtp = (B_EDT *)((char *)xedtp + sizeof(B_EDT) + xedtp->max_tc * sizeof(B_TC));

		}
	}
	if (xcount > 0){
		*sanity = XEDTMAGIC;
		xhead[xcount] = (long) 0;
		copytoboot(startsector,XEDTSTART,xedtp);
	}
}



/*
 * copytoboot- copies extended edt to boot section.
 *
 */

copytoboot(startsector,first,last)
long startsector,first,last;
{

	register mp;
	register sect;
	register numsect,i;


	sect = PHYS_INFO[P_CMDQ->b_dev - HARDDISK0].logicalst;
	sect += BOOTVTOC->v_part[7].p_start + startsector;
	mp =first;
	numsect = (last - first + (BSIZE-1))/BSIZE;

	 for (i=0; i< numsect;i++)
		if (wndisk_acs(sect,mp)){
			mp += BSIZE;
			sect++;
		}
		else {
			PRINTF("Extended equipped device table not complete\n");			return;
		}
}




/* routine to complete device entries in the EDT based on matches
 * between the device codes Self-Config. puts in the EDT and the
 * ones in the lookup table */



cmp_dev(board)
unsigned long board;	/* dummy variable for device slot number */
{
unsigned char i;	/* loop variable */


	/* search table for a code match */
	for (i = 0; i < (p_devt->num_dev); i++) {
		if (p_devt->dev_code[i].opt_code ==
			EDTP(board)->opt_code) {

			EDTP(board)->rq_size = p_devt->dev_code[i].rq_size;
			EDTP(board)->cq_size = p_devt->dev_code[i].cq_size;
			EDTP(board)->boot_dev = p_devt->dev_code[i].boot_dev;
			EDTP(board)->word_size = p_devt->dev_code[i].word_size;
			EDTP(board)->smrt_brd = p_devt->dev_code[i].smrt_brd;
			EDTP(board)->cons_cap = p_devt->dev_code[i].cons_cap;
			EDTP(board)->cons_file = p_devt->dev_code[i].cons_file;
			EDTP(board)->indir_dev = p_devt->dev_code[i].indir_dev;


			strcpy(EDTP(board)->dev_name,
				p_devt->dev_code[i].dev_name);
			strcpy(EDTP(board)->diag_file, EDTP(board)->dev_name);
			break;
		}
	}
	/* test for no match */
	if (i == p_devt->num_dev)
		{
		strcpy(EDTP(board)->dev_name,"*VOID*");
		return(FAIL);
		}
	return(PASS);
}


/* routine to complete subdevice entries for device in EDT by matching
 * codes with subdevice lookup table */



cmp_sbdev(board,j)
unsigned long board;	/* entry number in EDT */
unsigned char j;	/*  index to EDT subdevice entry */
{

unsigned char i;	/* dummy loop variable */

	/* loop through all values in the lookup table */


for (i=0; i < p_sbdevt->num_sbdev; i++)
	{
	/* compare lookup values to codes returned by DSD command */

	if (p_sbdevt->sbdev_code[i].opt_code
		== EDTP(board)->subdev[j].opt_code)
		{
		/* subdevice code matches; copy subdevice name */

		strcpy(EDTP(board)->subdev[j].name,
			p_sbdevt->sbdev_code[i].name);

		break;
		}
	}

/*
 * If the index ran through the subdevice look-up table and if it's on a
 * bootable device without an indirect edt, assume the ID code belongs to
 * an unlisted disk drive.  Try to construct a subdevice name.
 */

if (i >= p_sbdevt->num_sbdev)
	{
	if ((EDTP(board)->boot_dev == ON) && (EDTP(board)->indir_dev == OFF))
		{
		if (mkname(j) == PASS)
			{
			return(PASS);
			}
		}
	strcpy(EDTP(board)->subdev[j].name,"*VOID*");
	return(FAIL);
	}
return(PASS);
}

long fillscanf(string,format,args)
unsigned char *string,*format;
unsigned char *args;
{

extern unsigned char convert();
register unsigned char *temptr;
register long tp, buffer, cnv;
long digit;
unsigned char temp[10];
char *strstrt;			/* pointer to beginning of input string */
unsigned char **retptr;   /* a pointer to the first location on the stack */

retptr= &args;             /* set to the location first on the stack */

cnv = 0;                   /* start with no successful conversions   */

/* stay in this loop until out of format items */
while(*format != '\0'){

	tp = 0;
	while(*format != '%'){   /* find the first format article */
		if(*format == '\0'){    /* check for end of format */
			tp = 1;		/* set as exit flag of format loop */
			break;
		}
		format++;        /* point to format letter after the % */
	}

	if(tp)
		break;		/* leave format scanning loop; return */

	/* skip leading delimiters */
	while((*string == ' ') || (*string == '\t') || (*string == ',') ||
		(*string == '-') || (*string == '='))
			string++;

	/* seek the right format for the data */
	switch(*++format){

		/* string format */
		case 's':
			temptr = string;
			advance(&string);

			if(temptr != string) {
				cnv++;

				/* copy string to argument */
				while(temptr != string){
					(*((unsigned char *)*retptr)) = *temptr++;
					(*retptr)++;
				}
			/* terminate string */
			(*((unsigned char *)*retptr)) = '\0';

			} else {
				tp = 1;		/* set as exit flag of format loop */
			}

			break;

		/* character format */
		case 'c':
			if (*string != 0) {
				*((unsigned char *)*retptr) = *string;
				string++;
				cnv++;
			} else {
				tp = 1;		/* set as exit flag of format loop */
			}

			break;

		/* short or long hex format */
		case 'x':
		case 'X':
			/* set the hex to zero */
			for(tp = 0;tp < 8;tp++)
				temp[tp] = '0';

			temptr = string;  /*set temporary ptrs for conversion */

			/* find end of target */
			advance(&string);

			if(temptr != string) {
				cnv++;

				string--;

				/* copy to working storage */
				for(tp = 7;tp >= 0;tp--) {
					temp[tp] = *string;
					if(string == temptr)
						break;
					string--;
				}

				/* convert ascii to hex */
				buffer=0;
				for(tp=7;tp>=0;tp--){
					buffer |= convert(temp[7 - tp])<<(tp<<2);
				}

				/* back to end of target */
				advance(&string);
			
				/* place in the argument */
				if(*format=='x')
					(*((unsigned short *)*retptr))=
						(unsigned short)buffer;
				else
					(*((unsigned long *)*retptr))= buffer;
			} else {
				tp = 1;		/* set as exit flag of format loop */
			}
			break;
			

		/* short or long decimal */
		case 'd':
		case 'D':
			/* set the decimal to zero */
			for(tp = 9;tp >= 0;tp--)
				temp[tp] = '0';

			temptr = string;  /*set temporary ptrs for conversion */

			/* find end of target */
			advance(&string);

			if(temptr != string) {
				cnv++;

				string--;

				/* copy to working storage */
				for(tp = 9;tp >= 0;tp--) {
					temp[tp] = *string;
					if(string == temptr)
						break;
					string--;
				}

				/* convert ascii to decimal */
				buffer=0;
				/* if non-decimal input: decrement conversion count & set exit flag */
				if ((digit = temp[0] - '0') < 0 || digit > 9)
					{
					cnv--;
					tp = 1;
					break;
					}

				buffer = buffer + digit;
				for(tp = 1;tp < 10;tp++) {
					if ((digit = temp[tp] - '0') < 0 || digit > 9)
						{
						cnv--;
						buffer = 0;
						tp = 1;
						break;
						}
			 		buffer *= 10;
					buffer = buffer + digit;
				}

				/* back to end of target */
				advance(&string);

				/* check for negative #'s by scanning white space in front of string */
				digit = 1;
				while ((strstrt <= (char *)(temptr - digit)) &
					((*(temptr - digit)==' ')|(*(temptr - digit)=='\t')|
					(*(temptr - digit)==',')|(*(temptr - digit)=='=')|
					(*(temptr - digit)=='\n')|(*(temptr - digit)=='-')))
					{
					/* look for "-" */
					if (*(temptr - (digit++)) == '-')
						{
						buffer = -buffer;
						break;	/* got a "-"; stop looking */
						}
					}

				/* place in the argument */
				if(*format=='d')
					(*((unsigned short *)*retptr))=
						(unsigned short)buffer;
				else
					(*((unsigned long *)*retptr))= buffer;

			} else {
				tp = 1;		/* set as exit flag of format loop */
			}
			break;


		default:
			tp = 1;		/* set as exit flag of format loop */
			break;
	}

	/* go to next return argument */
	retptr++;
}
return(cnv);
}

/* function to move string pointer to end of the current argument */

advance(ptr)
char **ptr;
{
while((**ptr!=' ')&(**ptr!='\t')&(**ptr!='-')&(**ptr!=',')&
	(**ptr!='\0')&(**ptr!='=')&(**ptr!='\n'))
	(*ptr)++;
}

/* function to move string pointer to end of the current line */

nextline(ptr)
char **ptr;
{
while((**ptr!='\0')&(**ptr!='\n'))
	(*ptr)++;
(*ptr)++;
}

/* convert ascii character to hex, no error checking is done to save space */

unsigned char convert(c)
unsigned char c;
{

if((c >= '0') & (c <= '9'))
	return(c - '0');

else if(c >= 'a')
	return(c - 'a' + 10);

else
	return(c - 'A' + 10);
}

/*
 * Routine to construct names of subdevices for bootable devices
 * if the subdevices are not in the edt_data look-up table
 */

#include <sys/iobd.h>

/* define boot device's VTOC & pdinfo locations after MBOOT code */

#define BLKSIZE 0x200
#define MYPDINFO ((struct pdinfo *)(BOOTADDR + 2 * BLKSIZE))


mkname(sbdevnum)
unsigned char sbdevnum;		/* subdevice # */
{
struct pdinfo *sdvinfo;		/* ptr to pdinfo structure */
long size;			/* formatted disk capacity, Mb */
char str[40];			/* temp storage for string conversion */
char majmin;			/* combined major/minor, slot/subdevice # */
char slot;			/* slot # for device in question */


slot = EDTP(OPTION)->opt_slot;	/* assign for convenience */
majmin = ((slot & 0x0F) << 4) | (sbdevnum & 0x0F);

/* find address for subdevice pdinfo */

switch (majmin)
	{
	case FLOPDISK:
		/* integral floppy disk - we should not reach here */
		return(FAIL);
		break;

	case HARDDISK0:
	case HARDDISK1:
		/* save integral subdevice pdinfo start addr */
		sdvinfo = &(PHYS_INFO[majmin - HARDDISK0]);
		break;

	default:
		/* save peripheral subdevice pdinfo start addr */
		if (P_CMDQ->b_dev == majmin)
			/* use pdinfo copy for boot device - see setup function */
			sdvinfo = MYPDINFO;

		else	{
			/* use mboot space for non-boot pdinfo copy */

			sdvinfo  = (struct pdinfo *)(BOOTADDR);

			/* SYSGEN non-boot peripheral devices */

			if (P_CMDQ->b_dev >> 4 != slot)
				{
				/* reset peripheral before SYSGEN to be safe */
				IO_SMART(slot)->reset = 0;
				if (FW_SYSGEN(slot) == FAIL)
					{
					/* SYSGEN failed */
					return(FAIL);
					}
				}

			/* read pdinfo from non-boot peripheral subdevice */
			IOBLK_ACS(majmin, 0, BOOTADDR, BLKRD);

			/* reset non-boot peripheral after pdinfo read */

			if (P_CMDQ->b_dev >> 4 != slot)
				IO_SMART(slot)->reset = 0;
			}
		break;
	}

/* check pdsector's sanity word */

if (sdvinfo->sanity != VALID_PD)
	{
	return(FAIL);
	}

/* calculate formatted capacity in Mb */

size = (sdvinfo->cyls * sdvinfo->tracks * sdvinfo->sectors * sdvinfo->bytes) >> 20;

/*
 * build subdevice name - HDnnn-xxx
 *	nnn = formatted disk capacity in Mb
 *      xxx = 3 least significant hex digits of subdevice ID code
 */

strcpy(EDTP(OPTION)->subdev[sbdevnum].name, "HD");
swap(size, str, 10);	/* convert disk size to char. string */

strcat(EDTP(OPTION)->subdev[sbdevnum].name, str);
strcat(EDTP(OPTION)->subdev[sbdevnum].name, "-");

/* convert disk ID code to string */
swap((EDTP(OPTION)->subdev[sbdevnum].opt_code & 0xFFF), str, 0x10);
strcat(EDTP(OPTION)->subdev[sbdevnum].name, str);

return(PASS);
}

/* routine to convert numbers to ACSII strings with requested number base */

swap(number, str, base)
long number;	/* value to convert to ASCII */
char *str;	/* string for converted number */
char base;	/* number base for conversion */
{
long offset;

/* make one call of swap for each digit in number */

if (number / base > 0)
	{
	offset = swap(number / base, str, base);
	}
else	{
	offset = 0;
	}

/* convert digits to character, minding the base */

if ((number % base) > 9)
	str[offset] = number % base -10 + 'A';
else
	str[offset] = number % base + '0';

str[++offset] = 0;	/* clear char at current end of string */

return (offset);	/* return current end of string */
}
