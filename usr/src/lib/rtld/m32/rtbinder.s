	.ident	"@(#)rtld:m32/rtbinder.s	1.4"
	.file	"rtbinder.s"

# we got here because a call to a function resolved to
# a procedure linkage table entry - that entry did a CALL
# to the first PLT entry, which in turn did a JMP to _rtbinder
#
# the code sequence that got us here was
# 
# PLT entry for foo:
#	JMP	*got_off(%pc)	# which puts us at the next instruction:
#	PUSHW	&relocation offset	
#	JMP	-n(%pc) (jump to first PLT entry)
#
# 1st PLT entry:
#	PUSHW	got+1(%pc)	# the address of the link map
#	JMP	*got+2(%pc)	# address of rtbinder
#
#
# the stack at this point looks like:
# 	PC of return from call to foo
# 	AP of previous call
#	offset of relocation entry
#	addr of link_map entry for this reloc
# %sp->
#

	.text
	.globl	_binder
	.globl	_rtbinder
	.type	_rtbinder,@function
	.align	4
_rtbinder:
	SAVE	%r8
	MOVW	%r2,%r8		# save %r2 since it may be used
				# for structure returns
	MOVW	-36(%sp),%r0	# relocation offset
	MOVW	-32(%sp),%r1	# addr of link_map entry
	PUSHW	%r0
	PUSHW	%r1
	call	&2,_binder@PLT	# transfer control to rtld
				# rtld returns addr of 
				# function definition
	MOVW	%r8,%r2
	RESTORE	%r8		# restore old stack frame
	SUBW2	&8,%sp		# pop off lm addr and reloc offset
	JMP	0(%r0)		# transfer to function destination
	.size	_rtbinder,.-_rtbinder
