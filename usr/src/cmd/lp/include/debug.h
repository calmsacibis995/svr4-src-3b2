/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	DEBUG_H
#define	DEBUG_H
/*==================================================================*/
/*
**
*/
#ident	"@(#)lp:include/debug.h	1.5"

#ifdef	DEBUG
#include	<stdio.h>

#define	_MAX_NEST_COUNT	256

extern	int	_nestCount;
extern	char	*_FnNames [];
extern	char	*_FnNamep;
extern	char	*_Unknownp;
extern	char	*_Nullp;
extern	FILE	*_Debugp;

#ifdef	__STDC__
#define	DEFINE_FNNAME(fnName) \
	static	char	FnName [] = #fnName;
#else
#define	DEFINE_FNNAME(fnName) \
	static	char	FnName [] = "fnName";
#endif

#define	OPEN_DEBUG_FILE(path) \
	{\
		if (_Debugp) \
			_Debugp = freopen (path, "w+", _Debugp); \
		else \
			_Debugp = fopen (path, "w+"); \
	}

#define DEBUGs(s1)      \
	(void)  fprintf (_Debugp, "%s\n",	\
	((char *) s1 == NULL ? _Nullp : (char *) s1));\
	(void)	fflush (stderr);

#define DEBUGss(s1, s2)	\
	(void)  fprintf (_Debugp, "%s\t= %s\n",	\
	((char *) s1 == NULL ? _Nullp : (char *) s1),	\
	((char *) s2 == NULL ? _Nullp : (char *) s2));\
	(void)	fflush (_Debugp);

#define DEBUGsd(s1, i1)	\
	(void)  fprintf (_Debugp, "%s\t= %d\n",	\
	((char *) s1 == NULL ? _Nullp : (char *) s1),	\
	(int) i1);					\
	(void)	fflush (_Debugp);

#define DEBUGssx(s1, s2, i1)	\
	(void)  fprintf (_Debugp, "%s/%s\t= 0x%x\n",	\
	((char *) s1 == NULL ? _Nullp : (char *) s1),	\
	((char *) s2 == NULL ? _Nullp : (char *) s2),	\
	(int) i1);					\
	(void)	fflush (_Debugp);

#define DEBUGssd(s1, s2, i1)	\
	(void)  fprintf (_Debugp, "%s/%s\t= 0d%d\n",	\
	((char *) s1 == NULL ? _Nullp : (char *) s1),	\
	((char *) s2 == NULL ? _Nullp : (char *) s2),	\
	(int) i1);					\
	(void)	fflush (_Debugp);

#define DEBUGsss(s1, s2, s3)	\
	(void)  fprintf (_Debugp, "%s/%s\t= %s\n",	\
	((char *) s1 == NULL ? _Nullp : (char *) s1),	\
	((char *) s2 == NULL ? _Nullp : (char *) s2),	\
	((char *) s3 == NULL ? _Nullp : (char *) s3));\
	(void)	fflush (_Debugp);

#define	DUMP_BYTES(p, length) \
{ \
	unsigned int i, c; \
	unsigned char *cp; \
	for (i=0, cp=(unsigned char *)(p); i < length; i++, cp++) { \
		c = (unsigned int)*cp; \
		(void) fprintf (_Debugp, "[%02d] 0x%02x  ", i, c); \
		if (c & 0x80) (void) fprintf (_Debugp, "   "); \
		else { \
		c &= 0x7f; \
		if (c >= 0 && c <= 0x1f) \
		(void) fprintf(_Debugp,"^%c ", c+0x40); \
		else if (c == 0x20) (void) fprintf(_Debugp, "SP ");\
		else if (c == 0x7f) (void) fprintf(_Debugp, "DE ");\
		} \
		if (((i+1)%5) == 0) \
			(void)	fprintf (_Debugp, "\n"); \
	} \
	(void)	fprintf (_Debugp, "\n"); \
	(void)	fflush (_Debugp); \
}

#define	TRACEP(label)	\
	(void)	fprintf (_Debugp, "%s/%s\n", FnName,	\
	((char *) label == NULL ? "position" : (char *) label));\
	(void)	fflush (_Debugp);

#ifdef	__STDC__
#define	TRACE(variable)	\
	DEBUGssx(FnName, #variable, variable)

#define	TRACEd(variable)	\
	DEBUGssd(FnName, #variable, variable)

#define	TRACEs(variable)	\
	DEBUGsss(FnName, #variable, variable)

#define	TRACEb(variable, length)	\
	DEBUGssx(FnName, #variable, variable);	\
	DUMP_BYTES(variable, length)

#else
#define	TRACE(variable)	\
	DEBUGssx(FnName, "variable", variable)

#define	TRACEd(variable)	\
	DEBUGssd(FnName, "variable", variable)

#define	TRACEs(variable)	\
	DEBUGsss(FnName, "variable", variable)

#define	TRACEb(variable, length)	\
	DEBUGssx(FnName, "variable", variable);	\
	DUMP_BYTES(variable, length)

#endif

/*
#define	ENTRYP(fnNamep)	\
	if (_nestCount == _MAX_NEST_COUNT) \
		_FnNamep = fnNamep; \
	else \
		_FnNames [_nestCount++] = _FnNamep = fnNamep; \
	TRACEP("entry-point")

#define	EXITP \
	TRACEP("exit-point") \
	if (_nestCount == _MAX_NEST_COUNT || _nestCount == 0) \
		_FnNamep = _Unknownp; \
	else \
		_FnNamep = _FnNames [--_nestCount];
*/
#define	ENTRYP \
	TRACEP ("**ENTRY-POINT**")

#define	EXITP \
	TRACEP ("**EXIT-POINT**")
#else
#define	DEFINE_FNNAME(fnName)
#define	OPEN_DEBUG_FILE(path)
#define DEBUGs(s1)
#define DEBUGss(s1, s2)
#define DEBUGsd(s1, i1)
#define DEBUGssx(s1, s2, i1)
#define DEBUGssd(s1, s2, i1)
#define DEBUGsss(s1, s2, s3)
#define	DUMP_BYTES(p, length)
#define	TRACEP(label)
#define TRACE(variable)
#define TRACEd(variable)
#define TRACEs(variable)
#define TRACEb(variable, length)
#define	ENTRYP
#define	EXITP
#endif
/*==================================================================*/
#endif
