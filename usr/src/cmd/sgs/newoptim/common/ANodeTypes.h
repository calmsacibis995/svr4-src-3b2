/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/ANodeTypes.h	1.5"

/* USERADATA and USERATYPE are defined in target.h in Mach. Dep. */

struct addrdata				/* Node data. */
	{long int estim;		/* Address usage estimate. */
	 unsigned AddIndexP:1;		/* Address index item present. */
	 unsigned delete:1;		/* 1 if node is deleted. */
	 unsigned dup:1;		/* Dup: node promoted to translation. */
	 unsigned fp:2;			/* Floating Point item. */
	 unsigned gnaq:6;		/* GNAQ type. */
	 unsigned hide:1;		/* 1 if node is hidden. */
	 unsigned label:1;		/* 1 if label node.	*/
	 unsigned MDLpresent:1;		/* 1 if MDLength present. */
	 unsigned private:1;		/* 1 if private; else 0.	*/
	 unsigned safe:1;		/* Safe from volatility. */
	 unsigned tiq:1;		/* Type independent address quantity. */
	 unsigned ro:1;			/* 1 if read-only. */
	 unsigned translation:1;	/* 1 if translation node; else 0. */
	 unsigned candidate:1;		/* 1 if for candidate use only.	*/
	 unsigned auditerr:1;		/* 1 if audit error already reported. */
	 unsigned short int AddrIndex;	/* Address index. */
	 unsigned short int MDLength;	/* Maximum number of bytes for a GNAQ.*/
	 unsigned short int serial;	/* Address node serial number.	*/
	 unsigned short int slot;	/* Address table slot this is in. */
	};

					/* WARNING: Do not tamper with the */
					/* items in structures addrkey or  */
					/* addrmush, even their order, unless */
					/* you KNOW WHAT YOU ARE DOING!!!  */
struct addrkey				/* Node access key. */
	{long int con;			/* Constant portion of expression. */
	 char *string;			/* String portion of expression. */
	 unsigned char rega;		/* Register identifiers. */
	 unsigned char mode;		/* Address mode. */
	 unsigned char regb;
	 unsigned char tempid;		/* Id of a temp register (CSE).	*/
	};
struct addrmush				/* Stuff to make hashing and	*/
	{long int con;			/* comparing faster.	*/
	 char *string;
	 unsigned long int mrrt;
	};
union addrKEY
	{struct addrkey K;
	 struct addrmush M;
	};

struct addrent				/* Structure of a real address node. */
	{				/* Node management. */
	 struct addrent *next;		/* Hash table chain. */
	 struct addrent *privt;		/* Private table chain. */
	 struct addrent *forw;		/* Forward GNAQ list pointer. */
	 struct addrent *back;		/* Backward GNAQ list pointer. */
	 struct addrent *usesa;		/* Addresses used by this. */
	 struct addrent *usesb;
	 union addrKEY key;		/* Key items for this node. */
	 struct addrdata data;		/* Data items for this node. */
#ifdef USERADATA
	 USERATYPE userdata;		/* Target dependent data. */
#endif /* USERDATA */
	};


enum AN_GnaqType_E {Other=(1<<0),		/* GNAQ type. */
		    NAQ=(1<<1),		/* Non-Aliasable Quantity. */
		    ENAQ=(1<<2),	/* External Non-Aliasable Quantity. */
		    SENAQ=(1<<3),	/* External Static Non-Aliasable Quantity.*/
		    SNAQ=(1<<4),	/* Local Static Non-Aliasable Quantity. */
		    SV=(1<<5)		/* Stack Variable. */
		   };

enum AN_Mode_E	{Immediate,		/* Addressing modes. */
		 Absolute,
		 AbsDef,		/* Absolute deferred.. */
		 CPUReg,		/* CPU Register. */
		 Disp,			/* Displacement. */
		 DispDef,		/* Displacement deferred. */
		 PreDecr,		/* Auto pre-decrement. */
		 PreIncr,		/* Auto pre-increment. */
		 PostDecr,		/* Auto post-decrement. */
		 PostIncr,		/* Auto post-increment. */
		 IndexRegDisp,		/* Indexed register with displacement.*/
		 IndexRegScaling,	/* Indexed register with scaling. */
		 MAUReg,		/* MAU register. */
		 StatCont,		/* Status or Control bit. */
		 Raw,			/* Raw string.	*/
		 Undefined		/* Don't know. */
		};

enum AN_Fp_E	{MAYBE=0,		/* don't know. */
		 YES,			/* definitely floating point. */
		 NO			/* definitely not float point.*/
		};

typedef enum AN_GnaqType_E AN_GnaqType;	/* Type of a GNAQ type. */
typedef struct addrent *AN_Id;		/* Type of an address-node-identifier.*/
typedef enum AN_Mode_E AN_Mode;		/* Type of an address-mode-identifier.*/
