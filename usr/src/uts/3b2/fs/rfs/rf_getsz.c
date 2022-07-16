/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/rfs/rf_getsz.c	1.6"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/stream.h"
#include "sys/rf_adv.h"
#include "sys/nserve.h"
#include "sys/stat.h"
#include "sys/statfs.h"
#include "sys/vnode.h"
#include "sys/rf_messg.h"
#include "sys/buf.h"
#include "sys/cred.h"
#include "sys/pathname.h"
#include "sys/vfs.h"
#include "sys/list.h"
#include "sys/rf_cirmgr.h"
#include "sys/rf_debug.h"
#include "vm/seg.h"
#include "rf_admin.h"
#include "sys/rf_comm.h"
#include "rf_serve.h"
#include "sys/fs/rf_vfs.h"
#include "rfcl_subr.h"
#include "du.h"
