/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/RegId.h	1.3"

enum RegIdE	{REG_NONE,
		 CCODE_A,CCODE_C,CCODE_N,CCODE_V,CCODE_X,CCODE_Z,
		 CREG0,CREG1,CREG2,CREG3,CREG4,CREG5,CREG6,CREG7,
		 CREG8,CREG9,CREG10,CREG11,CREG12,CREG13,CREG14,CREG15,
		 CREG16,CREG17,CREG18,CREG19,CREG20,CREG21,CREG22,CREG23,
		 CREG24,CREG25,CREG26,CREG27,CREG28,CREG29,CREG30,CREG31,
		 CTEMP,
		 MREG0,MREG1,MREG2,MREG3,MREG4,MREG5,MREG6,MREG7,
		 MFPR,MASR
		};

#define	CFP	CREG9	/* Frame Pointer: a.k.a. Register 9. */
#define	CAP	CREG10	/* Argument Pointer: a.k.a. Register 10. */
#define	CPSW	CREG11	/* Processor Status Word: a.k.a. Register 11. */
#define	CSP	CREG12	/* Stack Pointer: a.k.a. Register 12. */
#define	CPCBP	CREG13	/* PCB Pointer: a.k.a. Register 13. */
#define	CISP	CREG14	/* Interrupt Stack Pointer: a.k.a. Register 14. */
#define	CPC	CREG15	/* Program Counter: a.k.a. Register 15. */

typedef enum RegIdE RegId;
