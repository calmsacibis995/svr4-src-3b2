#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

.ident	"@(#)kernel:ml/string.s	1.1"

# String functions copied from the C library.
#
# Fast assembler language version of the following C-program for
#			strcmp
# which represents the "standard" for the C-library.

#	/*
#	 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
#	 */
#
#	int
#	strcmp(s1, s2)
#	register char *s1, *s2;
#	{
#
#		if(s1 == s2)
#			return(0);
#		while(*s1 == *s2++)
#			if(*s1++ == '\0')
#				return(0);
#		return(*s1 - *--s2);
#	}

	.globl	strcmp
	.text
	.align	1
strcmp:
	save	&0			# only use 0 and 1
	movw	0(%ap),%r0		# get s1
	movw	4(%ap),%r1		# get s2
	cmpw	%r0,%r1
	jne	L1			# s1 == s2 implies strings equal
	jmp	L2			# optimize number of branches
L0:
	addw2	&1,%r0			# by incrementing at top
	addw2	&1,%r1
L1:
	cmpb	0(%r0),0(%r1)
	jne	L2			# done: *s1 != *s2
	cmpb	&0,0(%r0)
	jne	L0			# not done yet: (*s1==*s2&&*s1!='\0')
L2:
	subb3	0(%r1),0(%r0),%r0	# return (*s2 - *s1)
	ret	&0


# Fast assembler language version of the following C-program
#			strlen
# which represents the "standard" for the C-library.
#
# Given string s, return length (not including the terminating null).

#	strlen(s)
#	register char	*s;
#	{
#		register n;
#	
#		n = 0;
#		while (*s++)
#			n++;
#		return(n);
#	}

	.globl	strlen
	.text
	.align	1
strlen:
	save	&0		# pgm uses scratch reg 0
	movw	0(%ap),%r0	# ptr to s1
	jmp	M1
M0:
	addw2	&1,%r0
M1:
	cmpb	0(%r0),&0	# search for a terminating null
	jne	M0		# go back for more
	subw2	0(%ap),%r0	# calculate string length in r0
	ret	&0


# Fast assembler language version of the following C-program
#			strcpy
# which represents the "standard" for the C-library.
#
# Copy string s2 to s1.  s1 must be large enough. Return s1.
#
#	char	*
#	strcpy(s1, s2)
#	register char	*s1, *s2;
#	{
#		register char	*os1;
#	
#		os1 = s1;
#		while (*s1++ = *s2++)
#			;
#		return(os1);
#	}

	.globl	strcpy
	.text
	.align 4
strcpy:
	save	&0
	movw 	0(%ap),%r1	# s1 (destination string)
	movw	4(%ap),%r0	# s2 (source string)
	STRCPY			# BELLMAC 32 A string copy instruction
	movw	0(%ap),%r0	# return s1
	ret	&0
