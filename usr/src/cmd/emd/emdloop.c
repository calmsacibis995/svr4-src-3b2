/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)emd:cmd/emdloop.c	1.2"
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/dlpi.h>
#include <sys/stropts.h>
#include <sys/emduser.h>

char *teststr = "abbcbbbbaaaccaaaaabcaaaaacaabacaaaaabaaacccaaaaacabaabacaaabacaaaaaabababbcbaaaaccaaaabacaaaaaaacbaabaacaaaaaacaaaaaaaccbaaaacaaaabaacaaaabaaaaaaaaaaaccbaaaaaaaaaaabaaaaaacbaaaaaaaaabccaaaabbaa";
int fd;
struct eiseta eiseta;
struct strioctl strioc;
union DL_primitives *p;
dl_unitdata_req_t *reqp;
dl_unitdata_ind_t *indp;
dl_uderror_ind_t *errp;
dl_error_ack_t *nakp;
dl_bind_req_t *bindp;
char dbuf[256];
char cbuf[256];
struct strbuf ctl;
struct strbuf dat;
int ret;
char *cp;
int flag = 0;

main(argc, argv)
	int argc;
	char *argv[];
{
	if (argc != 2) {
		fprintf(stderr, "Usage: emdloop  device\n");
		exit(1);
	}
	if ((fd = open(argv[1], O_RDWR)) < 0) {
		perror ("open failed");
		exit(1);
	}
	strioc.ic_cmd = EI_GETA;
	strioc.ic_timout = INFTIM;
	strioc.ic_len = sizeof(struct eiseta);
	strioc.ic_dp = (char *)&eiseta;
	if (ioctl(fd, I_STR, &strioc) < 0) {
		perror("ioctl failed");
		exit(1);
	}
	dat.buf = NULL;
	dat.len = -1;
	dat.maxlen = 0;
	ctl.buf = cbuf;
	ctl.len = sizeof(dl_bind_req_t);
	ctl.maxlen = sizeof(cbuf);
	bindp = (dl_bind_req_t *)cbuf;
	bindp->dl_primitive = DL_BIND_REQ;
	bindp->dl_sap = 888;
	bindp->dl_max_conind = 0;
	bindp->dl_service_mode = 0;
	if (putmsg(fd, &ctl, &dat, 0) < 0) {
		perror("putmsg for bind failed");
		exit(1);
	}
	get();
	flag = 0;
	dat.buf = teststr;
	dat.len = strlen(teststr) + 1;
	dat.maxlen = dat.len;
	ctl.buf = cbuf;
	ctl.len = sizeof(*reqp) + PHYAD_SIZE;
	ctl.maxlen = sizeof(cbuf);
	reqp = (dl_unitdata_req_t *)cbuf;
	reqp->dl_primitive = DL_UNITDATA_REQ;
	reqp->dl_dest_addr_length = PHYAD_SIZE;
	reqp->dl_dest_addr_offset = sizeof(*reqp);
	cp = cbuf + sizeof(*reqp);
	memcpy(cp, eiseta.eis_addr, PHYAD_SIZE);
	printf("sending string\t<%s>\n", teststr);
	if (putmsg(fd, &ctl, &dat, 0) < 0) {
		perror("putmsg failed");
		exit(1);
	}
	get();
	exit(0);
}

get()
{
	dat.buf = dbuf;
	dat.len = 0;
	dat.maxlen = sizeof(dbuf);
	ctl.buf = cbuf;
	ctl.len = 0;
	ctl.maxlen = sizeof(cbuf);
	ret = getmsg(fd, &ctl, &dat, &flag);
	if (ret < 0) {
		perror("getmsg failed");
		exit(1);
	}
	if (ctl.len <= 0) {
		fprintf(stderr, "ctl.len = %d\n", ctl.len);
		exit(1);
	}
	p = (union DL_primitives *)ctl.buf;
	switch (p->dl_primitive) {
	case DL_UNITDATA_IND:
		indp = (dl_unitdata_ind_t *)cbuf;
		printf("DL_UNITDATA_IND\n");
		printf("%d bytes of data\n", dat.len);
		printf("data buffer is\t<%s>\n", dbuf);
		break;

	case DL_UDERROR_IND:
		errp = (dl_uderror_ind_t *)cbuf;
		printf("DL_UDERROR_IND\n");
		printf("error = %d\n", errp->dl_errno);
		break;

	case DL_ERROR_ACK:
		nakp = (dl_error_ack_t *)cbuf;
		printf("DL_ERROR_ACK\n");
		printf("errno = %d\n", nakp->dl_errno);
		printf("unix error = %d\n", nakp->dl_unix_errno);
		break;

	case DL_BIND_ACK:
		printf("DL_BIND_ACK\n");
		break;

	default:
		fprintf(stderr, "emdloop: unknown primitive %d\n", p->dl_primitive);
		exit(1);
	}
}
