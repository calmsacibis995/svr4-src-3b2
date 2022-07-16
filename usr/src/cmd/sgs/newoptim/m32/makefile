#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)newoptim:m32/makefile	1.27"

ROOT=
SGS=m32
OWN=bin
GRP=bin
CC=cc
CFLAGS=-O
LDLIBS=
LINT=lint
LINTFLAGS=
SGSBASE= ../..
INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSLIB)
STRIP=strip
CCSLIB=$(ROOT)/usr/ccs/lib
#
AUDIT=AUDIT
DEBUG=DEBUG
TMACH=BMAUMAC
INLINE=MACRO
PROFILE=NOPROFILE
ARFORMAT=PORTAR
OPTIM=newoptim
INC=$(ROOT)/usr/include
LINK_MODE=
MACH=m32
MACHINC=$(SGSBASE)/inc/$(MACH)
#
PRINT=pr
#
PRFLAGS=-f -n
#
CC_CMD=$(CC) $(CFLAGS) -c  $(INCFLAGS) \
	-D$(INLINE) -D$(AUDIT) -D$(DEBUG) -D$(ARFORMAT) -D$(TMACH) -D$(PROFILE)
LD_CMD=$(CC) $(LDLIBS) $(LINK_MODE)
LINT_CMD=$(LINT) $(LINTFLAGS) $(INCFLAGS) \
	-D$(INLINE) -D$(AUDIT) -D$(DEBUG) -D$(ARFORMAT) -D$(TMACH) -D$(PROFILE)
PRINT_CMD=$(PRINT) $(PRFLAGS)
#
COMDIR=../common
MDPDIR=.
OPSDIR=../tables
TABDIR=../tables
MACHINC=$(SGSBASE)/inc/m32
COMINC=$(SGSBASE)/inc/common
INCFLAGS= -I$(COMDIR) -I$(MDPDIR) -I$(COMINC) -I$(INC) -I$(MACHINC)
#
HFILES=$(COMDIR)/ALNodeDefs.h \
	$(COMDIR)/ALNodeType.h \
	$(COMDIR)/ANodeDefs.h \
	$(COMDIR)/ANodeTypes.h \
	$(COMDIR)/FNodeDefs.h \
	$(COMDIR)/FNodeTypes.h \
	$(COMDIR)/LineDefs.h \
	$(COMDIR)/LoopTypes.h \
	$(COMDIR)/OpTabDefs.h \
	$(COMDIR)/OperndType.h \
	$(COMDIR)/TNodeDefs.h \
	$(COMDIR)/TNodeTypes.h \
	$(COMDIR)/debug.h \
	$(COMDIR)/defs.h \
	$(COMDIR)/optim.h \
	$(MDPDIR)/CSections.h \
	$(MDPDIR)/OpTabTypes.h \
	$(MDPDIR)/RegIdDefs.h \
	$(MDPDIR)/Target.h \
	$(MDPDIR)/optab.h \
	$(MDPDIR)/RegId.h \
	$(MDPDIR)/RoundModes.h \
	$(MDPDIR)/olddefs.h \
	$(MDPDIR)/optutil.h
SOURCES=$(COMDIR)/ALNode.c \
	$(COMDIR)/ANode.c \
	$(COMDIR)/CmdLine.c \
	$(COMDIR)/FNode.c \
	$(COMDIR)/GlobalDefs.c \
	$(COMDIR)/Line.c \
	$(COMDIR)/LoopTypes.c \
	$(COMDIR)/MallocFree.c \
	$(COMDIR)/OpTab.c \
	$(COMDIR)/TNode.c \
	$(COMDIR)/fatal.c \
	$(COMDIR)/inter.c \
	$(COMDIR)/loop.c \
	$(COMDIR)/optim.c \
	$(COMDIR)/rer.c \
	$(MDPDIR)/ANOut.c \
	$(MDPDIR)/ANSetFlags.c \
	$(MDPDIR)/DebugInfo.c \
	$(MDPDIR)/Estim.c \
	$(MDPDIR)/Frame.c \
	$(MDPDIR)/IsADPrvate.c \
	$(MDPDIR)/IsLegal.c \
	$(MDPDIR)/OperndType.c \
	$(MDPDIR)/RegId.c \
	$(MDPDIR)/Target.c \
	$(MDPDIR)/TNOut.c \
	$(MDPDIR)/UseSet.c \
	$(MDPDIR)/brnaq.c \
	$(MDPDIR)/comutil.c \
	$(MDPDIR)/conv.c \
	$(MDPDIR)/cse.c \
	$(MDPDIR)/debug.c \
	$(MDPDIR)/funcdata.c \
	$(MDPDIR)/gra.c \
	$(MDPDIR)/htab.c \
	$(MDPDIR)/in.c \
	$(MDPDIR)/inline.c \
	$(MDPDIR)/local.c \
	$(MDPDIR)/local2.c \
	$(MDPDIR)/lookup.c \
	$(MDPDIR)/loopopt.c \
	$(MDPDIR)/optab.c \
	$(MDPDIR)/optutil.c \
	$(MDPDIR)/peep.c \
	$(MDPDIR)/sequence.c \
	$(MDPDIR)/value.c \
	$(MDPDIR)/w1opt.c \
	$(MDPDIR)/w2opt.c \
	$(MDPDIR)/w3opt.c
