/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/nl_langinfo.c	1.1"

#include <stdlib.h>
#include <nl_types.h>
#include <langinfo.h>
#include <locale.h>
#include <time.h>

#define MAX 64

extern char *gettxt();
extern size_t strftime();
extern struct lconv *localeconv();

static char *old_locale;

char *
nl_langinfo( item )
nl_item      item;
{
struct tm tm;
static char buf[MAX];
static char buf2[MAX];
struct lconv *currency;
char *s;
int size;

	switch (item) {
		/*
		 * The seven days of the week in their full beauty
		 */

		case DAY_1 :
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=0;
			size = strftime(buf,MAX,"%A",&tm);
			if (size)
				return buf;
			else
				return "Sunday";

		case DAY_2 :
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=1;
			size = strftime(buf,MAX,"%A",&tm);
			if (size)
				return buf;
			else
				return "Monday";

		case DAY_3 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=2;
			size = strftime(buf,MAX,"%A",&tm);
			if (size)
				return buf;
			else
				return "Tuesday";

		case DAY_4 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=3;
			size = strftime(buf,MAX,"%A",&tm);
			if (size)
				return buf;
			else
				return "Wednesday";

		case DAY_5 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=4;
			size = strftime(buf,MAX,"%A",&tm);
			if (size)
				return buf;
			else
				return "Thursday";

		case DAY_6 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=5;
			size = strftime(buf,MAX,"%A",&tm);
			if (size)
				return buf;
			else
				return "Friday";

		case DAY_7 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=6;
			size = strftime(buf,MAX,"%A",&tm);
			if (size)
				return buf;
			else
				return "Saturday";


		/*
		 * The abbreviated seven days of the week
		 */
		case ABDAY_1 :
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=0;
			size = strftime(buf,MAX,"%a",&tm);
			if (size)
				return buf;
			else
				return "Sun";

		case ABDAY_2 :
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=1;
			size = strftime(buf,MAX,"%a",&tm);
			if (size)
				return buf;
			else
				return "Mon";

		case ABDAY_3 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=2;
			size = strftime(buf,MAX,"%a",&tm);
			if (size)
				return buf;
			else
				return "Tue";

		case ABDAY_4 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=3;
			size = strftime(buf,MAX,"%a",&tm);
			if (size)
				return buf;
			else
				return "Wed";

		case ABDAY_5 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=4;
			size = strftime(buf,MAX,"%a",&tm);
			if (size)
				return buf;
			else
				return "Thur";

		case ABDAY_6 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=5;
			size = strftime(buf,MAX,"%a",&tm);
			if (size)
				return buf;
			else
				return "Fri";

		case ABDAY_7 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_wday=6;
			size = strftime(buf,MAX,"%a",&tm);
			if (size)
				return buf;
			else
				return "Sat";



		/*
		 * The full names of the twelve months...
		 */
		case MON_1 :
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=0;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "January";

		case MON_2 :
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=1;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "Feburary";

		case MON_3 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=2;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "March";

		case MON_4 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=3;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "April";

		case MON_5 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=4;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "May";

		case MON_6 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=5;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "June";

		case MON_7 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=6;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "July";

		case MON_8 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=7;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "August";

		case MON_9 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=8;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "September";

		case MON_10 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=9;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "October";

		case MON_11 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=10;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "November";

		case MON_12 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=11;
			size = strftime(buf,MAX,"%B",&tm);
			if (size)
				return buf;
			else
				return "December";

		/*
		 * ... and their abbreviated form
		 */
		case ABMON_1 :
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=0;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Jan";

		case ABMON_2 :
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=1;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Feb";

		case ABMON_3 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=2;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Mar";

		case ABMON_4 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=3;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Apr";

		case ABMON_5 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=4;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "May";

		case ABMON_6 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=5;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Jun";

		case ABMON_7 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=6;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Jul";

		case ABMON_8 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=7;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Aug";

		case ABMON_9 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=8;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Sep";

		case ABMON_10 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=9;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Oct";

		case ABMON_11 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=10;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Nov";

		case ABMON_12 : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_mon=11;
			size = strftime(buf,MAX,"%b",&tm);
			if (size)
				return buf;
			else
				return "Dec";

		case AM_STR : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_hour=1;
			size = strftime(buf,MAX,"%p",&tm);
			if (size)
				return buf;
			else
				return "AM";

		case PM_STR : 
			memset((void*)&tm,sizeof (struct tm),0);
			tm.tm_hour=13;
			size = strftime(buf,MAX,"%p",&tm);
			if (size)
				return buf;
			else
				return "PM";


		/*
		 * plus some special strings you might need to know
		 */

		case RADIXCHAR :
		case THOUSEP :
		case CRNCYSTR :

			currency = localeconv();
			switch (item) {

				case THOUSEP :
					return currency->thousands_sep;

				case RADIXCHAR :
					return currency->decimal_point;

				case CRNCYSTR : 
					return currency->currency_symbol;
			}

		/*
		 * Default string used to format date and time
		 *	e.g. Sunday, August 24 21:08:38 MET 1986
		 */

		case T_FMT :
			old_locale = setlocale(LC_MESSAGES,(char*)NULL);
			strcpy(buf,old_locale);
			(void)setlocale(LC_MESSAGES,setlocale(LC_TIME,(char*)NULL));
			s = gettxt("Xopen_info:1","%H:%M:%S");
			setlocale(LC_MESSAGES,buf);
			if (strcmp(s,"Message not found!!\n"))
				return s;
			else 
				return "%H:%M:%S";

		case D_FMT :
			old_locale = setlocale(LC_MESSAGES,(char*)NULL);
			strcpy(buf,old_locale);
			(void)setlocale(LC_MESSAGES,setlocale(LC_TIME,(char*)NULL));
			s = gettxt("Xopen_info:2","%m/%d/%y");
			setlocale(LC_MESSAGES,buf);
			if (strcmp(s,"Message not found!!\n"))
				return s;
			else 
				return "%m/%d/%y";

		case D_T_FMT :
			old_locale = setlocale(LC_MESSAGES,(char*)NULL);
			strcpy(buf,old_locale);
			(void)setlocale(LC_MESSAGES,setlocale(LC_TIME,(char*)NULL));
			s = gettxt("Xopen_info:3","%a %b %d %T %Z %Y");
			setlocale(LC_MESSAGES,buf);
			if (strcmp(s,"Message not found!!\n"))
				return s;
			else 
				return "%a %b %d %T %Z %Y";

		case YESSTR :
			old_locale = setlocale(LC_MESSAGES,(char*)NULL);
			strcpy(buf,old_locale);
			old_locale=setlocale(LC_ALL,(char*)NULL);
			if (*old_locale == '/') {
				/*
				 * composite locale
				 */
				old_locale++;
				s = buf2;
				while (*old_locale != '/')
					*s++ = *old_locale++;
				*s = '\0';
			} else
				strcpy(buf2,old_locale);
			old_locale = setlocale(LC_MESSAGES,buf2);
			s = gettxt("Xopen_info:4","yes");
			setlocale(LC_MESSAGES,buf);
			if (strcmp(s,"Message not found!!\n"))
				return s;
			else 
				return "yes";

		case NOSTR :
			old_locale = setlocale(LC_MESSAGES,(char*)NULL);
			strcpy(buf,old_locale);
			old_locale=setlocale(LC_ALL,(char*)NULL);
			if (*old_locale == '/') {
				/*
				 * composite locale
				 */
				old_locale++;
				s = buf2;
				while (*old_locale != '/')
					*s++ = *old_locale++;
				*s = '\0';
			} else
				strcpy(buf2,old_locale);
			old_locale = setlocale(LC_MESSAGES,buf2);
			(void)setlocale(LC_MESSAGES,old_locale);
			s = gettxt("Xopen_info:5","no");
			setlocale(LC_MESSAGES,buf);
			if (strcmp(s,"Message not found!!\n"))
				return s;
			else 
				return "no";

		default :
			return "";

	    }
}
