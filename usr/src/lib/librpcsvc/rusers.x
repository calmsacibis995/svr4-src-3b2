
#ident	"@(#)librpcsvc:rusers.x	1.1"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
*          All rights reserved.
*/ 
/* 
 * rusers.x
 * This file exists just for informational purposes.
 */

#ifdef RPC_HDR
%/*
% * the structure (data object) passed between the rusers program and
% * rpc.rusersd. For historical reasons it is the utmp structure for the
% * bsd sytems. It is the data object that is included in the stuctures 
% * passed between the client and the rusers service.
% */
%
#endif

struct ru_utmp {
	char	ut_line[8];		/* tty name */
	char	ut_name[8];		/* user id */
	char	ut_host[16];		/* host name, if remote */
	long	ut_time;		/* time on */
};

#ifdef RPC_HDR
%
%struct utmparr {
%	ru_utmp **uta_arr;
%	int uta_cnt;
%};
%typedef struct utmparr utmparr;
%bool_t xdr_utmparr();
%
#endif

struct utmpidle {
	ru_utmp ui_utmp;
	unsigned ui_idle;
};

#ifdef RPC_HDR
%
%struct utmpidlearr {
%	utmpidle **uia_arr;
%	int uia_cnt;
%};
%typedef struct utmpidlearr utmpidlearr;
%bool_t xdr_utmpidlearr();
%
#endif

program RUSERPROG {
	version RUSERSVERS_ORIG {
		u_long
		RUSERSPROC_NUM(void) = 1;
		utmparr
		RUSERSPROC_NAMES(void) = 2;
		utmparr
		RUSERSPROC_ALLNAMES(void) = 3;
	} = 1;
	version RUSERSVERS_IDLE {
		u_long
		RUSERSPROC_NUM(void) = 1;
		utmpidlearr
		RUSERSPROC_NAMES(void) = 2;
		utmpidlearr
		RUSERSPROC_ALLNAMES(void) = 3;
	} = 2;
} = 100002;

#ifdef RPC_HDR
%#define RUSERSVERS RUSERSVER_IDLE
%#define MAXUSERS 100
#endif
