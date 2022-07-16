/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:sys/dproc.h	1.3"




/*
 * special relocation types.
 */
#define R_SPEC1 (unsigned short)65535
#define R_SPEC2 (unsigned short)65534
#define R_SPEC3 (unsigned short)65533
#define R_SPEC4 (unsigned short)65532
#define R_SPEC5 (unsigned short)65531
#define R_SPEC6 (unsigned short)65530


typedef struct {
	char suffix[2];
	int vector;
	address value;
	} PCB_PATCH;

extern PCB_PATCH	pcb_patch[];
extern int 		nkpcb;

/*
 * Generate() routine commands
 *
 *				 WHAT	     #ARGS
 *			   ----------------  -----  ------------ARGUMENTS------------- */
#define G_TEXT	0	/*	       text	3   char* name, long size, char* init  */
#define G_DATA	1	/* initialized data	3   char* name, long size, char* init  */
#define G_UDATA 2	/*     uninit. data	2   char* name, long size
       */
#define G_BSS	3	/*		bss	2   char* name, long size
       */
#define G_PCB	4	/*		pcb	4   char* name, char* entry, ipl, vector#  */
#define G_IRTN	5	/* interupt routine	3   char* name, char* entry, minor     */
#define G_IOSYS 6	/*    I/O subsystem	0
       */

extern void generate();
extern void alloc_variables();
extern void loadriver();
extern void initdata();
