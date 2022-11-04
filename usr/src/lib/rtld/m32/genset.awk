#ident	"@(#)rtld:m32/genset.awk	1.8"

# Create rtsetaddr.s or rtabiaddr.s.
# _rt_setaddr() looks up the values of several special symbols that may be
# found in either the user's a.out or ld.so.  If the special symbol is in the
# a.out, set the address in the GOT, else do nothing.  This is in assembler
# because we have to access the GOT directly.
# The list of symbols is in genset.in.  All symbols that must appear in
# rtabiaddr.s have "MIN" as $2.
# If $2 is MIN and $3 is non-null, in the MIN case 
# we look up $1, but actually
# set $1$3 (names concatenated).
# If $2 is PRE, again we look up $1 but we set *$1$3@GOT, and the
# symbol is in both libraries 

BEGIN	{
	error = 0
	mode = 0	# 1: full libc, 2: min abi
	count = 0
}

$1 ~ /^@.*/	{
	# process an @ directive
	# @FULL: full libc
	# @MIN: minimal abi libc
	if (mode != 0) {
		printf "genset.awk: too many @ directives\n" | "cat -u >&2"
		error = 1
		exit
	}

	if ($1 == "@FULL") {
		printf "\t.file\t\"rtsetaddr.s\"\n"
		mode = 1
	} else if ($1 == "@MIN") {
		printf "\t.file\t\"rtabiaddr.s\"\n"
		mode = 2
	} else {
		printf "genset.awk: illegal mode " \
			"(@MIN: min abi, @FULL: full libc)\n" | \
			"cat -u >&2"
		error = 1
		exit
	}

	printf "\n\t.globl\t_rt_setaddr\n"
	printf "\t.text\n\t.type\t_rt_setaddr,@function\n"
	printf "_rt_setaddr:\n\tSAVE\t%%fp\n\tADDW2\t&12,%%sp\n"
	next
}

$0 ~ /^#.*/	{
	# skip comments
	next
}

NF == 0	{
	# skip blank lines
	next
}

{
	if (mode == 0) {
		printf "genset.awk: @ directive not set\n" | "cat -u >&2"
		error = 1
		exit
	}
	if (mode == 2 && ($2 != "MIN" && $2 != "PRE"))
		next
	# put out a .globl and a .string for the symbol
	if (mode == 2 && $2 == "MIN" && $3 != "") {
		printf "\t.globl\t%s%s\n", $3, $1
	}
	else if ($2 == "PRE") {
		printf "\t.globl\t%s%s\n", $3, $1
		printf "\t.globl\t%s\n", $1
	}
	else {
		printf "\t.globl\t%s\n", $1
	}
	printf "\t.section\t.rodata\n\t.align\t4\n"
	printf ".X%.3d:\t# %s\n\t.string\t\"%s\"\n", count, $1, $1

	# put out the code to look up the symbol and fix up the GOT.
	printf "\t.text\n"
	printf "# sym = _lookup(\"%s\", LO_ALL, _ld_loaded, &lm);\n", $1
	printf ".SYM%.3d:\n", count
	printf "\tPUSHAW\t.X%.3d@PC\n\tPUSHW\t&0\n\tPUSHW\t*_ld_loaded@GOT\n", \
		count
	printf "\tPUSHAW\t4(%%fp)\n"
	printf "\tcall\t&4, _lookup@PLT\n"
	printf "# if (sym)\n"
	if (mode == 2 && $2 == "MIN" && $3 != "") 
       		printf "#    %s%s@GOT = sym->st_value + (NAME(lm) ? ADDR(lm) : 0 );\n", $3, $1
	else if ($2 == "PRE") 
       		printf "#   *%s%s@GOT = sym->st_value + (NAME(lm) ? ADDR(lm) : 0 );\n", $3, $1
	else
       		printf "#    %s@GOT = sym->st_value + (NAME(lm) ? ADDR(lm) : 0 );\n", $1
	printf "\tMOVW\t%%r0,%%r1\n\tje\t.SYM%.3d\n", count + 1
	printf "\tMOVW\t4(%%fp),%%r0\n\tTSTW\t4(%%r0)\n\tje\t.J1%.3d\n", count
	printf "\tMOVW\t*4(%%fp),%%r0\n\tjmp\t.J2%.3d\n", count
	printf ".J1%.3d:\n\tCLRW\t%%r0\n.J2%.3d:\n\tADDW2\t4(%%r1),%%r0\n", \
		count, count
	if (mode == 2 && $2 == "MIN" && $3 != "") 
		printf "\tMOVW\t%%r0,%s%s@GOT\n", $3, $1
	else if ($2 == "PRE") 
		printf "\tMOVW\t%%r0,*%s%s@GOT\n", $3, $1
	else 
		printf "\tMOVW\t%%r0,%s@GOT\n", $1
	count++
}

END	{
	if (error == 1)
		exit

	printf ".SYM%.3d:\n", count

	if (mode == 1) {	# only for full libc
		# initialize the __first_list in stdio, this has to be doen
		# after fixing up the GOT entries, so that it has the address
		# of __iob from either the user's code, or the library,
		# as appropriate

		printf "# Initialize some values ....\n"
		printf "# Set up __first_link used in port/stdio/flush.c\n"
		printf "# struct Link __first_link = { &_iob[0], ... }\n"
		printf "\tMOVW\t__first_link@GOT, %%r0\n"
		printf "\tMOVW\t__iob@GOT, 0(%%r0)\n"
	}

	printf "\tRESTORE\t%%fp\n\tRET\n\t.size\t_rt_setaddr,.-_rt_setaddr\n"
}
