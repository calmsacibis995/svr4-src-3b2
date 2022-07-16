/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:fp/decconv.c	1.7"
/*
 * Decimal <=> Binary Conversion Routines
 *
 * The P754 standard requires conversions between the basic formats
 * (single and double) and a decimal format, which conforms to certain
 * accuracy constraints.  This file implements 4 routines which
 * convert from and to both single and double precision values.
 * (Conversion of extended values is not required.)
 *
 *	Source: "Contributions to a Proposed Standard for
 *	Binary Floating-Point Arithmetic" by Jerome T. Coonen
 *	PhD dissertation, UC Berkeley, June 1, 1984. 
 *
 *	Chapter 7 discusses a "fast" implementation when
 *	hardware floating-point is available (e.g. the MAU
 *	option), and includes error bounds analysis.
 *
 *	Appendix D gives a PASCAL implementation of the
 *	emulation routines, from which the C emulation
 *	code is derived.
 */

/*
 * The file ieeefp.h provides several type declarations used here,
 * as well as some enumerations and other things.  The declaration
 * for the "decimal" data type has been added to ieeefp.h because
 * fault handlers must be able to deal with any floating-point
 * data type which might be returned.
 *
 * The cpp identifier P754_NOFAULT is defined because we don't
 * want to force the user of these library routines to load the
 * full fault handler when a simple one will do.
 */

#define		P754_NOFAULT
#include	"synonyms.h"
#include	<ieeefp.h>
#include	<math.h>
#include	<stdlib.h>
#include	<string.h>

extern int	_fp_hw;		/* == 0 if no MAU, == 1 if MAU present */
extern fp_union	_getfltsw();	/* use this if a trap should be taken */
extern fp_rnd	fpsetround(), fpgetround();
extern fp_except fpgetmask(), fpsetmask(), fpgetsticky(), fpsetsticky();
extern int	isnand();

#define	true		1
#define	false		0
#define	boolean		int
#define	odd(X)		((X) & 0x1)
#define is_sinf(X)	((X).exp == 0xff && (X).frac == 0x0)
#define	is_szero(X)	((X).exp == 0x0 && (X).frac == 0x0)
#define is_dinf(X)	((X).exp == 0x7ff && (X).frac_lo == 0x0 && (X).frac_hi == 0x0)
#define	is_dzero(X)	((X).exp == 0x0 && (X).frac_lo == 0x0 && (X).frac_hi == 0x0)
#define	is_xinf(X)	((X).exp == 0x7fff && (X).J == 0x0 && (X).frac_lo == 0x0 \
			&& (X).frac_hi == 0x0)
#define	is_xzero(X)	((X).exp == 0x0 && (X).J == 0x0 && (X).frac_lo == 0x0 \
			&& (X).frac_hi == 0x0)

#define	DEC_PLUS	0 
#define	DEC_MINUS	1 
#define	DEC_NaN		2
#define	DEC_INF		3

/*
 * Typedefs for easier manipulation of floating-point number parts.
 * Note that for these to work, bit fields must be assigned from MSB
 * to LSB.  Also, note that bit 0 of an extended value is in the x[2]
 * word.
 */

typedef struct single_word {
	unsigned int	sign:1,		/* 1 => negative, 0 => positive */
			exp:8,		/* exponent bias is 1023 */
			frac:23;	/* implicit 1 bit to left of dec. point */
} single_word;

typedef struct double_word {
	unsigned int	sign:1,		/* 1 => negative, 0 => positive */
			exp:11,		/* exponent bias is 127 */
			frac_hi:20;	/* high part of fraction (implicit 1 bit) */
	unsigned int	frac_lo;	/* low part of fraction */
} double_word;

typedef struct triple_word {
	unsigned int	:16,		/* unused */
			sign:1,		/* 1 => negative, 0 => positive */
			exp:15;		/* exponent bias is 16383 */
	unsigned int	J:1,		/* explicit bit to left of dec. point */
			frac_hi:31;	/* high word of fraction */
	unsigned int	frac_lo;	/* low word of fraction */
} triple_word;

/* unions for casting between structs and floating point types */

typedef union SByte {
	float		s;
	single_word	m;
	int		w[1];
} SByte;

typedef union DByte {
	double		d;
	double_word	m;
	int		w[2];
} DByte;

typedef union XByte {
	fp_union	u;
	triple_word	m;
	int		w[3];
} XByte;

/*
 * The following ASMs are used to convert between the MAU extended type
 * and other data types, and to do arithmetic and comparisons on
 * extended values.  Note that the ASR is interrogated and modified
 * by calls to the routines fpgetmask(), fpsetmask(), fpgetround(),
 * and fpsetround(), rather than directly through the MIS "mmovta" and
 * "mmovfa" instructions.  This allows for some common code to be shared
 * between the MAU and emulation versions of the code, and reduces the
 * number of ASMs to be maintained.
 */

asm MMOVXS (src, dst)
{
% mem src,dst;
	mmovxs	* src, * dst
% error src,dst;
}

asm MMOVXD (src, dst)
{
% mem src,dst;
	mmovxd	* src, * dst
% error src,dst;
}

asm MMOVX10 (src, dst) 
{ 
% mem src,dst;
	mmovx10	* src, * dst
% error src,dst;
}

asm MMOVSX (src, dst)
{
% mem src,dst;
	mmovsx	* src, * dst
% error src,dst;
}

asm MMOVDX (src, dst)
{
% mem src,dst;
	mmovdx	* src, * dst
% error src,dst;
}

asm MMOV10X (src, dst)
{
% mem src,dst;
	mmov10x	* src, * dst
% error src,dst;
}

/* round to integer value */
asm MFRNDX1 (src)
{
% mem src;
	mfrndx1	* src
% error src;
}

asm MFMULX2 (src, dst)
{
% mem src, dst;
	mfmulx2 * src, * dst
% error src, dst;
}

asm MFDIVX2 (src, dst)
{
% mem src, dst;
	mfdivx2 * src, * dst
% error src, dst;
}

/* compare extended: return 1 if src1 >= src2, 0 otherwise */
asm int CMPGE (src1, src2)
{
% mem src1, src2; lab label;
	CLRW	%r0
	mfcmpx	* src1, * src2
	mjfl	label
	INCW	%r0
label:
% error src1, src2;
}

/* compare extended: return 1 if src1 < src2, 0 otherwise */
asm int CMPL (src1, src2)
{
% mem src1, src2; lab label;
	CLRW	%r0
	mfcmpx	* src1, * src2
	mjfge	label
	INCW	%r0
label:
% error src1, src2;
}

/*
 *	The next macros are used to copy float and double
 *	values around without the possibility of generating
 *	MAU operations.  These macros may be used even in the FPE parts.
 */
#define	MMOVSS(src,dst)	((int *)(dst))[0] = ((int *)(src))[0]

#define	MMOVDD(src,dst)	{ ((int *)(dst))[0] = ((int *)(src))[0]; \
			  ((int *)(dst))[1] = ((int *)(src))[1]; }

/*
 * Convert an extended-type integer to a null-terminated ASCII string.
 * "N" is the number of digits to be produced.  The sign is returned
 * separately.
 */
