/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/ANode.c	1.14"


/************************************************************************/
/*				ANode.c					*/
/*									*/
/*		This file contains the Address Node Utilities. All the	*/
/*	operations that require knowledge of the implementation of the	*/
/*	address node are meant to reside in this file.			*/
/*									*/
/************************************************************************/

#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	<ctype.h>
#include	<errno.h>
#if uts
#define LONG_MIN -2147483648
#else
#include	<limits.h>
#endif
#include	"defs.h"
#include	"debug.h"
#include	"OperndType.h"
#include	"ANodeTypes.h"
#include	"RegId.h"
#include	"olddefs.h"

#define	ADDRTABSIZ	587	/* Size of address table. */

static unsigned int AddrSerial = {0};	/* Address node serial number.*/
static AN_Id AddrTable[ADDRTABSIZ];	/*Address table pointers to link-list.*/
static char *AN_NullString = {""};	/* One null string is enough! */

#ifndef MACRO
static AN_Id BotANId = {NULL};		/* AN_Id with lowest estim. */
static AN_Id TopANId = {NULL};		/* AN_Id with highest estim. */
#else
AN_Id BotANId = {NULL};			/* AN_Id with lowest estim. */
AN_Id TopANId = {NULL};			/* AN_Id with highest estim. */
#endif /*MACRO*/

static unsigned int Size;			/* Size of these bit-vectors.	*/
static unsigned long int *Enaqs = {NULL};	/* Pointer to ENAQ bit-vector.*/
static unsigned long int *Senaqs = {NULL};	/* Pointer to SENAQ bit-vector.*/
static unsigned long int *Snaqs = {NULL};	/* Pointer to SNAQ bit-vector.*/
static struct addrdata PrNoAdData = {0};	/* Initial empty address data.	*/

	/* private functions */
STATIC int AN_Compar();			/* Address node comparison function. */
STATIC int AN_KeyCompar();		/* Address node comparison function. */
STATIC AN_Id ANFind();			/* Finds address nodes.	*/
STATIC unsigned int ANHashKey();	/* Hashes a key to an unsigned int. */
STATIC AN_Id ANMake();			/* Makes address nodes.	*/
STATIC AN_Id ANMakeNode();		/* Makes a node. */
STATIC void ANPutUses();		/* Puts in used AN_Id's. */
STATIC boolean ANSameKey();		/* Tests address keys for identity. */
	STATIC int
AN_Compar(arg1,arg2)		/* Comparison function for qsort. */
AN_Id *arg1;			/* First argument to compare. */
AN_Id *arg2;			/* Second argument to compare. */

{register short int serial1;
 register short int serial2;

 if((*arg2)->data.estim > (*arg1)->data.estim)
	return(1);
 if((*arg2)->data.estim < (*arg1)->data.estim)
	return(-1);
						/* Break ties.	*/
 serial1 = (*arg1)->data.serial;
 serial2 = (*arg2)->data.serial;
 return(serial1 - serial2);
}


	STATIC int
AN_KeyCompar(arg1,arg2)	/* Comparison function for qsort. */
register AN_Id *arg1;		/* First argument to compare. */
register AN_Id *arg2;		/* Second argument to compare. */


{extern AN_Mode GetAdMode();	/* Gets address node type;in this file. */
 register int temp;

 if((temp = ((int) GetAdMode(*arg1) - (int) GetAdMode(*arg2))) != 0)
	return(temp);			/* Modes don't match.	*/
 if((temp = strcmp((*arg1)->key.K.string,(*arg2)->key.K.string)) != 0)
	return(temp);			/* Expressions don't match. */
 if((temp = ((*arg1)->key.K.con - (*arg2)->key.K.con)) != 0)
	return(temp);
 if((temp = ((int) (*arg1)->key.K.rega - (int) (*arg2)->key.K.rega)) != 0)
	return(temp);			/* Registers A don't match. */
 temp = (int) (*arg1)->key.K.regb - (int) (*arg2)->key.K.regb;
 return(temp);				/* Registers B match? */
}
	STATIC AN_Id
ANFind(tempkey)		/* Find an address node with given key. */
				/* N.B.: We change your string pointer if */
				/* we find a match; it points to our */
				/* string with the same contents. */
register struct addrkey *tempkey;	/* Key to be used. */

{STATIC unsigned int ANHashKey();	/* Hashes a key to an unsigned int. */
 STATIC boolean ANSameKey();		/* Tests address keys for identity. */
 extern AN_Id AddrTable[];		/* Address table pointers. */
 register AN_Id ChainPointer;		/* Address node chain pointer. */
 unsigned int start;			/* Address pointer table index. */

 start = ANHashKey((struct addrmush *) tempkey); /*See if address node exists.*/
 ChainPointer = AddrTable[start];	/* Pointer to first (if any) in chain.*/
 while(ChainPointer)			/* Follow the chain. */
 	{if(ANSameKey(&ChainPointer->key.M,(struct addrmush *) tempkey) == FALSE)
		ChainPointer = ChainPointer->next;	/* Follow the chain. */
	 else
		{tempkey->string = ChainPointer->key.K.string;  /*Point to ours.*/
		 return(ChainPointer);	/* It matches: return its identifier. */
		}
	}
 return(NULL);				/* If not on the chain, return NULL. */
}
	STATIC unsigned int
ANHashKey(key_ptr)		/* Hash an address node key. */
struct addrmush *key_ptr;	/* Pointer to key to be hashed. */

{int nextchar;
 register char *sp;
 register unsigned int stringscan;	/* String scanner. */
 register unsigned long int sumL;	/* Where we perform the initial hash. */

 sumL  = (key_ptr->con << 1);
 sumL += key_ptr->mrrt;
 stringscan = 18;
 for(sp = key_ptr->string; (nextchar = *sp) != EOS; ++sp)
	{ /* this is really sumL += (nextchar << (stringscan % 18))
	   * without the % operation.
	   */
	 if(stringscan == 18){
		sumL += nextchar;
		stringscan = 1;
		}
	 else{
		sumL += (nextchar << stringscan);
	 	stringscan++;
		}
	}

 return(sumL % ADDRTABSIZ);				/* Return result. */
}
	STATIC AN_Id
ANMake(tempkey,tempstring)	/* Make an address node: if */
				/* tempstring is non-null, make */
				/* space for its string and insert */
				/* a pointer to it into the key. */
				/* N.B.: We change your string pointer if */
				/* we put a string pointer into our node. */
struct addrkey *tempkey;	/* Pointer to the key. */
char *tempstring;		/* Pointer to the string, if any. */

{STATIC AN_Id ANMakeNode();	/* Makes a node; in this file. */
 extern boolean IsLegalAd();		/* TRUE for legal addresses.	*/
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();			/* Handles fatal errors.	*/
 char *space;			/* Space for string pointer. */
 unsigned int stringsize;	/* Place for length of string.*/

 if(!IsLegalAd((AN_Mode) tempkey->mode,	/* See if this would be a legal	*/
		(RegId) tempkey->rega,	/* address.	*/
		(RegId) tempkey->regb,
		tempkey->con,
		tempkey->string))
	fatal("ANMake: request for illegal address.\n");

 if(tempstring != (char *) 0)		/* If there is a string pointer, */
	{stringsize = strlen(tempstring);	/* measure the string. */
	 if(stringsize == 0)			/* If it is null, */
		space = AN_NullString;		/* use standard null string. */
	 else
		{space = Malloc(stringsize+1);	/* Get space for it. */
		 if(space == NULL)		/* Is there space? */
			fatal("ANMake: no space for expression string (%d).\n",
				errno);
		 else
			{(void) strncpy(space,tempstring,(int) stringsize);
			 space[stringsize] = EOS;	/* Be sure it stops. */
			}
		}
	 tempkey->string = space;		/* Put pointer in key. */
	}

 return(ANMakeNode(tempkey));			/* Make desired node. */
}
	STATIC AN_Id
ANMakeNode(tempkey)		/* Make a new node and put in chain. */
struct addrkey *tempkey;	/* Key of new node to make. */

{STATIC void ANPutUses();	/* Puts in used AN_Id's; in this file. */
 extern void AN_SetFlags();	/* Sets address node flags; Mach. Dep. */
 extern unsigned int AddrSerial;	/* Address node serial number.*/
 extern AN_Id AddrTable[];	/* Address pointer table. */
 STATIC unsigned int ANHashKey();	/* Hashes a key; in this file. */
 extern AN_Id AddrTable[];	/* Address pointer table. */
 extern boolean IsADPrivate();	/* TRUE if an address node is private.	*/
 extern struct addrdata PrNoAdData;	/* Empty address node data structure. */
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Fatal error handler; in debug.c. */
 unsigned int index;		/* Address Pointer table index. */
 register AN_Id newnode;	/* Pointer to the new address node. */

 newnode = (AN_Id) Malloc(sizeof(struct addrent));	/* Get space for it. */
 if(newnode == NULL)				/* Did we get the space? */
	fatal("ANMakeNode: Malloc failed (%d).\n",errno);	/* No: give up.*/

 newnode->key.K = *tempkey;			/* Yes: fill in the key. */
						/* Set default data. */
 newnode->data = PrNoAdData;			/* Everything 0 to start. */
 newnode->data.serial = AddrSerial++;		/* Set node serial number. */
 if(AddrSerial == 0)				/* Did we use too many?	*/
	fatal("ANMakenode: too many address nodes.\n");
 newnode->data.private = (IsADPrivate(newnode)) ? 1 : 0;
 AN_SetFlags(newnode);				/* Set default flags. */

						/* Stick node in list. */
 index = ANHashKey(&newnode->key.M);		/* Where to start to insert. */
 newnode->next = AddrTable[index];		/* At beginning: just easier. */
 newnode->data.slot = (unsigned short) index;	/* Keep track where we are */
						/* (for GetAdNextNode()). */
 AddrTable[index] = newnode;

 newnode->forw = (AN_Id) 0;			/* Not in GNAQ list. */
 newnode->back = (AN_Id) 0;
 ANPutUses(newnode);				/* Put in Uses data.	*/

 return(newnode);
}
	long int
ANNormExpr(expr,stringp)		/* Normalizes an expression; */
					/* returns a long int that is the */
					/* numeric part of the expression. */
					/* Sets 'stringp' to point to the */
					/* string part */
char *expr;				/* Pointer to the expression. */
char **stringp;				/* Where to put string part. */

