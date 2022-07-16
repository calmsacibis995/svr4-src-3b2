/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/FNodeDefs.h	1.4"

extern	void	DoFnAlias();
extern	AN_Id	GetFnName();
extern	FN_Id	GetFnId();
extern	FN_Id	GetFnIdProbe();
extern	FN_Id	GetFnNextNode();
extern	FN_Id	GetFnPrevNode();
extern	TN_Id	GetFnFirst();
extern	TN_Id	GetFnLast();
extern	AN_Id	GetFnNextPrivateAd();

#ifdef MACRO
#define	IsFnAlias(F,A)	(IsAliased((F)->abegin,(A)))
#else
extern	boolean	IsFnAlias();
#endif

extern	boolean IsFnAuditerr();
extern	boolean IsFnBlackBox();
extern	boolean IsFnCandidate();
extern	boolean	IsFnDefined();

#ifdef MACRO
#define	IsFnLibrary(F)	((F)->library == 1 ? TRUE : FALSE)
#else
extern	boolean	IsFnLibrary();
#endif

extern	boolean	IsFnNODBL();	/* Tests nodblflg for this function. */

#ifdef MACRO
#define	IsFnMISconvert(F)	((F)->MISconvert == 1 ? TRUE : FALSE)
#else
extern	boolean	IsFnMISconvert();
#endif

#ifdef MACRO
#define	IsFnPAlias(F)		((F)->PAlias == 1 ? TRUE : FALSE)
#else
extern	boolean	IsFnPAlias();
#endif

extern	boolean	IsFnSetjmp();
extern	unsigned int	GetFnNumReg();
extern	unsigned int	GetFnRVRegs();
extern	unsigned long int	GetFnCalls();
extern	unsigned long int	GetFnExpansionLimit();
extern	unsigned long int	GetFnExpansions();
extern	unsigned long int	GetFnInstructions();
extern	unsigned long int	GetFnLocSz();
extern	unsigned long int	GetFnCandLocSz();
extern	void	DelFnNode();
extern	void	GetFnLocNm();
extern	void	GetFnPrivate();

#ifdef MACRO
#define	PutFnAlias(F,A)	PutAlias(&(F)->abegin,(A))
#else
extern	void	PutFnAlias();
#endif

extern 	void	PutFnAuditerr();
extern 	void	PutFnBlackBox();
extern	void	PutFnCalls();
extern	void	PutFnExpansionLimit();
extern	void	PutFnExpansions();
extern	void	PutFnCandidate();
extern	void	PutFnDefined();
extern	void	PutFnFunction();
extern	void	PutFnInstructions();
extern	void	PutFnLibrary();
extern	void	PutFnLocNm();
extern	void	PutFnLocSz();

#ifdef MACRO
#define	PutFnMISconvert(F,yes)	((F)->MISconvert = (yes)?1:0)
#else
extern	void	PutFnMISconvert();
#endif

extern	void	PutFnNODBL();	/* Sets nodblflg for this function. */
extern	void	PutFnCandLocSz();
extern	void	PutFnNumReg();

#ifdef MACRO
#define	PutFnPAlias(F,yes)	((F)->PAlias = (yes) ? 1 : 0)
#else
extern	void	PutFnPAlias();
#endif

extern	void	PutFnRVRegs();
extern	void	PutFnPrivate();
extern	void	PutFnSetjmp();
extern	void	SortFnNodes();

#ifdef MACRO
#define UndoFnAlias(F)	{UndoAlias((F)->abegin);FreeAlias(&((F)->abegin));}
#else
extern void UndoFnAlias();
#endif

