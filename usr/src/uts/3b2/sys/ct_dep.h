/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_CT_DEP_H
#define _SYS_CT_DEP_H

#ident	"@(#)head.sys:sys/ct_dep.h	11.2"

/*
 *	CTC Firmware - specific I/O dependencies
 */

/*
 *  Define the Application field of the Request and Completion Queues.
 *  This field contains parameter information for a Subdevice request.
 */
typedef	struct	{
	unsigned  long  blkno;		/*  physical block number */
	unsigned  long  jid;		/*  job id buffer address  */
	unsigned  long  jio_start;	/*  time stamp of job to FW */
} RAPP;

typedef struct {
	unsigned long jio_start;	/*  time stamp of job to FW */

	unsigned char rsrv1;
	unsigned char rsrv2;
	unsigned char rsrv3;
	unsigned char command;		/*  original command sent to FW */
} CAPP;

/*  Define number of elements in the request and completion queues  */

#define RQSIZE		16
#define	CQSIZE		(RQSIZE * 2)

/* Address for find request and completion queues. */

#define	R_ADDR	ct_rq
#define	C_ADDR	ct_cq

/*  Define the number of Subdevices for the EDT  */

#define	NUM_DEVS	2

/*  Define the total number of queues */

#define	NUM_QUEUES	1

/* Define the general purpose queue index */

#define GEN_QUE	0

/*  Define the EDT structure  */

typedef	struct	{
		long	opt_codes[NUM_DEVS];
		}EDT;

/* Board ID */

#define	D_CTC	5
#define BDID	D_CTC

/* FLoppy Tape subdevice ID */

#define FT25 4

/*
 *  CTC Specific return status defines for Common I/O queue entries
 *    (reserved 4 - 9 for new cio_defs.h defines)
 *
 *  Additional definitions can be found in file "cio_def.h".
 */

#define	FULL_Q		20		/*  request queue is full  */
#define	EMPTY_Q		21		/*  completion queue is empty  */
#define	UN_INIT		22		/*  ctsysgen() was never run  */
#define	MEM_FAIL	23		/*  bad memory address spec for CTC  */
#define	MEMFLW		24
#define	PAGE_ERR	25

#define	INVDEVN		30		/*  invalid device number  */
#define	NCONFDEV	31		/*  unconfigured device  */
#define	HDWERROR	32		/*  device hardware error  */
#define	RDONLY		33		/*  device is read only  */
#define	FORMWTNG	34		/*  format write error  */
#define	FORMRDNG	35		/*  format read error  */
#define	NOTRDY		36		/*  device not ready  */
#define	RWERROR		37		/*  read/write error  */
#define	CTNVTOC		38		/*  device contains no vtoc info */
#define	NOTOPN		39		/*  device not open */
#define	ISOPEN		40		/*  device is already opened */
#define	CTSRWER		41		/*  stream job failed last r/w */
#define	CTNOMEDIA	42		/*  no media preset */

/*    Opcodes of CTC commands - additional opcodes to those defined in header
 *				file "cio_defs.h".    
 *
 *    The opcodes are grouped according to request queue type. All Subdevice
 *    specific commands must be grouped together and have opcode values larger
 *    than CIO opcodes and general request opcodes.
 */

/*  Subdevice specific request opcodes  */

#define	CTCONFIG	30		/*  configure subdevices  */
#define	CTCLOSE		31		/*  close a subdevice  */
#define	CTFORMAT	32		/*  format media  */
#define	CTOPEN		33		/*  open a subdevice  */
#define	CTREAD		34		/*  read subdevice blocks  */
#define	CTWRITE		35		/*  write subdevice blocks  */
#define	CTVWRITE	36		/*  write-verify subdevice blocks */

/*
 * CTOPEN bytcnt flag defines:
 */

#define	NORM_OPN	0
#define	STREAM_RD	1
#define	STREAM_WT	2

/* define the size of a downloadable page on the CTC */

#define PAGESZ 1024

#endif	/* _SYS_CT_DEP_H */
