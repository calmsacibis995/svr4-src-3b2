/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/com/edt_def.h	1.1"
/*******************************************************************/
/*	edt_def.h - definitions associated with the edt		   */
/*		must follow diagnostic.h			   */
/*******************************************************************/


#define D_MAX 16	/* limit for lookup table device entries */

#define SD_MAX 132	/* limit for lookup table subdevice entries */

struct dev_code {
	unsigned opt_code:16;	/* code for device in option slot */
	unsigned rq_size:8;	/* size of request queue for device */
	unsigned cq_size:8;	/* size of completion queue */
	unsigned boot_dev:1;	/* set =1 if possible boot device */
	unsigned word_size:1;	/* zero = 8 bit */
				/* one = 16 bit */
	unsigned brd_size:1;	/* zero = single width */
				/* one = double width */
	unsigned smrt_brd:1;	/* zero = dumb board */
				/* one = intelligent board */
	unsigned cons_cap:1;	/* set =1 if can support console */
	unsigned cons_file:1;	/* set =1 if cons pump file needed */
	unsigned indir_dev:1;	/* set =1 if device has multiple */
				/* subdevice levels */
	unsigned pad:25;
	unsigned char dev_name[E_NAMLEN];	/* ASCII name for device */
	unsigned char diag_file[E_NAMLEN];	/* diagnostic file name */
};


typedef struct {
	unsigned long num_dev;	/* number of entries in device lookup table */
	struct dev_code  dev_code[D_MAX];	/* maximum lookup entries =D=MAX */
}DEV_TAB;





struct sbdev_code {
	unsigned short opt_code;	/* code for subdevices in option slot */
	unsigned char name[E_NAMLEN];	/* subdevice name	*/
};



typedef struct {
	unsigned char num_sbdev;	/* entries in subdev lookup table */
	struct sbdev_code sbdev_code[SD_MAX];	/* lookup table max=SD_MAX */
}SBDEV_TAB;





/* keyword defines for the parser */

#define UCL 0
#define SOAK 1
#define REP 2
#define PH 3
#define DGN 4
#define H 5
#define Q 6
#define HELP 7
#define QUIT 8
#define S 9
#define SHOW 10
#define L 11
#define LIST 12
#define QUESTION 13
#define ERRORINFO 14
#define DGN_JUNK 255
