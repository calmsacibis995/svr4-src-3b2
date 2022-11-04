/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/boot/boot.c	1.12"
#include	"sys/types.h"
#include	"sys/psw.h"
#include	"sys/elog.h"
#include	"sys/iobuf.h"
#include	"sys/boot.h"
#include	"sys/firmware.h"
#include	"sys/param.h"
#include	"sys/sbd.h"
#include 	"sys/csr.h"
#include 	"sys/immu.h"
#include	"sys/nvram.h"
#include	"sys/vtoc.h"
#include	"sys/edt.h"
#include	"sys/extbus.h"
#include	"sys/sys3b.h"
#include	"sys/dma.h"
#include	"sys/vnode.h"
#include	"sys/vfs.h"
#include	"sys/fs/bfs.h"
#include	"sys/fsiboot.h"
#include 	"sys/libfm.h"
#include	"sys/id.h"

char magic = FALSE;

#define NEXTPART "next part"



/* FAST boot program. Reads the contiguous boot filesystem. */

main()
{
	unsigned char check_config;
	unsigned char demand_config;
	off_t fd;
	off_t sfd;
	char oname[40];
	char name[40];
	struct bootattr battrs;
	struct bootattr eattrs;
	int (*entry)();
	off_t fso;
	struct boothdr bhdr;

	fso = get_fs();	/* Find the /stand partition in the VTOC */

	if (!fso){
		PRINTF("Boot: No /stand partition on device.\n");
		return (FAIL);
	}

	strcpy(oname, P_CMDQ->b_name);	/* Get requested pgm name from firmware */

	name_process(oname, name);	/* Get rid of all but last component */


	demand_config = FALSE;

	/* Loop forever until the final program name to be loaded is found. */

	for (;;)
	{
		/* Check for configuration on an AUTOBOOT.  UNIXBOOT is another
		   name for AUTOBOOT when the default file is being loaded. */

		if ((P_CMDQ->b_type == UNIXBOOT) ||
		    (P_CMDQ->b_type == AUTOBOOT))
			check_config = TRUE;
		else
			check_config = FALSE;

		/* If special unconfigurable programs are being loaded, do not
		   check for reconfiguration. */

		if ((STRCMP(name, DGMON) == 0) ||
		    (STRCMP(name, FILLEDT) == 0) ||
		    (STRCMP(name, LBOOT) == 0))
			check_config = FALSE;
		else
			if (P_CMDQ->b_type == UNIXBOOT)
				strcpy(name, "unix");

		if (P_CMDQ->b_type == AUTOBOOT){	
			RNVRAM((char *)(UNX_NVR->bootname), oname, 15);

			if (((STRCMP(oname, FASTBOOT) == 0)) && 
				((STRCMP(name,DGMON) == 0))){
					RUNFLG=REENTRY;
					LONGJMP(0);
				}

		}
                if (P_CMDQ->b_type == UNIXBOOT){
                        RNVRAM((char *)(UNX_NVR->bootname), oname, 15);

                        if (STRCMP(oname, AUBOOT) == 0){
                                WNVRAM(SYSTEM, (char *)(UNX_NVR->bootname), 15);				strcpy(name, LBOOT);
                                continue;
                        }
		}


		oname[0] = '\0';

		if ((((STRCMP(name, LBOOT)) != 0) && (P_CMDQ->b_type == UNIXBOOT)) ||
					(P_CMDQ->b_type == DEMANDBOOT))
			if (!demand_config)
				WNVRAM(oname, (char *)(UNX_NVR->bootname), 15);

		/* If this is a DEMANDBOOT and no name was given, prompt for one. */

		if ((P_CMDQ->b_type == DEMANDBOOT) && (*name == '\0'))
			prompt_user(name, fso);


		if (STRCMP(name, NEXTPART) == 0){
			fso= nextfso(fso);
			*name='\0';
			continue;
		}
		
		/* Special command for load-without-execution */

		if (STRCMP(name, MAGICMODE) == 0)
		{
			PRINTF("POOF!\n");
			magic = TRUE;
			*name='\0';
			continue;
		}

		/* Get the requested file's start block number */

		fd = open(fso, name, &battrs);

/*
		PRINTF("debug: fd from open = %x\n",fd);
*/
		if (fd == -1){
			/* 
			 * File not found.  If DEMANDBOOT or UNIXBOOT, 
			 * list /stand and prompt for a new one.  
			 * Otherwise fatal error. 
			 */

			PRINTF("File %s not found in /stand.\n",name);

			if (P_CMDQ->b_type == DEMANDBOOT || P_CMDQ->b_type == UNIXBOOT){
				list_dir(fso);
				prompt_user(name, fso);
				continue;
			}

			/* else */

			RUNFLG = FATAL;
			RST = 1;
			return(FAIL);
		}

		/*
		 * get header information about file.
		 */

		gethead(fd,&bhdr);

		if (bhdr.type == NONE){
			if (P_CMDQ->b_type == DEMANDBOOT){

				/* Boot pgm not executable.  
				   Reconfigure using the name typed as systemfile */

				WNVRAM(name, (char *)(UNX_NVR->bootname), 15);
				strcpy(name, LBOOT);
				demand_config = TRUE;
				continue;
			}

			if (P_CMDQ->b_type == UNIXBOOT){
				/* UNIX not executable.  Reconfigure using system */

				WNVRAM(SYSTEM, (char *)(UNX_NVR->bootname), 15);
				strcpy(name, LBOOT);
				demand_config = TRUE;
				continue;
			} else {
				/* Filledt or dgmon not executable */

				PRINTF("%s bad magic.\n", name);
				RUNFLG = FATAL;
				RST = 1;
				return(FAIL);
			}
		}


		if (!check_config){
			/* This is a DEMANDBOOT.  Load the program and execute it...
			   unless in magic mode. */

			entry = (int(*)())load(fd, &bhdr, check_config);
	
			if (magic){
				PRINTF("Magic Mode.  Name=%s, start address = %x.\n",
					name, entry);
				restart();
			}

			(*entry)();
		}

		/* else */

		/* AUTOBOOT.  Compare mtime with system file. */

		sfd = open(fso, SYSTEM, &eattrs);
		if (sfd == -1){
			entry = (int(*)())load(fd, &bhdr, check_config);
			if (entry == NULL){
				strcpy(name, LBOOT);
				WNVRAM(SYSTEM, (char *)(UNX_NVR->bootname), 15);
				continue;
			}
			(*entry)();
		}

		if (eattrs.mtime > battrs.mtime) {
			strcpy(name, LBOOT);
			WNVRAM(SYSTEM, (char *)(UNX_NVR->bootname), 15);
			continue;
		}

		/* else */

		entry = (int(*)())load(fd, &bhdr, check_config);
		if (entry == NULL)
		{
			strcpy(name, LBOOT);
			WNVRAM(SYSTEM, (char *)(UNX_NVR->bootname), 15);
			continue;
		}

		(*entry)();

		/*
		 * Something went very wrong, return to FIRMWARE
		 */
		 return(FAIL);
	}
}


min(a,b)
int a,b;
{
	if (a > b)
		return (b);
	else
		return (a);
}

prompt_user(name, fso)
register char	*name;
off_t fso;
{
	char nname[40];

	do

	{
		PRINTF("Enter name of boot file (or 'q' to quit): ");
		GETS(nname);
		if ((nname[0] == 'q') && (nname[1] == '\0'))
			restart();

		if (nname[0] == '\0')
			list_dir(fso);
	}

	while (nname[0] == '\0');

	name_process(nname, name);

	return (0);
}

name_process(namein, nameout)
char *namein;
char *nameout;
{
	char *name;

	if ((name = (char *)strrchr(namein, '/')) == NULL)
		name = namein;
	else
		name++;

	strcpy(nameout, name);

	return (0);
}
