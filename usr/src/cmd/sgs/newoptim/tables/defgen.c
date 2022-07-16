/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:tables/defgen.c	1.4"

#include <stdio.h>

#define	NONE	0
#define	SLASH	1
#define	COMMENT	2
#define	STAR	3

main(argc,argv)
int	argc;
char	*argv[];
{extern int atoi();
 int count, c;
 unsigned int start;
 int state;
 char type;

 if(argc != 5)
	{
	 fprintf(stderr,"Usage: defgen `wc -l <file>` <start_value> <type>\n");
	 exit(1);
	}
 start = atoi(argv[1]) - atoi(argv[3]);
 state = NONE;
 type = argv[4][0];
 (void) printf("#define\t%cLOWER\t%d\n#define\t",type,start++);

 while((c = getchar()) != EOF)
	 {switch(c)
		{case '\n':
			if(state == NONE)
				(void) printf("\t%d\n#define\t",
					start++);
			else
				(void) putchar(c);
			break;
		 case '/':
			if(state == NONE)
				state = SLASH;
			else if(state == STAR)
				state = NONE;
			putchar(c);
			break;
		 case '*':
			if(state == SLASH)
				state = COMMENT;
			else if(state == COMMENT)
				state = STAR;
			(void) putchar(c);
			break;
		 default:
			(void) putchar(c);
			break;
		}
	}
 (void) printf("%cUPPER\t%d\n",type,start);

 exit(0);
}
