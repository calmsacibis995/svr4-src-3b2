/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:sys/cunix.h	1.4"


extern char *raw_disk;
extern char *unix_abs;
extern char *loader;
extern char *conf_dir;
extern char *ifile;
extern char *slash_boot;
extern char *etcsystem;
extern char *conf_file;
extern char *pro_file;
extern char *epi_file;
extern char *larg;
extern int  escapehatch;
extern char cwd[];
extern unsigned char loader_type;

extern boolean DebugMode;
extern boolean QuietMode;
extern boolean confdebug;

extern struct s3bboot bootprogram;