{long int constant;		/* Constant part of expression. */
 register char *e;		/* Local expression scan pointer. */
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 long int i;
 char *p;			/* Extra local expression scan pointer. */
 unsigned int parenlvl;		/* Parenthesis level counter. */
 char sign;			/* Sign of result. */
 static char *s0;
 static char *sn;
 register char *string;		/* Output string. */
 extern long strtol();		/* Convert string to a long (3C).	*/
 extern char *ExtendCopyBuf();

 constant = 0L;					/* Initialize conversion. */
 e = expr;					/* Set local scan pointer. */
 p = expr;					/* Set extra scan pointer. */
 sign = NULL;
 if(s0 == NULL)
	{s0 = Malloc(EBUFSIZE+1);
	 if(s0 == NULL)
		fatal("ANNormExpr: out of string space\n");
	 sn = s0 + EBUFSIZE;
	}
 string = s0;
 for(;;)
	{SkipWhite(e);				/* Skip white space.	*/
	 if((*e == '+') || (*e == '-'))		/* If there is a sign, */
		sign = *e++;			/* use it. */
	 SkipWhite(e);				/* Skip white space.	*/
	 if((*e == '+') || (*e == '-'))		/* If there is another sign, */
		fatal("ANNormExpr: extra operator in expression: '%s`.\n",
			expr);
	 if(*e == EOS)
		{if((sign == '+') || (sign == '-'))	/* If a sign, */
			fatal( "ANNormExpr: missing operand in expression: '%s`.\n",
				expr);
		 break;
		}
	 if(sign == SPACE)
		fatal( "ANNormExpr: missing or unsupported operator in expression: '%s`.\n",
			expr);
	 /* We make the following assumption about the literals put
	    out by the compiler in operands: they are in the range
	    LONG_MIN to LONG_MAX.  With the ANSI version of strtol,
	    the conversion routine will work properly EXCEPT if we
	    are converting " - 2147483648 " which should be MIN_LONG,
	    but in the ANSI version gives - MAX_LONG with overflow.
	    One solution would be to pass the string the minus sign
	    as part of the string to strtol.  We take a more conservative
	    approach: check for the one bad case. */
	    
	    
	 errno = 0;
	 i = strtol(e,&p,0);
	 if (errno == ERANGE && sign == '-') {
		sign = '+';	
		i = LONG_MIN;
	 }
	 SkipWhite(p);				/* Skip white space.	*/
	 if((*p == '+') || (*p == '-') || (*p == NULL) || PIC_flag && (*p == AtChar))
			/* If operator or end,*/
		{switch(sign)			/*"Add" in last constant part.*/
			{case NULL:		/* (Fall through to '+' case.)*/
			 case '+':		/* Add it. */
				constant += i;
				endcase;
			 case '-':		/* Subtract it. */
				constant -= i;
				endcase;
			 default:		/* IMPOSSIBLE! */
				fatal("ANNormExpr: disaster.\n");
				endcase;
			}
		 sign = SPACE;
		 e = p;

	 /* Next statement is a patch for PIC style code:
		we are tricking the optimizer to treat
		something like X+4@PC as if it were X@PC+4(%pc).
		The optimizer thinks X@PC is just a plain old
		symbol. */

		 if (PIC_flag && (*e == AtChar)) {
			/* concatinate the @.. into the string part */
		     while((!isspace(*e)) && (*e != '+') && (*e != '-') && (*e != EOS))
			{if(string >= sn)
				string = ExtendCopyBuf(&s0,&sn,(unsigned)2*(sn-s0+1));
			 SkipWhite(e);
			 *string++ = *e++;
			}
		     if (*e != EOS)
		         fatal("ANNormExpr(): could not parse %s\n",expr);
		 } /* end of patch */

		 continue;
		     
		}
	 if(string >= sn)
		string = ExtendCopyBuf(&s0,&sn,(unsigned)2*(sn-s0+1));
	 *string++ = (sign == NULL) ? '+' : sign;	/* Put out the sign. */
	 while((!isspace(*e)) && (*e != '+') && (*e != '-') && (*e != EOS))
		{if(*e == '(')			/* Copy stuff in parenthesis. */
			{if(string >= sn)
				string = ExtendCopyBuf(&s0,&sn,(unsigned)2*(sn-s0+1));
			 *string++ = *e++;	/* Copy the (. */
			 parenlvl = 1;
			 while(parenlvl != 0)	/* Copy stuff in parenthesis. */
				{if(*e == ')')	/* Coming out of parens.? */
					parenlvl -= 1;	/* Yes. */
				 else if(*e == '(')	/* Getting in deeper? */
					parenlvl += 1;	/* Yes. */
				 else if(*e == EOS)
					fatal("ANNormExpr: missing ): '%s`.\n",
						expr);
				 if(!isspace(*e))	/* Skip whitespace.*/
					{if(string >= sn)
						string = ExtendCopyBuf(&s0,&sn,(unsigned)2*(sn-s0+1));
					 *string++ = *e;
					}
				 e++;		/* Increment scan pointer. */
				}		/* END OF while(parenlvl > 0) */
			}			/* END OF if(*e == '(') */
		 else
			{if(string >= sn)
				string = ExtendCopyBuf(&s0,&sn,(unsigned)2*(sn-s0+1));
			 *string++ = *e++;	/* Copy other. */
			}
		}
	 sign = SPACE;				/* Sign required if more. */
	}
 *string = EOS;					/* String terminator. */
 *stringp = s0;
 return(constant);				/* Return the constant part. */
}
	STATIC void
ANPutUses(an_id)		/* Put uses pointers in an address node. */
AN_Id an_id;			/* Id of address node to get pointers. */

{STATIC AN_Id ANFind();		/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();		/* Makes address nodes; in this file. */
 extern void fatal();		/* Handles fatal messages */
 AN_Mode mode;			/* Mode of addr node getting uses pointers. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;
 AN_Id used;			/* Id of used address. */

 an_id->usesa = an_id;		/* Initialize uses pointers. */
 an_id->usesb = an_id;

 switch(mode = (AN_Mode) an_id->key.K.mode)
	{
	 case IndexRegDisp:
	 case IndexRegScaling:
		tempkey.mode = (unsigned char) CPUReg;
		tempkey.rega = an_id->key.K.regb;
		tempkey.regb = (unsigned char) REG_NONE;
		tempkey.string = AN_NullString;
		tempkey.con = 0L;
		tempkey.tempid = (unsigned char) 0;

		used = ANFind(&tempkey);	/* Search for usesb node. */
		if(used == NULL)		/* If it doesn't exist yet, */
			used = ANMake(&tempkey,(char *) 0); /* make it. */
		an_id->usesb = used;		/* Put it in the node. */
		/* FALLTHRU */
	 case Disp:
	 case PreDecr:
	 case PreIncr:
	 case PostDecr:
	 case PostIncr:
		tempkey.mode = (unsigned char) CPUReg;
		tempkey.rega = an_id->key.K.rega;
		tempkey.regb = (unsigned char) REG_NONE;
		tempkey.string = AN_NullString;
		tempkey.con = 0L;
		tempkey.tempid = an_id->key.K.tempid;
		tempstring = (char *)0;
		endcase;
	 case DispDef:
		tempkey = an_id->key.K;
		tempkey.mode = (unsigned char) Disp;
		tempstring = tempkey.string;
		endcase;
	 case AbsDef:
		tempkey = an_id->key.K;
		tempkey.mode = (unsigned char) Absolute;
		tempstring = tempkey.string;
		endcase;
	 case Absolute:
	 case CPUReg:
	 case StatCont:
	 case Immediate:
	 case MAUReg:
	 case Raw:
		return;
		/* NOT REACHED */
	 default:
		fatal("ANPutUses: unknown mode (%d).\n", mode);
		endcase;
	}

 used = ANFind(&tempkey);			/* Search for usesa node. */
 if(used == NULL)				/* If it doesn't exist yet, */
	used = ANMake(&tempkey,tempstring);	/* make it. */
 an_id->usesa = used;				/* Put it in the node. */
	return;
}


	STATIC boolean
ANSameKey(key1,key2)		/* Compares keys of address nodes. */
register struct addrmush *key1;
register struct addrmush *key2;

{/*extern int strcmp();		** Compares strings; in C(3) library. */

 if(key1->mrrt != key2->mrrt)			/* Compare modes. */
	return(FALSE);				/* Different. */
 if(key1->con != key2->con)			/* Compare constant parts. */
	return(FALSE);				/* Different. */
 if(strcmp(key1->string,key2->string))		/* Compare strings. */
	return(FALSE);				/* Different. */
 return(TRUE);					/* Keys the same. */
}
	void
DelAdNodes()			/* Delete all the address nodes. */

{extern AN_Id AddrTable[];	/* Address table pointers. */
 extern AN_Id BotANId;		/* Bottom of GNAQ list. */
 register AN_Id ChainPointer;	/* Table chain pointer. */
 register AN_Id NextPointer;	/* Pointer to next node in chain. */
 extern AN_Id TopANId;		/* Top of GNAQ list. */
 register unsigned slot;	/* Address node pointer table index. */

 for(slot = 0; slot < ADDRTABSIZ; slot++)	/* Find all the hash slots. */
	{ChainPointer = AddrTable[slot];
	 AddrTable[slot] = NULL;		/* Re-Initialize. */
	 while(ChainPointer)			/* Follow the chain. */
		{NextPointer = ChainPointer->next;
		 if(ChainPointer->key.K.string != AN_NullString) /* Free */
						/* the string if we didn't */
						/* already. A string always */
						/* starts with a + or - and */
						/* has spaces squeezed out. */
						/* Therefore, if there is a */
						/* space, we put it there,  */
						/* so it is already free. */
						/* No-one will re-use while */
						/* this is going on. */
			{if(ChainPointer->key.K.string[0] != SPACE)
				{ChainPointer->key.K.string[0] = SPACE;
				 Free(ChainPointer->key.K.string);
				}
			}
		 Free(ChainPointer);		/* Free this node. */
		 ChainPointer = NextPointer;
		}
	}
 BotANId = NULL;				/* Re-initialize. */
 TopANId = NULL;				/* Re-initialize. */
 return;
}
	void
DelAdPrivateNodes()		/* Delete all the private address nodes. */

{extern AN_Id AddrTable[];	/* Address table pointers. */
 extern AN_Id BotANId;		/* Bottom of GNAQ list. */
 register AN_Id ChainPointer;	/* Table chain pointer. */
 register AN_Id NextPointer;	/* Pointer to next node in chain. */
 register AN_Id PrevPointer;	/* Pointer to previous node in chain. */
 extern AN_Id TopANId;		/* Top of GNAQ list. */
 boolean deleteflag;		/* TRUE if item deleted. */
 register unsigned slot;	/* Address node pointer table index. */


 do	{deleteflag = FALSE;		/* None deleted in this pass. */
	 for(slot = 0; slot < ADDRTABSIZ; slot++)	/*Find all hash slots.*/
		{ChainPointer = AddrTable[slot]; 
		 while(ChainPointer)			/* Follow the chain. */
			{NextPointer = ChainPointer->next;
			 if(ChainPointer->data.delete != 1) /* UnMarked. */
			 	{if(ChainPointer->data.private == 1)	/* If private,*/
					{ChainPointer->data.delete = 1;	/* delete it.*/
					 deleteflag = TRUE;
					}
						/* If it uses a private node, */
						/* delete it too. */
			 	 else if(ChainPointer->usesa != ChainPointer
					&& (ChainPointer->usesa->data.delete 
						== 1))
					{ChainPointer->data.delete = 1;
					 deleteflag = TRUE;
					}
			 	 else if(ChainPointer->usesb != ChainPointer
					&& (ChainPointer->usesb->data.delete 
						== 1))
					{ChainPointer->data.delete = 1;
					 deleteflag = TRUE;
					}
				}
			 ChainPointer = NextPointer;
			}
		}
	} while(deleteflag);

						/* We now know all those */
						/* that should be deleted. */
 for(slot = 0; slot < ADDRTABSIZ; slot++)	/* Scan all slots and delete. */
	{ChainPointer = AddrTable[slot];	/* Start of chain. */
	 PrevPointer = NULL;			/* No-one previous yet. */
	 while(ChainPointer)			/* Follow the chains. */
		{NextPointer = ChainPointer->next;
		 if(ChainPointer->data.delete == 1)	/* If marked, */
			{
			 Free(ChainPointer);		/* delete it and */
			 if(PrevPointer)		/* fix chain. */
				PrevPointer->next = NextPointer;
			 else
				AddrTable[slot] = NextPointer;
			}
		 else
			PrevPointer = ChainPointer;
		 ChainPointer = NextPointer;	/* Advance along the chain. */
		}
	}
 BotANId = NULL;				/* Re-initialize. */
 TopANId = NULL;				/* Re-initialize. */
 return;
}
	void
DelAdTransNodes()		/* Delete all the address translation nodes. */
				/* If they are marked as labels, just change */
				/* back to normal nodes.	*/

{extern AN_Id AddrTable[];	/* Address table pointers. */
 register AN_Id ChainPointer;	/* Table chain pointer. */
 extern boolean IsAdTranslation();	/*TRUE for trans. nodes; in this file.*/
 register AN_Id NextPointer;	/* Pointer to next node in chain. */
 register AN_Id PrevPointer;	/* Pointer to previous node in chain. */
 register unsigned slot;	/* Address node pointer table index. */

 for(slot = 0; slot < ADDRTABSIZ; slot++)	/* Find all the hash slots. */
	{ChainPointer = AddrTable[slot];
	 PrevPointer = NULL;			/* No-one before us. */
	 while(ChainPointer)			/* Follow the chain. */
		{NextPointer = ChainPointer->next;
		 if(!IsAdTranslation(ChainPointer))	/* Skip those that */
			PrevPointer = ChainPointer;	/*are not translation */
		 else
			{if(ChainPointer->data.dup == 1) /* Duplicated node? */
				{ChainPointer->data.dup = 0;	/*Make normal.*/
				 ChainPointer->data.translation = 0;
				 ChainPointer->usesa = ChainPointer;
				}
			 else			/*No: normal translation node.*/
				{if(PrevPointer) /*If there's a previous node*/
					 PrevPointer->next = NextPointer;
				 else			/* point to next node.*/
					AddrTable[slot] = NextPointer;	
				 Free(ChainPointer);	/* Free this node. */
				}
			} /* END OF translation node prcessing.	*/
		 ChainPointer = NextPointer;
		} /* END OF while(ChainPointer) */
	} /* END OF for(slot = 0; slot < ADDRTABSIZ; slot++) */
 return;
}
	AN_Id
GetAdAbsDef(expr)	/* Get address-node-identifier of absolute deferred. */
char *expr;			/* Pointer to the expression. */


{STATIC AN_Id ANFind();		/* Finds address nodes; in this file.	*/
 STATIC AN_Id ANMake();		/* Makes address nodes; in this file.	*/
 extern long int ANNormExpr();	/* Normalizes expressions; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Node we are finding or making.	*/
 struct addrkey tempkey;	/* Temporary address key.	*/
 char *tempstring;

 tempkey.mode = (unsigned char) AbsDef;		/* Make temporary local key. */
 tempkey.rega = (unsigned char) REG_NONE;	/* No register A used.	*/
 tempkey.regb = (unsigned char) REG_NONE;	/* No register B used.	*/
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(AbsDef,Tnone);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it.	*/

 return(node);					/* Return id of desired node. */
}
	AN_Id
