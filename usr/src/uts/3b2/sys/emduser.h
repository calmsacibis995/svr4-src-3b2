/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)emd:sys/emduser.h	1.5"

/*
 * EMD user interface definitions.
 */


/*
 * Ethernet address size.
 */
#define PHYAD_SIZE	6

#define PUMPBSIZE 256*3

/*
 * Pump command structure.
 */
struct eipump {
	unsigned short size;
	char *address;
	unsigned long flags;
	unsigned char data[PUMPBSIZE];
};

/*
 * Structure for EI_SETA ioctl.
 */
struct eiseta {
	char	eis_addr[PHYAD_SIZE];
};

/*
 * ioctl definitions
 */
#define EICODE		('E' << 8)
#define EI_RESET	(EICODE | 1)
#define EI_LOAD		(EICODE | 2)
#define EI_FCF		(EICODE | 3)
#define EI_SYSGEN	(EICODE | 4)
#define EI_SETID	(EICODE | 5)
#define EI_TURNON	(EICODE | 6)
#define EI_ALLOC	(EICODE | 7)
#define EI_TERM		(EICODE | 8)
#define EI_TURNOFF	(EICODE | 9)
#define EI_SETA		(EICODE | 10)
#define EI_GETA		(EICODE | 11)

/*
 * Ethernet header format.
 */
struct ehead {
	char dest_addr[PHYAD_SIZE];
	char src_addr[PHYAD_SIZE];
	union {
		ushort val;
		unsigned char bytes[2];
	} pro_type;
};

#define EHEADSIZE 14
#define DATINDSIZE PHYAD_SIZE+PHYAD_SIZE+DL_UNITDATA_IND_SIZE

#define EMDMINPSZ	64
#define EMDMAXPSZ	1514

/*
 * Error return codes.
 */

#define EMD_NOTOPEN		0x101
#define EMD_NOTBIND		0x102
#define EMD_ISBIND		0x103
#define EMD_802_ERROR		0x104
#define EMD_BADPARA		0x105
#define EMD_NOMEM		0x106
#define EMD_DUP_MUXTYPE		0x107
#define EMD_PACK_SIZE		0x108
#define EMD_ALR_OPEN		0x109
#define EMD_NOSTRUCT		0x10a
#define EMD_MAX_MSGLEN		0x10b
#define EMD_MTC_ONLY		0x10c
#define EMD_BADPROT		0x118
#define EMD_BADPULL		0x122	/* more than 4 cont blocks, and */
					/* pullup failed */
#define EMD_EVTOP		0x133
#define EMD_EFW			0x134
#define EMD_XPAGE		0x135	/* too many buffers crossed page */
					/* boundary */

/*
 * Complete return codes.
 */
#define	EMD_FINISHED		0x200	/* just return don't free up the bp */