OBJECTS=ANSetFlags.o \
	ANOut.o \
	ANode.o \
	CmdLine.o \
	Estim.o \
	FNode.o \
	Frame.o \
	ALNode.o \
	DebugInfo.o \
	GlobalDefs.o \
	IsADPrvate.o \
	IsLegal.o Line.o \
	LoopTypes.o \
	MallocFree.o \
	OpTab.o \
	OperndType.o \
	RegId.o \
	Target.o \
	TNOut.o \
	TNode.o \
	UseSet.o \
	brnaq.o \
	comutil.o \
	conv.o \
	cse.o \
	debug.o \
	fatal.o \
	funcdata.o \
	gra.o \
	htab.o \
	in.o \
	inline.o \
	inter.o \
	local.o \
	local2.o \
	lookup.o \
	loop.o \
	loopopt.o \
	optim.o \
	optab.o \
	optutil.o \
	peep.o \
	rer.o \
	sequence.o \
	value.o \
	w1opt.o \
	w2opt.o \
	w3opt.o
PRODUCTS=$(OPTIM)
#
OPCODES=$(OPSDIR)/is25.ops.alc $(OPSDIR)/is25.ops.cb $(OPSDIR)/cpu.ops.al\
        $(OPSDIR)/mis.ops.al $(OPSDIR)/mis.ops.cb $(OPSDIR)/cpu.ops.asm\
        $(OPSDIR)/cpu.ops.c $(OPSDIR)/cpu.ops.int $(OPSDIR)/cpu.ops.conv\
        $(OPSDIR)/mis.ops.int $(OPSDIR)/mis.ops.conv
#
TABLES=$(TABDIR)/is25.tab.alc $(TABDIR)/is25.tab.cb $(TABDIR)/cpu.tab.al\
       $(TABDIR)/mis.tab.al $(TABDIR)/mis.tab.cb $(TABDIR)/cpu.tab.asm\
       $(TABDIR)/cpu.tab.c $(TABDIR)/cpu.tab.int $(TABDIR)/cpu.tab.conv\
       $(TABDIR)/mis.tab.int $(TABDIR)/mis.tab.conv 
#
all:		$(PRODUCTS)

$(PRODUCTS):	$(OBJECTS)
		$(LD_CMD) -o $(OPTIM) $(OBJECTS) -lmalloc -lc
