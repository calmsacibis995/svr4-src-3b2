/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/parse.c	1.2"

/* Function to parse up the input request and fill in the structure 
* request as defined in diagnostic.h.  
* dgn unit no   =    default request
* dgn unit no ucl =  dmnd on, ucl flag on
* dgn unit no rep =  rpt flag on, rept_cnt set to repeat no in decimal
* dgn unit no ph=x-y = partl flag on, beg phsz set, end phsz either
*			equal to beg phsz or next argument 
* dgn unit no soak = dmnd off, ucl flag on, rpt flag off, partl flag on 
*/

#include <sys/edt.h>
#include <sys/diagnostic.h>
#include <sys/firmware.h>
#include <edt_def.h>

extern struct request p_req;  /* diagnostic request flags */
#define P_REQ (&p_req)

#define TRUE 1
#define FALSE 0

#define KEYWORDS 13

      /* keywords for diagnostics */
static char *keywords[]={ 
		"UCL",
		"SOAK",
		"REP",
		"PH",
		"DGN",
		"H",
		"Q",
		"HELP",
		"QUIT",
		"S",
		"SHOW",
		"L",
		"LIST"
		};

char option[E_NAMLEN];		/* option name string */
unsigned char optno;		/* option number */
unsigned char opt_type;		/* flag to diagnose an option type */
unsigned char ph_set;		/* flag for PH option */
unsigned char cmnd_code;	/* code for DGN, H or Q command */
unsigned long soak_rept;	/* repeat count for soak */
char *start_ptr;		/* pointer to start of command buffer */

extern void dgnerror();
extern unsigned char beg_last;	/* flag to start diagnostics with last phase */
extern unsigned char end_last;	/* flag to end diagnostics with last phase */

/* function to skip delimiters in the input and find the next non-delimiter */
/* character in the input */

#define DELIMITS 5   /* number of delimiters */
static char delimiters[]={
		'\0',
		'\t',
		' ',
		'=',
		' '
		};

static
void
next_token(ptr,my_dlm)

char **ptr;
char my_dlm;	/* delimiter specified by calling routine */

{
char i,   /* counter variable */
found;    /* this variable is true if the element is found in the table */

	/* copy user specified delimiter to array */

	delimiters[4] = my_dlm;

		/* strip the keyword */
		found=FALSE;
		while(!found){

			(*ptr)++;

			/* Search through the table for a delimiter */
			for(i=0;((!found) & (i<DELIMITS));i++)
				if((**ptr) == delimiters[i])
					found=1;


			/* if end of string return */
			if((i==0) & found)
				return;

		}

		/* skip the delimiters */
		found=TRUE;       /* found = TRUE for entering the loop */
		while(found){
			found=FALSE;  /* set not found */

			for(i=0;((!found) & (i<DELIMITS));i++)
				if((**ptr) == delimiters[i]){

					/* if a \0 found return */
					if(i==0)
						return;
					else{
						(*ptr)++;
						found=TRUE;  /* the
								delimiter was 
								found */
					}
				}

		}

}



#define BAD 0xbad1ad
/* function to get the keyword from the parser table */
static int
getkey(ptr)
char *ptr;
{
char i;
 
	for(i=0;i<KEYWORDS;i++)
		if(!STRCMP(ptr,keywords[i]))
			return(i);

	return(BAD);
}


