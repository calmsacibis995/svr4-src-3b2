/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/time_comm.c	1.20"
/*
 * Functions that are common to ctime(3C) and cftime(3C)
 */
#ifndef ABI
#ifdef __STDC__
	#pragma weak tzset = _tzset
#endif
#endif
#include	"synonyms.h"
#include	"shlib.h"
#include	<ctype.h>
#include  	<stdio.h>
#include  	<limits.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include 	<time.h>
#include 	<stdlib.h>
#include 	<string.h>
#include 	<tzfile.h>

static char	*getdigit();
static char	*gettime();
static int	getdst();
static void	getusa();
static char	*getzname();
static int	sunday();
static long 	detzcode();
static int	_tzload();

extern void	_ltzset();

#define	SEC_PER_MIN	60
#define	SEC_PER_HOUR	(60*60)
#define	SEC_PER_DAY	(24*60*60)
#define	SEC_PER_YEAR	(365*24*60*60)
#define	LEAP_TO_70	(70/4)

#define MAXTZNAME	3
#define	year_size(A)	(((A) % 4) ? 365 : 366)

static time_t 	start_dst ;    /* Start date of alternate time zone */
static time_t  	end_dst;      /* End date of alternate time zone */
extern const short	__month_size[];

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif /* !TRUE */

static struct tm *	offtime();

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
};

struct state {
	int		timecnt;
	int		typecnt;
	int		charcnt;
	time_t		*ats;
	char	*types;
	struct ttinfo	*ttis;
	char		*chars;
};

static struct state	*_tz_state;

static int		_tz_is_set = FALSE;

struct tm *
localtime(timep)
const time_t *timep;
{
	register struct tm *		tmp;
	register int			i;
	time_t				t;
	register struct state 		*s;
	long				daybegin, dayend;
	time_t				curr;


	if (!_tz_is_set)
		_ltzset(*timep);
	s = _tz_state;

	t = *timep - timezone;
	tmp = gmtime(&t);
	if (!daylight)
		return(tmp);

	if (s != 0 && start_dst)
		/* Olson method */
		curr = t;
	else
		/* this was POSIX time */
		curr = tmp->tm_yday*SEC_PER_DAY + tmp->tm_hour*SEC_PER_HOUR +
			tmp->tm_min*SEC_PER_MIN + tmp->tm_sec;

	if ( start_dst == 0 && end_dst == 0)
		getusa(&daybegin, &dayend, tmp);
	else
	{
		daybegin = start_dst;
		dayend  = end_dst;
	}
	if (curr >= daybegin && curr < dayend)
	{
		t = *timep - altzone;
		tmp = gmtime(&t);
		tmp->tm_isdst = 1;
	}
	return(tmp);
}

struct tm *
gmtime(clock)
const time_t *clock;
{
	register struct tm *	tmp;

	tmp = offtime(clock, 0L);
	tzname[0] = "GMT";
	return tmp;
}

extern const int	__mon_lengths[2][MONS_PER_YEAR];

extern const int	__year_lengths[2];

static struct tm *
offtime(clock, offset)
time_t *	clock;
long		offset;
{
	register struct tm *	tmp;
	register long		days;
	register long		rem;
	register int		y;
	register int		yleap;
	register const int *		ip;
	static struct tm	tm;

	tmp = &tm;
	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	rem += offset;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = (int) (rem / SECS_PER_MIN);
	tmp->tm_sec = (int) (rem % SECS_PER_MIN);
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYS_PER_WEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for ( ; ; ) {
			yleap = isleap(y);
			if (days < (long) __year_lengths[yleap])
				break;
			++y;
			days = days - (long) __year_lengths[yleap];
		}
	else do {
		--y;
		yleap = isleap(y);
		days = days + (long) __year_lengths[yleap];
	} while (days < 0);
	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;
	ip = __mon_lengths[yleap];
	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days = days - (long) ip[tmp->tm_mon];
	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
	return tmp;
}

double
difftime(time1, time0)
time_t time1;
time_t time0;
{
	return((time1 - time0));
}

