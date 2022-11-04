/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:common/rtfcns.c	1.4"

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "rtinc.h"
#include <fcntl.h>

#define ERRSIZE 512	/* size of buffer for error messages */

/* utility routines for run-time linker
 * some are duplicated here from libc (with different names)
 * to avoid name space collissions
 */

/* null function used as place where a debugger can set a breakpoint */
void _r_debug_state()
{
	return;
}

/* function called by atexit(3C) - goes through link_map
 * and invokes each shared object's _fini function (skips a.out)
 */
void _rt_do_exit()
{
	register struct rt_private_map *lm;
	void (*fptr)();

	lm = (struct rt_private_map *)NEXT(_ld_loaded); /* skip a.out */

	while (lm) {
		fptr = FINI(lm);
		if (fptr)
			(*fptr)();
		lm = (struct rt_private_map *)NEXT(lm);
	}
}


/* internal version of strlen */

int _rtstrlen(s)
register CONST char *s;
{
	register CONST char *s0 = s + 1;

	while (*s++ != '\0')
		;
	return (s - s0);
}
	
/* Local heap allocator.  Very simple, does not support storage freeing. */
VOID *_rtmalloc(nb)
unsigned int nb;
{
	static caddr_t cp = 0;
	static caddr_t sp = 0;
	caddr_t tp;

	DPRINTF(LIST, (2, "rtld: _rtmalloc(0x%x)\n", nb));
	/* we always round to double-word boundary */
	if ((cp + DROUND(nb)) >= sp) {
		/* map in at least a page of anonymous memory */
		/* open /dev/zero if not open, for reading anonymous memory */
		if (_devzero_fd == -1) {
			if ((_devzero_fd = _open(DEV_ZERO, O_RDONLY)) == -1) {
				_rt_lasterr("ld.so: %s: can't open %s",_proc_name,DEV_ZERO);
				return 0 ;
			}
		}
		if ((sp = _mmap(0, PROUND(nb), (PROT_READ|PROT_WRITE),
		    MAP_PRIVATE, _devzero_fd, 0)) == (caddr_t)-1) {
			_rt_lasterr("ld.so: %s: can't malloc space",_proc_name);
			return 0;
		}
		cp = sp;
		sp += PROUND(nb);
	}
	tp = cp;
	cp += DROUND(nb);
	return(tp);
}

