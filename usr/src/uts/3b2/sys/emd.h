/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)emd:sys/emd.h	1.9"

/*
 * Ethernet Media Driver header file.
 */

/*
 * Module information definitions.
 */
#define EMD_MID		2
#define EMD_NAME	"EMD"
#define EMD_HIWAT	4800
#define EMD_LOWAT	2000

/*
 * Stream state structure - one per stream (minor device).
 */
struct emd {
	queue_t	*emd_rdq;	/* read queue of stream */
	mblk_t	*emd_head;	/* front of pending message list */
	mblk_t	*emd_tail;	/* end of pending message list */
	ushort	emd_state;	/* DLPI state flags */
	ushort	emd_sap;	/* protocol type of this stream */
	ushort	emd_seq;	/* sequence number for pending messages */
	ushort	emd_bid;	/* board id for this dev */
};

/*
 * Device-dependent definitions.
 */
#define SRECSIZE 128	/* small receive buffer size */
#define LRECSIZE 1536	/* big receive buffer size */
#define PASS	1
#define FAIL	0

#define MSGLENMAX	5	/* maximum message length in blocks */

/*
 * Information kept about each EMD board.
 * NOTE: The size of this structure in in the emd master file in 
 * raw fromat. This size must be calculated by using the sizes of
 * each item in this structure.
 */
struct emd_board {
	int		eb_major;	/* major dev number of the EMD board */
	struct emd	*eb_emdp;	/* start of emd structures for board */
	RQUEUE		eb_rq;		/* EMD request queue */
	CQUEUE		eb_cq;		/* EMD completion queue */
	mblk_t		*eb_srbuf[RQSIZE];	/* firmware sm receive bufs */
	mblk_t		*eb_lrbuf[RQSIZE];	/* firmware lg receive bufs */
	unsigned short	eb_state;	/* EMD state flag */
	unsigned char	eb_addr[PHYAD_SIZE]; /* network address */
	SG_DBLK 	eb_sgdblk;	/* EMD system sysgen data block */
	mblk_t		*eb_ind[RQSIZE+RQSIZE];
	long		eb_timeid;	/* value of the firmware timer */
	struct emd	*eb_initep;
	int		eb_waitcnt;
	int		eb_emdnext;
};

/*
 * EMD board states
 */
#define EB_DOWN		0x00
#define EB_UNINIT	0x01
#define EB_RESET	0x02
#define EB_LOAD		0x03
#define EB_FCF		0x04
#define EB_SYSGEN	0x05
#define EB_SETID	0x06
#define EB_TURNON	0x07
#define EB_ALLOC	0x08
#define EB_INIT		0x0f	/* in process of initializing */
#define EB_UP		0x10

/*
 * Macros for mapping firmware job id to/from (minor device, message block).
 *  Bits 0-14 : index for emd board receive buffer or minor device number
 *  Bit  15   : 0 if minor device, 1 if receive job
 *  Bits 16-31: sequence number
 */
#define id2seqid(id)	(ushort)(((unsigned long)(id) >> 16))
#define id2idx(id)	(ushort)(((id) & 0x7fff) - 1)
#define id2minor(id)	(dev_t)((id) & 0x7fff)
#define emdmkid(md, seq) (((unsigned long)(seq) << 16) | ((md) & 0x7fff))
#define isrecv(id)	((id) & 0x8000)
#define mkrecid(idx, seq) (((unsigned long)(seq)<<16)|((idx)+1)|0x8000)

#define getid2bid(getid)	(int)((getid) >> 8)
#define getid2idx(getid)	(int)((getid) & 0xff)
#define mkgetid(bid, idx)	(long)(((bid) << 8) | ((idx) & 0xff))

#define emd_add(ep, mp)	{					\
			    if (ep->emd_tail) {			\
				ep->emd_tail->b_next = mp;	\
				ep->emd_tail = mp;		\
			    } else {				\
				ep->emd_head = mp;		\
				ep->emd_tail = mp;		\
			    } 					\
			}

#define emd_rmv(ep, mp)	{					\
			    register mblk_t *tmp;		\
			    register mblk_t *pmp = NULL;	\
								\
			    for (tmp = ep->emd_head; tmp;	\
			      tmp = tmp->b_next) {		\
				if (tmp == mp)			\
				    break;			\
				pmp = tmp;			\
			    }					\
			    if (tmp) {				\
				if (pmp)			\
				    pmp->b_next = tmp->b_next;	\
				else				\
				    ep->emd_head = tmp->b_next;	\
				if (tmp == ep->emd_tail)	\
				    ep->emd_tail = pmp;		\
			    }					\
			}

#define splemd() spl6()

#define EMDTIME	6000L	/* timeout value */
#define DLYVALUE 50000	/* delay for EMD to reset */

/*
 * Cmd/Status of common I/O queue entries.
 */
#define CS_STATUS	0	/* queue entry is a status */
#define CS_COMMAND	1	/* queue entry is a command */

#define GE_QUE	0	
#define RD_S_QUE	1
#define RD_L_QUE 2

/*
 *	Opcodes of Common I/O queue entries
 */