GetAdAbsolute(type,expr)	/* Get address-node-identifier of absolute. */
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to the expression. */


{STATIC AN_Id ANFind();	/* Finds address node; in this file. */
 STATIC AN_Id ANMake();	/* Makes address node; in this file. */
 extern long int ANNormExpr();	/* Normalizes expressions; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;

 tempkey.mode = (unsigned char) Absolute;	/* Make temporary local node. */
 tempkey.rega = (unsigned char) REG_NONE;	/* No registers for this mode.*/
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize the expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(Absolute,type);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired identifier. */
}
	AN_Id
GetAdAddIndInc(from,to,an_id,incr) /* Want node with one level more indirect. */
OperandType from;		/* Type of the operand an_id.	*/
OperandType to;			/* Type of the returned AN_Id.	*/
				/* Increment desired for new node.	*/
register AN_Id an_id;		/* One more than this one. */
long int incr;			/* Desired increment.	*/

{STATIC AN_Id ANFind();	/* Finds address nodes; in this file. */
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Local key structure. */

						/* Copy fixed part of key. */
 tempkey = an_id->key.K;
 tempkey.con = an_id->key.K.con + incr;
 tempkey.con -= LSBOffset((AN_Mode) tempkey.mode,from);

						/* Select new mode; it */
 switch(an_id->key.K.mode)			/* depends on current mode. */
	{case Absolute:
		tempkey.mode = (unsigned char) AbsDef;
		endcase;
	 case CPUReg:
		tempkey.mode = (unsigned char) Disp;
		endcase;
	 case Disp:
		tempkey.mode = (unsigned char) DispDef;
		endcase;
	 case AbsDef:
	 case StatCont:
	 case DispDef:
	 case Immediate:
		tempkey.mode = (unsigned char) Absolute;
		endcase;
	 case IndexRegDisp:
	 case IndexRegScaling:
	 case MAUReg:
	 case PreDecr:
	 case PreIncr:
	 case PostDecr:
	 case PostIncr:
	 case Raw:
		fatal("GetAdAddIndInc: illegal mode (%d).\n",an_id->key.K.mode);
		endcase;
	 default:
		fatal("GetAdAddIndInc: unknown mode (0x%x).\n",
			an_id->key.K.mode);
	}

 tempkey.con += LSBOffset((AN_Mode) tempkey.mode,to);
 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,(char *) 0);	/* make it. */
 return(node);					/* Return id of desired node. */
}
	AN_Id
GetAdAddToKey(type,an_id,constant)	/*Add constant to key of address node.*/
OperandType type;		/* Type of operand.	*/
register AN_Id an_id;		/* Address node. */
long int constant;		/* Constant to be added. */

{extern AN_Id ANFind();	/* Finds address node; in this file.*/
 extern AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern boolean IsLegalAd();	/* TRUE for legal addresses.	*/
 extern int LSBOffset();	/* Returns LSB offset of data type.	*/
 long int Constant;		/* Hardware version of constant.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary local key. */

 tempkey = an_id->key.K;			/* Copy similar part.	*/
 tempkey.con = an_id->key.K.con + constant; /*Add constant part.*/
						/* Test Hardware version of */
 Constant = tempkey.con - LSBOffset((AN_Mode) tempkey.mode,type);

 switch(an_id->key.K.mode)			/* Create used node(s) if nec.*/
	{case Absolute:
	 case AbsDef:
		endcase;
	 case CPUReg:
	 case StatCont:
	 case MAUReg:
	 case Raw:
		fatal("GetAdAddToKey: illegal mode (0x%x).\n",an_id->key.K.mode);
		endcase;
	 case Disp:
	 case DispDef:
	 case Immediate:
	 case IndexRegDisp:
	 case IndexRegScaling:
	 case PostDecr:
	 case PostIncr:
	 case PreDecr:
	 case PreIncr:
		endcase;
	 default:
		fatal("GetAdAddToKey: unknown mode (0x%x).\n",
			an_id->key.K.mode);
		endcase;
	}

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	{if(!IsLegalAd((AN_Mode) tempkey.mode,	/* See if this would  be a */
			(RegId) tempkey.rega,	/* legal address.	*/
			(RegId) tempkey.regb,
			Constant,tempkey.string))
		fatal("GetAdAddToKey: request for illegal address.\n");
	 node = ANMake(&tempkey,(char *) 0);	/* make it. */
	}
 return(node);					/* Return id of desired node. */
}

#ifndef MACRO
	unsigned int
GetAdAddrIndex(an_id)		/* Get address-index */
AN_Id an_id;			/* for this address node. */

{extern void fatal();		/* Handles fatal errors; in debug.c. */
 extern char *extaddr();	/* Prints an address node; in Mach. Dep. */

 if(an_id->data.AddIndexP)			/* If there is an entry, */
	return(an_id->data.AddrIndex);		/* give them the data item. */

 fatal("GetAdAddrIndex: no address-index present in address node: %s.\n",
	extaddr(an_id,TBYTE));
 /*NOTREACHED*/
}
#endif
	AN_Id
GetAdCPUReg(reg_id)	/* Get address-node-identifier of CPU register. */
RegId reg_id;			/* Identifier of desired CPU register. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */

 tempkey.mode = (unsigned char) CPUReg;		/* Make temporary local key. */
 tempkey.rega = (unsigned char) reg_id;		/* Keep in first entry.*/
 tempkey.regb = (unsigned char) REG_NONE;
 tempkey.string = AN_NullString;
 tempkey.con = 0L;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,(char *) 0);	/* make it. */

 return(node);					/* Return desired node id. */
}
	AN_Id
GetAdChgRegAInc(type,an_id,reg_id,inc)	/* "Change" register A of an address */
				/* node, and add to constant.	*/
OperandType type;		/* Type of operand.	*/
register AN_Id an_id;		/* Address node. */
RegId reg_id;			/* New register to use.	*/
long int inc;			/* Increment to add.	*/

{extern AN_Id ANFind();		/* Finds address node; in this file.*/
 extern AN_Id ANMake();		/* Makes address nodes; in this file. */
 extern boolean IsLegalAd();	/* Determines if address is legal.	*/
 extern int LSBOffset();
 long int Constant;		/* Constant part of address key.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary local key. */

 tempkey = an_id->key.K;			/* Copy similar part.	*/
 tempkey.rega = (unsigned char) reg_id;		/* Change register.	*/
 tempkey.con += inc;				/* Increment the constant. */
 Constant = tempkey.con - LSBOffset((AN_Mode) tempkey.mode,type);

 switch(an_id->key.K.mode)			/* Create used node(s) if nec.*/
	{case Absolute:
	 case AbsDef:
	 case StatCont:
	 case Immediate:
	 case MAUReg:
	 case Raw:
		fatal("GetAdChgRegA: no register A for this mode.\n");
		endcase;
	 case CPUReg:
	 case Disp:
	 case DispDef:
	 case IndexRegDisp:
	 case IndexRegScaling:
	 case PostDecr:
	 case PostIncr:
	 case PreDecr:
	 case PreIncr:
		endcase;
	 default:
		fatal("GetAdChgRegA: unknown mode (0x%x).\n",an_id->key.K.mode);
		endcase;
	}

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	{if(!IsLegalAd((AN_Mode) tempkey.mode,	/* See if this would  be a */
			(RegId) tempkey.rega,	/* legal address.	*/
			(RegId) tempkey.regb,
			Constant,tempkey.string))
		fatal("GetAdChgRegAInc: request for illegal address.\n");
	 node = ANMake(&tempkey,(char *) 0);	/* make it. */
	} /* END OF if(node == NULL) */
 return(node);					/* Return id of desired node. */
}

#ifdef W32200
	AN_Id
GetAdChgRegBInc(type,an_id,reg_id,inc)	/* "Change" register B of an address */
				/* node, and add to constant.	*/
OperandType type;		/* Type of operand.	*/
register AN_Id an_id;		/* Address node. */
RegId reg_id;			/* New register to use.	*/
long int inc;			/* Increment to add.	*/

{extern AN_Id ANFind();		/* Finds address node; in this file.*/
 extern AN_Id ANMake();		/* Makes address nodes; in this file. */
 extern boolean IsLegalAd();	/* Determines if address is legal.	*/
 extern int LSBOffset();	/* Returns LSB offset of a data type.	*/
 long int Constant;
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary local key. */

 tempkey = an_id->key.K;			/* Copy similar part.	*/
 tempkey.regb = (unsigned char) reg_id;		/* Change register.	*/
 tempkey.con += inc;			/* Increment the constant. */
 Constant = tempkey.con - LSBOffset((AN_Mode) tempkey.mode,type);

 switch(an_id->key.K.mode)			/* Create used node(s) if nec.*/
	{case Absolute:
	 case AbsDef:
	 case CPUReg:
	 case Disp:
	 case DispDef:
	 case StatCont:
	 case Immediate:
	 case MAUReg:
	 case PostDecr:
	 case PostIncr:
	 case PreDecr:
	 case PreIncr:
	 case Raw:
		fatal("GetAdChgRegB: no register B for this mode.\n");
		endcase;
	 case IndexRegDisp:
	 case IndexRegScaling:
		endcase;
	 default:
		fatal("GetAdChgRegB: unknown mode (0x%x).\n",an_id->key.K.mode);
		endcase;
	}

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	{if(!IsLegalAd((AN_Mode) tempkey.mode,	/* See if this would  be a */
			(RegId) tempkey.rega,	/* legal address.	*/
			(RegId) tempkey.regb,
			Constant,tempkey.string))
		fatal("GetAdChgRegBInc: request for illegal address.\n");
	 node = ANMake(&tempkey,(char *) 0);	/* make it. */
	} /* END OF if(node == NULL) */
 return(node);					/* Return id of desired node. */
}
#endif
	AN_Id
GetAdDisp(type,expr,reg_id)	/* Get addr-node-id of register-displacement. */
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to displacement expression. */
RegId reg_id;			/* Identifier of base register. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Desired node. */
 struct addrkey tempkey;	/* Space for local key. */
 char *tempstring;

 tempkey.mode = (unsigned char) Disp;		/* Make desired node. */
 tempkey.rega = (unsigned char) reg_id;
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(Disp,type);		/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it does not exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired id. */
}
	AN_Id
GetAdDispDef(expr,reg_id)	/* Get addr-node-id of reg-disp-deferred. */


char *expr;			/* Pointer to displacement expression. */
RegId reg_id;			/* Identifier of base register. */

{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Space for local key. */
 char *tempstring;

 tempkey.mode = (unsigned char) DispDef;	/* Make desired node. */
 tempkey.rega = (unsigned char) reg_id;
 tempkey.regb = (unsigned char) REG_NONE;
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(DispDef,Tnone);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired identifier. */
}
	AN_Id
GetAdDispDefTemp(expr,index)	/* Get addr-node-id of reg-disp-deferred */
				/* off of a temp			 */

char *expr;			/* Pointer to displacement expression. */
unsigned int index;			/* Identifier of base register. */

{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Space for local key. */
 char *tempstring;

 tempkey.mode = (unsigned char) DispDef;	/* Make desired node. */
 tempkey.rega = (unsigned char) CTEMP;		/* Make rega a temp. */
 tempkey.regb = (unsigned char) REG_NONE;
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(DispDef,Tnone);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) index;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired identifier. */
}
	AN_Id
GetAdDispInc(type,expr,reg_id,incr) /*Get addr-node-id of register-displacement.*/
OperandType type;		/* Type of address we are dealing with.	*/
				/* Add an increment to the displacement.*/
char *expr;			/* Pointer to displacement expression. */
RegId reg_id;			/* Identifier of base register. */
long int incr;			/* Increment to be added.	*/


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Desired node. */
 struct addrkey tempkey;	/* Space for local key. */
 char *tempstring;

 tempkey.mode = (unsigned char) Disp;		/* Make desired node. */
 tempkey.rega = (unsigned char) reg_id;
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(Disp,type);		/* Correct for offset.	*/
 tempkey.con += incr;				/* Add the increment.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it does not exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired id. */
}
	AN_Id
GetAdDispTemp(type,expr,index)	/* Get addr-node-id of register-displacement */
				/*  off of a temp.			     */
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to displacement expression. */
unsigned int index;			/* Identifier of base register. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Desired node. */
 struct addrkey tempkey;	/* Space for local key. */
 char *tempstring;

 tempkey.mode = (unsigned char) Disp;		/* Make desired node. */
 tempkey.rega = (unsigned char) CTEMP;		/* Make rega a temp. */
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(Disp,type);		/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) index;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it does not exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired id. */
}
	long int
GetAdEstim(an_id)		/* Get address use estimate of this node. */
AN_Id an_id;			/* Address-node-identifier of this node. */


{
 return(an_id->data.estim);		/* Decode address node entry. */
}


	char *
