%{
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
%}
%{
/*	  All Rights Reserved  	*/
%}

%{
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
%}
%{
/*	The copyright notice above does not evidence any   	*/
%}
%{
/*	actual or intended publication of such source code.	*/
%}

%{
#ident	"@(#)mkboot-3b2:mkboot.l	1.6.1.1"
%}

%Start	COMMENT HEADER DATA Q

%%


<HEADER,DATA>0[xX][0-9a-fA-F]+	|
<HEADER,DATA>0[0-7]+		|
<HEADER,DATA>[0-9]+		{
				yylval.number = strtol( yytext, (char**)NULL, 0 );
				return( NUMBER );
				}

<DATA>[_a-zA-Z][_a-zA-Z0-9]*[ \t]*\([ \t]*\)	{
				if ( nstring-string > MAXSTRING-yyleng-1 )
					yyfatal( "string table overflow" );

				strcpy( yylval.string=nstring, strtok(yytext," \t(") );
				nstring += strlen(nstring) + 1;

				return( ROUTINE );
				}
<DATA>[_a-zA-Z][_a-zA-Z0-9]*	{
				if ( nstring-string > MAXSTRING-yyleng-1 )
					yyfatal( "string table overflow" );
				strncpy( yylval.string=nstring, yytext, yyleng+1 );
				nstring += yyleng + 1;
				return( DWORD );
				}
<DATA>'.'			{
				yylval.number = yytext[1] & 0xFF;
				return( NUMBER );
				}
<DATA>'\\\\'			|
<DATA>'\\n'			|
<DATA>'\\v'			|
<DATA>'\\t'			|
<DATA>'\\b'			|
<DATA>'\\r'			|
<DATA>'\\f'			{
				static char convert[] = "\\\\n\nv\vt\tb\br\rf\f";
				yylval.number = *(strchr(convert,yytext[2]) + 1) & 0xFF;
				return( NUMBER );
				}
<DATA>'\\[0-7]'			|
<DATA>'\\[0-7][0-7]'		|
<DATA>'\\[0-7][0-7][0-7]'	{
				yylval.number = strtol( yytext+2, (char**)NULL, 8 );
				if ( yylval.number > 255 )
					yyerror( "illegal character constant: '\\%o'", yylval.number );
				return( NUMBER );
				}
<DATA>\"			{
				BEGIN Q;
				}

<Q>([^"]|\\\")*			{
				yylval.string = copystring( yytext );
				return( STRING );
				}
<Q>\"				{
				BEGIN DATA;
				}

<HEADER>[_a-zA-Z][_a-zA-Z0-9]*	{
				if ((yylval.word = strdup(yytext)) == NULL)
					yyerror("can't malloc space for %s\n",yytext);
				return( HWORD );
				}

<HEADER,DATA>[-,*=%&#+/(){}[\]]	{
				return( yytext[0] );
				}
<HEADER,DATA>[ \t]		;
<HEADER,DATA>\n			{
				BEGIN 0;
				}

<COMMENT>.			;
<COMMENT>\n			{
				BEGIN 0;
				}

^\*				{
				BEGIN COMMENT;
				}
^\$\$				{
				return( 0 );
				}
^[a-zA-Z0-9][a-zA-Z0-9]*	{
				BEGIN HEADER;
				if ((yylval.word = strdup(yytext)) == NULL)
					yyerror("can't malloc space for %s\n",yytext);
				return( HWORD );
				}
^\-				{
				BEGIN HEADER;
				return( '-' );
				}
^[ \t]				{
				BEGIN DATA;
				}
^\n				;
.				{
				yyerror( "illegal character 0%o", yytext[0] );
				}
