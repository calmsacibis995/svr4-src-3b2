/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)npack:sys/npack.h	1.3.1.6"

#define NETADDRLEN	6		/* network address length in bytes */
					/* (same as PHYAD_SIZE in emduser.h) */

#ifdef uts
typedef unsigned char	unchar;
#endif

/*  
 *	PACK protocol header
 */
typedef	struct	{
	unchar	pk_type;		/*  packet type - ACK, DATA, CTRL */
	ushort	pk_seqno;		/*  packet sequence number */
	ushort	pk_ackno;		/*  packet ACK sequence number */
	ushort	pk_srclink;		/*  source link number */
	ushort	pk_dstlink;		/*  destination link number */
	ushort	pk_size;		/*  size of this packet	*/
}  PACKHDR;

/*
 *  Various constants required by pack
 */
#define PCKHDRSZ	sizeof(PACKHDR)
#define HDRWORD		4
#define HDROFFSET	2
#define COMPATSIZE	16		/* for compatibility with old driver */
					/* 14 + HDROFFSET */
#define PACK_ID		9999
#define FLOW_ID		1987
#define MINPKSIZE	50
#define MINBUFSZ	64
#define NEEDBUF		3
#define DUP		0
#define NODUP		1
#define FLOWCNTL	2

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif
#ifndef NULL
#define NULL		0
#endif

/*
 * MUX link states.
 */
#define UNLINKED	0	/* not linked */
#define WAITLINK	1	/* processing link ioctl */
#define LINKED		2	/* active */

/*
 *	Define Packet Type
 */
#define	DATAP	0x01		/* data packet */
#define	ACKP	0x02		/* acknowledgement packet */
#define CTRLP	0x04		/* control packet */
#define	SANITYP	0x08		/* sanity packet */
#define	SNDACK	0x10		/* send back ack immediately */
#define	EXDATAP	0x20		/* expedited data packet */
#define	STOPP	0x40		/* transmitter must block */

/*
 *	TI characteristics of the PACK module
 */
#define ETSDU_SIZE	0
#define CDATA_SIZE	128
#define DDATA_SIZE	128
#define ADDR_SIZE	sizeof(struct pckaddr)
#define OPT_SIZE	sizeof(struct pckopt)
#define MAX_CONN_IND	5
#define DFLT_NRETRY	1
#define DFLT_INTERVAL	(5*HZ)

/*
 *	Sequence structure for listener connect establishment
 */
struct	pckseq {
	long	seqno;			/* connect sequence number */
	long	srclink;		/* caller's mindev */
	char	srcnaddr[NETADDRLEN];	/* caller's network addr */
};

/*
 *	Retransmit queue structure
 */
struct retransq {
	mblk_t	*rq_head;
	mblk_t	*rq_tail;
};

/*
 *	Multiplexed low level device structure.
 */
struct	pckdev {
	char	phy_naddr[NETADDRLEN];	/* local network address */
	short	lastrun;		/* last VC to be scheduled */
	short	linkstate;		/* condition of link */
	queue_t *respq;			/* send ioctl response up this q */
	queue_t	*qbot;			/* upper queue of linked device */
};

/*
 *	PACK virtual circuit structure
 */
struct	pck_vc {
	unchar	vc_state;		/* state of this circuit for TLI */
	unchar	vc_seqcnt;		/* # of outstanding conn_ind */
	ushort	vc_flags;		/* various flags - see below */
	struct	pckseq vc_seq[MAX_CONN_IND];	/* conn_ind sequence numbers */
	long	vc_qlen;		/* max # outstanding conn_ind */
	struct	retransq vc_retrnsq;	/* retransmit queue */
	ushort	vc_cpsn;		/* current packet sequence number */
	ushort	vc_epsn;		/* expected packet sequence number */
	ushort	vc_sackno;		/* next piggy-back ack number */
	ushort	vc_rackno;		/* last received ack number */
	ushort	vc_qcnt;		/* current queue count (retrns) */
	ushort	vc_nretry;		/* number of retry */
	ushort	vc_interval;		/* time interval between retries */
	ushort	vc_srclink;		/* source link number */
	char	vc_dstnadr[NETADDRLEN];	/* destination network address */
	ushort	vc_dstlink;		/* destination link number */
	long	vc_lnktimer;		/* link timer id */
	long	vc_acktimer;		/* ack timer id */
	queue_t	*vc_rdq;		/* stream read queue */
	mblk_t	*vc_creqmp;		/* msg for sending connect request */
	mblk_t	*vc_datreq;		/* msg for sending data request */
	long	vc_reason;		/* disconnect reason */
};