GetAdExpression(type,an_id)	/* Get the address expression.*/
OperandType type;		/* Type of address we are dealing with.	*/
AN_Id an_id;			/* Address node identifier. */


{extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 static char *b0 = NULL;
 static char *bn = NULL;
 long int constant;		/* Constant part of address.	*/
 extern void fatal();
 boolean needplus;		/* TRUE if plus sign needed. */
 int size;			/* Size of string in address node. */
 int nsize;
 boolean skipplus;
 char *string;			/* Where to put the expression. */
 extern void ExtendBuf();

 if(an_id == NULL)				/* Make sure we got an an_id. */
	fatal("GetAdExpression: an_id was NULL.\n");	/* We didn't.	*/
 if(b0 == NULL)			/* initialize buffer space */
	{b0 = Malloc(EBUFSIZE+1);
	 if(b0 == NULL)
		fatal("GetAdExpression: out of space\n");
	 bn = b0 + EBUFSIZE;
	}
 size = strlen(an_id->key.K.string);		/* Measure string size. */
 nsize = size + 1 + MAXLONGDIG;
 if(nsize > bn - b0)
	ExtendBuf(&b0,&bn,(unsigned)nsize+1);
 string = b0;
 constant = an_id->key.K.con;			/* Handier access to this. */
 constant -= LSBOffset((AN_Mode) an_id->key.K.mode,type); /*Correct for type.*/
 if((size == 0) && (constant == 0) &&		/* If both are zero, */
		(an_id->key.K.mode != (unsigned char) Raw))	/*and not raw,*/
	{*string++ = '0';			/* give a zero. */
	 *string = EOS;
	 size++;
	}
 needplus = ((size > 0) && (constant > 0)) ? TRUE : FALSE;
 skipplus = (*an_id->key.K.string == '+') ? TRUE : FALSE;
 (void) sprintf(string,needplus ? "%s+%ld" : "%s%.0ld",
		skipplus ? an_id->key.K.string + 1 : an_id->key.K.string,
		constant);
 return b0;
}
	unsigned short int
GetAdGNAQSize(an_id)		/* Get GNAQ size of this node. */
AN_Id an_id;			/* Address-node-identifier of this node. */

{extern void fatal();		/* Handles fatal errors; in common. */

 if(an_id->data.MDLpresent == 0)	/* Is it there? */
	fatal("GetAdGNAQSize: data not present.\n");
 return(an_id->data.MDLength);		/* Decode address node entry. */
}


#ifndef MACRO
	AN_GnaqType
GetAdGnaqType(an_id)		/* Get GNAQ type of this node. */
AN_Id an_id;			/* Address-node-identifier of this node. */

{
 return((AN_GnaqType) an_id->data.gnaq);
}
#endif
	AN_Id
GetAdId(index)			/* Get AN_Id of node with specified index. */
register unsigned int index;	/* Index to look for.	*/

{extern AN_Id GetAdNextGNode();	/* Gets next node in GNAQ list.	*/
 register AN_Id an_id;

 for(an_id = GetAdNextGNode((AN_Id) NULL);	/* Scan GNAQ list.	*/
		an_id != (AN_Id) NULL;
		an_id = GetAdNextGNode(an_id))
	{if(an_id->data.AddrIndex == index)	/* Match? */
		return(an_id);			/* Yes: done.	*/
	}
 return((AN_Id) NULL);				/* No match.	*/
}


	AN_Id
GetAdImmediate(expr)		/* Get address-node-identifier of immediate. */
char *expr;			/* Pointer to the immediate expression. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;

 tempkey.mode = (unsigned char) Immediate;	/* Make temporary local key. */
 tempkey.rega = (unsigned char) REG_NONE;
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize the expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired node id. */
}
	AN_Id
GetAdIndexRegDisp(type,expr,regidA,regidB) /*Get addr-node-id of index reg disp.*/
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to displacement expression. */
RegId regidA;			/* Identifier of first register. */
RegId regidB;			/* Identifier of second register. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;

 tempkey.mode = (unsigned char) IndexRegDisp;	/* Make temporary key. */
 tempkey.rega = (unsigned char) regidA;
 tempkey.regb = (unsigned char) regidB;
						/* Normalize the expresson. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(IndexRegDisp,type);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If not found, */
	node = ANMake(&tempkey,tempstring);	/* make one. */

 return(node);					/* Return desired identifier. */
}
	AN_Id
				/* Get address-node-identifier of indexed */
				/* registers with scaling and displacement. */
GetAdIndexRegScaling(type,expr,regidA,regidB)
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to the expression. */
RegId regidA;			/* Identifier of first register. */
RegId regidB;			/* Identifier of second register. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;

 tempkey.mode = (unsigned char) IndexRegScaling;	/* Fill in key. */
 tempkey.rega = (unsigned char) regidA;
 tempkey.regb = (unsigned char) regidB;
						/* Normalize expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(IndexRegScaling,type);	/* Correct for offset.*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node.*/
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired identifier. */
}
	AN_Id
GetAdMAUReg(reg_id)	/* Get address-node-identifier of MAU register. */
RegId reg_id;			/* Identifier of desired MAU register. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */

 tempkey.mode = (unsigned char) MAUReg;		/* Make temporary local key. */
 tempkey.rega = (unsigned char) reg_id;		/* Keep in first entry.*/
 tempkey.regb = (unsigned char) REG_NONE;
 tempkey.string = AN_NullString;
 tempkey.con = 0L;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it does not exist yet, */
	node = ANMake(&tempkey,(char *) 0);	/* make it. */

 return(node);					/* Return desired node id. */
}

	AN_Mode
GetAdMode(an_id)		/* Get address mode type of this node. */
AN_Id an_id;			/* Address-node-identifier of this node. */


{
 return((AN_Mode) an_id->key.K.mode);		/* Decode address node entry. */
}
	AN_Id
GetAdNextGNode(an_id)		/* Get next node in GNAQ list. */
AN_Id an_id;			/* Node at which to start. */


{extern AN_Id TopANId;		/*  Top of GNAQ list (if sorted). */

 if(an_id == NULL)				/* If we start at beginning, */
	return(TopANId);			/* start at top of the list. */
 return(an_id->forw);				/* All OK: return next. */
}


	AN_Id
GetAdNextNode(an_id)		/* Gets "next" address node. If you start */
				/* NULL and then give the previous result, */
				/* you will get all the address nodes and */
				/* then a NULL.	*/
register AN_Id an_id;		/* Node for which the next one is desired. */

{extern AN_Id AddrTable[];	/* The address pointer table.	*/
 register AN_Id ChainPointer;	/* Chain pointer for addresses.	*/
 register int index;		/* Location in AddrTable.	*/

 if(an_id == (AN_Id) NULL)			/* Start at beginning?	*/
	index = -1;				/* Yes.	*/
 else if(an_id->next != (AN_Id) NULL)		/* Is next one easy to find? */
	return(an_id->next);			/* Yes: return it.	*/
 else						/* No: too bad: find index. */
	index = an_id->data.slot;		/* Find slot we are in.	*/
 for(index += 1; index < ADDRTABSIZ; index++)	/* Look for next one.	*/
	{ChainPointer = AddrTable[index];
	 if(ChainPointer)
		return(ChainPointer);
	}
 return((AN_Id) NULL);				/* End of the line.	*/
}


	AN_Id
GetAdNextPNode(an_id)		/* Get next node in Private node list. */
AN_Id an_id;			/* Node at which to start. */

{extern void fatal();		/* Handles fatal errors.	*/

 if(an_id == NULL)				/* If we start at beginning, */
	fatal("GetAdNextPNode: must specify explicit starting node.\n");
 return(an_id->privt);				/* All OK: return next. */
}
	AN_Id
GetAdNoNumber(an_id)		/* Get an address node like given one, but */
AN_Id an_id;			/* with no numeric offset, if one exists.  */
				/* Otherwise, return NULL.	*/
{struct addrkey tempkey;	/* Make key for search.	*/
 STATIC AN_Id ANFind();	/* Finds an address node if one exists.	*/

 tempkey = an_id->key.K;	/* Make key of desired address.	*/
 tempkey.con = 0;		/* (Want one with no numeric offset.	*/

 return(ANFind(&tempkey));	/* Return its AN_Id if it exists.	*/
}


	long int
GetAdNumber(type,an_id)		/* Returns numeric part of an address. */
OperandType type;		/* Type of address we are dealing with.	*/
AN_Id an_id;			/* Address whose numeric part is wanted.*/

{extern int LSBOffset();	/* Gives offset of LSB from address.	*/

						/* Get it from address node. */
 return(an_id->key.K.con - LSBOffset((AN_Mode) an_id->key.K.mode,type));
}
	AN_Id
GetAdPostDecr(type,expr,reg_id)	/* Get addr-node-id of auto-post-decrement. */
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to expression. */
RegId reg_id;			/* Register-identifier to be used. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;

 tempkey.mode = (unsigned char) PostDecr;	/* Make temporary key. */
 tempkey.rega = (unsigned char) reg_id;
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize the expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(PostDecr,type);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired identifier. */
}
	AN_Id
GetAdPostIncr(type,expr,reg_id)	/* Get addr-node-id of auto-post-increment. */
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to expression. */
RegId reg_id;			/* Register-identifier to be used. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;

 tempkey.mode = (unsigned char) PostIncr;
 tempkey.rega = (unsigned char) reg_id;
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize the expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(PostIncr,type);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired identifier. */
}
	AN_Id
GetAdPreDecr(type,expr,reg_id)	/* Get addr-node-id of auto-pre-decrement. */
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to the expression. */
RegId reg_id;			/* Register-identifier to be used. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;

 tempkey.mode = (unsigned char) PreDecr;
 tempkey.rega = (unsigned char) reg_id;
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize the expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(PreDecr,type);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Find desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired identifier. */
}
	AN_Id
GetAdPreIncr(type,expr,reg_id)	/* Get addr-node-id of auto-pre-increment. */
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Pointer to the expression. */
RegId reg_id;			/* Register-identifier to be used. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 extern long int ANNormExpr();	/* Normalizes an expression; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */
 char *tempstring;

 tempkey.mode = (unsigned char) PreIncr;
 tempkey.rega = (unsigned char) reg_id;
 tempkey.regb = (unsigned char) REG_NONE;
						/* Normalize the expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(PreIncr,type);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,tempstring);	/* make it. */

 return(node);					/* Return desired identifier. */
}


	AN_Id
GetAdPrevGNode(an_id)		/* Get previous node in GNAQ list. */
AN_Id an_id;			/* Node at which to start. */


{extern AN_Id BotANId;		/* Bottom of GNAQ list (if sorted). */

 if(an_id == NULL)				/* If we start at end, */
	return(BotANId);			/* start at end of the list. */
 return(an_id->back);				/* All OK: return previous. */
}
	AN_Id
GetAdRaw(string)		/* Get address-node-identifier of text. */
				/* This puts the unnormalized string into */
				/* an address node.	*/
char *string;			/* Pointer to the string. */


{STATIC AN_Id ANFind();	/* Finds address node; in this file. */
 STATIC AN_Id ANMake();	/* Makes address node; in this file. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */

 tempkey.mode = (unsigned char) Raw;		/* Make temporary local node. */
 tempkey.rega = (unsigned char) REG_NONE;	/* No registers for this mode.*/
 tempkey.regb = (unsigned char) REG_NONE;
 tempkey.con = 0;
 tempkey.string = string;			/* Provisional string pointer.*/
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,string);	/* make it. */

 return(node);					/* Return desired identifier. */
}
	RegId
GetAdRegA(an_id)		/* Get first register of this node. */
AN_Id an_id;			/* Address-node-identifier of this node. */


{extern void fatal();		/* Handles fatal errors; in debug.c. */
 extern char *extaddr();	/* Prints an address node; in Mach. Dep. */

 if((RegId) an_id->key.K.rega == REG_NONE)	/* Is there one? */
	 fatal("GetAdRegA: register A not defined in address node: %s.\n",
		extaddr(an_id,Tbyte));
 return((RegId) an_id->key.K.rega);	/* Pick it from the address node. */
}


	RegId
GetAdRegB(an_id)		/* Get second register of this node. */
AN_Id an_id;			/* Address-node-identifier of this node. */


{extern void fatal();		/* Handles fatal errors; in debug.c. */
 extern char *extaddr();	/* Prints address node; in Mach. Dep. */

 if((RegId) an_id->key.K.regb == REG_NONE)
	 fatal("GetAdRegB: register B not defined in address node: %s.\n",
		extaddr(an_id,TBYTE));

 return((RegId) an_id->key.K.regb);	/* Pick it from the address node. */
}
	AN_Id
GetAdRemInd(from,to,an_id)	/* Want node with one level less indirect. */
OperandType from;		/* Type of operand, an_id.	*/
OperandType to;			/* Type of returned AN_Id.	*/
register AN_Id an_id;		/* One less than this one. */

