/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/OperndType.h	1.1"

enum OperandTypes_E {	Tnone,
			Timm8,Tsbyte,Tbyte,Tubyte,
			Timm16,Tshalf,Thalf,Tuhalf,
			Timm32,Tsword,Tword,Tuword,T1word,Tsingle,
			T2word,Tdouble,
			T3word,Tdblext,
			Tdecint,
			Taddress,
			Tany,
			Tspec,Tbin,Tfp		/* NEVER in address nodes! */
		    };

typedef enum OperandTypes_E OperandType;
				/*  FOLLOWING WILL BE DELETED SOMEDAY! */
#define	TNONE	(unsigned char) Tnone
#define	TIMM8	(unsigned char) Timm8
#define	TSBYTE	(unsigned char) Tsbyte
#define	TBYTE	(unsigned char) Tbyte
#define	TUBYTE	(unsigned char) Tubyte
#define	TIMM16	(unsigned char) Timm16
#define	TSHALF	(unsigned char) Tshalf
#define	THALF	(unsigned char) Thalf
#define	TUHALF	(unsigned char) Tuhalf
#define	TIMM32	(unsigned char) Timm32
#define	TSWORD	(unsigned char) Tsword
#define	TWORD	(unsigned char) Tword
#define	TUWORD	(unsigned char) Tuword
#define	T1WORD	(unsigned char) T1word
#define	TSINGLE	(unsigned char) Tsingle
#define	T2WORD	(unsigned char) T2word
#define	TDOUBLE	(unsigned char) Tdouble
#define	T3WORD	(unsigned char) T3word
#define	TDBLEXT	(unsigned char) Tdblext
#define	TDECINT	(unsigned char) Tdecint
#define	TADDRESS	(unsigned char) Taddress
#define	TANY	(unsigned char) Tany
#define	TSPEC	(unsigned char) Tspec
#define	TBIN	(unsigned char) Tbin
#define	TFP	(unsigned char) Tfp