#define _EIDLM		1		/* EI download memory */
#define _EIULM		2		/* EI upload memory */
#define _EIFCF		3		/* EI force function call */
#define _EIDOS		4		/* EI determine operational status */
#define _EIDSD		5		/* EI determin subdevices */
#define _EIPHYAD	6		/* EI physical address set */
#define _EIOFF		7		/* EI off */
#define _EION		8		/* EI on */
#define _EIBDEAF	9		/* EI broadcast deaf (disable) */
#define _EIBHEAR	10		/* EI broadcast hear (enable) */
#define _EISEND		11		/* EI send packet */
#define _EIRECV		12		/* EI receive packet */
#define _EISTATS	13		/* EI vital statistics */
#define _EIOMCAST	14		/* EI open multi cast port */
#define _EICMCAST	15		/* EI close multi cast port */
#define _EIGSEND	16		/* EI gather send */

/*
 *	Status size
 */
#define STAT_SIZ	8

/*
 *	Error returns
 */
#define UNINIT	-1
#define FULQUE	-2
#define EMPQUE	-3
#define BAD_ID	-4
#define MEM_FAIL -5
#define MEMFLW	-6
#define PAGE_ERR	-7

#define RAM_SIZ	16384
#define CHK_INIT	0xaaaaaaaa

/*
 *	Pointer to the sysgen data block
 */
#define SG_PTR	*((long *)0x2000000)

/*
 *	define interrupt to ni
 */
#define	SET	(char *) 0
#define INT_1	*((char *)0x200003)

/*
 * Macros for calling firmware functions.
 */
#define emdfw_off(id,BID)	emdfw_job(id,_EIOFF,0,0,CS_STATUS,BID,GE_QUE)
#define emdfw_on(id,BID)	emdfw_job(id,_EION,0,0,CS_STATUS,BID,GE_QUE)
#define emdfw_bhear(id,BID)	emdfw_job(id,_EIBHEAR,0,0,CS_STATUS,BID,GE_QUE)
#define emdfw_bdeaf(id,BID)	emdfw_job(id,_EIBDEAF,0,0,CS_STATUS,BID,GE_QUE)
#define emdfw_clear(BID)	emdfw_job(0,_EICLEAR,0,0,CS_STATUS,BID,GE_QUE)
#define	EMD_DELAY()		{int n;for(n=0;n<DLYVALUE;n++);}
#define	EMD_RESET(bid)		{char n; n = *((char *)(emd_addr[bid] + 5));}
#define	EMD_ASS(bid)		{char n; n = *((char *)(emd_addr[bid] + 3));}
#define	EMD_ULDELD(bid)		{RULOAD(bid,RD_S_QUE) = RLOAD(bid,RD_S_QUE); \
				 RULOAD(bid,RD_L_QUE) = RLOAD(bid,RD_L_QUE); \
				 RULOAD(bid,GE_QUE) = RLOAD(bid,GE_QUE); \
				 CLOAD(bid) = CULOAD(bid); }

/*
 * The folowing is from the file lla.h.
 */
#define RULOAD(ID,QUE)	emd_bd[ID].eb_rq.queue[QUE].p_queues.bit16.unload
#define RLOAD(ID,QUE)	emd_bd[ID].eb_rq.queue[QUE].p_queues.bit16.load
#define CULOAD(ID)	emd_bd[ID].eb_cq.queue.p_queues.bit16.unload
#define CLOAD(ID)	emd_bd[ID].eb_cq.queue.p_queues.bit16.load
#define RQEMPTY(ID,QUE)	(RLOAD(ID,QUE) == RULOAD(ID,QUE))
#define RQFULL(ID,QUE)	(((RULOAD(ID,QUE)/sizeof(RENTRY) + RQSIZE - RLOAD(ID,QUE)/sizeof(RENTRY)) % RQSIZE) ==  1)
#define CQEMPTY(ID)	(CULOAD(ID) == CLOAD(ID))
#define NEW_RLPTR(ID,QUE)	((1 + (RLOAD(ID,QUE)/sizeof(RENTRY))) % RQSIZE)
#define NEW_CUPTR(ID)	((1 + CULOAD(ID)/sizeof(CENTRY)) % CQSIZE)
#define	RQENTRY(ID,QUE) ((RLOAD(ID,QUE) < RULOAD(ID,QUE)) ? ((RQSIZE - RULOAD(ID,QUE)/sizeof(RENTRY)) + RLOAD(ID,QUE)/sizeof(RENTRY)) : ((RLOAD(ID,QUE)/sizeof(RENTRY)) - (RULOAD(ID,QUE)/sizeof(RENTRY))))

/*
 * EMDLOG(mid,sid,level,flags,fmt,args) should be used for those trace
 * calls that are only to be made during debugging.
 */
#ifdef DEBUG
#define EMDLOG(A, B, C, D, E, F)	((void)((emdlog) && strlog((A), (B), (C), (D), (E), (F))))
#else
#define EMDLOG(A, B, C, D, E, F)
#endif

#define REUSEABLE(BP, SIZE) \
	(((BP)->b_datap->db_ref == 1) && \
	(((BP)->b_datap->db_lim - (BP)->b_datap->db_base) >= (SIZE)))

#define BUMPSEQ(eq)	{ if (++(eq)->emd_seq == 0) ++(ep)->emd_seq; }

/*
 * Structure for sending multiple message blocks to the firmware
 */
struct eigsend {
	long eig_addr;
	short eig_size;
	short eig_last;
};

#define eiacpy(FROM, TO) { \
				register char *from = (char *)(FROM); \
				register char *to = (char *)(TO); \
				*to++ = *from++; \
				*to++ = *from++; \
				*to++ = *from++; \
				*to++ = *from++; \
				*to++ = *from++; \
				*to = *from; \
			 }