time_t mktime(timeptr)
struct tm *timeptr;
{
	extern const int __yday_to_month[];
	extern const int __lyday_to_month[];
	struct tm	*tptr;
	long	secs;
	int	temp;
	int	isdst = 0;
	long	daybegin, dayend;

	secs = timeptr->tm_sec + SEC_PER_MIN * timeptr->tm_min +
		SEC_PER_HOUR * timeptr->tm_hour +
		SEC_PER_DAY * (timeptr->tm_mday - 1);

	if (timeptr->tm_mon >= 12) {
		timeptr->tm_year += timeptr->tm_mon / 12;
		timeptr->tm_mon %= 12;
	} else if (timeptr->tm_mon < 0) {
		temp = -timeptr->tm_mon;
		timeptr->tm_year -= (1 + temp / 12);
		timeptr->tm_mon = 12 - temp % 12;
	}

	secs += SEC_PER_YEAR * (timeptr->tm_year - 70) +
		 SEC_PER_DAY * ((timeptr->tm_year + 3)/4 - 1 - LEAP_TO_70);
		
	if (timeptr->tm_year % 4 == 0)
		secs += SEC_PER_DAY * __lyday_to_month[timeptr->tm_mon];
	else
		secs += SEC_PER_DAY * __yday_to_month[timeptr->tm_mon];

	_ltzset(secs);
	if (timeptr->tm_isdst > 0) {
		secs += altzone;
	} else {
		if (timeptr->tm_isdst < 0)
			isdst = -1;
		secs += timezone;
	}
	tptr = localtime((time_t *)&secs);

	if (isdst == -1) {
		if (start_dst == 0 && end_dst == 0)
			getusa(&daybegin, &dayend, tptr);
		else {
			daybegin = start_dst;
			dayend = end_dst;
		}
		temp = tptr->tm_sec + SEC_PER_MIN * tptr->tm_min +
			SEC_PER_HOUR * tptr->tm_hour + SEC_PER_DAY * tptr->tm_yday;
		if (temp >= daybegin && temp < dayend) {
			secs -= (timezone - altzone);
			tptr = localtime((time_t *)&secs);
		}
	}

	*timeptr = *tptr;
	if (secs < 0)
		return(-1);
	else
		return(secs);
}

void
_ltzset(tim)
time_t tim;
{
	register char *	name;
	int i;

	_tz_is_set = TRUE;
	if((name = getenv("TZ")) == 0 || *name == '\0')
		return; /* TZ is not present, use GMT */
	if (name[0] == ':') /* Olson method */ {
		name++;
		if (_tzload(name) != 0) {
			_tz_state = 0;
			return;
		}
		else {
			/*
			** Set tzname elements to initial values.
			*/
			if (tim == 0)
				tim = time(NULL);
			tzname[0] = tzname[1] = &_tz_state->chars[0];
			timezone = -_tz_state->ttis[0].tt_gmtoff;
			daylight = 0;
			start_dst = end_dst = 0;
			for (i=0;  i < _tz_state->timecnt && _tz_state->ats[i] < tim; i++) {
				register struct ttinfo *	ttisp;

				ttisp = &_tz_state->ttis[_tz_state->types[i]];
				if (ttisp->tt_isdst) {
					tzname[1] = &_tz_state->chars[ttisp->tt_abbrind];
					daylight = 1;
					altzone = -ttisp->tt_gmtoff;
					start_dst = _tz_state->ats[i];
					if (i+1 < _tz_state->timecnt)
						end_dst = _tz_state->ats[i+1];
					else
						end_dst = INT_MAX;
				} else {
					tzname[0] = &_tz_state->chars[ttisp->tt_abbrind];
					timezone = -ttisp->tt_gmtoff;
				}
			}
		}
	}
	else /* POSIX method */ {
		/* Get main time zone name and difference from GMT */
		if ( ((name = getzname(name,tzname[0])) == 0) || 
			((name = gettime(name,&timezone,1)) == 0)) {
				/* No offset from GMT given, so use GMT */
				tzname[0] = "GMT";
				return;
		}
		altzone = timezone - SEC_PER_HOUR;
		start_dst = end_dst = 0;
		daylight = 0;


		/* Get alternate time zone name */
		if ( (name = getzname(name,tzname[1])) == 0)
			return;

		daylight = 1;

		/* If the difference between alternate time zone and
		 * GMT is not given, use one hour as default.
		 */
		if (*name == '\0') 
			return; 
		if (*name != ';' && *name != ',')
		if ( (name = gettime(name,&altzone,1)) == 0 || 
			(*name != ';' && *name != ','))
				return;
		getdst(name + 1,&start_dst, &end_dst);
	}
}

