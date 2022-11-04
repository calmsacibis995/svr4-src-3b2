.ident	"@(#)libc-m32:sys/adjtime.s	1.1"

# OS library -- adjtime

# error = adjtime(tv, otv)

        .set    __adjtime,138*8

        .globl  _cerror

_fwdef_(`adjtime'):
	MCOUNT
        MOVW    &4,%r0
        MOVW    &__adjtime,%r1
        GATE
        jgeu    .noerror
        jmp     _cerror
.noerror:
        CLRW    %r0
        RET
