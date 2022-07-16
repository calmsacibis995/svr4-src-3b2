//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libexp/common/Place.C	1.2"

#include "Place.h"
#include <stdlib.h>

// For purposes of ordering assume:
//    1. order by PlaceMark value (see Place.h).
//    2. pUnknown is effectively unique.

int
Place::operator<(Place& p)
{
    if (kind == p.kind) {
	switch (kind) {
	case pAddress:  return addr < p.addr;
	case pRegister: return reg  < p.reg;
	case pUnknown:  return 0;
	default:        abort();
	}
    }
    return kind < p.kind;
}

int
Place::operator>(Place& p)
{
    if (kind == p.kind) {
	switch (kind) {
	case pAddress:  return addr > p.addr;
	case pRegister: return reg  > p.reg;
	case pUnknown:  return 0;
	default:        abort();
	}
    }
    return kind > p.kind;
}

int
Place::operator==(Place& p)
{
    if (p.kind == kind) {
	switch (kind) {
	case pAddress:  return addr == p.addr;
	case pRegister: return reg  == p.reg;
	case pUnknown:  return 1;
	default:        abort();
	}
    }
    return 0;
}