void
tzset()
{
	_ltzset(0);
}

static char *
getzname(p, tz)
char *p;
char *tz;
{
	int n = MAXTZNAME;

	if (!isalpha(*p))
		return(0);
	do
	{
		*tz++ = *p ;
	} while (--n > 0 && isalpha(*++p) );
	while(isalpha(*p))
		p++;
	while(--n >= 0)
		*tz++ = ' ';		/* Pad with blanks */
	return(p);	
}

static char *
gettime(p, timez, f)
char *p;
time_t *timez;
int f;
{
	register time_t t = 0;
	int d, sign = 0;

	d = 0;
	if (f)
		if ( (sign = (*p == '-')) || (*p == '+'))
			p++;
	if ( (p = getdigit(p,&d)) != 0)
	{
		t = d * SEC_PER_HOUR;
		if (*p == ':')
		{
			if( (p = getdigit(p+1,&d)) != 0)
			{
				t += d * SEC_PER_MIN;
				if (*p == ':')
				{
					if( (p = getdigit(p+1,&d)) != 0)
						t += d;
				}
			}
		}
	}
	if(sign)
		*timez = -t;
	else
		*timez = t;
	return(p);
}

static char *
getdigit(ptr, d)
char *ptr;
int *d;
{


	if (!isdigit(*ptr))
		return(0);
 	*d = 0;
	do
	{
		*d *= 10;
		*d += *ptr - '0';
	}while( (isdigit(*++ptr)));
	return(ptr);
}

static int
getdst(p, s, e)
char *p;
time_t *s;
time_t *e;
{
	int lsd,led;
	time_t st,et;
	st = et = 0; 		/* Default for start and end time is 00:00:00 */
	if ( (p = getdigit(p,&lsd)) == 0 )
		return(0);
	lsd -= 1; 	/* keep julian count in sync with date  1-366 */
	if ( (*p == '/') &&  ((p = gettime(p+1,&st,0)) == 0) )
			return(0);
	if (*p == ',')
	{
		if ( (p = getdigit(p+1,&led)) == 0 )
			return(0);
		led -= 1; 	/* keep julian count in sync with date  1-366 */
		if ((*p == '/') &&  ((p = gettime(p+1,&et,0)) == 0) )
				return(0);
	}
	/* Convert the time into seconds */
	*s = (long)(lsd * SEC_PER_DAY + st);
	*e = (long)(led * SEC_PER_DAY + et - (timezone - altzone));
	return(1);
}

static void
getusa(s, e, t)
int *s;
int *e;
struct tm *t;
{
	extern const struct {
		int	yrbgn;
		int	daylb;
		int	dayle;
	} __daytab[];
	int i = 0;

	while (t->tm_year < __daytab[i].yrbgn) /* can't be less than 70	*/
		i++;
	*s = __daytab[i].daylb; /* fall through the loop when in correct interval	*/
	*e = __daytab[i].dayle;

	*s = sunday(t, *s);
	*e = sunday(t, *e);
	*s = (long)(*s * SEC_PER_DAY + 2 * SEC_PER_HOUR);
	*e = (long)(*e * SEC_PER_DAY + SEC_PER_HOUR);
	return;
}

static int
sunday(t, d)
register struct tm *t;
register int d;
{
	if(d >= 58)
		d += year_size(t->tm_year) - 365;
	return(d - (d - t->tm_yday + t->tm_wday + 700) % 7);
}