parse(buff)
char *buff;
{
char cmnd[15],*temp_ptr;	/* temporary command buffer */
long getout;   		 	/* an error occured, break out of loop */
unsigned char ucl_set;		/* flag for UCL option */
unsigned char soak_set;		/* flag for SOAK option */
unsigned char rep_set;		/* flag for REP option */
unsigned char dup_opt;		/* flag for redundant options */

long tmpno;			/* temporary storage for optno */
unsigned char frst_try;		/* first pass flag - option switch statement */

	/* initialize the DGN option flags and parameters */

ucl_set=OFF;
soak_set=OFF;
rep_set=OFF;
ph_set=OFF;
dup_opt=OFF;
beg_last = OFF;
end_last = OFF;

getout=FALSE;   /* no errors yet */

P_REQ->dmnd = OFF;
P_REQ->uncond = OFF;
P_REQ->rpt = OFF;
P_REQ->rept_cnt = 1;
P_REQ->beg_phsz = 1;
P_REQ->end_phsz = MAXPHSZ;
P_REQ->partl = OFF;
soak_rept = 0;

optno = 0;
opt_type = OFF;
option[0] = '\0';

	start_ptr = buff;	/* save pointer to command buffer */

	/* convert options to upper case */
	for(temp_ptr=buff;*temp_ptr!= '\0';temp_ptr++)
		*temp_ptr=toupper(*temp_ptr);

	/* check for DGN, H or Q request in first token */
	if (*buff != '\0') {
		SSCANF(buff,"%s",cmnd);

		switch (getkey(cmnd)) {	/* switch on keyword */

			case DGN:
				cmnd_code = DGN;
				break;

			case H:
			case HELP:
				cmnd_code = H;
				break;

			case Q:
			case QUIT:
				cmnd_code = Q;
				break;
			case S:
			case SHOW:
				cmnd_code = S;
				break;
			case L:
			case LIST:
				cmnd_code = L;
				break;


			default:
				cmnd_code = DGN_JUNK;
				getout = TRUE;
				/* syntax error; print error message */
				dgnerror(8);
				return(!getout);
		}
	}
	else	{
		/* command string is NULL */
		getout = TRUE;
		return(!getout);
		}

	next_token(&buff,' ');  /* skip to the next token */
	
	/* continue parsing only for DGN or phase L(ist) */
	if (cmnd_code != DGN && cmnd_code != L)
		{
		/* print error message if options are used with these commands */
		if (*buff != '\0')
			{
			getout = TRUE;
			/* syntax error; print error message */
			dgnerror(8);
			cmnd_code = DGN_JUNK;
			}
		return (!getout);
		}


	frst_try = TRUE;	/* set flag for first keyword test
				 * on possible option (device)
				 */

	/* main loop for parsing */

	while((*buff != '\0')&(!getout)){  /* while string left and no errors */
		
		SSCANF(buff,"%s",cmnd);    /* get the next option word */

		switch(getkey(cmnd)){      /* switch on the keyword index */

			case UCL:
				frst_try = FALSE; /* clear 1st try flag */

				if (ucl_set)	/* option already set? */
				{
					getout=TRUE;
					dup_opt=TRUE;
					break;
				}

				else
				{
				/* this option turns on output */
				P_REQ->uncond=ON;
				P_REQ->dmnd=ON;  /* This is a demand request */
				next_token(&buff,' ');	/* skip to next token */
				ucl_set=TRUE;	/* set ucl flag */
				}

				break;

			
			case SOAK:
				frst_try = FALSE; /* clear 1st try flag */

				if (soak_set)	/* option already set? */
				{
					getout=TRUE;
					dup_opt=TRUE;
					break;
				}

				else
				{
				/* This option has no output or input */
				P_REQ->dmnd=OFF;  /* on soak turn off demand */
				P_REQ->uncond=ON; /* turn on unconditional */
				P_REQ->partl=ON;  /* run DEMAND phases */


				next_token(&buff,' ');  /* skip to the next token */
				soak_set=TRUE;	/* set soak flag */
				cmnd_code=SOAK; /* re-define to indicate
						 * SOAK request */
				}

				break;
			
			case REP:
				frst_try = FALSE; /* clear 1st try flag */

				if (rep_set)	/* option already set? */
				{
					getout=TRUE;
					dup_opt=TRUE;
					break;
				}

				else
				{
				rep_set = TRUE;

				/* this option has no effect on output */
				P_REQ->rpt=ON;   /* repeat factor is given */
				next_token(&buff,' '); /* skip to the number */

				/* check for spurious '-' */

				if (*buff == '-')
					getout=TRUE;

				/* get the repeat count */

				SSCANF(buff,"%D",&((P_REQ->rept_cnt)));

				/* check the repeat value */

				if ((P_REQ->rept_cnt) > MAXRPT ||
					P_REQ->rept_cnt == 0)
					getout=TRUE;

				if (getout)
					{
					/* bad repeat value; print error message */
					dgnerror(9);
					}

				next_token(&buff,' ');  /* skip to the next token */
				}

				break;
			
			case PH:
				frst_try = FALSE; /* clear 1st try flag */

				if (ph_set)	/* option already set? */
				{
					getout=TRUE;
					dup_opt=TRUE;
					break;
				}

				else
				{
				/* this option turns on demand,     */
				/* interactive phases and input output */
				P_REQ->partl=ON; /* partial option is given */
				P_REQ->dmnd=ON;  /* This is a demand request */

				next_token(&buff,' '); /* skip to the number */

				/* check for spurious '-' */
				if (*buff == '-')
					getout=TRUE;

				/* get the beginning phase */

				if (*buff == '*') {
		/* if "all" alias, set end_last flag (beg_phsz and
		 * end_phsz already set to 1 and MAXPHSZ)
		 */
					end_last = ON;
					ph_set=TRUE;
					next_token(&buff,' '); /* skip to next token*/
					break;
				}

				if (*buff == '$') {
		/*  if last phase alias, set flag */
					beg_last = ON;
					end_last = ON;
				}

				else {
					SSCANF(buff,"%D",&(P_REQ->beg_phsz));

					if (P_REQ->beg_phsz == 0)
						getout=TRUE;
				}

				next_token(&buff,'-'); /* skip to next token*/

				/* is it a phase range? */

				if (*buff == '$') {
		/* if last phase alias, set flag and end_phsz */

					end_last = ON;
					P_REQ->end_phsz = MAXPHSZ;
					next_token(&buff,' ');  /* get next token */

				}

				else
				if((*buff >= '1') && (*buff <= '9')) {

					/* get the end phase */

					SSCANF(buff,"%D",&(P_REQ->end_phsz));
					next_token(&buff,' ');  /* get next token */
				}

	/* a single phase value was specified:
	 * if last phase in table not requested for start,
	 * set beginning and ending phases equal */

					else if ( !beg_last )
						P_REQ->end_phsz=P_REQ->beg_phsz;

				ph_set=TRUE;
				}

		/* check phase range limits */
				if (P_REQ->beg_phsz > MAXPHSZ ||
					P_REQ->end_phsz > MAXPHSZ ||
					P_REQ->beg_phsz > P_REQ->end_phsz)
					getout=TRUE;

				if (getout)
					{
					/* print invalid phase request error message */
					dgnerror(10);
					}


				break;

			default:

	/* treat unrecognized keywords as options (devices) only when
	 * they follow DGN keyword
	 */

				if (frst_try) {
					frst_try = FALSE;	/* clear flag */

				/* get the option name */

					SSCANF(buff,"%s",option);
				
				/* skip option name */;

					next_token(&buff,' ');

				/* check for spurious '-' */

					if (*buff == '-')
						getout=TRUE;

				/* see if a unit number has been given */

					if((*buff >= '0') && (*buff <= '9')){
						SSCANF(buff,"%D",&tmpno);
						optno = (unsigned char) tmpno;

				/* skip option number */

						next_token(&buff,' ');

				/* Diagnose a single option number */

						opt_type=OFF;

		/* Turn on I/O for single board DGN requests.  If SOAK requested
		 * flag is turned off after complete string has been parsed. */

						P_REQ->dmnd=ON;
					}

					else {

				/* diagnose option type */

						opt_type=ON;

				/* set initial option number */

						optno=0;
					}


					if (getout)
						{
						/* print invalid unit number message */
						dgnerror(7);
						}
				}

	/* report failure for keywords not in string's option position */

				else {
				/* syntax error; print error message */
				dgnerror(8);
					getout=TRUE;
				}

		}

	/* Check the rudundant option flag */

		if (dup_opt)
			{
			/* print error message */
			dgnerror(11);
			}

	/* Are SOAK and UCL both requested? */
		if (ucl_set & soak_set)
			{
			getout=TRUE;
			/* print error message */
			dgnerror(12);
			}

	/* Is the phase option requested for more than one device type */
		if (!STRCMP(option,"\0") && (ph_set == TRUE))
			{
			getout=TRUE;
			/* print error message */
			dgnerror(13);
			}
	}

	/* For phase list requests, check that the device type and only
	 * the device type has been specified.
	 */

		if (cmnd_code == L && (ucl_set == ON || soak_set == ON ||
			ph_set == ON || rep_set == ON || opt_type == OFF))
			{
			getout = TRUE;
			/* print error message */
			dgnerror(14);
			}

	/* For SOAK option with phases, demand flag may be set if
	 * PH entered after SOAK.  Turn off I/O if so.
	 */

		if (soak_set)
			P_REQ->dmnd=OFF;

	/* For SOAK option, turn off P_REQ->rep flag and save 
	 * P_REQ->rept_cnt value */

		if (soak_set & rep_set) {
			soak_rept= P_REQ->rept_cnt;
			P_REQ->rpt=OFF;
			P_REQ->rept_cnt=1;
		}



return(!getout);
}