static void binstr (X, A, N, SIGN)
triple_word *	X;	/* convert this value to decimal */
char *		A;	/* address of string to fill with digits */
int		N;	/* write this many digits */
char *		SIGN;
{
	int	BCD[3];

	MMOVX10(X,&BCD[0]);		/* get bcd version of f.p. number X */

	switch (BCD[2] & 0xf) {	/* set sign of X */
	case 11: case 13:	
		*SIGN = DEC_MINUS;	break;
	default:
		*SIGN = DEC_PLUS;	break;
	}

	A += N;			/* adjust A to point to end of digit string */

	switch (N) {		/* unpack digits */
	case 17:	A[-17] = (BCD[0] >> 4  & 0xf) + '0';	/* NO breaks! */
			/*FALLTHROUGH*/
	case 16:	A[-16] = (BCD[0] >> 0  & 0xf) + '0';
			/*FALLTHROUGH*/
	case 15:	A[-15] = (BCD[1] >> 28 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 14:	A[-14] = (BCD[1] >> 24 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 13:	A[-13] = (BCD[1] >> 20 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 12:	A[-12] = (BCD[1] >> 16 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 11:	A[-11] = (BCD[1] >> 12 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 10:	A[-10] = (BCD[1] >> 8  & 0xf) + '0';
			/*FALLTHROUGH*/
	case 9:		A[-9] =  (BCD[1] >> 4  & 0xf) + '0';
			/*FALLTHROUGH*/
	case 8:		A[-8] =  (BCD[1] >> 0  & 0xf) + '0';
			/*FALLTHROUGH*/
	case 7:		A[-7] =  (BCD[2] >> 28 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 6:		A[-6] =  (BCD[2] >> 24 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 5:		A[-5] =  (BCD[2] >> 20 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 4:		A[-4] =  (BCD[2] >> 16 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 3:		A[-3] =  (BCD[2] >> 12 & 0xf) + '0';
			/*FALLTHROUGH*/
	case 2:		A[-2] =  (BCD[2] >> 8  & 0xf) + '0';
			/*FALLTHROUGH*/
	case 1:		A[-1] =  (BCD[2] >> 4  & 0xf) + '0';
			A[0] = '\0';
	}
}

/*
 * Convert an ASCII digit string to extended floating-point.  "N" is
 * the length of the digit string.  (Since the string is null-terminated,
 * it is not strictly necessary, but having it available lets us avoid
 * seeking to the end of the string.  "PAD" is the number of implicit
 * trailing zeros to "append" to the string.
 */
static void strbin (A, X, N, PAD, SIGN)
char *		A;	/* convert this string to extended */
triple_word *	X;	/* location to put result */
int		N;	/* number of digits in A */
int		PAD;	/* pad string with this many zeros */
char		SIGN;	/* separate sign for the string */
{
	int	BCD[3];

	BCD[0] = BCD[1] = 0;
	BCD[2] = 10;		/* plus */

	A += (N + PAD - 1);	/* adjust A to point to end of digit string */

	switch (N+PAD) {
	case 17:	BCD[0] |= (A[-16] & 0xf) << 4;	if (PAD == 16) break;
			/*FALLTHROUGH*/
	case 16:	BCD[0] |= (A[-15] & 0xf);	if (PAD == 15) break;
			/*FALLTHROUGH*/
	case 15:	BCD[1] |= (A[-14] & 0xf) << 28;	if (PAD == 14) break;
			/*FALLTHROUGH*/
	case 14:	BCD[1] |= (A[-13] & 0xf) << 24;	if (PAD == 13) break;
			/*FALLTHROUGH*/
	case 13:	BCD[1] |= (A[-12] & 0xf) << 20;	if (PAD == 12) break;
			/*FALLTHROUGH*/
	case 12:	BCD[1] |= (A[-11] & 0xf) << 16;	if (PAD == 11) break;
			/*FALLTHROUGH*/
	case 11:	BCD[1] |= (A[-10] & 0xf) << 12;	if (PAD == 10) break;
			/*FALLTHROUGH*/
	case 10:	BCD[1] |= (A[-9] & 0xf) << 8;	if (PAD == 9) break;
			/*FALLTHROUGH*/
	case 9:		BCD[1] |= (A[-8] & 0xf) << 4;	if (PAD == 8) break;
			/*FALLTHROUGH*/
	case 8:		BCD[1] |= (A[-7] & 0xf);	if (PAD == 7) break;
			/*FALLTHROUGH*/
	case 7:		BCD[2] |= (A[-6] & 0xf) << 28;	if (PAD == 6) break;
			/*FALLTHROUGH*/
	case 6:		BCD[2] |= (A[-5] & 0xf) << 24;	if (PAD == 5) break;
			/*FALLTHROUGH*/
	case 5:		BCD[2] |= (A[-4] & 0xf) << 20;	if (PAD == 4) break;
			/*FALLTHROUGH*/
	case 4:		BCD[2] |= (A[-3] & 0xf) << 16;	if (PAD == 3) break;
			/*FALLTHROUGH*/
	case 3:		BCD[2] |= (A[-2] & 0xf) << 12;	if (PAD == 2) break;
			/*FALLTHROUGH*/
	case 2:		BCD[2] |= (A[-1] & 0xf) << 8;	if (PAD == 1) break;
			/*FALLTHROUGH*/
	case 1:		BCD[2] |= (A[0] & 0xf) << 4;
	}

	MMOV10X(&BCD[0],X);	/* convert to extended and write to result */
	X->sign = SIGN;
}

/* Return 10^n (exactly) for non-negative n */
static void power10 (n, z)
int		n;
triple_word *	z;
{
	const static triple_word	ten_to_the[] = {
		{ 0x0, 0x3fff, 0x1, 0x00000000, 0x00000000 },	/* 10^0 */
		{ 0x0, 0x4002, 0x1, 0x20000000, 0x00000000 },	/* 10^1 */
		{ 0x0, 0x4005, 0x1, 0x48000000, 0x00000000 },	/* 10^2 */
		{ 0x0, 0x4008, 0x1, 0x7a000000, 0x00000000 },	/* 10^3 */
		{ 0x0, 0x400c, 0x1, 0x1c400000, 0x00000000 },	/* 10^4 */
		{ 0x0, 0x400f, 0x1, 0x43500000, 0x00000000 },	/* 10^5 */
		{ 0x0, 0x4012, 0x1, 0x74240000, 0x00000000 },	/* 10^6 */
		{ 0x0, 0x4016, 0x1, 0x18968000, 0x00000000 },	/* 10^7 */
		{ 0x0, 0x4019, 0x1, 0x3ebc2000, 0x00000000 },	/* 10^8 */
		{ 0x0, 0x401c, 0x1, 0x6e6b2800, 0x00000000 },	/* 10^9 */
		{ 0x0, 0x4020, 0x1, 0x1502f900, 0x00000000 },	/* 10^10 */
		{ 0x0, 0x4023, 0x1, 0x3a43b740, 0x00000000 },	/* 10^11 */
		{ 0x0, 0x4026, 0x1, 0x68d4a510, 0x00000000 },	/* 10^12 */
		{ 0x0, 0x402a, 0x1, 0x1184e72a, 0x00000000 },	/* 10^13 */
		{ 0x0, 0x402d, 0x1, 0x35e620f4, 0x80000000 },	/* 10^14 */
		{ 0x0, 0x4030, 0x1, 0x635fa931, 0xa0000000 },	/* 10^15 */
		{ 0x0, 0x4034, 0x1, 0x0e1bc9bf, 0x04000000 },	/* 10^16 */
		{ 0x0, 0x4037, 0x1, 0x31a2bc2e, 0xc5000000 },	/* 10^17 */
		{ 0x0, 0x403a, 0x1, 0x5e0b6b3a, 0x76400000 },	/* 10^18 */
		{ 0x0, 0x403e, 0x1, 0x0ac72304, 0x89e80000 },	/* 10^19 */
		{ 0x0, 0x4041, 0x1, 0x2d78ebc5, 0xac620000 },	/* 10^20 */
		{ 0x0, 0x4044, 0x1, 0x58d726b7, 0x177a8000 },	/* 10^21 */
		{ 0x0, 0x4048, 0x1, 0x07867832, 0x6eac9000 },	/* 10^22 */
		{ 0x0, 0x404b, 0x1, 0x2968163f, 0x0a57b400 },	/* 10^23 */
		{ 0x0, 0x404e, 0x1, 0x53c21bce, 0xcceda100 },	/* 10^24 */
		{ 0x0, 0x4052, 0x1, 0x04595161, 0x401484a0 },	/* 10^25 */
		{ 0x0, 0x4055, 0x1, 0x256fa5b9, 0x9019a5c8 },	/* 10^26 */
		{ 0x0, 0x4058, 0x1, 0x4ecb8f27, 0xf4200f3a }	/* 10^27 */
	};
	const static triple_word	pten[] = {
		{ 0x0, 0x4058, 0x1, 0x4ecb8f27, 0xf4200f3a },	/* 10^27 */
		{ 0x0, 0x40b5, 0x1, 0x50cf4b50, 0xcfe20766 },	/* 10^55 */
		{ 0x0, 0x4165, 0x1, 0x5a01ee64, 0x1a708dea },	/* 10^108 */
		{ 0x0, 0x42ab, 0x1, 0x1f79a169, 0xbd203e41 }	/* 10^206 */
	};
	const static triple_word	XINF =
		{ 0x0, 0x7fff, 0x0, 0x00000000, 0x00000000 };	/* +infinity */
	const static int	pexp[] = { 27, 55, 108, 206 };
	const static int	pfix[] = { 0, 1, 1, -1 };
	triple_word	fixed;
	int		i;
	fp_rnd		rnd;

	if (n > 423) {
		*z = XINF;
		return;
	}
	*z = ten_to_the[0];		/* z = 1.0 */
	rnd = fpsetround(FP_RN);
	for (i=3; i>=0; i--)
		if (n >= pexp[i]) {
			fixed = pten[i];
			switch (rnd) {
			case FP_RP:
				if (pfix[i] == -1) fixed.frac_lo++;
				break;
			case FP_RM:
			case FP_RZ:
				if (pfix[i] == 1) fixed.frac_lo--;
				break;
			}
			MFMULX2(&fixed,z);
			n -= pexp[i];
			if (pfix[i] != 0)
				(void) fpsetsticky(fpgetsticky() | FP_X_IMP);
		}
	MFMULX2(&ten_to_the[n],z);
	(void) fpsetround(rnd);
}

/*
 * Return x := x * 10^SCALE.
 */
static void scale (x, SCALE)
triple_word	*x;
int		SCALE;
{
	fp_rnd		RSAVE; 
	fp_except	IXSAVE;
	triple_word	z;

	/* S0 */
	RSAVE = fpgetround();
	if (RSAVE != FP_RN) {
		if (RSAVE == FP_RM && x->sign == 1 || RSAVE == FP_RP && x->sign == 0)
			(void) fpsetround(FP_RP);
		else
			(void) fpsetround(FP_RM);
		if (SCALE < 0)
			if (fpgetround() == FP_RP)
				(void) fpsetround(FP_RM);
			else if (fpgetround() == FP_RM)
				(void) fpsetround(FP_RP);
	}
	/* S1 */
	power10((SCALE<0 ? -SCALE : SCALE),&z);
	/* S2 */
	IXSAVE = fpgetsticky() & FP_X_IMP;
	(void) fpsetsticky(fpgetsticky() & ~FP_X_IMP);
	(void) fpsetround(FP_RZ);
	if (SCALE > 0)
		MFMULX2(&z,x);
	else
		MFDIVX2(&z,x);
	/* S3 */
	x->frac_lo |= ((fpgetsticky() & FP_X_IMP) ? 1 : 0);
	(void) fpsetround(RSAVE);
	(void) fpsetsticky(fpgetsticky() | IXSAVE);
	if (is_xinf(*x))
		(void) fpsetsticky(fpgetsticky() | FP_X_OFL);
	else if (is_xzero(*x))
		(void) fpsetsticky(fpgetsticky() | FP_X_UFL);
}

/* 
 * Approximate floor(log(X)) [base 10] by taking exponent of X [in base 2]
 * and multiplying by log(2) [base 10].  The function may return a
 * value which is too low by 1.  The code looks odd because implicit fixed-
 * point arithmetic is used.
 *
 * Using the algorithm from 7.2.3 of Coonen:
 * 0)	Set LOG2 to the high 8 bits of log(2) = 0.4D104D427... [base 16]
 * 	so that the implicit decimal point is to the left of the low byte.
 * 1)	Set L2X to e + 0.f, where x = 2^e * 1.f.  The fraction f is
 *	truncated to 8 bits.  L2X consumes less than 24 bits (not counting
 *	sign extension), with the implicit binary point to the left of
 *	the low byte.
 * 2)	If L2X < 0, increase LOG2 by one unit in its last place.
 * 3)	The result is LOGX = floor(LOG2 * L2X), which is a fixed-point
 *	(i.e. integer) multiplication.  The floor function is achieved
 *	by truncation in 2's complement, so we return the (sign-extended)
 *	value in the high-order 16 bits of the 32 bit product.
 */
static int LOG10 (z)
triple_word	z;
{
	int	L2X;
	short	sresult;

	L2X = (((int)z.exp - 16383) * 256) + (int)((int)z.frac_hi >> 23);
	((unsigned short *)&sresult)[0] = (L2X * (int)(L2X < 0 ? 0x4e : 0x4d)) >> 16;
	return ((int)sresult);
}

/* Convert integer i to string a with len digits */
static void itoa (i, a, len, sign)
int	i, len;
char	*a;
char	*sign;
{
	int	j;

	if (i < 0) {
		*sign = DEC_MINUS;
		i = -i;
	}
	else
		*sign = DEC_PLUS;
	for (j=len-1; j>=0; j--) {
		a[j] = i % 10 + '0';
		i /= 10;
	}
	a[len] = '\0';
}

/*
Emulation routines for decimal conversion from Appendix D
of Coonen.
*/

/*
Correctly-rounded conversions between unpacked binary and
decimal floating-point formats.  Numbers have the form:
	(-1)^sign * radix^exp * significand
with an implicit radix point after the first digit (decimal)
or bit (binary).  Numbers need not be normalized unless underflow
causes denormalization.  Translations between the unpacked
formats are not part of this unit.

Each conversion is governed by an environment record with
rounding and underflow information.  These are dealt with
according to proposed IEEE floating-point standards P754
(binary) and P854 (radix-independent).  That is, underflowed
values are denormalized and overflowed values are set to
either the format's largest value or to the next bigger value
(the latter is intended to represent IEEE infinity).
*/

/*
The constants specify properties of the binary and decimal
formats.  A decimal value is a packed array of BCD digits.
A binary value is a packed array of bytes, with 8 bits per
byte in this implementation.

The constants DEXPMAX and BEXPMAX are not tight bounds.
Rather, they limit the width of the decimal and binary buffers
that must be used to hold input values.  The bounds should
at least cover the range of exponents of all representable
numbers in a NORMALIZED form.
*/

#define	DDIGLEN		17	/* max decimal precision		*/
#define	DEXPMAX		999	/* max magnitude of decimal exponent	*/
#define	BBITLEN		53	/* max binary precision in bits, plus 1	*/
#define	BEXPMAX		1023	/* max magnitude of binary exponent 	*/
#define	BITSDIG		8	/* bits per machine 'digit' (byte) 	*/
#define	BDIGLEN		6	/* max bytes = BBITLEN/BITSDIG, less 1 	*/
#define	MAXB		255	/* byte ranges from 0 to 255 		*/
#define	DBUFLEN		1050	/* DEXPMAX+DDIGLEN+several 		*/
/* Coonen claims the formula BBUFLEN = (BEXPMAX/BITSDIG)+BDIGLEN+several*/
/* is right, but it is insufficient for large decimal exponents		*/
#define	BBUFLEN		420	/* (DEXPMAX+2)*log2(10)/BITSDIG+several */
#define	MAXB2		((MAXB+1) / 2)

enum Style { FixedStyle, FloatStyle };

typedef struct UnpDec {
	int		sgn;		/* 0..1 */
	int		exp;		/* -DEXPMAX..DEXPMAX */
	unsigned char	dig[DDIGLEN+1];	/* 0..9 */
} UnpDec;

typedef struct UnpBin {
	int		sgn;		/* 0..1 */
	int		exp;		/* -BEXPMAX..BEXPMAX */
	unsigned char	dig[BDIGLEN+1];	/* 0..255 */
} UnpBin;

/*
If style is FloatStyle, pre is the number of significant digits
output; if style is FixedStyle pre is the number, possibly negative,
of fraction digits output.  Because it is presumed that decimal
to binary conversion will only be used to convert to machine types,
type FloatStyle is presumed in the D2BEnv.
*/

typedef struct B2DEnv {
	int		pre;
	enum Style	style;
	int		MinExp;
	int		MaxExp;
} B2DEnv;

typedef struct D2BEnv {
	int		pre;
	int		MinExp;
	int		MaxExp;
} D2BEnv;

/*
Binary and decimal values are manipulated in wide byte and
digit buffers.  For efficiency, the values head and tail
refer to the most and least significant ends of the 'relevant'
part of the string.  An exponent is maintained separately.
Depending on time and space constraints, a DBuf dig may either
be a packed hex nibble (0..4) or a full byte.  Though consuming
twice as much space, and unable to take advantage of a computer's
BCD operations in assembly-language support routines, the latter
are much more easily indexed.
*/

typedef struct DBuf {
	int		head;
	int		tail;
	unsigned char	dig[DBUFLEN+1];	/* 0..255 */
} DBuf;

typedef struct BBuf {
	int		head;
	int		tail;
	unsigned char	dig[BBUFLEN+1];	/* 0..MAXB */
} BBuf;

/*
Bin2Dec and Dec2Bin employ exactly the same conversion strategies,
so together they are serviced by corresponding sets of utilities for
handling DBufs and BBufs.  Here is a list of the utilities:

BDZero		-- clear two Bufs to zero
BRight, DRight	-- shift a Buf right n digs
BTimes2, DTimes2-- Buf * 2
BInc		-- add 0..9 in the last dig of a BBuf
BWidth		-- find width of a BBuf in bits
BUflow, DUflow	-- denormalize a Buf, if necessary, before rounding
BRound, DRound	-- round a Buf
BOflow, DOflow	-- check and handle Buf overflow, after rounding

Both Bin2Dec and Dec2Bin require two BBufs and DBufs, a working Buf
and a temporary for intermediate calculations.  For efficiency, a
temporary is passed as a 'var' parameter (pointer) to any utility itself
requiring a temporary Buf.
*/

/* Called by Dec2Bin and Bin2Dec to initialize. */
static void BDZero (bx, dx)
BBuf	*bx;
DBuf	*dx;
{
	int	i;

	for (i=0; i <= BBUFLEN; i++)
		bx->dig[i] = 0;		/* set all digs to zero */
	bx->head = BBUFLEN;		/* set head and tail to last dig */
	bx->tail = BBUFLEN;
	for (i=0; i <= DBUFLEN; i++)
		dx->dig[i] = 0;
	dx->head = DBUFLEN;
	dx->tail = DBUFLEN;
}

/*
Called by BRound to remove Guard and Sticky bit positions, by BUflow
to denormalize, and by Dec2Bin to remove excess integer digits.
bx.head is not updated rightward if all bits are shifted from the
leading word.  Since bit shifts are only done for the last
(n mod BITSDIG) bits, this is not a particularly time-consuming
routine.
*/
static void BRight (bx, n)
BBuf	*bx;
int	n;
{
	int	i, j, k;
	boolean	S;

	S = false;
	k = n / BITSDIG;			/* number of full bytes to shift */
	for (i=BBUFLEN-k+1; i <= BBUFLEN; i++)
		S = S || (bx->dig[i] != 0);	/* OR doomed bits to S */
	for (i=BBUFLEN-k; i >= bx->head; i--)
		bx->dig[i+k] = bx->dig[i];	/* shift right K bytes */
	for (i=bx->head; i <= bx->head+k-1; i++)
		bx->dig[i] = 0;			/* clear lead K bytes */
	for (i=1; i <= (n % BITSDIG); i++) {
		S = S || odd(bx->dig[bx->tail]);	/* record lowest bit */
		for (j=BBUFLEN; j>=bx->head+k; j--)
			if (odd(bx->dig[j-1]))		/* bx.head > 1 here */
				bx->dig[j] = MAXB2 + (int)bx->dig[j] / 2;
			else
				bx->dig[j] = (int)bx->dig[j] / 2;
	}
	if (S && ! odd(bx->dig[BBUFLEN]))	/* force sticky bit */
		bx->dig[BBUFLEN] = bx->dig[BBUFLEN] + 1;
}

/*
Called by Bin2Dec to convert integer, Dec2Bin to convert fraction.
Replace by external assembly-language routine for high speed.
*/
static void BTimes2 (bx)
BBuf	*bx;
{
	int	i, sum, iC;

	iC = 0;		/* integer carry flag */
	for (i=bx->tail; i >= bx->head; i--) {
		sum = bx->dig[i] + bx->dig[i] + iC;
		if (sum > MAXB) {
			iC = 1;
			bx->dig[i] = sum - (MAXB + 1);
		}
		else {
			iC = 0;
			bx->dig[i] = (unsigned char)sum;
		}
	}
	if (iC != 0) {	/* check for carry out of bx.dig[bx.head] */
		bx->head--;
		bx->dig[bx->head] = 1;
	}
}

/*
Called by BRound to add 1 ulp, and by Dec2Bin to add a digit.
Add 0<=m<=9 into BBuf by adding m into low byte and
propagating carry.  Return true if and only if this is a
carry out of the bx.dig[bx.head].
*/
static boolean BInc (m, bx)
int	m;
BBuf	*bx;
{
	int	i, sum;
	boolean	C, ret;

	ret = false;			/* assume no carry out */
	sum = bx->dig[BBUFLEN] + m;
	if (sum <= MAXB)
		bx->dig[BBUFLEN] = (unsigned char)sum;	/* easy case, no carry out */
	else {
		bx->dig[BBUFLEN] = sum - (MAXB + 1);
		C = true;
		i = BBUFLEN;
		while (C) {
			i--;
			sum = bx->dig[i] + 1;
			C = sum > MAXB;
			if (C)
				bx->dig[i] = 0;
			else
				bx->dig[i] = (unsigned char)sum;
		}
		if (i < bx->head) {
			ret = true;
			bx->head = i;	/* in this case i = bx.head-1 */
		}
	}
	return (ret);
}

/*
Called by Bin2Dec to convert fraction digits and by Dec2Bin
to convert integer digits.  Replace by external assembly-language
routine for high speed.
*/
static void BTimes10 (bx)
BBuf	*bx;
{
	int	i, sum, iC;

	iC = 0;
	for (i=bx->tail; i >= bx->head; i--) {
		sum = (10 * bx->dig[i]) + iC;
		bx->dig[i] = sum % (MAXB + 1);
		iC = sum / (MAXB + 1);
	}
	if (iC != 0) {
		bx->head--;
		bx->dig[bx->head] = (unsigned char)iC;
	}
}

/*
Called by Dec2Bin to determine how many fraction bits to find.
Dead dig != 0 since BRight has not been called yet.
*/
static int BWidth (bx)
BBuf	*bx;
{
	int	i, j;

	/* overshoot, as though lead bit of lead dig is 1 */
	i = (BBUFLEN - bx->head + 1) * BITSDIG;
	/* correct by decrementing i for leading 0s of leading dig */
	j = bx->dig[bx->head];
	while (j < MAXB2) {
		i--;
		j = j + j;
	}
	return (i);
}

/* Called by Dec2Bin */
static void BUflow (bx, b, e)
BBuf	*bx;
UnpBin	*b;
D2BEnv	*e;
{
	int	i;

	i = b->exp - e->MinExp;
	if (i < 0) {
		BRight(bx, -i);		/* denormalize */
		/* mark tiny; BRound determines true Uflow */
		(void) fpsetsticky(fpgetsticky() | FP_X_UFL);
		b->exp = e->MinExp;
	}
	else
		(void) fpsetsticky(fpgetsticky() & ~FP_X_UFL);
}

/* Called by Dec2Bin */
static void BRound (bx, b)
BBuf	*bx;
UnpBin	*b;
{
	int	LowDig;
	boolean	L, G, S, A;

	/* bx has two extra trailing bits, Guard and Sticky */
	LowDig = bx->dig[BBUFLEN];
	S = odd(LowDig);
	if (S)
		LowDig--;
	G = odd(LowDig / 2);
	if (G)
		LowDig -= 2;
	L = odd(LowDig / 4);		/* least significant bit */
	bx->dig[BBUFLEN] = (unsigned char)LowDig;	/* replace stripped low byte */
	BRight(bx, 2);			/* right-aligned significand */

	/* set inexact flag, and suppress uflow if exact */
	if (G || S)
		(void) fpsetsticky(fpgetsticky() | FP_X_IMP);
	else
		(void) fpsetsticky(fpgetsticky() & ~FP_X_UFL);

	/* A <- whether to add 1 in L's bit position */
	switch (fpgetround()) {
	case FP_RZ:
		A = false;
		break;
	case FP_RP:
		A = (b->sgn == 0) && (G || S);
		break;
	case FP_RM:
		A = (b->sgn == 1) && (G || S);
		break;
	case FP_RN:
		A = G && (S || L);
	}
	if (A)				/* add an ULP and check for carry-out */
		if (BInc(1, bx)) {
			BRight(bx, 1);
			b->exp++;
		}
}

/*
Called by Dec2Bin
Set to HUGE or INFINITY according to P754/P854 criteria.
HUGE has maximum exponent and all 1 bits; INFINITY has just
larger exponent and bits 1000...00.
*/
static void BOflow (bx, b, e)
BBuf	*bx;
UnpBin	*b;
D2BEnv	*e;
{
	int	i, fix;

	if (b->exp > e->MaxExp) {
		(void) fpsetsticky(fpgetsticky() | FP_X_OFL);

		/* force inexact on any overflow */
		(void) fpsetsticky(fpgetsticky() | FP_X_IMP);

		/* decide between HUGE and INFINITY */
		switch(fpgetround()) {
		case FP_RN:	fix = 1;		break;
		case FP_RP:	fix = (b->sgn == 0);	break;
		case FP_RM:	fix = (b->sgn == 1);	break;
		default:	fix = 0;		break;
		}
		b->exp = e->MaxExp + fix;	/* force excessive exponent */
		BRight(bx, e->pre - 1);		/* clear all but leading 1 */
		for (i=1; i <= e->pre-1; i++) {	/* denormalize */
			BTimes2(bx);
			bx->dig[BBUFLEN] += (1 - fix);
		}
	}
	else
		(void) fpsetsticky(fpgetsticky() & ~FP_X_OFL);
}

/*
Called by DUflow to denormalize, by DRound to remove Guard and Sticky
digit positions, and by Bin2Dec to remove excess integer digits.
dx.head is not incremented.
*/
static void DRight (dx, n)
DBuf	*dx;
int	n;
{
	int	i;
	boolean	S;

	S = false;
	for (i = DBUFLEN-n+1; i <= DBUFLEN; i++)
		S = S || (dx->dig[i] != 0);		/* OR doomed digits to S */
	for (i = DBUFLEN-n; i >= dx->head; i--)
		dx->dig[i+n] = dx->dig[i];		/* move right n digits */
	for (i = dx->head; i <= dx->head+n-1; i++)
		dx->dig[i] = 0;				/* clear lead n digits */
	if (S)
		dx->dig[DBUFLEN]++;			/* OK if >9 */
}

/* Called by Bin2Dec to convert integer, by Dec2Bin to convert fraction. */
static void DTimes2 (dx)
DBuf	*dx;
{
	int	i, sum, iC;

	iC = 0;						/* integer carry flag */
	for (i=dx->tail; i >= dx->head; i--) {
		sum = dx->dig[i] + dx->dig[i] + iC;
		if (sum > 9) {
			iC = 1;
			dx->dig[i] = sum - 10;
		}
		else {
			iC = 0;
			dx->dig[i] = (unsigned char)sum;
		}
	}
	if (iC != 0) {		/* check for carry out of the dx.dig[dx.head] */
		dx->head--;
		dx->dig[dx->head] = 1;
	}
}

/* Called by Bin2Dec. */
static void DUflow (dx, d, e)
DBuf	*dx;
UnpDec	*d;
B2DEnv	*e;
{
	int	i;

	i = d->exp - e->MinExp;
	if (i < 0) {
		DRight(dx, -i);		/* denormalize */
		/* mark tiny; DRound determines true Uflow */
		(void) fpsetsticky(fpgetsticky() | FP_X_UFL);
		d->exp = e->MinExp;
	}
	else
		(void) fpsetsticky(fpgetsticky() & ~FP_X_UFL);
}

/* Called by Bin2Dec. */
static void DRound (dx, d, e)
DBuf	*dx;
UnpDec	*d;
B2DEnv	*e;
{
	int	i, iG, sum;
	boolean	L, S, A;

	/* dx has 2 extra trailing digits, Guard and Sticky, to be ignored */
	S = dx->dig[DBUFLEN] != 0;
	iG = dx->dig[DBUFLEN - 1];
	L = odd(dx->dig[DBUFLEN - 2]);			/* low bit of LSD */

	/* set inexact flag, and suppress uflow if exact */
	if (iG || S)
		(void) fpsetsticky(fpgetsticky() | FP_X_IMP);
	else
		(void) fpsetsticky(fpgetsticky() & ~FP_X_UFL);

	/* A <- whether to add 1 in L's bit position */
	switch (fpgetround()) {
	case FP_RZ:
		A = false;
		break;
	case FP_RP:
		A = d->sgn == 0 && (iG != 0 || S);
		break;
	case FP_RM:
		A = d->sgn == 1 && (iG != 0 || S);
		break;
	case FP_RN:
		A = (iG > 5) || (iG == 5) && (L || S);
		break;
	}
	if (A) {		/* add an ULP and check for carry-out */
		S = true;	/* use to propagate carry */
		i = DBUFLEN-1;	/* will discard low 2 digits */
		while (S) {
			i--;
			sum = dx->dig[i] + 1;
			S = sum > 9;
			if (S)
				dx->dig[i] = 0;
			else
				dx->dig[i] = (unsigned char)sum;
		}
		if (i < dx->head)
			if (e->style == FloatStyle) {
				dx->dig[dx->head] = 1;	/* carry out at left */
				d->exp++;
			}
			else
				dx->head = i;
	}
}

/*
Called by Bin2Dec.
Set to HUGE or INFINITY according to P754/P854 criteria.
HUGE has maximum exponent and all nines; INFINITY has just
larger exponent and decimal digits 1000....00.
*/
static void DOflow (dx, d, e)
DBuf	*dx;
UnpDec	*d;
B2DEnv	*e;
{
	int	i, fix;

	if (d->exp > e->MaxExp) {
		(void) fpsetsticky(fpgetsticky() | FP_X_OFL);

		/* force inexact on any overflow */
		(void) fpsetsticky(fpgetsticky() | FP_X_IMP);

		/* decide between HUGE and INFINITY */
		switch(fpgetround()) {
		case FP_RN:	fix = 0;		break;
		case FP_RP:	fix = !(d->sgn == 0);	break;
		case FP_RM:	fix = !(d->sgn == 1);	break;
		default:	fix = 1;		break;
		}
		d->exp = e->MaxExp + 1 - fix;		/* force big exponent */
		dx->dig[dx->head] = (8 * fix) + 1;	/* either 9 or 1 */
		for (i=dx->head+1; i<=DBUFLEN-2; i++)
			dx->dig[i] = 9 * fix;		/* either 9 or 0 */
	}
	else
		(void) fpsetsticky(fpgetsticky() & ~FP_X_OFL);
}

/*
Conversions between UnpDec and UnpBin records.  For convenience in
packing the results of Dec2Bin, if e.pre is not a multiple of
BITSDIG then the e.pre output bits are right-aligned in the leading
((e.pre / BITSDIG) + 1) bytes of b.dig[].  Of course the implicit
binary point is still to the right of the first bit of b.dig[0].

Both conversions follow the same strategy:

0) If input has all zero digits, then the result is 0, else...

1) Align input in Buf as 0.XXXXXXX * RADIX^exp, with dig[0] = 0
   and the significand shifted far enough right that exp >= 0.

2) Convert integer part, that is until exp = 0.

3) If no nonzero output digit has been found, then convert
   the fraction up to the first nonzero digit.

4) The object is to have exactly P+2 significant digits/bits,
   the last one sticky in the sense of P754 rounding.  If there
   are too many already, then right shift and gather lost digits
   in sticky; otherwise, convert until there are just P+2.
   Gather unconverted digits/bits into sticky.

5) If result is tiny in the sense of P754, then right shift
   (denormalize) it until the exponent is the minimum allowed.

6) Round the result to p digits/bits.

7) Deal with overflow according to P754, that is replacing an
   overflowed result with either INFINITY or HUGE.

Both conversions align their input to the left of a Buf, up to
dig[0], and form their output aligned to the right in its Buf.

The conversions set the environment underflow, overflow, and
inexact bits.
*/

static void Bin2Dec (e, b, d)
B2DEnv	*e;
UnpBin	*b;
UnpDec	*d;
{
	int	i, j, BExp;
	boolean	S;
	BBuf	bx;
	DBuf	dx;

	d->sgn = b->sgn;			/* copy sign */
	for (i=0; i<=DDIGLEN; i++)
		d->dig[i] = 0;			/* place all zero digits */

	/* Step 0: check for all zeros */
	S = true;				/* assume the significand is zero */
	for (i=0; i<=BDIGLEN; i++)
		S = S && (b->dig[i] == 0);
	if (S)
		d->exp = e->MinExp;		/* process zero */
	else {
		BExp = b->exp + 1;		/* align binary pt left of lead bit */
		if (BExp >= 0)			/* significand in dig[(0+j)..] */
			j = 1;
		else
			j = 2 - (BExp / BITSDIG);

		/* Setp 1: set bx to input b, aligned */
		BDZero(&bx, &dx);
		bx.head = 1;
		bx.tail = BDIGLEN + j;
		for (i=0; i<=BDIGLEN; i++)
			bx.dig[i+j] = b->dig[i];

		/* Adjust BExp < 0, since bx shifted right to the nearest byte */
		BExp = (BITSDIG * (j-1)) + BExp;	/* j=1 when BExp>=0 */
		d->exp = e->pre + 1;	/* dec point after lead dig, then G and S */

		/* Step 2: convert integer part of bx */
		while (BExp > 0) {
			DTimes2(&dx);		/* make way for the next bit */
			BTimes2(&bx);		/* get next bit in bx.dig[0] */
			BExp--;
			if (bx.dig[0] != 0) {
				dx.dig[DBUFLEN]++;
				bx.dig[0] = 0;
			}
		}

		/* Step 3: guarantee some nonzero digit in dx */
		while (dx.dig[dx.head] == 0) {
			BTimes10(&bx);
			dx.dig[DBUFLEN] = bx.dig[0];
			d->exp--;
		}
		bx.dig[0] = 0;

		/* Step 4: check for too many or too few digits */
		if (e->style == FloatStyle)
			j = (DBUFLEN - dx.head + 1) - (e->pre + 2);
		else
			j = -e->pre;		/* number of 'fraction' digits */
		if (j < 0) {			/* j too few digits */
			/* make room for -j more digits */
			for (i=dx.head; i<=DBUFLEN; i++) {
				dx.dig[i+j] = dx.dig[i];
				dx.dig[i] = 0;
			}
			dx.head += j;
			/* get -j fraction digits */
			for (i=(DBUFLEN+1+j); i <= DBUFLEN; i++) {
				BTimes10(&bx);
				dx.dig[i] = bx.dig[0];
				bx.dig[0] = 0;
			}
		}
		else {				/* j too many digits already */
			DRight(&dx, j);
			dx.head += j;
		}

		/* fix exp for j-char shift */
		d->exp += j;

		S = false;
		for (i=bx.head; i <= bx.tail; i++)
			S = S || (bx.dig[i] != 0);	/* unconverted bits -> sticky */
		if (S)
			dx.dig[DBUFLEN]++;
		DUflow(&dx, d, e);
		DRound(&dx, d, e);
		DOflow(&dx, d, e);
		for (i=dx.head; i <= DBUFLEN-2; i++)
			d->dig[i - dx.head] = dx.dig[i];
	}
}

static void Dec2Bin (e, d, b)
D2BEnv	*e;
UnpDec	*d;
UnpBin	*b;
{
	int	i, j, DExp;
	boolean	S;
	BBuf	bx;
	DBuf	dx;

	b->sgn = d->sgn;			/* copy sign */
	for (i=0; i <= BDIGLEN; i++)
		b->dig[i] = 0;			/* place all zero bits */

	/* Step 0: check for all zeros */
	S = true;
	for (i=0; i <= DDIGLEN; i++)
		S = S && (d->dig[i] == 0);
	if (S)					/* process zero */
		b->exp = e->MinExp;
	else {
		/* Steps 1 and 2: convert integer part and align fraction in dx */
		BDZero(&bx, &dx);	/* initialize bx and dx */
		b->exp = e->pre + 1;	/* dec point after lead dig, then G and S */
		DExp = d->exp + 1;	/* align binary point before dig[0] */
		if (DExp >= 0) {
			for (i=0; i <= DExp-1; i++) {	/* compute integer part */
				BTimes10(&bx);
				if (i <= DDIGLEN)
					S = BInc((int)(d->dig[i]), &bx);
					/* but ignore carry-out S */
			}
			j = DExp;	/* index of first fraction digit */
		}
		else
			j = 0;		/* index of first fraction digit */
		for (i=j; i <= DDIGLEN; i++)		/* align fraction digits */
			dx.dig[i + 1 - DExp] = d->dig[i];

		dx.head = 1;
		dx.tail = DDIGLEN + 1 - DExp;
		if (dx.tail < dx.head)
			dx.tail = dx.head;

		/* Step 3: guarantee some nonzero digit in bx */
		while (bx.dig[bx.head] == 0) {
			DTimes2(&dx);
			bx.dig[bx.head] = dx.dig[0];
			b->exp--;
		}
		dx.dig[0] = 0;

		/* Step 4: check for too many or too few bits */
		j = BWidth(&bx) - (e->pre + 2);
		if (j < 0) {				/* -j too few bits */
			for (i=1; i <= -j; i++) {
				BTimes2(&bx);		/* make room for frac bit */
				DTimes2(&dx);		/* next frac bit in dig[0] */
				bx.dig[BBUFLEN] += dx.dig[0];
				dx.dig[0] = 0;
			}
		}
		else					/* j too many bits already */
			BRight(&bx, j);

		/* Final adjustments according to shift above. */
		b->exp += j;

		S = false;
		for (i=dx.head; i <= dx.tail; i++)
			S = S || (dx.dig[i] != 0);
			/* unconverted digits -> sticky */
		if (S && ! odd(bx.dig[BBUFLEN]))
			bx.dig[BBUFLEN]++;

		BUflow(&bx, b, e);
		BRound(&bx, b);
		BOflow(&bx, b, e);

		/* Finally, store trailing e.pre bits, right adjusted. */
		/* Fix exponent for possible leading 0s in first byte. */
		j = e->pre % BITSDIG;
		if (j != 0)
			b->exp += (BITSDIG - j);
		j = bx.tail - ((e->pre - 1) / BITSDIG);
		for (i=j; i <= bx.tail; i++)
			b->dig[i-j] = bx.dig[i];
	}
}

/*
Convert between Single and Double and the unpacked Bin types.
The original code assumed a byte ordering in which less significant
bytes are at lower addresses.  The current code uses a struct which
defines each field of the floating-point number separately.  The
struct MUST be compiled such that bit fields are assigned from MSB
to LSB, and words are assigned from lower to higher addresses.

The bits in UnpBin are right aligned so that no shifting is
required when they are moved to the P754 types (Single and Double).
However, the exponent field must be modified to account for any leading
zeros.
*/

static void Bin2S (b, s)
UnpBin	b;
float	*s;
{
	SByte	t;

	t.m.sign = b.sgn;
	t.m.exp = b.exp + 127;			/* bias the exponent */
	t.m.frac = (b.dig[0] & 0x7f) << 16;
	t.m.frac |= b.dig[1] << 8;
	t.m.frac |= b.dig[2] << 0;
	/* if denormalized value, adjust exponent bias */
	if ((b.dig[0] < 128) && (t.m.exp == 1))
		t.m.exp--;
	*s = t.s;
}

static void Bin2D (b, d)
UnpBin	b;
double	*d;
{
	DByte	t;

	t.m.sign = b.sgn;
	t.m.exp = b.exp + 
			1023 - 			/* bias exponent */
			3;			/* fix for lead zeros */
	t.m.frac_hi = (b.dig[0] & 0x0f) << 16;
	t.m.frac_hi |= b.dig[1] << 8;
	t.m.frac_hi |= b.dig[2] << 0;
	t.m.frac_lo = b.dig[3] << 24;
	t.m.frac_lo |= b.dig[4] << 16;
	t.m.frac_lo |= b.dig[5] << 8;
	t.m.frac_lo |= b.dig[6] << 0;
	/* if denormalized, adjust exponent bias */
	if ((b.dig[0] < 16) && (t.m.exp == 1))
		t.m.exp--;
	*d = t.d;
}

static void Bin2X (b, x)
UnpBin		b;
triple_word	*x;
{
	x->sign = b.sgn;
	x->exp = b.exp + 16383; 		/* bias exponent */
	x->J = (int)(b.dig[0] & 0x80) >> 7;
	x->frac_hi = (b.dig[0] & 0x7f) << 24;
	x->frac_hi |= b.dig[1] << 16;
	x->frac_hi |= b.dig[2] << 8;
	x->frac_hi |= b.dig[3] << 0;
	x->frac_lo = b.dig[4] << 24;
	x->frac_lo |= b.dig[5] << 16;
	x->frac_lo |= b.dig[6] << 8;
	x->frac_lo |= b.dig[7] << 0;
	if ((x->J == 0) && (x->exp == 1))
		x->exp--;
}

static void S2Bin (s, b)
float	*s;
UnpBin	*b;
{
	SByte	t;
	int	i;

	t.s = *s;
	b->sgn = t.m.sign;
	b->exp = t.m.exp - 127;
	for (i=0; i <= BDIGLEN; i++)
		b->dig[i] = 0;
	b->dig[0] = ((int)t.m.frac >> 16) & 0x7f;
	b->dig[1] = ((int)t.m.frac >> 8) & 0xff;
	b->dig[2] = ((int)t.m.frac >> 0) & 0xff;
	if (b->exp == -127)			/* correct bias of minimum exp */
		b->exp++;
	else
		b->dig[0] |= 0x80;		/* force explicit leading 1 */
}

static void D2Bin (d, b)
double	*d;
UnpBin	*b;
{
	DByte	t;
	int	i;

	t.d = *d;
	b->sgn = t.m.sign;
	b->exp = t.m.exp - 
			1023 +	 		/* bias adjust */
			3;			/* adjust for mis-aligned fraction */
	for (i=0; i <= BDIGLEN; i++)	
		b->dig[i] = 0;
	b->dig[0] = ((int)t.m.frac_hi >> 16) & 0x0f;
	b->dig[1] = ((int)t.m.frac_hi >> 8) & 0xff;
	b->dig[2] = ((int)t.m.frac_hi >> 0) & 0xff;
	b->dig[3] = (t.m.frac_lo >> 24) & 0xff;
	b->dig[4] = (t.m.frac_lo >> 16) & 0xff;
	b->dig[5] = (t.m.frac_lo >> 8) & 0xff;
	b->dig[6] = (t.m.frac_lo >> 0) & 0xff;
	if (t.m.exp == 0)			/* correct bias of minimum exp */
		b->exp++;
	else
		b->dig[0] |= 0x10;		/* force explicit leading 1 */
}

static void UnpDec2Dec (u, d)
UnpDec	*u;
decimal	*d;
{
	int	exp, j;

	d->sign = u->sgn;			/* copy sign */
	for (j=0; j<d->ilen; j++)		/* copy ilen digits */
		d->i[j] = u->dig[j] + '0';
	d->i[d->ilen] = '\0';			/* NULL-terminate i */
	
	exp = u->exp - (d->ilen - 1);
	itoa(exp, d->e, d->elen, &(d->esign));
}

static void Dec2UnpDec (d, u)
decimal	*d;
UnpDec	*u;
{
	int	exp, j;

	/* change external decimal format to internal */
	u->sgn = d->sign;				/* copy sign */
	for (j=0; j<d->ilen; j++)			/* copy len digits */
		u->dig[j] = d->i[j] - '0';
	for (j=d->ilen; j<=DDIGLEN; j++)		/* fill trailing w/ 0s */
		u->dig[j] = 0;
	exp = atoi(d->e);
	if (d->esign == DEC_MINUS)
		exp = -exp;
	u->exp = exp + (d->ilen - 1);
}

/* end of emulation routines */

/* return TRUE if string is invalid as a signed digit string */
static int badstr (str, len)
char 	*str;
int	len;
{
	char	*end;

	for (end=str+len; str < end; str++)
		if (*str < '0' || *str > '9') return (true);
	return (false);
}

/* Choose the "right" fault type and call _getfltsw. */
static fp_union fault_handler (faults, format, op1, op2, rres)
fp_except	faults;
char *		format;
fp_union *	op1; 
fp_union *	op2;
fp_union *	rres;
{
	fp_ftype		fault_type;

	if (faults & FP_X_OFL && faults & FP_X_IMP)
		fault_type = FP_IN_OVFLW;
	else if (faults & FP_X_OFL)
		fault_type = FP_OVFLW;
	else if (faults & FP_X_UFL && faults & FP_X_IMP)
		fault_type = FP_IN_UFLW;
	else if (faults & FP_X_UFL)
		fault_type = FP_UFLW;
	else if (faults & FP_X_IMP)
		fault_type = FP_INXACT;
	else if (faults & FP_X_INV)
		fault_type = FP_INVLD;
	else
		fault_type = UNKNOWN;
	return (_getfltsw(fault_type, FP_CONV, format, op1, op2, rres));
}

void _s2dec (xin, dec, p)
float	*xin;	/* value to convert */
decimal	*dec;	/* result */
int	p;	/* precision */
{
	SByte		sxin;
	fp_except	old_sticky;
	fp_except	new_sticky;
	fp_union	op1, op2, rres, result;
	int		faults;
	char		format[4];

	/* for MAU */
	int		N, LOGX, SCALE;
	fp_except	IXMASK;
	triple_word	x, y, w3, abs_x;

	/* for FPE */
	B2DEnv	E;
	UnpDec	D;
	UnpBin	B;

	/* check input for sanity */
	dec->elen = 2;

	MMOVSS(xin,&sxin.s);

	old_sticky = fpsetsticky(0);
	new_sticky = 0;
	if (is_sinf(sxin.m)) {		/* infinity */
		dec->esign = DEC_INF;
		dec->sign = sxin.m.sign;
	}
	else if (isnanf(*xin) || dec->ilen < 1 || dec->ilen > 9 ||
			p < 0 || p >= dec->ilen) {		/* NaN */
		dec->esign = DEC_NaN;
		dec->sign = sxin.m.sign;
		new_sticky |= FP_X_INV;
	}
	else if (is_szero(sxin.m)) {	/* zero */
		(void) memset(dec->i, (int) '0', dec->ilen);
		dec->i[dec->ilen] = '\0';
		(void) strcpy(dec->e, "00");
		dec->sign = sxin.m.sign;
		dec->esign = DEC_PLUS;
		return;
	}
	else if (_fp_hw) {		/* use MAU capabilities */
		N = dec->ilen;
		MMOVSX(xin,&x);
		abs_x = y = x;
		abs_x.sign = 0;		/* absolute value */
		LOGX = LOG10(abs_x);
		for (;;) {
			SCALE = N - LOGX - 1;
			scale(&x,SCALE);
			IXMASK = fpsetmask(fpgetmask() & ~FP_X_IMP) & FP_X_IMP;
			MFRNDX1(&x);
			(void) fpsetmask(fpgetmask() | IXMASK);
			abs_x = x;
			abs_x.sign = 0;
			power10(N,&w3);
			if (CMPGE(&abs_x,&w3)) {
				LOGX++;
				x = y;
				continue;
			}
			power10(N-1,&w3);
			if (CMPL(&abs_x,&w3)) {
				w3.sign = x.sign;
				x = w3;
			}
			break;
		}
		binstr(&x, dec->i, dec->ilen, &dec->sign);
		itoa(p-SCALE, dec->e, 2, &dec->esign);
	}
	else {
		/* set up the environment */
		E.style = FloatStyle;
		E.pre = dec->ilen;
		E.MinExp = -126;
		E.MaxExp = 127;

		/* do the conversion */
		S2Bin(xin, &B);
		Bin2Dec(&E, &B, &D);
		D.exp += p;		/* adjust exponent for precision */
		UnpDec2Dec(&D, dec);
	}
	new_sticky |= fpgetsticky();

	/* handle exceptions */
	(void) fpsetsticky(new_sticky | old_sticky);	/* update sticky bits */
	faults = new_sticky & fpgetmask();	/* mask out non-trapping exceptions */
	
	if (faults) {
		format[0] = (char) FP_F;
		format[1] = (char) FP_NULL;
		format[2] = (char) FP_DEC;
		format[3] = (char) FP_DEC;

		MMOVSS(xin,&op1.f);
		rres.dec = dec;
		result = fault_handler(faults, format, &op1, &op2, &rres);
		if (result.dec == 0) {
			/* fault handler tried to return zero, but failed
			 * because union contains a pointer
			 */
			(void) memset(dec->i, (int) '0', dec->ilen);
			dec->i[dec->ilen] = '\0';
			(void) strcpy(dec->e, "00");
			dec->sign =
			dec->esign = DEC_PLUS;
		}
		else {
			*dec = *(result.dec);
		}
	}
}

void _d2dec (xin, dec, p)
double	*xin;	/* value to convert */
decimal	*dec;	/* result */
int	p;	/* precision */
{
	DByte		dxin;
	fp_except	old_sticky;
	fp_except	new_sticky;
	fp_union	op1, op2, rres, result;
	int		faults;
	char		format[4];

	/* for MAU */
	int		N, LOGX, SCALE;
	fp_except	IXMASK;
	triple_word	x, y, w3, abs_x;

	/* for FPE */
	B2DEnv	E;
	UnpDec	D;
	UnpBin	B;

	dec->elen = 3;

	MMOVDD(xin,&(dxin.d));

	old_sticky = fpsetsticky(0);
	new_sticky = 0;
	if (is_dinf(dxin.m)) {		/* infinity */
		dec->esign = DEC_INF;
		dec->sign = dxin.m.sign;
	}
	else if (isnand(*xin) || dec->ilen < 1 || dec->ilen > 17 ||
			p < 0 || p >= dec->ilen) {	/* NaN */
		dec->esign = DEC_NaN;
		dec->sign = dxin.m.sign;
		new_sticky |= FP_X_INV;
	}
	else if (is_dzero(dxin.m)) {	/* zero */
		(void) memset(dec->i, (int) '0', dec->ilen);
		dec->i[dec->ilen] = '\0';
		(void) strcpy(dec->e, "000");
		dec->sign = dxin.m.sign;
		dec->esign = DEC_PLUS;
		return;
	}
	else if (_fp_hw) {
		N = dec->ilen;
		MMOVDX(xin,&x);
		abs_x = y = x;
		abs_x.sign = 0;		/* absolute value */
		LOGX = LOG10(abs_x);
		for (;;) {
			SCALE = N - LOGX - 1;
			scale(&x,SCALE);
			IXMASK = fpsetmask(fpgetmask() & ~FP_X_IMP) & FP_X_IMP;
			MFRNDX1(&x);
			(void) fpsetmask(fpgetmask() | IXMASK);
			abs_x = x;
			abs_x.sign = 0;
			power10(N,&w3);
			if (CMPGE(&abs_x,&w3)) {
				LOGX++;
				x = y;
				continue;
			}
			power10(N-1,&w3);
			if (CMPL(&abs_x,&w3)) {
				w3.sign = x.sign;
				x = w3;
			}
			break;
		}
		binstr(&x, dec->i, dec->ilen, &dec->sign);
		itoa(p-SCALE, dec->e, 3, &(dec->esign));
	}
	else {
		/* set up the environment */
		E.style = FloatStyle;
		E.pre = dec->ilen;
		E.MinExp = -1022;
		E.MaxExp = 1023;
	
		/* do the conversion */
		D2Bin(xin, &B);
		Bin2Dec(&E, &B, &D);
		D.exp += p;		/* adjust exponent for precision */
		UnpDec2Dec(&D, dec);
	}
	new_sticky |= fpgetsticky();

	/* handle exceptions */
	(void) fpsetsticky(old_sticky | new_sticky);
	faults = fpgetmask() & new_sticky;
	if (faults) {
		format[0] = (char) FP_D;
		format[1] = (char) FP_NULL;
		format[2] = (char) FP_DEC;
		format[3] = (char) FP_DEC;

		MMOVDD(xin,&(op1.d));
		rres.dec = dec;
		result = fault_handler(faults, format, &op1, &op2, &rres);
		if (result.dec == 0) {
			/* fault handler tried to return zero, but failed
			 * because union contains a pointer
			 */
			(void) memset(dec->i, (int) '0', dec->ilen);
			dec->i[dec->ilen] = '\0';
			(void) strcpy(dec->e, "000");
			dec->sign =
			dec->esign = DEC_PLUS;
		}
		else {
			*dec = *(result.dec);
		}
	}
}

void _dec2s (dec, x, p)
decimal	*dec;	/* value to convert */
float	*x;	/* place to put result */
int	p;	/* precision */
{
	const static single_word SNAN = { 0x0, 0xff, 0x7fffff };	/* NaN */
	const static single_word SINF = { 0x0, 0xff, 0x0 };		/* +inf */
	const static single_word SZERO = { 0x0, 0x0, 0x0 };		/* +0 */
	const static single_word SHUGE = { 0x0, 0xfe, 0x7fffff };	/* +huge */
	SByte		temp;
	XByte		t;
	fp_except	old_sticky;
	fp_except	new_sticky;
	int		faults;
	int		exponent;
	char		format[4];
	fp_union	op1, op2, rres;
	char		*old_i, *old_e;

	/* for MAU */
	int		NMAX, SCALE, LOST, ZERO_PAD, N, cnt;
	triple_word	x3;
	fp_except	MASK;

	/* for FPE */
	D2BEnv	E;
	UnpDec	D;
	UnpBin	B;

	/* strip leading zeros */
	for (old_i = dec->i; dec->i[0] == '0'; dec->i++)
		if (dec->i[1] == '\0')
			break;			/* don't overrun the zero string */
	for (old_e = dec->e; dec->e[0] == '0'; dec->e++)
		if (dec->e[1] == '\0')
			break;			/* don't overrun the zero string */
	dec->ilen = strlen(dec->i);
	dec->elen = strlen(dec->e);
	old_sticky = fpsetsticky(0);
	new_sticky = 0;
	if (dec->esign == DEC_INF) {	/* infinity */
		temp.m = SINF;
		temp.m.sign = dec->sign;
		MMOVSS(&temp.s,x);
	}
	else if (dec->ilen < 1 || dec->ilen > 9 || 
			dec->elen < 1 || dec->elen > 2 || 
			p < 0 || p >= (int)strlen(old_i) ||
			dec->esign == DEC_NaN ||
			badstr(dec->i, dec->ilen) || badstr(dec->e, dec->elen)) {
		/* NaN or garbled input */
		temp.m = SNAN;
		MMOVSS(&temp.s,x);
		new_sticky |= FP_X_INV;
	}
	else if (dec->i[0] == '0') {	/* zero */
		temp.m = SZERO;
		temp.m.sign = dec->sign;
		MMOVSS(&temp.s,x);
	}
	else if (_fp_hw) {
		MASK = fpsetmask(0);
		NMAX = 9;
		N = dec->ilen;
		/* D1 */
		SCALE = atoi(dec->e) * (dec->esign == 0 ? 1 : -1) - p;
		/* D2 */
		ZERO_PAD = 0;
		LOST = 0;
		if (N > NMAX) {
			for (cnt=NMAX+1; cnt<=N; cnt++)
				LOST = LOST || (dec->i[cnt] != '0');
			SCALE += N - NMAX;
		}
		/* D3 */
		else if (SCALE > 0) {
			ZERO_PAD = (NMAX-N<SCALE ? NMAX-N : SCALE);
			SCALE -= ZERO_PAD;
		}
		else if (SCALE < 0) {
			for (; dec->i[N] == '0' && SCALE != 0; N--, SCALE++) ;
		}
		/* D4 */
		strbin(dec->i, &x3, N, ZERO_PAD, dec->sign);
		/* D5 */
		scale(&x3, SCALE);
		/* D6 */
		x3.frac_lo |= LOST;
		MMOVXS(&x3,x);
		exponent = x3.exp - 16383;
		(void) fpsetmask(MASK);
	}
	else {
		/* set up the environment */
		E.pre = 24;
		E.MinExp = -126;
		E.MaxExp = 127;

		/* do the conversion */
		Dec2UnpDec(dec, &D);
		D.exp -= p;
		Dec2Bin(&E, &D, &B);
		Bin2S(B, x);
	}
	new_sticky |= fpgetsticky();
	dec->i = old_i;			/* restore original strings */
	dec->e = old_e;

	/* handle exceptions */
	(void) fpsetsticky(old_sticky | new_sticky);
	faults = fpgetmask() & new_sticky;
	if (faults != 0) {
		format[0] = (char) FP_DEC;
		format[1] = (char) FP_NULL;
		format[2] = (char) FP_F;
		format[3] = (char) FP_F;
		op1.dec = dec;
		if (!_fp_hw) {
			E.MinExp = -16382;
			E.MaxExp = 16383;
			Dec2Bin(&E, &D, &B);
			Bin2X(B, &x3);
			exponent = B.exp;
		}
		/* Adjust result for fault handler */
		if (faults & FP_X_OFL && exponent - 192 > 16383 ||
		    faults & FP_X_UFL && exponent + 192 < -16382) {
			temp.m = SNAN;
			MMOVSS(&temp.s,&rres.f);
		}
		else if (faults & FP_X_OFL) {
			format[2] = (char) FP_X;
			t.m = x3;
			t.m.exp -= 192;
			rres = t.u;
		}
		else if (faults & FP_X_UFL) {
			format[2] = (char) FP_X;
			t.m = x3;
			t.m.exp += 192;
			rres = t.u;
		}
		else
			MMOVSS(x,&rres.f);
		
		t.u = fault_handler(faults, format, &op1, &op2, &rres);
		MMOVSS(&t.u.f,x);
	}
	else if (new_sticky & FP_X_OFL) {
		switch (fpgetround()) {
		case FP_RN:
			temp.m = SINF;
			break;
		case FP_RP:
			if (dec->sign == DEC_PLUS)
				temp.m = SINF;
			else
				temp.m = SHUGE;
			break;
		case FP_RM:
			if (dec->sign == DEC_PLUS)
				temp.m = SHUGE;
			else
				temp.m = SINF;
			break;
		case FP_RZ:
			temp.m = SHUGE;
			break;
		}
		temp.m.sign = dec->sign;
		MMOVSS(&temp.s,x);
	}
}

void _dec2d (dec, x, p)
decimal	*dec;	/* value to convert */
double	*x;	/* place to put result */
int	p;	/* precision */
{
	const static double_word DNAN = { 0x0, 0x7ff, 0xfffff, 0xffffffff };	/* NaN */
	const static double_word DINF = { 0x0, 0x7ff, 0x0, 0x0 };		/* +inf */
	const static double_word DZERO = { 0x0, 0x0, 0x0, 0x0 };		/* +0 */
	const static double_word DHUGE = { 0x0, 0x7fe, 0xfffff, 0xffffffff };	/* +huge */
	DByte		temp;
	XByte		t;
	fp_except	old_sticky;
	fp_except	new_sticky;
	int		faults;
	int		exponent;
	fp_union	op1, op2, rres;
	char		format[4];
	char		*old_i, *old_e;

	/* for MAU */
	triple_word	x3;
	int		SCALE, LOST, ZERO_PAD, N, NMAX, cnt;
	fp_except	MASK;

	/* for FPE */
	D2BEnv	E;
	UnpDec	D;
	UnpBin	B;

	/* strip leading zeros */
	for (old_i = dec->i; dec->i[0] == '0'; dec->i++)
		if (dec->i[1] == '\0')
			break;			/* don't overrun the zero string */
	for (old_e = dec->e; dec->e[0] == '0'; dec->e++)
		if (dec->e[1] == '\0')
			break;			/* don't overrun the zero string */
	dec->ilen = strlen(dec->i);
	dec->elen = strlen(dec->e);
	old_sticky = fpsetsticky(0);
	new_sticky = 0;
	if (dec->esign == DEC_INF) {	/* infinity */
		temp.m = DINF;
		temp.m.sign = dec->sign;
		MMOVDD(&temp.d,x);
	}
	else if (dec->ilen < 1 || dec->ilen > 17 || 
			dec->elen < 1 || dec->elen > 3 ||
			p < 0 || p >= (int)strlen(old_i) ||
			dec->esign == DEC_NaN||
			badstr(dec->i, dec->ilen) || badstr(dec->e, dec->elen)) {
		/* NaN or garbled input */
		temp.m = DNAN;
		MMOVDD(&temp.d,x);
		new_sticky |= FP_X_INV;
	}
	else if (dec->i[0] == '0') {	/* zero */
		temp.m = DZERO;
		temp.m.sign = dec->sign;
		MMOVDD(&temp.d,x);
	}
	else if (_fp_hw) {
		MASK = fpsetmask(0);
		NMAX = 17;
		N = dec->ilen;
		/* D1 */
		SCALE = atoi(dec->e) * (dec->esign == 0 ? 1 : -1) - p;
		/* D2 */
		ZERO_PAD = 0;
		LOST = 0;
		if (N > NMAX) {
			for (cnt=NMAX+1; cnt<=N; cnt++)
				LOST = LOST || (dec->i[cnt] != '0');
			SCALE += N - NMAX;
		}
		/* D3 */
		else if (SCALE > 0) {
			ZERO_PAD = (NMAX-N<SCALE ? NMAX-N : SCALE);
			SCALE -= ZERO_PAD;
		}
		else if (SCALE < 0) {
			for (; dec->i[N] == '0' && SCALE != 0; N--, SCALE++) ;
		}
		/* D4 */
		strbin(dec->i, &x3, N, ZERO_PAD, dec->sign);
		/* D5 */
		scale(&x3, SCALE);
		/* D6 */
		x3.frac_lo |= LOST;
		MMOVXD(&x3,x);
		exponent = x3.exp - 16383;
		(void) fpsetmask(MASK);
	}
	else {
		/* set up the environment */
		E.pre = 53;
		E.MinExp = -1022;
		E.MaxExp = 1023;

		/* do the conversion */
		Dec2UnpDec(dec, &D);
		D.exp -= p;
		Dec2Bin(&E, &D, &B);
		Bin2D(B, x);
	}
	new_sticky |= fpgetsticky();
	dec->i = old_i;
	dec->e = old_e;

	/* handle exceptions */
	(void) fpsetsticky(old_sticky | new_sticky);
	faults = fpgetmask() & new_sticky;
	if (faults != 0) {
		format[0] = (char) FP_DEC;	/* op1's type */
		format[1] = (char) FP_NULL;	/* op2's type */
		format[2] = (char) FP_D;	/* intermediate value's type */
		format[3] = (char) FP_D;	/* result's type */
		op1.dec = dec;
		if (!_fp_hw) {
			E.MinExp = -16382;
			E.MaxExp = 16383;
			Dec2Bin(&E, &D, &B);
			Bin2X(B, &x3);
			exponent = B.exp;
		}
		/* Adjust result for fault handler */
		if (faults & FP_X_OFL && exponent - 1536 > 16383 ||
		    faults & FP_X_OFL && is_xinf(x3) ||
		    faults & FP_X_UFL && exponent + 1536 < -16382 ||
		    faults & FP_X_UFL && is_xzero(x3)) {
			temp.m = DNAN;
			rres.d = temp.d;
		}
		else if (faults & FP_X_OFL) {
			format[2] = (char) FP_X;
			t.m = x3;
			t.m.exp -= 1536;
			rres = t.u;
		}
		else if (faults & FP_X_UFL) {
			format[2] = (char) FP_X;
			t.m = x3;
			t.m.exp += 1536;
			rres = t.u;
		}
		else
			MMOVDD(x,&rres.d);
		
		t.u = fault_handler(faults, format, &op1, &op2, &rres);
		MMOVDD(&t.u.d,x);
	}
	else if (new_sticky & FP_X_OFL) {
		switch (fpgetround()) {
		case FP_RN:
			temp.m = DINF;
			break;
		case FP_RP:
			if (dec->sign == DEC_PLUS)
				temp.m = DINF;
			else
				temp.m = DHUGE;
			break;
		case FP_RM:
			if (dec->sign == DEC_PLUS)
				temp.m = DHUGE;
			else
				temp.m = DINF;
			break;
		case FP_RZ:
			temp.m = DHUGE;
			break;
		}
		temp.m.sign = dec->sign;
		MMOVDD(&temp.d,x);
	}
}
