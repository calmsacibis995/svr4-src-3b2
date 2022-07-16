/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/TNodeDefs.h	1.3"

extern	void	DelTxNode();
extern	void	DelTxNodes();
extern	void	GetTxDead();
extern	void	GetTxLive();
extern	unsigned int	GetTxLoopDepth();
extern	boolean	GetTxLoopFlag();
extern	unsigned int	GetTxLoopSerial();
extern	LoopType	GetTxLoopType();

#ifndef MACRO
extern	TN_Id	GetTxNextNode();
#else
#define	GetTxNextNode(tn_id) \
	(((tn_id) == (TN_Id) NULL) ? \
		((FirstTNId->forw == &ntail) ? \
			(TN_Id) NULL : FirstTNId->forw) : \
		(((tn_id)->forw == &ntail) ? \
			(TN_Id) NULL : (tn_id)->forw))
#endif /*MACRO*/


#ifndef MACRO
extern	unsigned short int	GetTxOpCodeX();
#else
#define	GetTxOpCodeX(tn_id)	(tn_id->op)
#endif

#ifndef MACRO
extern	AN_Id	GetTxOperandAd();
#else
#define GetTxOperandAd(tn_id,opd)	((AN_Id) tn_id->addrs[opd])
#endif

extern	unsigned char GetTxOperandFlags();
extern	AN_Mode	GetTxOperandMode();

#ifndef MACRO
extern	OperandType	GetTxOperandType();
#else
#define GetTxOperandType(tn_id,opd)	((OperandType) tn_id->types[opd])
#endif

#ifndef MACRO
extern	TN_Id	GetTxPrevNode();
#else
#define	GetTxPrevNode(tn_id) \
	(((tn_id) == (TN_Id) NULL) ? \
		((LastTNId->back == &n0) ? \
			(TN_Id) NULL : LastTNId->back) : \
		(((tn_id)->back == &n0) ? \
			(TN_Id) NULL : (tn_id)->back))
#endif /*MACRO*/

extern	RoundMode	GetTxRMode();
extern	unsigned long int	GetTxUniqueId();
extern	boolean	IsTxAuditerr();

#ifndef MACRO
extern	boolean	IsTxBlackBox();
#else
#define	IsTxBlackBox(tn_id)	((tn_id->blackbox == 1) ? TRUE : FALSE)
#endif

extern	boolean	IsTxAnyCall();

#ifndef MACRO
extern	boolean	IsTxAux();
#else
#define	IsTxAux(tn_id)	((tn_id->op <= AUPPER) ? TRUE : FALSE)
#endif

#ifndef MACRO
extern	boolean	IsTxBr();
#else
extern struct opent optab[];
#define IsTxBr(tn_id)	((boolean) (optab[tn_id->op].oflags & (CBR | UNCBR)))
#endif

extern	boolean	IsTxCBr();
extern 	boolean	IsTxCPUOpc();
extern	boolean	IsTxGenOpc();
extern	boolean	IsTxHL();

#ifndef MACRO
extern	boolean	IsTxLabel();
#else
#define	IsTxLabel(tn_id)	((tn_id == (TN_Id) NULL) ? FALSE : (tn_id->op == LABEL) ? TRUE : (tn_id->op == HLABEL) ? TRUE : FALSE)
#endif

#ifndef MACRO
extern	boolean	IsTxOperandVol();
#else
extern unsigned int Max_Ops;
#define	IsTxOperandVol(tn_id,op)	((tn_id == (TN_Id) NULL) ? FALSE : ((boolean) (tn_id->flags[op] & VOLATILE)) )
#endif

extern	boolean	IsTxMIS();
extern	boolean	IsTxProtected();
extern	boolean	IsTxRet();
extern	boolean	IsTxRev();
extern	boolean	IsTxSPI();
extern	void	RevTxBr();
extern	boolean	IsTxSame();
extern	boolean	IsTxSameOps();
extern	boolean		IsTxUncBr();
extern	TN_Id	MakeTxNodeAfter();
extern	TN_Id	MakeTxNodeBefore();
extern	TN_Id	MoveTxNodeAfter();
extern	TN_Id	MoveTxNodeBefore();
extern	TN_Id	ReadTxNodeAfter();	/* Reads text node from stream.	*/
extern	TN_Id	ReadTxNodeBefore();	/* Reads text node from stream.	*/
extern	void	PutTxAuditerr();
extern	void	PutTxBlackBox();
extern	void	PutTxDead();
extern	void	PutTxLive();
extern	void	PutTxLoopDepth();
extern	void	PutTxLoopFlag();
extern	void	PutTxLoopSerial();
extern	void	PutTxLoopType();
extern	void	PutTxOpCodeX();

#ifndef MACRO
extern	void	PutTxOperandAd();
#else
#define	PutTxOperandAd(tn_id,op,an_id)	tn_id->addrs[op] = an_id
#endif

extern	void	PutTxOperandFlags();

#ifndef MACRO
extern	void	PutTxOperandType();
#else
#define	PutTxOperandType(tn_id,op,type)	(tn_id->types[op] = (unsigned char) type)
#endif

#ifndef MACRO
extern	void	PutTxOperandVol();
#else
#define	PutTxOperandVol(tn_id,op,flag)	(tn_id->flags[op] = flag ? \
		(tn_id->flags[op] | VOLATILE) : (tn_id->flags[op] & ~VOLATILE))
#endif

extern	void	PutTxProtected();
extern	void	PutTxRMode();
extern	void	PutTxSPI();
extern	void	PutTxUniqueId(); /*Inserts UniqueId(line number)in text node.*/
extern	void	WriteTxNode();	/* Writes a text node to a stream.	*/

#ifdef MACRO
 extern TN_Id FirstTNId;
 extern TN_Id LastTNId;
 extern NODE n0;
 extern NODE ntail;
#endif /*MACRO*/