/*
 *	Flags associated with the virtual circuit
 */
#define VC_SENDACK	0x01		/* send ack */
#define VC_LNKTOUT	0x02		/* link timed out */
#define VC_ACKTOUT	0x04		/* ack timed out */
#define VC_SANITY	0x08		/* sanity packet */
#define VC_CONNTOUT	0x10		/* connect request timed out */
#define VC_FATAL	0x20		/* fatal error on VC */
#define VC_SENDSTOP	0x40		/* send stop packet to transmitter */
#define VC_LOCALFC	0x80		/* local receiver flow controlled */
#define VC_REMFC	0x100		/* remote receiver flow controlled */
#define VC_NEWCON	0x200		/* other end of connection is a new */
					/* style NPACK driver */
#define VC_URSRV	0x400		/* upper read svc procedure active */

/*
 *	PACK address and options structures
 */
struct pckaddr {
	long	link;			/* minor device of stream (VC) */
	char	phynaddr[NETADDRLEN];	/* low level device address */
};

struct pckopt {
	ushort	nretry;			/* no. of connect request retries */
	ushort	interval;		/* interval in ticks between retries */
};

/*
 *	Control messages passed between peer PACK modules
 */
struct pm_connect {
	long	cmd;			/* always PM_CONNECT */
	long	SRC_length;		/* source address length */
	long	SRC_offset;		/* source address offset */
};

struct pm_accept {
	long	cmd;			/* always PM_ACCEPT */
	long	RES_length;		/* responding address length */
	long	RES_offset;		/* responding address offset */
};

struct pm_discon {
	long	cmd;			/* always PM_ACCEPT */
	long	reason;			/* reason for disconnect */
	long	SRC_length;		/* source address length */
	long	SRC_offset;		/* source address offset */
};

union pm_ctrl {
	long			cmd;		/* PACK control cmd */
	struct pm_connect	connect;	/* connect indication */
	struct pm_accept	accept;		/* connect confirmation */
	struct pm_discon	discon;		/* disconnect indication */
};

/*
 *	various macros
 */
#define VC_NUM(X)		((struct pck_vc *)(X) - pck_vc)
#define SEQIN(LOW, X, HIGH)	(((LOW)<=(HIGH))?(((X)>=(LOW)) && \
					     	  ((X)<=(HIGH)))  \
						:(((X)<=(HIGH)) ||\
						  ((X)>=(LOW))))

/*  
 *	incoming commands for virtual circuit set up 
 */
#define	PM_CONNECT	0
#define	PM_ACCEPT	1
#define	PM_DISCONNECT	2

/*  
 *	reason for disconnect
 */
#define	VC_REJECT	1	/* reject a conection request */
#define	VC_CLOSE	2	/* closed by the other end */
#define	VC_CONNFAIL	3	/* fail to contact remote side */
#define	VC_LINKDOWN	4	/* link down */
#define	VC_USERINIT	5	/* normal user-initiated disconnect */
#define VC_PACKFAIL	6	/* fatal error in PACK */

#define DATREQSIZE	NETADDRLEN+DL_UNITDATA_REQ_SIZE

/*
 * PCKLOG(mid,sid,level,flags,fmt,args) should be used for those trace
 * calls that are only to be made during debugging.
 */
#ifdef DEBUG
#define PCKLOG(A, B, C, D, E, F, G, H)	((void)((pcklog) && strlog((A), (B), (C), (D), (E), (F), (G), (H))))
#else
#define PCKLOG(A, B, C, D, E, F, G, H)
#endif