{STATIC AN_Id ANFind();		/* Searches for address nodes; in this file. */
 STATIC AN_Id ANMake();		/* Makes address nodes; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 AN_Id node;			/* Id of desired node. */
 extern char *extaddr();	/* Prints address to string; in Mach. Dep. */
 struct addrkey tempkey;	/* Local key structure. */

 tempkey = an_id->key.K;			/* Copy fixed part of key. */
 tempkey.con -= LSBOffset((AN_Mode) tempkey.mode,from);
 tempkey.tempid = (unsigned char) 0;

						/* Select new mode; it */
 switch(an_id->key.K.mode)			/* depends on current mode. */
	{case AbsDef:
		tempkey.mode = (unsigned char) Absolute;
		endcase;
	 case DispDef:
		tempkey.mode = (unsigned char) Disp;
		endcase;
	 case Absolute:
	 case CPUReg:
	 case StatCont:
	 case Disp:
	 case Immediate:
	 case IndexRegDisp:
	 case IndexRegScaling:
	 case MAUReg:
	 case PreDecr:
	 case PreIncr:
	 case PostDecr:
	 case PostIncr:
	 case Raw:
		fatal("GetAdRemInd: illegal mode (%d) (%s).\n",
			an_id->key.K.mode,extaddr(an_id,Tbyte));
		/*NOTREACHED*/
		endcase;
	 default:
		fatal("GetAdRemInd: unknown mode (0x%x).\n",an_id->key.K.mode);
		/*NOTREACHED*/
		endcase;
	}

 tempkey.con += LSBOffset((AN_Mode) tempkey.mode,to);
 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If not found, */
	node = ANMake(&tempkey,(char *)0);	/* make it. */

 return(node);					/* Return desired node id. */
}
	AN_Id
GetAdStatCont(cc_id)	/* Get address-node-identifier of condition codes. */
RegId cc_id;			/* Identifier of desired condition code. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */

 tempkey.mode = (unsigned char) StatCont;	/* Make temporary local key. */
 tempkey.rega = (unsigned char) cc_id;		/* Keep in first entry. */
 tempkey.regb = (unsigned char) REG_NONE;
 tempkey.string = AN_NullString;
 tempkey.con = 0L;
 tempkey.tempid = (unsigned char) 0;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,(char *) 0);	/* make it. */

 return(node);					/* Return desired node id. */
}
	AN_Id
GetAdTemp(index)	/* Get address-node-identifier of Temps. */
unsigned int index;	/* Identifier of desired Temp. */


{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file. */
 AN_Id node;			/* Id of desired node. */
 struct addrkey tempkey;	/* Temporary address key. */

 tempkey.mode = (unsigned char) CPUReg;		/* Make temporary local key. */
 tempkey.rega = (unsigned char) CTEMP;		/* Keep in first entry.*/
 tempkey.regb = (unsigned char) REG_NONE;
 tempkey.string = AN_NullString;
 tempkey.con = 0L;
 tempkey.tempid = (unsigned char) index;

 node = ANFind(&tempkey);			/* Search for desired node. */
 if(node == NULL)				/* If it doesn't exist yet, */
	node = ANMake(&tempkey,(char *) 0);	/* make it. */

 return(node);					/* Return desired node id. */
}


#ifndef MACRO
	unsigned int
GetAdTempIndex(an_id)		/* Get the index of a temp id */
AN_Id an_id;
{
 extern void fatal();

 if((RegId) an_id->key.K.rega != CTEMP)
	fatal("GetAdTempIndex called with non-Temp an_id\n");
 return(an_id->key.K.tempid);
}
#endif
	AN_Id
GetAdTranslation(type,expr,an_id)	/* Get a translation node using an_id.*/
OperandType type;		/* Type of address we are dealing with.	*/
char *expr;			/* Translation node's expression. */
AN_Id an_id;			/* Node used by translation node. */

{STATIC AN_Id ANFind();	/* Finds address nodes; in this file.	*/
 STATIC AN_Id ANMake();	/* Makes address nodes; in this file.	*/
 extern long int ANNormExpr();	/* Normalizes expressions; in this file. */
 extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 AN_Id node;			/* AN_Id of desired node.	*/
 struct addrkey tempkey;	/* Temporary address key.	*/
 char *tempstring;

 tempkey.mode = (unsigned char) Absolute;	/* Translation nodes are */
 tempkey.rega = (unsigned char) REG_NONE;	/* Absolute nodes, only */
 tempkey.regb = (unsigned char) REG_NONE;	/* to a specified AN_Id. */
						/* Normalize expression. */
 tempkey.con = ANNormExpr(expr,&tempstring);
 tempkey.con += LSBOffset(Absolute,type);	/* Correct for offset.	*/
 tempkey.string = tempstring;
 tempkey.tempid = (unsigned char) 0;

 if((node = ANFind(&tempkey)) == NULL)		/* If one of these doesn't */
	node = ANMake(&tempkey,tempstring);	/* exist, make one.	*/
 else						/* If it exists already, */
	node->data.dup = 1;			/* mark it 'duplicated'. */
 node->data.translation = 1;			/* It is a translation node. */
 node->usesa = an_id;				/* It uses specified one. */
						/* (We cannot use ANPutUses */
						/* because it has no way to  */
						/* compute an_id.	*/

 return(node);					/* Return desired AN_Id. */
}

#ifndef MACRO
	AN_Id
GetAdUsedId(an_id,index)	/* Get a node used by this one. */
AN_Id an_id;			/* Id of this node. */
unsigned int index;		/* Index of next one. */

{extern void fatal();		/* Handles fatal errors; in debug.c. */

 if(an_id == NULL)				/* If we got none, */
	return(NULL);				/* give none. */
 switch(index)
	{case 0:
		if(an_id->usesa == an_id)
			return((AN_Id) NULL);
		return(an_id->usesa);
		/*NOTREACHED*/
		endcase;
	 case 1:
		if(an_id->usesb == an_id)
			return((AN_Id) NULL);
		return(an_id->usesb);
		/*NOTREACHED*/
		endcase;
	 default:
		fatal("GetAdUsedId: invalid index (%u).\n",index);
		/*NOTREACHED*/
		endcase;
	}
 /*NOTREACHED*/
}
#endif
	AN_Id
HideAdPrivateNodes()		/* Hide all the private address nodes.	*/
				/* Make them into a single linked list	*/
				/* and return the AN_Id of the first one. */

{extern AN_Id AddrTable[];	/* Address table pointers.	*/
 extern AN_Id BotANId;		/* Bottom of GNAQ list.	*/
 register AN_Id ChainPointer;	/* Table chain pointer. */
 register AN_Id NextPointer;	/* Pointer to next node in chain.	*/
 AN_Id PrevPointer;		/* Pointer to previous node in chain.	*/
 extern AN_Id TopANId;		/* Top of GNAQ list.	*/
 AN_Id first_an_id;		/* Head of hidden list.	*/
 boolean chgflag;		/* TRUE if item hidden.	*/
 AN_Id last_an_id;		/* End of hidden list.	*/
 register unsigned slot;	/* Address node pointer table index.	*/

 do	{chgflag = FALSE;			/* None hidden in this pass. */
	 for(slot = 0; slot < ADDRTABSIZ; slot++)	/*Find all hash slots.*/
		{ChainPointer = AddrTable[slot];
		 while(ChainPointer)		/* Follow the chain. */
			{NextPointer = ChainPointer->next;
			 if(ChainPointer->data.hide != 1)
				{		/* If it is private, */
			 	 if(ChainPointer->data.private == 1)
					{	/* hide it.*/
					 ChainPointer->data.hide = 1;
					 chgflag = TRUE;
					}
						/* If it uses a private node, */
						/* hide it too. */
				 else if(ChainPointer->usesa != ChainPointer
					&& ChainPointer->usesa->data.hide == 1)
					{ChainPointer->data.hide = 1;
					 chgflag = TRUE;
					}
				 else if(ChainPointer->usesb != ChainPointer
					&& ChainPointer->usesb->data.hide == 1)
					{ChainPointer->data.hide = 1;
					 chgflag = TRUE;
					}
				}
			 if(ChainPointer->data.candidate != 1)
				{if(ChainPointer->usesa != ChainPointer 
				    && ChainPointer->usesa->data.candidate == 1)
					{ChainPointer->data.candidate = 1;
					 chgflag = TRUE;
					}
				 else if(ChainPointer->usesb != ChainPointer
				    && ChainPointer->usesb->data.candidate == 1)
					{ChainPointer->data.candidate = 1;
					 chgflag = TRUE;
					}
				}
			 ChainPointer = NextPointer;
			}
		}
	} while(chgflag);

						/* We now know all those */
						/* that should be hidden. */
 first_an_id = (AN_Id) NULL;			/* None in the list yet. */
 last_an_id = (AN_Id) NULL;			/* None in the list yet. */
 for(slot = 0; slot < ADDRTABSIZ; slot++)	/* Scan all slots and hide. */
	{ChainPointer = AddrTable[slot];	/* Start of chain. */
	 PrevPointer = NULL;			/* No-one previous yet. */
	 while(ChainPointer)			/* Follow the chains. */
		{NextPointer = ChainPointer->next;
		 ChainPointer->forw = (AN_Id) NULL;	/* Remove GNAQ list. */
		 ChainPointer->back = (AN_Id) NULL;
		 ChainPointer->data.AddIndexP = 0;
		 if(ChainPointer->data.hide == 1)	/* If marked, */
			{if(PrevPointer)		/* fix chain. */
				PrevPointer->next = NextPointer;
			 else
				AddrTable[slot] = NextPointer;
			 if(first_an_id == (AN_Id) NULL)	/* Make list */
				{first_an_id = ChainPointer;	/* of hidden */
				}
			 else
				last_an_id->privt = ChainPointer;
			 last_an_id = ChainPointer;
			 ChainPointer->next = (AN_Id) NULL;	/* nodes. */
			 ChainPointer->privt = (AN_Id) NULL;
			} /* END OF if(ChainPointer->data.hide == 1) */
		 else
			PrevPointer = ChainPointer;
		 ChainPointer = NextPointer;	/* Advance along the chain. */
		} /* END OF while(ChainPointer) */
	}
 BotANId = (AN_Id) NULL;			/* Re-initialize. */
 TopANId = (AN_Id) NULL;			/* Re-initialize. */
 return(first_an_id);
}
	void
HideAdCandidate()		/* Hide all the private address nodes.	*/
				/* Simply remove them from AddrTable.	*/

{extern AN_Id AddrTable[];	/* Address table pointers.	*/
 extern AN_Id BotANId;		/* Bottom of GNAQ list.	*/
 register AN_Id ChainPointer;	/* Table chain pointer. */
 register AN_Id NextPointer;	/* Pointer to next node in chain.	*/
 AN_Id PrevPointer;		/* Pointer to previous node in chain.	*/
 extern AN_Id TopANId;		/* Top of GNAQ list.	*/
 boolean hideflag;		/* TRUE if item hidden.	*/
 register unsigned slot;	/* Address node pointer table index.	*/

 do	{hideflag = FALSE;			/* None hidden in this pass. */
	 for(slot = 0; slot < ADDRTABSIZ; slot++)	/*Find all hash slots.*/
		{ChainPointer = AddrTable[slot];
		 while(ChainPointer)		/* Follow the chain. */
			{NextPointer = ChainPointer->next;
			 if(ChainPointer->data.hide != 1)
				{		/* If it is private, */
			 	 if(ChainPointer->data.private == 1)
					{	/* hide it.*/
					 ChainPointer->data.hide = 1;
					 hideflag = TRUE;
					}
						/* If it uses a private node, */
						/* hide it too. */
				 else if(ChainPointer->usesa != ChainPointer
					&& ChainPointer->usesa->data.hide == 1)
					{ChainPointer->data.hide = 1;
					 hideflag = TRUE;
					}
				 else if(ChainPointer->usesb != ChainPointer
					&& ChainPointer->usesb->data.hide == 1)
					{ChainPointer->data.hide = 1;
					 hideflag = TRUE;
					}
				}
			 ChainPointer = NextPointer;
			}
		}
	} while(hideflag);

						/* We now know all those */
						/* that should be hidden. */
 for(slot = 0; slot < ADDRTABSIZ; slot++)	/* Scan all slots and hide. */
	{ChainPointer = AddrTable[slot];	/* Start of chain. */
	 PrevPointer = NULL;			/* No-one previous yet. */
	 while(ChainPointer)			/* Follow the chains. */
		{NextPointer = ChainPointer->next;
		 ChainPointer->forw = (AN_Id) NULL;	/* Remove GNAQ list. */
		 ChainPointer->back = (AN_Id) NULL;
		 ChainPointer->data.AddIndexP = 0;
		 if(ChainPointer->data.hide == 1)	/* If marked, */
			{if(PrevPointer)		/* fix chain. */
				PrevPointer->next = NextPointer;
			 else
				AddrTable[slot] = NextPointer;
			 ChainPointer->next = (AN_Id) NULL;	/* nodes. */
			} /* END OF if(ChainPointer->data.hide == 1) */
		 else
			PrevPointer = ChainPointer;
		 ChainPointer = NextPointer;	/* Advance along the chain. */
		} /* END OF while(ChainPointer) */
	}
 BotANId = (AN_Id) NULL;			/* Re-initialize. */
 TopANId = (AN_Id) NULL;			/* Re-initialize. */
 return;
}