#
# common .o's
#
ALNode.o:	$(INC)/stdio.h $(INC)/malloc.h $(COMDIR)/defs.h \
		$(MDPDIR)/RegId.h $(COMDIR)/ALNodeType.h \
		$(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/ALNode.c
		$(CC_CMD) $(COMDIR)/ALNode.c
ANode.o:	$(INC)/stdio.h $(INC)/malloc.h $(INC)/string.h \
		$(INC)/errno.h $(INC)/ctype.h \
		$(COMDIR)/defs.h $(COMDIR)/debug.h $(COMDIR)/OperndType.h \
		$(COMDIR)/ANodeTypes.h $(MDPDIR)/RegId.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/ANode.c
		$(CC_CMD) $(COMDIR)/ANode.c
CmdLine.o:	$(INC)/stdio.h $(INC)/ctype.h $(INC)/string.h \
		$(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(COMDIR)/ANodeTypes.h $(MDPDIR)/olddefs.h $(MACHINC)/sgs.h \
		$(COMDIR)/CmdLine.c
		$(CC_CMD) $(COMDIR)/CmdLine.c
DebugInfo.o:	$(INC)/ctype.h $(INC)/stdio.h $(INC)/string.h \
		$(COMDIR)/defs.h $(COMDIR)/ANodeTypes.h \
		$(MDPDIR)/olddefs.h $(MDPDIR)/OpTabTypes.h \
		$(COMDIR)/ALNodeType.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/FNodeTypes.h $(COMDIR)/FNodeDefs.h \
		$(MDPDIR)/CSections.h $(MDPDIR)/RegId.h \
		$(COMINC)/dwarf.h optab.h \
		$(MDPDIR)/DebugInfo.c
		$(CC_CMD) $(MDPDIR)/DebugInfo.c
FNode.o:	$(INC)/stdio.h $(INC)/malloc.h $(INC)/string.h \
		$(COMDIR)/defs.h $(COMDIR)/ANodeTypes.h \
		$(MDPDIR)/olddefs.h $(COMDIR)/LoopTypes.h \
		$(COMDIR)/OperndType.h optab.h $(MDPDIR)/RegId.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h \
		$(COMDIR)/ALNodeType.h $(COMDIR)/ALNodeDefs.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/OpTabDefs.h \
		$(COMDIR)/FNodeTypes.h \
		$(COMDIR)/FNode.c
		$(CC_CMD) $(COMDIR)/FNode.c
GlobalDefs.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h $(MDPDIR)/olddefs.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/ALNodeType.h \
		$(COMDIR)/FNodeTypes.h $(COMDIR)/optim.h \
		$(COMDIR)/GlobalDefs.c
		$(CC_CMD) $(COMDIR)/GlobalDefs.c
Line.o:		$(COMDIR)/defs.h $(COMDIR)/ANodeTypes.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/LoopTypes.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/RoundModes.h \
		$(INC)/stdio.h $(COMDIR)/TNodeDefs.h \
		$(MDPDIR)/OpTabTypes.h \
		$(COMDIR)/Line.c
		$(CC_CMD) $(COMDIR)/Line.c
LoopTypes.o:	$(INC)/string.h $(COMDIR)/defs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/LoopTypes.c
		$(CC_CMD) $(COMDIR)/LoopTypes.c
MallocFree.o:	$(INC)/stdio.h $(INC)/malloc.h $(COMDIR)/defs.h \
		$(COMDIR)/MallocFree.c
		$(CC_CMD) $(COMDIR)/MallocFree.c
OpTab.o:	optab.h $(COMDIR)/defs.h $(MDPDIR)/OpTabTypes.h \
		$(COMDIR)/OpTab.c
		$(CC_CMD) $(COMDIR)/OpTab.c
TNode.o:	$(INC)/stdio.h $(INC)/malloc.h $(COMDIR)/defs.h $(MDPDIR)/RegId.h \
		$(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/olddefs.h $(COMDIR)/TNodeTypes.h optab.h \
		$(COMDIR)/optim.h $(MDPDIR)/RoundModes.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/OpTabDefs.h \
		$(COMDIR)/TNode.c
		$(CC_CMD) $(COMDIR)/TNode.c
fatal.o:	$(INC)/stdio.h $(INC)/string.h \
		$(INC)/varargs.h \
		$(COMDIR)/defs.h $(COMDIR)/fatal.c
		$(CC_CMD) $(COMDIR)/fatal.c
inter.o:	$(INC)/stdio.h $(INC)/malloc.h $(COMDIR)/defs.h $(MDPDIR)/RegId.h \
		$(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h optab.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/optim.h \
		$(COMDIR)/inter.c
		$(CC_CMD) $(COMDIR)/inter.c
loop.o:		$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(COMDIR)/ANodeTypes.h $(MDPDIR)/olddefs.h optab.h \
		$(COMDIR)/ANodeDefs.h $(MDPDIR)/RegId.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/RoundModes.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h $(COMDIR)/FNodeTypes.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/FNodeDefs.h \
		$(COMDIR)/ALNodeType.h \
		$(COMDIR)/loop.c
		$(CC_CMD) $(COMDIR)/loop.c
optim.o:	$(INC)/malloc.h $(INC)/stdio.h $(INC)/memory.h \
		$(COMDIR)/defs.h \
		$(COMDIR)/debug.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/optim.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/TNodeDefs.h optab.h \
		$(MDPDIR)/OpTabTypes.h \
		$(COMDIR)/optim.c
		$(CC_CMD) $(COMDIR)/optim.c
rer.o:		$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/ANodeTypes.h \
		$(MDPDIR)/RegId.h $(COMDIR)/ANodeDefs.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h optab.h $(COMDIR)/optim.h \
		$(MDPDIR)/OpTabTypes.h \
		$(COMDIR)/rer.c
		$(CC_CMD) $(COMDIR)/rer.c
#
# m32 .o's
#
ANOut.o:	$(INC)/stdio.h $(INC)/string.h $(COMDIR)/defs.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h \
		$(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/olddefs.h \
		$(MDPDIR)/ANOut.c
		$(CC_CMD) $(MDPDIR)/ANOut.c
ANSetFlags.o:	$(COMDIR)/defs.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h \
		$(MDPDIR)/ANSetFlags.c
		$(CC_CMD) $(MDPDIR)/ANSetFlags.c
Estim.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/ANodeTypes.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h optab.h \
		$(MDPDIR)/Target.h $(INC)/values.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/debug.h \
		$(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/Estim.c
		$(CC_CMD) $(MDPDIR)/Estim.c
Frame.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(MDPDIR)/RegId.h \
		$(COMDIR)/OperndType.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(COMDIR)/LoopTypes.h \
		$(MDPDIR)/RoundModes.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h \
		$(COMDIR)/ALNodeType.h \
		$(COMDIR)/FNodeTypes.h $(COMDIR)/FNodeDefs.h \
		$(MDPDIR)/OpTabTypes.h $(MDPDIR)/Frame.c
		$(CC_CMD) $(MDPDIR)/Frame.c
IsADPrvate.o:	$(COMDIR)/defs.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h \
		$(MDPDIR)/IsADPrvate.c
		$(CC_CMD) $(MDPDIR)/IsADPrvate.c
IsLegal.o:	$(COMDIR)/defs.h $(COMDIR)/ANodeTypes.h $(MDPDIR)/olddefs.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/Target.h optab.h \
		$(MDPDIR)/IsLegal.c
		$(CC_CMD) $(MDPDIR)/IsLegal.c
OperndType.o:	$(INC)/string.h $(COMDIR)/defs.h $(COMDIR)/OperndType.h \
		$(COMDIR)/ANodeTypes.h \
		$(MDPDIR)/OperndType.c
		$(CC_CMD) $(MDPDIR)/OperndType.c
RegId.o:	$(INC)/string.h $(COMDIR)/defs.h $(MDPDIR)/Target.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/RegId.h \
		$(MDPDIR)/RegId.c
		$(CC_CMD) $(MDPDIR)/RegId.c
Target.o:	$(INC)/stdio.h $(INC)/string.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/defs.h \
		$(MDPDIR)/Target.h \
		$(MDPDIR)/Target.c
		$(CC_CMD) $(MDPDIR)/Target.c
UseSet.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/ANodeTypes.h \
		$(MDPDIR)/olddefs.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RegIdDefs.h $(MDPDIR)/RoundModes.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h \
		$(COMDIR)/FNodeTypes.h $(COMDIR)/FNodeDefs.h optab.h \
		$(MDPDIR)/OpTabTypes.h $(MDPDIR)/Target.h \
		$(COMDIR)/ALNodeType.h \
		$(MDPDIR)/UseSet.c
		$(CC_CMD) $(MDPDIR)/UseSet.c
TNOut.o:	$(INC)/ctype.h $(INC)/stdio.h $(COMDIR)/defs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(MDPDIR)/olddefs.h $(MDPDIR)/RoundModes.h \
		$(COMDIR)/OpTabDefs.h $(MDPDIR)/OpTabTypes.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h optab.h \
		$(COMDIR)/optim.h $(MDPDIR)/optutil.h \
		$(MDPDIR)/TNOut.c
		$(CC_CMD) $(MDPDIR)/TNOut.c
brnaq.o:	$(INC)/stdio.h $(INC)/malloc.h $(INC)/string.h $(COMDIR)/defs.h \
		$(COMDIR)/debug.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/TNodeTypes.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/TNodeDefs.h \
		$(COMDIR)/OpTabDefs.h optab.h \
		$(MDPDIR)/brnaq.c
		$(CC_CMD) $(MDPDIR)/brnaq.c
comutil.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/ANodeTypes.h \
		$(MDPDIR)/olddefs.h $(COMDIR)/TNodeTypes.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/TNodeDefs.h optab.h \
		$(MDPDIR)/comutil.c
		$(CC_CMD) $(MDPDIR)/comutil.c
conv.o:		$(INC)/stdio.h  $(INC)/malloc.h $(INC)/string.h \
		$(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(COMDIR)/LoopTypes.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/RoundModes.h \
		$(MDPDIR)/olddefs.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h optab.h $(COMDIR)/optim.h \
		$(COMDIR)/OpTabDefs.h $(MDPDIR)/OpTabTypes.h $(MDPDIR)/optutil.h \
		$(COMDIR)/ALNodeType.h \
		$(COMDIR)/FNodeDefs.h $(COMDIR)/FNodeTypes.h \
		$(MDPDIR)/conv.c
		$(CC_CMD) $(MDPDIR)/conv.c
cse.o:		$(INC)/stdio.h $(INC)/string.h $(COMDIR)/defs.h \
		$(COMDIR)/debug.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h \
		$(MDPDIR)/RegIdDefs.h $(COMDIR)/ANodeDefs.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/TNodeTypes.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/TNodeDefs.h \
		$(COMDIR)/FNodeDefs.h $(COMDIR)/FNodeTypes.h \
		$(COMDIR)/ALNodeType.h $(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/optutil.h optab.h \
		$(MDPDIR)/cse.c
		$(CC_CMD) $(MDPDIR)/cse.c
debug.o:	$(INC)/stdio.h $(INC)/string.h \
		$(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(COMDIR)/ANodeTypes.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/FNodeTypes.h \
		$(COMDIR)/TNodeDefs.h $(COMDIR)/FNodeDefs.h optab.h \
		$(COMDIR)/optim.h $(MDPDIR)/optutil.h \
		$(COMDIR)/ALNodeType.h $(MDPDIR)/OpTabTypes.h \
		$(MDPDIR)/debug.c
		$(CC_CMD) $(MDPDIR)/debug.c
funcdata.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(MDPDIR)/RegId.h \
		$(MDPDIR)/RegIdDefs.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(COMDIR)/LoopTypes.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/RoundModes.h \
		$(MDPDIR)/olddefs.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h $(COMDIR)/FNodeTypes.h \
		$(COMDIR)/ALNodeType.h \
		$(COMDIR)/FNodeDefs.h optab.h $(COMDIR)/optim.h \
		$(COMDIR)/OpTabDefs.h $(MDPDIR)/OpTabTypes.h $(MDPDIR)/optutil.h \
		$(MDPDIR)/funcdata.c
		$(CC_CMD) $(MDPDIR)/funcdata.c
gra.o:		$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(COMDIR)/ANodeTypes.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h $(MDPDIR)/RoundModes.h \
		$(COMDIR)/ANodeDefs.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h $(COMDIR)/FNodeTypes.h \
		$(COMDIR)/FNodeDefs.h optab.h $(COMDIR)/optim.h \
		$(COMDIR)/ALNodeType.h $(MDPDIR)/OpTabTypes.h \
		$(MDPDIR)/gra.c
		$(CC_CMD) $(MDPDIR)/gra.c
htab.o:		$(MDPDIR)/OpTabTypes.h optab.h htab.c
		$(CC_CMD) htab.c
in.o:		$(INC)/ctype.h $(INC)/stdio.h \
		$(INC)/memory.h $(INC)/string.h \
		$(COMDIR)/defs.h \
		$(COMDIR)/debug.h $(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h \
		$(COMDIR)/OperndType.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/optim.h $(MDPDIR)/optutil.h \
		$(COMDIR)/FNodeTypes.h $(COMDIR)/FNodeDefs.h \
		$(COMDIR)/ALNodeType.h $(MDPDIR)/OpTabTypes.h \
		$(MDPDIR)/in.c
		$(CC_CMD) $(MDPDIR)/in.c
inline.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(MDPDIR)/RegId.h \
		$(MDPDIR)/RegIdDefs.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(COMDIR)/LoopTypes.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/RoundModes.h \
		$(MDPDIR)/olddefs.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h $(COMDIR)/FNodeTypes.h \
		$(COMDIR)/FNodeDefs.h optab.h $(COMDIR)/optim.h \
		$(MDPDIR)/optutil.h $(COMDIR)/debug.h \
		$(COMDIR)/ALNodeDefs.h $(COMDIR)/ALNodeType.h \
		$(COMDIR)/OpTabDefs.h $(MDPDIR)/OpTabTypes.h \
		$(MDPDIR)/inline.c
		$(CC_CMD) $(MDPDIR)/inline.c
local.o:	$(INC)/ctype.h $(INC)/stdio.h $(INC)/malloc.h \
		$(INC)/memory.h $(INC)/string.h \
		$(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(MDPDIR)/CSections.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(COMDIR)/LoopTypes.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/FNodeTypes.h \
		$(COMDIR)/ALNodeDefs.h $(COMDIR)/ALNodeType.h \
		$(COMDIR)/FNodeDefs.h $(COMDIR)/TNodeDefs.h optab.h \
		$(COMDIR)/optim.h $(MDPDIR)/optutil.h \
		$(MDPDIR)/Target.h \
		$(COMINC)/storclass.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/local.c
		$(CC_CMD) $(MDPDIR)/local.c
local2.o:	$(INC)/stdio.h $(COMDIR)/defs.h \
		$(MACHINC)/sgs.h $(COMDIR)/debug.h \
		optab.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/TNodeTypes.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/TNodeDefs.h \
		$(COMDIR)/OpTabDefs.h $(COMDIR)/optim.h \
		$(COMDIR)/ALNodeType.h $(COMDIR)/FNodeTypes.h \
		$(MDPDIR)/RegIdDefs.h $(COMDIR)/FNodeDefs.h \
		$(MDPDIR)/local2.c
		$(CC_CMD) $(MDPDIR)/local2.c
lookup.o:	$(INC)/stdio.h $(INC)/string.h $(MDPDIR)/OpTabTypes.h \
		optab.h \
		$(MDPDIR)/lookup.c
		$(CC_CMD) $(MDPDIR)/lookup.c
loopopt.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/debug.h \
		optab.h $(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/TNodeTypes.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/TNodeDefs.h \
		$(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/loopopt.c
		$(CC_CMD) $(MDPDIR)/loopopt.c
optab.o:	$(COMDIR)/ANodeTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/OpTabTypes.h optab.h \
		optab.c
		$(CC_CMD) optab.c
optutil.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(MDPDIR)/RegId.h \
		$(MDPDIR)/RegIdDefs.h $(COMDIR)/ANodeTypes.h \
		$(COMDIR)/ANodeDefs.h $(COMDIR)/LoopTypes.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/RoundModes.h \
		$(MDPDIR)/olddefs.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h optab.h $(COMDIR)/optim.h \
		$(MDPDIR)/optutil.h $(MDPDIR)/Target.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/optutil.c
		$(CC_CMD) $(MDPDIR)/optutil.c
peep.o:		$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(MDPDIR)/RegId.h $(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RoundModes.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h optab.h \
		$(COMDIR)/optim.h $(MDPDIR)/optutil.h \
		$(MDPDIR)/OpTabTypes.h \
		$(MDPDIR)/peep.c
		$(CC_CMD) $(MDPDIR)/peep.c
sequence.o:	$(INC)/stdio.h $(COMDIR)/defs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RoundModes.h \
		$(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/ALNodeDefs.h $(COMDIR)/ALNodeType.h \
		$(COMDIR)/FNodeTypes.h $(COMDIR)/FNodeDefs.h \
		$(MDPDIR)/olddefs.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h \
		$(MDPDIR)/OpTabTypes.h \
		$(MDPDIR)/sequence.c
		$(CC_CMD) $(MDPDIR)/sequence.c
value.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RoundModes.h \
		$(COMDIR)/ANodeTypes.h optab.h $(COMDIR)/optim.h \
		$(COMDIR)/ANodeDefs.h $(MDPDIR)/olddefs.h \
		$(COMDIR)/TNodeTypes.h $(MDPDIR)/optutil.h \
		$(COMDIR)/TNodeDefs.h $(COMDIR)/FNodeTypes.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/FNodeDefs.h \
		$(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/value.c
		$(CC_CMD) $(MDPDIR)/value.c
w1opt.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h \
		$(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/olddefs.h $(MDPDIR)/RoundModes.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h optab.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/OpTabDefs.h \
		$(COMDIR)/optim.h $(MDPDIR)/optutil.h \
		$(MDPDIR)/w1opt.c
		$(CC_CMD) $(MDPDIR)/w1opt.c
w2opt.o:	$(INC)/stdio.h $(COMDIR)/defs.h $(COMDIR)/debug.h \
		$(MDPDIR)/RegId.h $(MDPDIR)/RegIdDefs.h \
		$(COMDIR)/ANodeTypes.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/LoopTypes.h $(COMDIR)/OperndType.h \
		$(MDPDIR)/olddefs.h $(MDPDIR)/RoundModes.h \
		$(COMDIR)/TNodeTypes.h $(COMDIR)/TNodeDefs.h \
		$(COMDIR)/ALNodeType.h \
		$(COMDIR)/FNodeTypes.h $(COMDIR)/FNodeDefs.h optab.h \
		$(COMDIR)/optim.h $(MDPDIR)/optutil.h $(MDPDIR)/Target.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/w2opt.c
		$(CC_CMD) $(MDPDIR)/w2opt.c
w3opt.o:	$(COMDIR)/defs.h $(COMDIR)/ANodeTypes.h $(MDPDIR)/olddefs.h \
		$(MDPDIR)/RegId.h $(COMDIR)/ANodeDefs.h \
		$(COMDIR)/OperndType.h $(COMDIR)/LoopTypes.h \
		$(MDPDIR)/RoundModes.h $(COMDIR)/TNodeTypes.h \
		$(COMDIR)/TNodeDefs.h optab.h $(MDPDIR)/optutil.h \
		$(MDPDIR)/OpTabTypes.h $(COMDIR)/OpTabDefs.h \
		$(MDPDIR)/Target.h \
		$(INC)/stdio.h $(MDPDIR)/w3opt.c
		$(CC_CMD) $(MDPDIR)/w3opt.c
#
optab.c optab.h:    $(TABLES) $(TABDIR)/tabgen $(OPCODES) defgen
		chmod +x $(TABDIR)/tabgen
		$(TABDIR)/tabgen $(TABDIR) $(OPSDIR)
		chmod 0640 optab.c optab.h
#
htab.c:		$(COMDIR)/ANodeTypes.h \
		$(COMDIR)/OperndType.h $(MDPDIR)/OpTabTypes.h optab.h \
		$(MDPDIR)/lookup.c $(TABDIR)/htabgen.c optab.c
		cp $(MDPDIR)/lookup.c hlookup.c
		cp optab.c hoptab.c
		cc -O -I$(COMDIR) -I$(MDPDIR) $(TABDIR)/htabgen.c hlookup.c hoptab.c   # host compiler required
		./a.out >htab.c
		\rm -f a.out htabgen.o hlookup.[co] hoptab.[co]
#
defgen:      	$(OPSDIR)/defgen.c
	    	cc -O $(OPSDIR)/defgen.c  # host compiler required
		mv a.out defgen
#
clean:		
		-rm -f $(OBJECTS)
clobber:	clean
		-rm -f is25.ops1 is25.ops2 is25.ops3 is25.ops4
		-rm -f defgen htab.c optab.c optab.h
		-rm -f $(PRODUCTS)
install:	all
		cp $(OPTIM) $(OPTIM).bak
		$(STRIP) $(OPTIM)
		/bin/sh $(INS) 0755 $(OWN) $(GRP) $(INSDIR)/$(SGS)$(OPTIM) $(OPTIM)
		mv $(OPTIM).bak $(OPTIM)
lintit:		$(SOURCES)
		$(LINT_CMD) $(SOURCES) >$(LINT).out
print:		
		$(MDPDIR)/CLEAN
		mv optab.c optab
		cb -l 120 optab > optab.c
		$(PRINT_CMD) $(HEADERS) $(SOURCES)
		mv optab optab.c
