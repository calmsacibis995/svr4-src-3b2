//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libsymbol/common/dbtools.C	1.1"

#include "Interface.h"
#include "Tag.h"
#include "TYPE.h"

/* -- dbtools.C contains utility routines to support debugging.
 *    When all debugging flags are turned off this file should
 *    NOT be linked into the final product.
*/

void
TYPE::dump(char *label)
{
    if (label == 0) label = "TYPE";
    printe("%s: ", label);
    printe("TYPE::dump(char*) not done yet\n");
}