/* internal version of strncmp */
static int _rtstrncmp(s1, s2, n)
register CONST char *s1, *s2;
register int n;
{
	n++;
	if (s1 == s2)
		return(0);
	while (--n != 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return((n == 0) ? 0 : *s1 - *(s2-1));
}

/* internal getenv routine - only a few strings are relevant */
CONST char *
_readenv(envp, bmode)
CONST char **envp;
int *bmode;
{
	register CONST char *s1;
	int i;
	CONST char *envdirs = 0;

	if (envp == (CONST char **)0)
		return((char *)0);
	while (*envp != (CONST char *)0) {
		s1 = *envp++;
		if (*s1++ != 'L' || *s1++ != 'D' || *s1++ != '_' )
			continue;

#ifdef DEBUG
		if (_rtstrncmp( s1, "DEBUG=", 6 ) == 0) {
			i = (int)(s1[6] - '0');
			if (i > 0 && i <= MAXLEVEL)
				_debugflag = i;
		}
#endif

		i = sizeof("TRACE_LOADED_OBJECTS") - 1;
		if (_rtstrncmp( s1, "TRACE_LOADED_OBJECTS", i) == 0) {
			s1 += i;
			if ( *s1 == '\0' || *s1 == '=' )
				_rt_tracing = 1;
		}

		if (_rtstrncmp( s1, "WARN", 4) == 0) {
			s1 += 4;
			if ( *s1 == '\0' || *s1 == '=' )
				_rt_warn = 1;
		}

		i = sizeof("LIBRARY_PATH") - 1;
		if (_rtstrncmp( s1, "LIBRARY_PATH", i ) == 0) {
			s1 += i;
			if ( *s1 == '\0' || *s1 == '=' )
				envdirs = ++s1;
		}

		i = sizeof("BIND_NOW") - 1;
		if (_rtstrncmp( s1, "BIND_NOW", i ) == 0) {
			s1 += i;
			if ( *s1 == '=' || *s1 == '\0' )
				*bmode = RTLD_NOW;
		}
	}

	/* LD_WARN is meaningful only if tracing */
	if (!_rt_tracing)
		_rt_warn = 0;

	return envdirs;
}


/* internal version of strcmp */
int _rtstrcmp(s1, s2)
register CONST char *s1, *s2;
{

	if (s1 == s2)
		return(0);
	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return(*s1 - *(s2-1));
}

/* internal version of strcpy */
char *_rtstrcpy(s1, s2)
register char *s1;
register CONST char *s2;
{
	char *os1 = s1;

	while (*s1++ = *s2++)
		;
	return(os1);
}

/* _rtstr3cpy - concatenate 3 strings */
char *_rtstr3cpy(s1, s2, s3, s4)
register char *s1;
register CONST char *s2, *s3, *s4;
{
	char *os1 = s1;

	while (*s1++ = *s2++)
		;
	--s1;
	while (*s1++ = *s3++)
		;
	--s1;
	while (*s1++ = *s4++)
		;
	return(os1);
}


/*  Local "fprintf"  facilities.  */

static char *printn ARGS((int, unsigned long, int, char *, register char *, char *));
static void doprf ARGS((int , CONST char *, va_list , char *));
static void _rtwrite ARGS((int fd, char *buf, int len));


/*VARARGS2*/
#ifdef __STDC__
void _rtfprintf(int fd, CONST char *fmt, ...)
#else
void _rtfprintf(fd, fmt, va_alist)
int fd;
char *fmt;
va_dcl
#endif
{
	va_list adx;
	char linebuf[ERRSIZE];


#ifdef __STDC__
	va_start(adx, fmt);
#else
	va_start(adx);
#endif

	doprf(fd, fmt, adx, linebuf);
	va_end(adx);
}

/* error recording function - we write the error string to
 * a static buffer and set a global pointer to point to the string
 */

/*VARARGS1*/
#ifdef __STDC__
void _rt_lasterr(CONST char *fmt, ...)
#else
void _rt_lasterr(fmt, va_alist)
char *fmt;
va_dcl
#endif
{
	va_list adx;
	static char *errptr = 0;
	static CONST char *no_space = "ld.so: can't allocate space";

#ifdef __STDC__
	va_start(adx, fmt);
#else
	va_start(adx);
#endif

	if (!errptr)
		if ((errptr = _rtmalloc(ERRSIZE)) == 0) {
			_rt_error = (char *)no_space;
			return;
		}

	doprf(-1, fmt, adx, errptr);
	va_end(adx);
	_rt_error = errptr;
}

static void doprf(fd, fmt, adx, linebuf)
int fd;
CONST char *fmt;
char *linebuf;
va_list adx;
{
	register char c;		
	register char *lbp, *s;
	int i, b, num;	

#define	PUTCHAR(c)	{\
			if (lbp >= &linebuf[ERRSIZE]) {\
				_rtwrite(fd, linebuf, lbp - linebuf);\
				lbp = linebuf;\
			}\
			*lbp++ = (c);\
			}

	lbp = linebuf;
	while ((c = *fmt++) != '\0') {
		if (c != '%') {
			if (c == '\n')
				PUTCHAR('\r');
			PUTCHAR(c);
		}
		else {
			c = *fmt++;
			num = 0;
			switch (c) {
		
			case 'x': 
			case 'X':
				b = 16;
				num = 1;
				break;
			case 'd': 
			case 'D':
			case 'u':
				b = 10;
				num = 1;
				break;
			case 'o': 
			case 'O':
				b = 8;
				num = 1;
				break;
			case 'c':
				b = va_arg(adx, int);
				for (i = 24; i >= 0; i -= 8)
					if ((c = ((b >> i) & 0x7f)) != 0) {
						if (c == '\n')
							PUTCHAR('\r');
						PUTCHAR(c);
					}
				break;
			case 's':
				s = va_arg(adx, char*);
				while ((c = *s++) != 0) {
					if (c == '\n')
						PUTCHAR('\r');
					PUTCHAR(c);
				}
				break;
			case '%':
				PUTCHAR('%');
				break;
			}
			if (num) {
				lbp = printn(fd, va_arg(adx, unsigned long), b,
					linebuf, lbp, &linebuf[ERRSIZE]);
			}
			
		}
	}
	(void)_write(fd, linebuf, lbp - linebuf);
}

/* Printn prints a number n in base b. */
static char *printn(fd, n, b, linebufp, lbp, linebufend)
int fd, b;
unsigned long n;
char *linebufp, *linebufend;
register char *lbp;
{
	char prbuf[11];			/* Local result accumulator */
	register char *cp;
	CONST char *nstring = "0123456789abcdef";

#undef PUTCHAR
#define	PUTCHAR(c)	{\
			if (lbp >= linebufend) {\
				_rtwrite(fd, linebufp, lbp - linebufp);\
				lbp = linebufp;\
			}\
			*lbp++ = (char)(c);\
			}

	if (b == 10 && (int)n < 0) {
		PUTCHAR('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = nstring[n % b];
		n /= b;
	} while (n);
	do {
		PUTCHAR(*--cp);
	} while (cp > prbuf);
	return (lbp);
}

static void _rtwrite(fd, buf, len)
int fd, len;
char *buf;
{
	if (fd == -1) {
		*(buf + len) = '\0';
		return;
	}
	(void)_write(fd, buf, len);
}
