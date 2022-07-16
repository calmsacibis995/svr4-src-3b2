/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/ANodeDefs.h	1.7"

extern	long int	ANNormExpr();

extern	void	DelAdNodes();
extern	void	DelAdPrivateNodes();
extern	void	DelAdTransNodes();

extern	AN_Id	GetAdAbsDef();
extern	AN_Id	GetAdAbsolute();
extern	AN_Id	GetAdAddIndInc();

#ifndef MACRO
extern	unsigned int	GetAdAddrIndex();
#else
#define	GetAdAddrIndex(an_id)	((unsigned int) (an_id)->data.AddrIndex)
#endif

extern	AN_Id	GetAdChgRegAInc();
extern	AN_Id	GetAdAddToKey();
extern	AN_Id	GetAdCPUReg();
extern	AN_Id	GetAdStatCont();
extern	AN_Id	GetAdDisp();
extern	AN_Id	GetAdDispInc();
extern	AN_Id	GetAdDispDef();
extern	AN_Id	GetAdDispDefTemp();
extern	AN_Id	GetAdDispTemp();

#ifndef MACRO
extern	long int	GetAdEstim();
#else
#define	GetAdEstim(an_id)	((an_id)->data.estim)
#endif

extern	char *GetAdExpression();
extern	unsigned short int	GetAdGNAQSize();

#ifndef MACRO
extern	AN_GnaqType	GetAdGnaqType();
#else
#define	GetAdGnaqType(an_id)	((AN_GnaqType) (an_id)->data.gnaq)
#endif

extern	AN_Id	GetAdId();
extern	AN_Id	GetAdImmediate();
extern	AN_Id	GetAdIndexRegDisp();
extern	AN_Id	GetAdIndexRegScaling();
extern	AN_Id	GetAdMAUReg();

#ifndef MACRO
extern	AN_Mode	GetAdMode();
#else
#define	GetAdMode(an_id)	((AN_Mode) (an_id)->key.K.mode)
#endif

#ifndef MACRO
extern	AN_Id	GetAdNextGNode();
#else
extern AN_Id TopANId;
#define	GetAdNextGNode(anid)	(((anid) == (AN_Id) NULL) ? \
					TopANId : (anid)->forw)
#endif /*MACRO*/

extern	AN_Id	GetAdNextNode();
extern	AN_Id	GetAdNextPNode();	/* Get next private address node. */
extern	AN_Id	GetAdNoNumber();
extern	long int	GetAdNumber();
extern	AN_Id	GetAdPostDecr();
extern	AN_Id	GetAdPostIncr();
extern	AN_Id	GetAdPreDecr();
extern	AN_Id	GetAdPreIncr();
extern	AN_Id	GetAdPrevGNode();
extern	AN_Id	GetAdRaw();
extern	RegId	GetAdRegA();
extern	RegId	GetAdRegB();
extern	AN_Id	GetAdRemInd();
extern 	AN_Id	GetAdTemp();

#ifndef MACRO
extern	unsigned int	GetAdTempIndex();
#else
#define	GetAdTempIndex(A)	((unsigned int) (A)->key.K.tempid)
#endif

extern	AN_Id	GetAdTranslation();

#ifndef MACRO
extern	AN_Id	GetAdUsedId();
#else
#define	GetAdUsedId(an_id,index) \
	((index == 0)	? \
		(((an_id)->usesa == (an_id)) ? (AN_Id) NULL : (an_id)->usesa) \
			: \
		(((an_id)->usesb == (an_id)) ? (AN_Id) NULL : (an_id)->usesb) \
	)
#endif /*MACRO*/

#ifdef W32200
extern	AN_Id	GetAdChgRegBInc();
#endif

extern	void	HideAdCandidate();
extern	AN_Id	HideAdPrivateNodes();

extern	boolean	IsAdAbsDef();
extern	boolean	IsAdAbsolute();
extern	boolean	IsAdAddInd();

#ifndef MACRO
extern	boolean	IsAdAddrIndex();
#else
#define	IsAdAddrIndex(an_iD)	((an_iD)->data.AddIndexP == 1)
#endif

#ifndef MACRO
extern	boolean	IsAdArg();
#else
#define	IsAdArg(an_id)		(((AN_Mode) (an_id)->key.K.mode == Disp) && \
			   	 ((RegId) (an_id)->key.K.rega == CAP))
