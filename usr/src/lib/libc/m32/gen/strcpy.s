	.file	"strcpy.s"
.ident	"@(#)libc-m32:gen/strcpy.s	1.5"
# Fast assembler language version of the following C-program
#			strcpy
# which represents the `standard' for the C-library.

# Copy string s2 to s1.  s1 must be large enough. Return s1.

#	char *
#	strcpy(s1, s2)
#	register char *s1, *s2;
#	{
#		register char *os1;
#	
#		os1 = s1;
#		while (*s1++ = *s2++)
#			;
#		return(os1);
#	}
#
#
	.globl	strcpy
	.text
	.align 4
_fgdef_(strcpy):
	save	&0
	MCOUNT
	movw 	0(%ap),%r1	# s1 (destination string)
	movw	4(%ap),%r0	# s2 (source string)
	STRCPY			# BELLMAC-32A string copy instruction
	movw	0(%ap),%r0	# return s1
	ret	&0