#ifndef MACRO
	boolean
IsAdArg(an_id)			/* TRUE if an_id is an argument.	*/
AN_Id an_id;

{extern boolean IsAdDisp();	/* TRUE if displacement; in this file.	*/

 if(!IsAdDisp(an_id))				/* TRUE if an_id is a	*/
	return(FALSE);				/* displacement	*/
 if(GetAdRegA(an_id) == CAP)			/* from the AP.	*/
	return(TRUE);
 return(FALSE);
}
#endif


	boolean
IsAdAuto(an_id)			/* TRUE if an_id is an automatic.	*/
AN_Id an_id;

{extern boolean IsAdDisp();	/* TRUE if displacement; in this file.	*/

 if(!IsAdDisp(an_id))				/* TRUE if an_id is a	*/
	return(FALSE);				/* displacement	*/
 if(GetAdRegA(an_id) == CFP)			/* from the FP.	*/
	return(TRUE);
 return(FALSE);
}


	boolean
IsAdAbsDef(an_id)		/* Test if this node is AbsDef. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == AbsDef)	/* Test it. */
	return(TRUE);				/* It is AbsDef. */
 return(FALSE);					/* It is not AbsDef. */
}


	boolean
IsAdAbsolute(an_id)		/* Test if this node is Absolute. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == Absolute)	/* Test it. */
	return(TRUE);				/* It is Absolute. */
 return(FALSE);					/* It is not Absolute. */
}
	boolean
IsAdAddInd(an_id)		/* TRUE if additional indirect allowed. */
AN_Id an_id;			/* Want to add indirect to this node. */

{extern boolean IsLegalAd();	/* TRUE if legal address.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */

 switch(an_id->key.K.mode)			/* Depends on mode. */
	{case Absolute:
		if(IsLegalAd(AbsDef,
				(RegId) an_id->key.K.rega,
				(RegId) an_id->key.K.regb,
				an_id->key.K.con,an_id->key.K.string))
			return(TRUE);
		endcase;
	 case CPUReg:
		if(IsLegalAd(Disp,
				(RegId) an_id->key.K.rega,
				(RegId) an_id->key.K.regb,
				an_id->key.K.con,an_id->key.K.string))
			return(TRUE);
		endcase;
	 case Disp:
		if(IsLegalAd(DispDef,
				(RegId) an_id->key.K.rega,
				(RegId) an_id->key.K.regb,
				an_id->key.K.con,an_id->key.K.string))
			return(TRUE);
		endcase;
	 case Immediate:
		if(IsLegalAd(Absolute,
				(RegId) an_id->key.K.rega,
				(RegId) an_id->key.K.regb,
				an_id->key.K.con,an_id->key.K.string))
			return(TRUE);
		endcase;
	 case AbsDef:
	 case DispDef:
	 case IndexRegDisp:
	 case IndexRegScaling:
	 case PostDecr:
	 case PostIncr:
	 case PreDecr:
	 case PreIncr:
	 case Raw:
	 case StatCont:
		endcase;
	 default:
		fatal("IsAdAddInd: unknown mode (%d).\n",an_id->key.K.mode);
		endcase;
	}
 return(FALSE);
}

#ifndef MACRO
	boolean
IsAdAddrIndex(an_id)		/* Test if this node has address-index. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.AddIndexP == 1)			/* Test it. */
	return(TRUE);				/* It has ld identifier. */
 return(FALSE);					/* It has no ld identifier. */
}
#endif


#ifndef MACRO
	boolean
IsAdCPUReg(an_id)		/* Test if this node is CPUReg. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == CPUReg)	/* Test it. */
	return(TRUE);				/* It is CPU Register. */
 return(FALSE);					/* It is not CPU Register. */
}
#endif

	boolean
IsAdDisp(an_id)			/* Test if this node is Disp. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == Disp)		/* Test it. */
	return(TRUE);				/* It is Displacement. */
 return(FALSE);					/* It is not Displacement. */
}


	boolean
IsAdDispDef(an_id)		/* Test if this node is DispDef. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == DispDef)	/* Test it. */
	return(TRUE);				/* It is DispDeferred. */
 return(FALSE);					/* It is not DispDef. */
}

#ifndef MACRO
	boolean
IsAdENAQ(an_id)			/* Test if this node is ENAQ. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.gnaq == (unsigned) ENAQ)	/* Test it. */
	return(TRUE);				/* It is ENAQ. */
 return(FALSE);					/* It is not ENAQ. */
}
#endif

 	boolean
IsAdFP(an_id)			/* Test if this node is FP. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.fp == (unsigned) YES)			/* Test it. */
	return(TRUE);				/* It is FP. */
 return(FALSE);					/* It is not FP. */
}

	boolean
IsAdGNAQSize(an_id)		/* TRUE if GNAQ size present; else FALSE. */
AN_Id an_id;			/* AN_Id of node to be tested. */

{
 if(an_id->data.MDLpresent == 1)
	return(TRUE);
 else
	return(FALSE);
}

#ifndef MACRO
	boolean
IsAdImmediate(an_id)		/* Test if this node is Immediate. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == Immediate)	/* Test it. */
	return(TRUE);				/* It is Immediate. */
 return(FALSE);					/* It is not Immediate. */
}
#endif
	boolean
IsAdIndexRegDisp(an_id)		/* Test if this node is IndexRegDisp. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == IndexRegDisp)	/* Test it. */
	return(TRUE);				/* It is IndexRegDisp. */
 return(FALSE);					/* It is not IndexRegDisp. */
}


	boolean
IsAdIndexRegScaling(an_id)	/* Test if this node is IndexRegScaling. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == IndexRegScaling)	/* Test it. */
	return(TRUE);				/* It is IndexRegScaling. */
 return(FALSE);					/* It is not IndexRegScaling. */
}


	boolean
IsAdLabel(an_id)		/* Test if this node is a Label. */
AN_Id an_id;			/* Address-node-identifier of node to test. */

{
 if(an_id->data.label == 1)			/* Test it. */
	return(TRUE);				/* It is LABEL. */
 return(FALSE);					/* It is not LABEL. */
}


	boolean
IsAdMAUReg(an_id)		/* Test if this node is MAUReg. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == MAUReg)	/* Test it. */
	return(TRUE);				/* It is MAU register. */
 return(FALSE);					/* It is not MAU register. */
}

#ifndef MACRO
	boolean
IsAdNAQ(an_id)			/* Test if this node is NAQ. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.gnaq == (unsigned) NAQ)		/* Test it. */
	return(TRUE);				/* It is NAQ. */
 return(FALSE);					/* It is not NAQ. */
}
#endif

	boolean
IsAdNotFP(an_id)		/* Test if this node is Not FP. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.fp == (unsigned) NO)			/* Test it. */
	return(TRUE);				/* It is not FP. */
 return(FALSE);					/* It is not FP. */
}

	boolean
IsAdNumber(an_id)		/* TRUE if address is pure number. */
AN_Id an_id;			/* AN_Id of address to be tested. */

{

 if(an_id->key.K.string[0] == EOS)		/* If no string part, */
	return(TRUE);				/* it is numeric. */
 return(FALSE);					/* Otherwise, it is not. */
}
	boolean
IsAdPreDecr(an_id)		/* Test if this node is PreDecr. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == PreDecr)	/* Test it. */
	return(TRUE);				/* It is Auto PreDecr. */
 return(FALSE);					/* It is not Auto PreDecr. */
}

	boolean
IsAdPreIncr(an_id)		/* Test if this node is PreIncr. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == PreIncr)	/* Test it. */
	return(TRUE);				/* It is Auto PreIncrement. */
 return(FALSE);					/* It is not Auto PreIncr. */
}

	boolean
IsAdPostDecr(an_id)		/* Test if this node is PostDecr. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == PostDecr)	/* Test it. */
	return(TRUE);				/* It is PostDecrement. */
 return(FALSE);					/* It is not PostDecrement. */
}


	boolean
IsAdPostIncr(an_id)		/* Test if this node is PostIncr. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == PostIncr)	/* Test it. */
	return(TRUE);				/* It is PostIncrement. */
 return(FALSE);					/* It is not PostIncrement. */
}
	boolean
IsAdPrivate(an_id)		/* TRUE if an_id is private.	*/
AN_Id an_id;			/* AN_Id of node whose status is wanted. */

{
 if(an_id->data.private == 1)
	return(TRUE);
 return(FALSE);
}

	boolean
IsAdProbe(an_id,newmode)	/* Is an address, but with newmode, present?*/
AN_Id an_id;			/* Address.	*/
AN_Mode newmode;		/* The new mode.	*/

{STATIC AN_Id ANFind();	/* Finds an address node; in this file.	*/
 struct addrkey key;		/* Key for ANFind().	*/

 key = an_id->key.K;				/* Copy the key.	*/
 key.mode = (unsigned char) newmode;		/* Apply the new mode.	*/
 if(ANFind(&key) != (AN_Id) NULL)		/* Is node already there? */
	return(TRUE);
 return(FALSE);
}
	boolean
IsAdRO(an_id)			/* Test if this node is Read Only. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.ro == 1)		/* Test it. */
	return(TRUE);				/* It is Read only. */
 return(FALSE);					/* It is not Read only. */
}

	boolean
IsAdRaw(an_id)			/* Test if this node is Raw. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == Raw)		/* Test it. */
	return(TRUE);				/* It is Raw. */
 return(FALSE);					/* It is not Raw. */
}
	boolean
IsAdRemInd(type,an_id)		/* TRUE if less indirect allowed. */
OperandType type;		/* Type of address we are dealing with.	*/
AN_Id an_id;			/* Want to add indirect to this node. */

{extern int LSBOffset();	/* Gives offset of LSB from address.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */

 switch(an_id->key.K.mode)			/* Depends on mode. */
	{case Absolute:
	 case AbsDef:
	 case DispDef:
		return(TRUE);
		/*NOTREACHED*/
		endcase;
	 case Disp:
		if(an_id->key.K.string[0] == EOS)	/* IsAdNumber(an_id) */
			{if(an_id->key.K.con - LSBOffset(Disp,type) == 0)
				return(TRUE);
			}
		endcase;
	 case CPUReg:
	 case Raw:
	 case StatCont:
	 case Immediate:
	 case IndexRegDisp:
	 case IndexRegScaling:
	 case PostDecr:
	 case PostIncr:
	 case PreDecr:
	 case PreIncr:
		endcase;
	 default:
		fatal("IsAdRemInd: unknown mode (%d).\n",an_id->key.K.mode);
		endcase;
	}
 return(FALSE);
}

#ifndef MACRO
	boolean
IsAdSENAQ(an_id)		/* Test if this node is SENAQ. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.gnaq == (unsigned) SENAQ)	/* Test it. */
	return(TRUE);				/* It is SENAQ. */
 return(FALSE);					/* It is not SENAQ. */
}
#endif

#ifndef MACRO
	boolean
IsAdSNAQ(an_id)			/* Test if this node is SENAQ. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.gnaq == (unsigned) SNAQ)	/* Test it. */
	return(TRUE);				/* It is SENAQ. */
 return(FALSE);					/* It is not SENAQ. */
}
#endif

	boolean
IsAdSV(an_id)			/* Test if this node is SV. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.gnaq == (unsigned) SV)		/* Test it. */
	return(TRUE);				/* It is SV. */
 return(FALSE);					/* It is not SV. */
}

	boolean
IsAdSafe(an_id)			/* Test if this node is Safe. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.safe == 1)			/* Test it. */
	return(TRUE);				/* It is safe from volatility. */
 return(FALSE);					/* It is not safe. */
}
	boolean
IsAdStatCont(an_id)		/* Test if this node is StatCont. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == StatCont)	/* Test it. */
	return(TRUE);				/* It is StatCont. */
 return(FALSE);					/* It is not StatCont. */
}

#ifndef MACRO
	boolean
IsAdTemp(an_id)			/* Test if this node is Temp Reg. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if((AN_Mode) an_id->key.K.mode == CPUReg
		&& (RegId) an_id->key.K.rega == CTEMP)	/* Test it. */
	return(TRUE);				/* It is Temp Register. */
 return(FALSE);					/* It is not Temp Register. */
}
#endif

	boolean
IsAdTIQ(an_id)			/* Test if this node is Safe. */
AN_Id an_id;			/* Address-node-identifier of node to test. */


