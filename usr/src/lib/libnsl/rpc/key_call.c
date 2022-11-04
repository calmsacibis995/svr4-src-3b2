/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)librpc:key_call.c	1.3"

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

#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)key_call.c 1.14 89/05/02 Copyr 1986 Sun Micro";
#endif 

/*
 * key_call.c, Interface to keyserver
 *
 * setsecretkey(key) - set your secret key
 * encryptsessionkey(agent, deskey) - encrypt a session key to talk to agent
 * decryptsessionkey(agent, deskey) - decrypt ditto
 * gendeskey(deskey) - generate a secure des key
 */

#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>

#define KEY_TIMEOUT	5	/* per-try timeout in seconds */
#define KEY_NRETRY	12	/* number of retries */

#ifdef DEBUG
#define debug(msg)	(void) fprintf(stderr, "%s\n", msg);
#else
#define debug(msg)
#endif /* DEBUG */

static char *MESSENGER = "/usr/sbin/keyenvoy";

key_setsecret(secretkey)
	char *secretkey;
{
	keystatus       status;

	if (!key_call((u_long) KEY_SET, xdr_keybuf, secretkey,
		      xdr_keystatus, (char *) &status)) {
		return (-1);
	}
	if (status != KEY_SUCCESS) {
		debug("set status is nonzero");
		return (-1);
	}
	return (0);
}

key_encryptsession(remotename, deskey)
	char *remotename;
	des_block *deskey;
{
	cryptkeyarg arg;
	cryptkeyres res;

	arg.remotename = remotename;
	arg.deskey = *deskey;
	if (!key_call((u_long)KEY_ENCRYPT, xdr_cryptkeyarg, (char *)&arg,
			xdr_cryptkeyres, (char *)&res)) {
		return (-1);
	}
	if (res.status != KEY_SUCCESS) {
		debug("encrypt status is nonzero");
		return (-1);
	}
	*deskey = res.cryptkeyres_u.deskey;
	return (0);
}

key_decryptsession(remotename, deskey)
	char *remotename;
	des_block *deskey;
{
	cryptkeyarg arg;
	cryptkeyres res;

	arg.remotename = remotename;
	arg.deskey = *deskey;
	if (!key_call((u_long)KEY_DECRYPT, xdr_cryptkeyarg, (char *)&arg,
			xdr_cryptkeyres, (char *)&res)) {
		return (-1);
	}
	if (res.status != KEY_SUCCESS) {
		debug("decrypt status is nonzero");
		return (-1);
	}
	*deskey = res.cryptkeyres_u.deskey;
	return (0);
}

key_gendes(key)
	des_block *key;
{
	if (!key_call((u_long)KEY_GEN, xdr_void, (char *)NULL,
			xdr_des_block, (char *)key)) {
		return (-1);
	}
	return (0);
}

static
key_call(proc, xdr_arg, arg, xdr_rslt, rslt)
	u_long proc;
	bool_t (*xdr_arg)();
	char *arg;
	bool_t (*xdr_rslt)();
	char *rslt;
{
	XDR xdrargs;
	XDR xdrrslt;
	FILE *fargs;
	FILE *frslt;
	void (*osigchild)();
#ifdef WEXITSTATUS 
	int status;
#else
	union wait status;
#endif
	pid_t wpid;
	pid_t pid;
	int success;
	uid_t ruid;
	uid_t euid;

	success = 1;
	osigchild = signal(SIGCHLD, SIG_DFL);
	/* This MUST not be SIG_IGN under SYSV */

	/*
	 * We are going to exec a set-uid program which makes our effective uid
	 * zero, and authenticates us with our real uid. We need to make the
	 * effective uid be the real uid for the setuid program, and
	 * the real uid be the effective uid so that we can change things back.
	 */
	euid = geteuid();
	ruid = getuid();
	/*(void) setreuid(euid, ruid);*/
	if (euid != ruid)
		(void) setuid(euid); /*eff ->real*/ /*dubious if it works*/
	pid = _rpc_openchild(MESSENGER, &fargs, &frslt);
	if (euid != ruid)
		(void) setuid(ruid); /*restore real*/
	/*(void) setreuid(ruid, euid);*/
	if (pid < 0) {
		debug("open_streams");
		return (0);
	}
	xdrstdio_create(&xdrargs, fargs, XDR_ENCODE);
	xdrstdio_create(&xdrrslt, frslt, XDR_DECODE);

	if (!xdr_u_long(&xdrargs, &proc) || !(*xdr_arg)(&xdrargs, arg)) {
		debug("xdr args");
		success = 0;
	}
	(void) fclose(fargs);

	if (success && !(*xdr_rslt)(&xdrrslt, rslt)) {
#ifdef DEBUG
		perror("xdr rslt");
#endif
		success = 0;
	}

	(void) fclose(frslt);
	while (((wpid = wait(&status)) != pid) && (wpid != -1));
	if (wpid < 0 || 
#ifdef WEXITSTATUS
		WEXITSTATUS(status)) {
#else
		(status.w_retcode)) {
#endif
#ifdef DEBUG
		fprintf(stderr, "wait: %d %d %x\n", wpid, pid, status);
#endif
		success = 0;
	}
	(void) signal(SIGCHLD, osigchild);
	return (success);
}

