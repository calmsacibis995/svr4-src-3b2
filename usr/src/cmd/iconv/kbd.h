/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)iconv:kbd.h	1.1"



/*
 * Misc definitions fundamental to the implementation.  If any of them
 * change, everything has to be re-compiled.  Mostly these are boundary
 * conditions.  Entries with "+1" mean that the last position is reserved
 * for the null character, as they are character STRING sizes.
 */
#define KBDNL	16	/* max name length (+1) */
#define KBDIMAX	128	/* max length of a search string */
#define KBDOMAX 256	/* max length of a result string */
#define KBDLNX	16	/* max links in composite */
#define KBDBUFSIZE 512
#define KBDREADBUF (KBDBUFSIZE / 8)


/*
 * cornode c_flags.  Anything not defined here is RESERVED!  Don't
 * touch, please.
 */
#define ND_RESULT	0x01
#define ND_TIMER	0x02
#define ND_INLINE	0x04
#define ND_LAST		0x80
#define ND_RESERVED	0x78	/* all the reserved bits */

/*
 * the final node structure "in core node"
 */
struct cornode {
	unsigned short c_child;
	unsigned char c_val;
	unsigned char c_flag;	/* high bit on == end of list */
};

/*
 * One header per file
 */
struct kbd_header {
	unsigned char h_magic[10];	/* magic number */
	short h_ntabs;	/* number of tables in the file */
};

/*
 * t_flag definitions:
 */
#define KBD_ONE	0x01	/* contains one-one */
#define KBD_FULL 0x02	/* "filled" root */
#define KBD_ERR 0x04	/* contains Error node */
#define KBD_TIME 0x08	/* default timed table */
#define KBD_ALP 0x10	/* reserved ALP bit */
#define KBD_COT 0xFF	/* is a composite table, NOT a real table */

/*
 * One "table" structure per table, "h_ntabs" per file.
 * FIX ME: could be divvied up better so that LESS needs to be
 * kept on disk -- there are lots of unused fields stored out there.
 * Also, remove t_swtch -- it's being phased out.
 */
struct kbd_tab {
	unsigned char t_name[KBDNL];	/* name of table */
	unsigned short t_nodes;	/* number of NODES */
	unsigned short t_text;	/* total length of result text */
	unsigned short t_flag;	/* flags (incl. oneone flag) */
	unsigned short t_error;	/* error "text" space */
	unsigned char t_min;	/* min & max root values */
	unsigned char t_max;

	struct cornode *t_nodep;/* in-core pointer to nodes */
	unsigned char *t_textp;	/* in-core pointer to top of text */
	unsigned char *t_oneone;/* one-one 256-byte direct map */
	struct kbd_tab *t_next;	/* in-core next table */
	struct tablink *t_child;/* in-core for composite tables */
	unsigned int t_asize;	/* in-core sizeof actual allocation */
	unsigned short t_ref;	/* in-core reference count */
};

#define KBD_MAGIC	"kbd!map"
#define KBD_HOFF	8	/* offset in h_magic[] of version number */

/*
 * Version stamps:  The 3B2 and 386 byte order are incompatible, therefore
 * they must have different version numbers to insure that tables compiled
 * for one don't get loaded on the other.  The loader will reject them
 * right away.  Don't define another unless it really won't work for
 * your machine.
 */

#ifdef u3b2
# define KBD_VER		1	/* 3B2 version */
#else
# ifdef i386
#    define KBD_VER		2	/* 386 version */
# endif
#endif