{
 if(an_id->data.tiq == 1)			/* Test it. */
	return(TRUE);				/* It is TIQ. */
 return(FALSE);					/* It is not TIQ. */
}


	boolean
IsAdTranslation(an_id)		/* Returns TRUE if translation node. */
AN_Id an_id;			/* AN_Id of node to be tested. */

{
 if(an_id->data.translation == 1)		/* Test it. */
	return(TRUE);				/* It is a translation node. */
 return(FALSE);					/*It's not a translation node.*/
}

#ifndef MACRO
	boolean
IsAdUses(an_id1,an_id2)		/* Returns TRUE if an_id1 uses an_id2. */
				/* "Uses" means if you change the value	*/
				/* in an_id2, then the value in an_id1	*/
				/* is also changed.	*/
register AN_Id an_id1;
register AN_Id an_id2;


{extern boolean IsAdUses();	/* Ourself! */
 extern void fatal();		/* Handles fatal errors.	*/
 static unsigned int level;	/* Recursion level counter.	*/

 if(level++ > 5)				/* Probably should never */
	fatal("IsAdUses: stuck in loop.\n");	/* Get deeper than 2 or 3. */

 if(an_id1 == an_id2)				/* If we use ourself,	*/
	{level--;
	 return(TRUE);				/* that counts.	*/
	}
 if(an_id1->usesa == an_id2)			/* See if it does. */
	{level--;
	 return(TRUE);				/* an_id1 does use an_id2. */
	}
 if(an_id1->usesb == an_id2)			/* See if it does. */
	{level--;
	 return(TRUE);				/* an_id1 does use an_id2. */
	}
 if(an_id1 != an_id1->usesa)
	{if(IsAdUses(an_id1->usesa,an_id2))
		{level--;
		 return(TRUE);			/*an_id1 uses an_id2,indirect.*/
		}
	}
 if(an_id1 != an_id1->usesb)
	{if(IsAdUses(an_id1->usesb,an_id2))
		{level--;
		 return(TRUE);
		}
	}
 level--;
 return(FALSE);					/* It does not. */
}
#endif
	boolean
IsAdValid(an_id)		/* TRUE if valid AN_Id supplied.	*/
register AN_Id an_id;		/* AN_Id to be tested.	*/

{extern AN_Id AddrTable[];	/* Address pointer table.	*/
 register AN_Id ChainPointer;	/* Address chain pointer.	*/
 register unsigned int index;	/* Address table index.	*/

 if(an_id == (AN_Id) NULL)
	return(FALSE);
 if(an_id->data.candidate == 1)	/* ignore candidate-only nodes. */
	return(TRUE);

 for(index = 0; index < ADDRTABSIZ; index++)	/* Scan all address nodes to */
	{ChainPointer = AddrTable[index];	/* see if an_id is there. */
	 while(ChainPointer)			/* Follow the chains.	*/
		{if(ChainPointer == an_id)	/* Does this one match?	*/
			return(TRUE);		/* Yes: a winner.	*/
		 ChainPointer = ChainPointer->next;	/* No: try another. */
		}
	}
 return(FALSE);					/* A Loser.	*/
}
	void
OrAdEnaqs(array,words)		/* OR ENAQ bit-vector to array.	*/
unsigned long int *array;	/* Array to OR to.	*/
unsigned int words;		/* Number of words in array.	*/

