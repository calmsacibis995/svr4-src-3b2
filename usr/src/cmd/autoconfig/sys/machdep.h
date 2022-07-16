/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:sys/machdep.h	1.6"

extern dev_t		rootdev;
extern dev_t		dumpdev;
extern dev_t		swapdev;
extern dev_t		mirrordev[];
extern int		swapdone;
extern struct bootobj	swapfile;



#define RX	0x80	/* vectors are paired receive/transmit */
#define XR	0x40	/* vectors are paired transmit/receive */
#define T	1
#define F	0




/*
 * Default system devices from VTOC
 */
extern int	VTOC_major;		/* board slot of boot device (major number) */
extern int	VTOC_minor;		/* device number on controller */
extern int	VTOC_root;		/* partition number */
extern int	VTOC_swap;		/* partition number */
extern int	VTOC_nswap;		/* partition size */

extern char            *MAJOR;                 /* real address */
extern char            *MINOR;                 /* real address */

extern int	      (**io_init)();	/* ==> malloc()'ed copy */
extern int	      (**next_init)();	/* ==> next entry in *io_init[] */
extern int	      (**io_start)();	/* ==> malloc()'ed copy */
extern int	      (**next_start)();	/* ==> next entry in *io_start[] */

extern int	      (**pwr_clr)();	/* ==> malloc()'ed copy */
extern int	      (**next_pwrclr)();/* ==> next entry in *pwr_clr[] */

extern int	      (**io_poll)();	/* ==> malloc()'ed copy */
extern int	      (**next_poll)();	/* ==> next entry in *io_poll[] */
extern int	      (**io_halt)();	/* ==> malloc()'ed copy */
extern int	      (**next_halt)();	/* ==> next entry in *io_halt[] */

extern address sizemem();

extern void build_io_sys();