static int
_tzload(name)
register char *	name;
{
	register int	i;
	register int	fid;
	register struct state *s = _tz_state;

	if (s == 0) {
		s = (struct state *)calloc(1, sizeof (*s));
		if (s == 0)
			return -1;
		_tz_state = s;
	}
	if (name == 0 && (name = TZDEFAULT) == 0)
		return -1;
	{
		register char *	p;
		register int	doaccess;
		char		fullname[MAXPATHLEN];

		doaccess = name[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == 0)
				return -1;
			if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
				return -1;
			(void) strcpy(fullname, p);
			(void) strcat(fullname, "/");
			(void) strcat(fullname, name);
			/*
			** Set doaccess if '.' (as in "../") shows up in name.
			*/
			while (*name != '\0')
				if (*name++ == '.')
					doaccess = TRUE;
			name = fullname;
		}
		if (doaccess && access(name, 4) != 0)
			return -1;
		if ((fid = open(name, 0)) == -1)
			return -1;
	}
	{
		register char *			p;
		register struct tzhead *	tzhp;
		char				buf[8192];

		i = read(fid, buf, sizeof buf);
		if (close(fid) != 0 || i < sizeof *tzhp)
			return -1;
		tzhp = (struct tzhead *) buf;
		s->timecnt = (int) detzcode(tzhp->tzh_timecnt);
		s->typecnt = (int) detzcode(tzhp->tzh_typecnt);
		s->charcnt = (int) detzcode(tzhp->tzh_charcnt);
		if (s->timecnt > TZ_MAX_TIMES ||
			s->typecnt == 0 ||
			s->typecnt > TZ_MAX_TYPES ||
			s->charcnt > TZ_MAX_CHARS)
				return -1;
		if (i < sizeof *tzhp +
			s->timecnt * (4 + sizeof (char)) +
			s->typecnt * (4 + 2 * sizeof (char)) +
			s->charcnt * sizeof (char))
				return -1;
		if (s->ats)
			free(s->ats);
		if (s->types)
			free(s->types);
		if (s->ttis)
			free(s->ttis);
		if (s->timecnt == 0) {
			s->ats = 0;
			s->types = 0;
		} else {
			s->ats =
			  (time_t *)calloc(s->timecnt, sizeof (time_t));
			if (s->ats == 0)
				return -1;
			s->types =
			  (char *)calloc(s->timecnt,
			  sizeof (char));
			if (s->types == 0) {
				free(s->ats);
				return -1;
			}
		}
		s->ttis =
		  (struct ttinfo *)calloc(s->typecnt, sizeof (struct ttinfo));
		if (s->ttis == 0) {
			if (s->ats)
				free(s->ats);
			if (s->types)
				free(s->types);
			return -1;
		}
		s->chars =
		  (char *)calloc(s->charcnt+1, sizeof (char));
		if (s->chars == 0) {
			if (s->ats)
				free(s->ats);
			if (s->types)
				free(s->types);
			free(s->ttis);
			return -1;
		}
		p = buf + sizeof *tzhp;
		for (i = 0; i < s->timecnt; ++i) {
			s->ats[i] = detzcode(p);
			p += 4;
		}
		for (i = 0; i < s->timecnt; ++i)
			s->types[i] = (unsigned char) *p++;
		for (i = 0; i < s->typecnt; ++i) {
			register struct ttinfo *	ttisp;

			ttisp = &s->ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			ttisp->tt_abbrind = (unsigned char) *p++;
		}
		for (i = 0; i < s->charcnt; ++i)
			s->chars[i] = *p++;
		s->chars[i] = '\0';	/* ensure '\0' at end */
	}
	/*
	** Check that all the local time type indices are valid.
	*/
	for (i = 0; i < s->timecnt; ++i)
		if (s->types[i] >= s->typecnt)
			return -1;
	/*
	** Check that all abbreviation indices are valid.
	*/
	for (i = 0; i < s->typecnt; ++i)
		if (s->ttis[i].tt_abbrind >= s->charcnt)
			return -1;

	return 0;
}

static long
detzcode(codep)
char *	codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}
