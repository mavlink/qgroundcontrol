%{
/* $XConsortium: lex.l,v 5.4 91/08/26 10:55:26 gildea Exp $ */

/*****************************************************************
Copyright (c) 1989,1990, 1991 by Sun Microsystems, Inc. and the X Consortium.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Sun Microsystems,
the X Consortium, and MIT not be used in advertising or publicity 
pertaining to distribution of the software without specific, written 
prior permission.  

SUN MICROSYSTEMS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT 
SHALL SUN MICROSYSTEMS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/


#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "strokegen.h"

#if defined(ISC) && defined(SYSV) && defined(SYSV386) && __STDC__
extern double atof(char *);
#endif

#ifdef FLEX_SCANNER
int yylineno;
#endif

%}
%%
\'[^']*\' |
\"[^"]*\"		return string(yytext, yyleng);
#.*			;
[ ,;\t\n]*              /* natural dilimters */ ;

[a-zA-Z][a-zA-Z0-9_.]*	{
				int	token;
				if (token = res_words(yytext))
					return token;
				return string(yytext, yyleng);
		 	}

[+-]?[0-9]+\.?[0-9]*[eE][+-]?[0-9]+ |
[+-]?[0-9]+\.[0-9]*     |
\.[0-9]+		{
                                yylval.dval = atof(yytext);
				return REAL;
                        }
[+-]?[0-9]+#[0-9]+	{
				return INTEGER;
			}
[+-]?[0-9]+		{
				yylval.ival = atoi(yytext);
				return INTEGER;
			}
[()]			;
%%

int
res_words(str)
char	str[];
{
	static	struct	res_strct {
		char	*word;
		int	token;
		} res_table[] = {
		{"BOTTOM",		BOTTOM},
		{"CENTER",		CENTER},
                {"PROPERTIES",          PROPERTIES},
		{"CLOSE",		CLOSE},
		{"FONTNAME",		FONTNAME},
		{"INDEX",		INDEX},
		{"MAGIC",		MAGIC},
		{"OPEN",		OPEN},
		{"RIGHT",		RIGHT},
		{"STROKE",		STROKE},
		{"TOP",			TOP},
		{"VERTICES",		VERTICES},
		{"BEARING",		BEARING},
		{"L_SPACE",		L_SPACE},
		{"WIDTH",		WIDTH},
		{"R_SPACE",		R_SPACE},
		{"NUM_CH",		NUM_CH},
		{0, 0}
		};

		{
			register struct res_strct *reserved;

			reserved = res_table;

			do
				if (!strcmp(str, reserved->word))
					break;
			while ((++reserved)->word != 0);
			return reserved->token;
		}
}

int
string(str, n)
char	*str;
int	n;
{
	if (*str == '\"' || *str == '\'')
	{
		str++;
		n -= 2;	/* one for EOL, one for end quote */
	}
	if ((yylval.cval = (char *)malloc(n+1)) != NULL)
	{
		strncpy(yylval.cval, str, n);
		yylval.cval[n] = '\0';
		return STRING;
	}
	else
		return 0;
}