{extern unsigned long int *Enaqs;	/* Bit-vector of ENAQS.	*/
 extern unsigned int Size;	/* Size of bit-vectors.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 register unsigned int vector;

 if(Enaqs == (unsigned long int *) NULL)
	fatal("OrAdEnaqs: bit-vector not initialized.\n");
 if(words > Size)
	fatal("OrAdEnaqs: words (%u) too big (%u).\n",words,Size);

 for(vector = 0; vector < words; vector++)
	*(array + vector) |= *(Enaqs + vector);

 return;
}


	void
OrAdSenaqs(array,words)		/* OR SENAQ bit-vector to array.	*/
unsigned long int *array;	/* Array to OR to.	*/
unsigned int words;		/* Number of words in array.	*/

{extern unsigned long int *Senaqs;	/* Bit-vector of SENAQS.	*/
 extern unsigned int Size;	/* Size of bit-vectors.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 register unsigned int vector;

 if(Senaqs == (unsigned long int *) NULL)
	fatal("OrAdSenaqs: bit-vector not initialized.\n");
 if(words > Size)
	fatal("OrAdSenaqs: words (%u) too big (%u).\n",words,Size);

 for(vector = 0; vector < words; vector++)
	*(array + vector) |= *(Senaqs + vector);

 return;
}


	void
OrAdSnaqs(array,words)		/* OR SENAQ bit-vector to array.	*/
unsigned long int *array;	/* Array to OR to.	*/
unsigned int words;		/* Number of words in array.	*/

{extern unsigned long int *Snaqs;	/* Bit-vector of SENAQS.	*/
 extern unsigned int Size;	/* Size of bit-vectors.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 register unsigned int vector;

 if(Snaqs == (unsigned long int *) NULL)
	fatal("OrAdSnaqs: bit-vector not initialized.\n");
 if(words > Size)
	fatal("OrAdSnaqs: words (%u) too big (%u).\n",words,Size);

 for(vector = 0; vector < words; vector++)
	*(array + vector) |= *(Snaqs + vector);

 return;
}

	void
PutAdCandidate(an_id,status)	/* Set candidate use only status. */
AN_Id an_id;			/* Address node identifier. */
boolean status;			/* True or false. */


{
 an_id->data.candidate = status ? 1 : 0;
}

#ifndef MACRO
	void
PutAdEstim(an_id,value)		/* Put use estimate in address node. */
AN_Id an_id;			/* Address node identifier. */
long int value;			/* Value to put in. */


{
 an_id->data.estim = value;
 return;
}
#endif

	void
PutAdFP(an_id,status)		/* Set floating point status. */
AN_Id an_id;			/* Address node identifier. */
enum AN_Fp_E status;			/* Value to assign. */


{extern void fatal();		/* Handles fatal errors; in debug.c. */

 switch(status)
	{case YES:
	 case NO:
	 case MAYBE:
		an_id->data.fp = (unsigned int)status;
		endcase;
	 default:
		fatal("PutAdFP: invalid argument (%d).\n",status);
		endcase;
	}
 return;
}
	void
PutAdGNAQSize(an_id,value)	/* Put GNAQ size in address node. */
AN_Id an_id;			/* Address node identifier. */
unsigned short int value;		/* Value to put in. */

{
 an_id->data.MDLength = value;			/* Put the value in. */
 an_id->data.MDLpresent = 1;			/* Mark it present. */
 return;
}


#ifndef MACRO
	void
PutAdGnaqType(ad_id,type)	/* Put GNAQ type in this node. */
AN_Id ad_id;			/* Address-node-identifier of this node. */
AN_GnaqType type;		/* The type of this address node. */


{extern void fatal();		/* Takes care of fatal errors; in debug.c. */

 switch(type)				/* Decode address node entry. */
	{case Other:
	 case NAQ:
	 case ENAQ:
	 case SNAQ:
	 case SENAQ:
	 case SV:
		ad_id->data.gnaq = (unsigned) type;
		endcase;
	 default:			/* Logic error if here. */
		fatal("PutAdGnaqType: invalid type (0x%x).\n",type);
		endcase;
	}
 return;
}
#endif

	void
PutAdLabel(an_id,status)	/* Set Label status. */
AN_Id an_id;			/* Address node identifier. */
boolean status;			/* Value to assign. */

{
 an_id->data.label = status ? 1 : 0;
 return;
}
	void
PutAdRO(an_id,status)		/* Set Read Only status. */
AN_Id an_id;			/* Address node identifier. */
boolean status;			/* Value to assign. */

{
 if(status == TRUE)
	an_id->data.ro = 1;
 else
	an_id->data.ro = 0;
 return;
}


	void
PutAdSafe(an_id,status)		/* Set Safety-from-volatility status. */
AN_Id an_id;			/* Address node identifier. */
boolean status;			/* Value to assign. */

{
 if(status == TRUE)
	an_id->data.safe = 1;
 else
	an_id->data.safe = 0;
 return;
}


	void
PutAdTIQ(an_id,status)		/* Set Type-Independant-Quantity status. */
AN_Id an_id;			/* Address node identifier. */
boolean status;			/* Value to assign. */

{
 an_id->data.tiq = status ? 1 : 0;
 return;
}
	void
RestoreAdPrivate(firstone)	/* Restore private address nodes.	*/
AN_Id firstone;			/* AN_Id of first of list.	*/

{extern AN_Id AddrTable[];	/* Address table pointers.	*/
 register AN_Id an_id;
 register unsigned int index;
 register AN_Id nextone;

 an_id = firstone;
 while(an_id)
	{index = an_id->data.slot;		/* What slot does it go in? */
	 nextone = an_id->privt;		/* What is the next one, if any? */
	 if(an_id->data.candidate == 1)		/* Don't restore candidate nodes */
		{an_id = nextone;
		 continue;
		}
	 an_id->next = AddrTable[index];	/* Insert this at head	*/
	 AddrTable[index] = an_id;		/* the list.	*/
	 an_id->data.hide = 0;			/* Unhide it.	*/
	 an_id = nextone;
	}
 return;
}
	void
SortAdGnaq(type,Thresh)		/* Sort [type] GNAQs by estim. */
register unsigned int type;	/* Types of GNAQs to participate in the sort. */
unsigned int Thresh;		/* If != 0, don't sort if less than this. */
				/* N.B.: Immediates never participate.	*/
				/*	 Labels never participate either. */


{extern AN_Id BotANId;		/* Lowest sorted address node id. */
 register AN_Id ChainPointer;	/* To follow chain from address pointer table.*/
 STATIC int AN_Compar();	/* Address node comparison function. */
 extern unsigned int AddrSerial;	/* Address node serial number.*/
 extern AN_Id AddrTable[];	/* Address pointer table. */
 extern unsigned long int *Enaqs;	/* Enaq bit-vector table.	*/
 extern unsigned int GnaqListSize;	/* Current size of GNAQ list.	*/
 extern boolean IsAdLabel();	/* TRUE if address is a label.	*/
 extern unsigned int MaxGNAQNodes;	/* Maximum size of GNAQ list.	*/
 extern unsigned long int *Senaqs;	/* Senaq bit-vector table.	*/
 extern unsigned int Size;	/* Size of bit-vector tables.	*/
 extern unsigned long int *Snaqs;	/* Senaq bit-vector table.	*/
 extern AN_Id TopANId;		/* Highest sorted address node id. */
 extern int errno;		/* UNIX error number.	*/
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 register unsigned int index;	/* Index into address pointer table. */
 extern void qsort();		/* Quicker-Sort; in C(3) library. */
 AN_Id *sort_table;		/* Table of address nodes to sort.*/
 AN_Id sti;			/* Sort table index. */

 if(AddrSerial == 0)				/* If no address nodes,	*/
	return;					/* don't bother.	*/

 sort_table = (AN_Id *) Malloc(AddrSerial * sizeof(AN_Id));	/* Get space. */
						/* We need no more than total */
						/* number of address nodes.   */
 if(sort_table == NULL)				/* Did we get it? */
	fatal("SortAdGnaq: Malloc failed (%d).\n",errno);	/*No: give up.*/

 GnaqListSize = 0;				/* Initialize counter. */
 for(index = 0; index < ADDRTABSIZ; index++)	/* Scan all address nodes. */
	{ChainPointer = AddrTable[index];
	 while(ChainPointer)			/* Follow the chain. */
						/* Prepare sort-table for */
						/* those that participate. */
		{if((unsigned int) ChainPointer->data.gnaq & type)
			{if(((AN_Mode) ChainPointer->key.K.mode != Immediate) &&
					!IsAdLabel(ChainPointer))
				sort_table[GnaqListSize++] = ChainPointer;
			}
		 ChainPointer->data.AddIndexP = 0;	/*Address Index Undef.*/
		 ChainPointer = ChainPointer->next;	/* Follow the chain. */
		}
	}

						/* Update max GNAQ list size. */
 MaxGNAQNodes = (GnaqListSize > MaxGNAQNodes) ? GnaqListSize : MaxGNAQNodes;

 if((Thresh == 0) || (GnaqListSize >= Thresh))
	qsort((char *) sort_table,GnaqListSize,sizeof(AN_Id),AN_Compar);

 if(Enaqs != (unsigned long int *) NULL)	/* Get new bit vector space. */
	Free((char *) Enaqs);
 if(Senaqs != (unsigned long int *) NULL)
	Free((char *) Senaqs);
 if(Snaqs != (unsigned long int *) NULL)
	Free((char *) Snaqs);
 Size = (GnaqListSize + sizeof(unsigned long int)*B_P_BYTE - 1) /
	(sizeof(unsigned long int)*B_P_BYTE);
 Enaqs = (unsigned long int *) Malloc(Size * sizeof(unsigned long int));
 Senaqs = (unsigned long int *) Malloc(Size * sizeof(unsigned long int));
 Snaqs = (unsigned long int *) Malloc(Size * sizeof(unsigned long int));
 for(index = 0; index < Size; index++)
	{*(Enaqs + index) = 0;
	 *(Senaqs + index) = 0;
	 *(Snaqs + index) = 0;
	}

 for(index = 0; index < GnaqListSize; index++)	/* Make linked list. */
	{sti = sort_table[index];		/* Quickie pointer. */
	 sti->forw =				/* Assign forward pointer. */
		(index < (GnaqListSize - 1)) ?	/* If not at end, */
			sort_table[index + 1] :	/* put in forward pointer. */
			NULL;			/* No forward pointer. */
	 sti->back =				/* Assign backward pointer. */
		(index == 0) ?			/* If at beginning, */
			NULL :			/* no backward pointer. */
			sort_table[index - 1];	/* Put in backward pointer. */
	 sti->data.AddrIndex = (unsigned short) index;	/* Put in address index, */
	 sti->data.AddIndexP = 1;		/* and mark it present. */
	 switch((AN_GnaqType) sti->data.gnaq)	/* Populate bit-vectors. */
		{case Other:
			endcase;
		 case NAQ:
			endcase;
		 case ENAQ:
			set_bit(Enaqs,index);
			endcase;
		 case SNAQ:
			set_bit(Snaqs, index);
			endcase;
		 case SENAQ:
			set_bit(Senaqs,index);
			endcase;
		 case SV:
			endcase;
		 default:
			fatal("SortAdGnaq: unknown GNAQ type.\n");
			endcase;
		}
	}

 BotANId = sort_table[GnaqListSize - 1];	/* Set endpoints for */
 TopANId = sort_table[0];			/* GetAdNextGNode &c. */
 Free(sort_table);				/* Release storage. */
 return;
}
	void
addrprint(title)		/* Print sorted address nodes. */
char *title;			/* Title of printout.	*/


{AN_Id ChainPointer;		/* To follow chain from address pointer table.*/
 STATIC int AN_KeyCompar();	/* Address node comparison function. */
 extern AN_Id AddrTable[];	/* Address pointer table. */
 AN_GnaqType Gtype;		/* Type of gnaq in case we call fatal(). */
 extern boolean IsAdLabel();	/* True if address is a label.	*/
 extern void addraudit();	/* Audits address nodes.	*/
 register AN_Id an_id;		/* Quickie reference to sort_table[index]. */
 extern int errno;		/* UNIX error number. */
 extern void fatal();		/* Handles fatal errors; in common. */
 char *gnaq;			/* External form of GNAQ type. */
 register unsigned int index;	/* Index into address pointer table. */
 unsigned int maxaddrnodes;	/* Maximum address nodes to sort. */
 unsigned int number;		/* Number of nodes to sort. */
 extern unsigned int praddr();	/* Prints an address node; in Mach. Dep. */
 extern void qsort();		/* Quicker-Sort; in C(3) library. */
 AN_Id *sort_table;		/* Table of address nodes to sort.*/
 extern char *extaddr();	/* Print address to string; in ANOut.c. */

 fprintf(stdout,"%c		***** %s *****\n",	/* Do title.	*/
		ComChar,title);
 addraudit(title);
 maxaddrnodes = 0;				/* Count address nodes that */
 for(index = 0; index < ADDRTABSIZ; index++)	/* will participate in sort. */
	{number = 0;				/*Count number in each bucket.*/
	 ChainPointer = AddrTable[index];	/* We need this to allocate */
	 while(ChainPointer)			/* table for sort keys. */
		{maxaddrnodes += 1;		/* We found one. */
		 number += 1;			/* For bucket counter. */
		 if(DBdebug(4,XTB_AD))
			{if(number == 1)
				fputc(ComChar,stdout);
			 (void) praddr(ChainPointer,Tubyte,stdout);
			 putc(SPACE,stdout);
			}
		 ChainPointer = ChainPointer->next;
		}
	 if((number != 0) && DBdebug(3,XTB_AD))
		fprintf(stdout,"%c\tBin %4.1u contains %4.u nodes.\n",
				ComChar,index,number);
	}

 if(maxaddrnodes == 0)				/* If no address nodes,	*/
	fatal("addrprint: no address nodes.\n");	/* trouble.	*/

 sort_table = (AN_Id *) Malloc(maxaddrnodes * sizeof(AN_Id));	/* Get space. */
 if(sort_table == NULL)				/* Did we get it? */
	fatal("addrprint: Malloc failed (%d).\n",errno);	/*No: give up.*/

 number = 0;					/* Initialize counter. */
 for(index = 0; index < ADDRTABSIZ; index++)	/* Scan all address nodes. */
	{ChainPointer = AddrTable[index];
	 while(ChainPointer)			/* Follow the chain. */
						/* Prepare sort-table for */
						/* those that participate. */
		{sort_table[number++] = ChainPointer;
		 if(number > maxaddrnodes)	/* Exceed our implementation?*/
			fatal("addrprint: disaster.\n");
		 ChainPointer = ChainPointer->next;	/* Follow the chain. */
		}
	}

 qsort((char *) sort_table,number,sizeof(AN_Id),AN_KeyCompar);

 fprintf(stdout,"\n%c\tL  Estimate\tGnaqT FP AdIndx Safe TIQ Serial\tAddress\n\n",
		ComChar);

 for(index = 0; index < number; index++)	/* Now print them. */
	{an_id = sort_table[index];		/* (Quicker reference.) */
	 switch((Gtype = (AN_GnaqType) an_id->data.gnaq))
		{case Other:
			gnaq = "Other";
			endcase;
		 case NAQ:
			gnaq = "NAQ";
			endcase;
		 case SNAQ:
			gnaq = "SNAQ";
			endcase;
		 case ENAQ:
			gnaq = "ENAQ";
			endcase;
		 case SENAQ:
			gnaq = "SENAQ";
			endcase;
		 case SV:
			gnaq = "SV";
			endcase;
		 default:
			fatal("addrprint: illegal GNAQ type (0x%8.8x).\n",
				Gtype);
			endcase;
		}
	 fprintf(stdout,
			"%c\t%c %10.1ld\t%5.5s %2.2s %6.*hu %4.4s %3.3s  %5.1hu\t%s  \n",
			ComChar,
			(IsAdLabel(an_id)) ? '*' : SPACE,
			GetAdEstim(an_id),
			gnaq,
			(IsAdFP(an_id) ? "FP" :
				(IsAdNotFP(an_id)) ? AN_NullString : "?"),
			an_id->data.AddIndexP,
			(an_id->data.AddIndexP) ? an_id->data.AddrIndex : 0,
			(IsAdSafe(an_id) ? "Safe" : AN_NullString),
			(IsAdTIQ(an_id) ? "TIQ" : AN_NullString),
			an_id->data.serial,
			extaddr(an_id,TBYTE));
	}

 Free(sort_table);				/* Release storage. */
 return;
}
	void
addraudit(title)		/* Audits address nodes.	*/
char *title;			/* Message to identify audit.	*/


{
#ifdef AUDIT
 extern AN_Id AddrTable[];	/* The address pointer table.	*/
 extern AN_Id BotANId;		/* Bottom of GNAQ list. */
 extern AN_Id TopANId;		/* Top of GNAQ list. */
 register AN_Id ChainPointer;	/* Address chain pointer.	*/
 extern boolean IsAdValid();	/* Checks if AN_Id is a valid address.	*/
 register char *cp;		/* Character scan pointer.	*/
 struct addrdata *data;		/* Pointer to data part of node.	*/
 boolean failed;		/* TRUE if audit error detected.	*/
 extern void fatal();		/* Handles fatal errors.	*/
 register unsigned int index;	/* Address table index.	*/
 struct addrkey *key;		/* Pointer to key part of node.	*/
 extern unsigned int praddr();	/* Prints an address node. */

 failed = FALSE;				/* No errors yet.	*/
 for(index = 0; index < ADDRTABSIZ; index++)	/* Examine each address node. */
	{ChainPointer = AddrTable[index];
	 while(ChainPointer)			/* Follow each chain.	*/
		{data = &ChainPointer->data;
		 key = &ChainPointer->key.K;

		 if(data->auditerr == 1)	/* Complained already? */
			{ChainPointer = ChainPointer->next;
			 continue;
			}
		 if(TopANId && ChainPointer->forw) /* Validate pointers. */
			if(!IsAdValid(ChainPointer->forw))
				{fprintf(stdout,
					"addraudit: %s\n\tforw:\t%u\n",
					title,(unsigned)(ChainPointer->forw));
				 data->auditerr = 1;
				 failed = TRUE;
				}
		 if(BotANId && ChainPointer->back) /* Need be no pointers, */
			if(!IsAdValid(ChainPointer->back))
				{fprintf(stdout,
					"addraudit: %s\n\tback:\t%u\n",
					title,(unsigned)(ChainPointer->back));
				 data->auditerr = 1;
				 failed = TRUE;
				}
		 if(ChainPointer->usesa != ChainPointer) /* but,if they exist,*/
			{if(!IsAdValid(ChainPointer->usesa))
				{fprintf(stdout,
					"addraudit: %s\n\tusesa:\t%u\n",
					title,(unsigned)(ChainPointer->usesa));
				 data->auditerr = 1;
				 failed = TRUE;
				}
			} /* END OF if(ChainPointer->usesa != ChainPointer) */
		 if(ChainPointer->usesb != ChainPointer) /*they must be valid.*/
			{if(!IsAdValid(ChainPointer->usesb))
				{fprintf(stdout,
					"addraudit: %s\n\tusesb:\t%u\n",
					title,(unsigned)(ChainPointer->usesb));
				 data->auditerr = 1;
				 failed = TRUE;
				}
			} /* END OF if(ChainPointer->usesb != ChainPointer) */

		 switch(key->mode)		/* The mode must be valid. */
						/* The appropriate modes */
						/* must have uses pointers. */
			{case Immediate:	/* These are the good modes. */
			 case AbsDef:
			 case CPUReg:
			 case MAUReg:
			 case StatCont:
			 case Raw:
				if(data->label == 1)	/*Label bit must be 0.*/
					{fprintf(stdout,
						"addraudit: %s\n\tlabel bit set  in non-absolute address\n",
						title);
					 data->auditerr = 1;
					 failed = TRUE;
					}
				endcase;
			 case Absolute:
				endcase;
			 case IndexRegDisp:
			 case IndexRegScaling:
				if((ChainPointer->usesb == (AN_Id) NULL) ||
						(ChainPointer->usesb ==
							ChainPointer))
					{fprintf(stdout,
					    "addraudit: %s\n\tbad usesb ",
					    title);
					 (void)praddr(ChainPointer,Tbyte,
						stdout);
					 fprintf(stdout,"\n");
					 data->auditerr = 1;
					 failed = TRUE;
					}
				/* FALLTHRU */
			 case Disp:
			 case DispDef:
			 case PreDecr:
			 case PreIncr:
			 case PostDecr:
			 case PostIncr:
				if((ChainPointer->usesa == NULL) ||
						(ChainPointer->usesa ==
							ChainPointer))
					{fprintf(stdout,
					    "addraudit: %s\n\tbad usesa ",
					    title);
					 (void)praddr(ChainPointer,Tbyte,
						stdout);
					 fprintf(stdout,"\n");
					 data->auditerr = 1;
					 failed = TRUE;
					}
				if(data->label == 1)	/*Label bit must be 0.*/
					{fprintf(stdout,
						"addraudit: %s\n\tlabel bit set  in non-absolute address\n",
						title);
					 data->auditerr = 1;
					 failed = TRUE;
					}
				endcase;
			 default:		/* Others are No Good.	*/
				fprintf(stdout,"addraudit: bad mode.\n");
				failed = TRUE;
				endcase;
			} /* END OF switch(key->mode) */

		 cp = key->string;		/* String must contain	*/
		 while(*cp)			/* only printing	*/
			{if(*cp++ < SPACE)	/* characters.	*/
						/* SPACE is the lowest	*/
						/* printing ASCII character. */
				{key->string = "NON-PRINT";	/* fixup */
				 fprintf(stdout,
					"addraudit: %s\tnon-printing: ",
					title);
				 (void)praddr(ChainPointer,Tubyte,stdout);
				 fprintf(stdout,"\n");
				 data->auditerr = 1;
				 failed = TRUE;
				}
			} /* END OF while(*cp) */
		 ChainPointer = ChainPointer->next;	/* Advance scan. */
		} /* END OF while(ChainPointer) */
	} /* END OF for(index = 0; index < ADDRTABSIZ; index++) */

 if(failed)
	fatal("addraudit: address table error.\n");

#endif /*AUDIT*/
 return;
}
