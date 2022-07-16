/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sh:name.h	1.7.1.1"
/*
 *	UNIX shell
 */


#define	N_ENVCHG 0020
#define N_RDONLY 0010
#define N_EXPORT 0004
#define N_ENVNAM 0002
#define N_FUNCTN 0001

#define N_DEFAULT 0

struct namnod
{
	struct namnod	*namlft;
	struct namnod	*namrgt;
	unsigned char	*namid;
	unsigned char	*namval;
	unsigned char	*namenv;
	int	namflg;
};