#endif /*MACRO*/

extern	boolean	IsAdAuto();

#ifndef MACRO
extern	boolean	IsAdCPUReg();
#else
#define	IsAdCPUReg(an_id)	((AN_Mode) (an_id)->key.K.mode == CPUReg)
#endif

#ifndef MACRO
extern	boolean	IsAdDisp();
#else
#define	IsAdDisp(an_id)		((AN_Mode) (an_id)->key.K.mode == Disp)
#endif /*MACRO*/

extern	boolean	IsAdDispDef();

#ifndef MACRO
extern	boolean	IsAdENAQ();
#else
#define	IsAdENAQ(a)		((a)->data.gnaq == (unsigned)ENAQ)
#endif

extern	boolean	IsAdFP();
extern	boolean	IsAdGNAQSize();

#ifndef MACRO
extern	boolean	IsAdImmediate();
#else
#define	IsAdImmediate(an_id)	((AN_Mode) (an_id)->key.K.mode == Immediate)
#endif

extern	boolean	IsAdIndexRegDisp();
extern	boolean	IsAdIndexRegScaling();
extern	boolean	IsAdLabel();
extern	boolean	IsAdMAUReg();

#ifndef MACRO
extern	boolean	IsAdNAQ();
#else
#define	IsAdNAQ(a)		((a)->data.gnaq == (unsigned)NAQ)
#endif

extern	boolean IsAdNotFP();
extern	boolean	IsAdNumber();
extern	boolean	IsAdPostDecr();
extern	boolean	IsAdPostIncr();
extern	boolean	IsAdPreDecr();
extern	boolean	IsAdPreIncr();
extern	boolean IsAdPrivate();
extern	boolean IsAdProbe();
extern	boolean	IsAdRO();
extern	boolean	IsAdRaw();
extern	boolean	IsAdRemInd();

#ifndef MACRO
extern	boolean	IsAdSNAQ();
#else
#define	IsAdSNAQ(a)		((a)->data.gnaq == (unsigned)SNAQ)
#endif

#ifndef MACRO
extern	boolean	IsAdSENAQ();
#else
#define	IsAdSENAQ(a)		((a)->data.gnaq == (unsigned)SENAQ)
#endif

extern	boolean	IsAdSV();
extern	boolean	IsAdSafe();
extern	boolean	IsAdStatCont();

#ifndef MACRO
extern	boolean	IsAdTemp();
#else
#define	IsAdTemp(A)		((((AN_Mode) (A)->key.K.mode) == CPUReg) && \
				 (((RegId) (A)->key.K.rega) == CTEMP))
#endif

extern	boolean	IsAdTIQ();

#ifndef MACRO
extern	boolean	IsAdTranslation();
#else
#define	IsAdTranslation(an_id)	((an_id)->data.translation == 1)
#endif

#ifndef MACRO
extern	boolean	IsAdUses();
#else
#define	IsAdUses(a1,a2) \
	((	((a1) == (a2)) || \
		((a1)->usesa == (a2)) || \
		((a1)->usesa->usesa == (a2)) || \
		((a1)->usesa->usesb == (a2)) || \
		((a1)->usesb == (a2)) || \
		((a1)->usesb->usesa == (a2)) || \
		((a1)->usesb->usesb == (a2)) \
	 ) ? TRUE : FALSE \
	)
#endif /*MACRO*/

extern	boolean	IsAdValid();

extern	void	OrAdEnaqs();	/* ORs ENAQ bit-vector to an array.	*/
extern	void	OrAdSenaqs();	/* ORs SENAQ bit-vector to an array.	*/
extern	void	OrAdSnaqs();	/* ORs SNAQ bit-vector to an array.	*/

extern void PutAdCandidate();

#ifndef MACRO
extern	void	PutAdEstim();
#else
#define	PutAdEstim(an_id,value)	(an_id)->data.estim = (value)
#endif

extern	void	PutAdFP();
extern	void	PutAdGNAQSize();

#ifndef MACRO
extern	void	PutAdGnaqType();
#else
#define	PutAdGnaqType(an_id,type)	(an_id)->data.gnaq = (unsigned) (type)
#endif

extern	void	PutAdLabel();
extern	void	PutAdRO();
extern	void	PutAdSafe();
extern	void	PutAdTIQ();

extern	void	RestoreAdPrivate();
extern	void	SortAdGnaq();
