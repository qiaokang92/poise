%{
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include<stdio.h>
#include<string.h>

using namespace std;
#define YY_DECL extern "C" int yylex()

#include "poise.tab.h"  // to get the token types that we return

%}
%%
"//".*      {/* Comment line, do nothing */ }
if			{ return IF; }
then			{ return THEN; }
else 			{ return ELSE; }
drop			{ return DROP; }
fwd1			{ return FWD1; }
fwd2			{ return FWD2; }
fwd3			{ return FWD3; }
fwd4			{ return FWD4; }
fwd5			{ return FWD5; }
fwd6			{ return FWD6; }
fwd7			{ return FWD7; }
fwd8			{ return FWD8; }
fwd9			{ return FWD9; }
fwd10			{ return FWD10; }
fwd11			{ return FWD11; }
fwd12			{ return FWD12; }
fwd13			{ return FWD13; }
fwd14			{ return FWD14; }
match			{ return MATCH; }
def			{ return DEF; }
"list"[0-9]		{ yylval.sval = strdup(yytext); return LISTVAR; }
":="			{ return ASSIGN;}
in          { return IN; }
notin       { return NOTIN; }
count			{ return COUNT; }
monitor			{ return MONITOR; }
"("			{ return LP; }
")"			{ return RP; }
"["			{ return LB; }
"]"			{ return RB; }
"<"			{ return LT; }
"<="			{ return LE; }
">"			{ return GT; }
">="			{ return GE; }
"=="			{ return EQ; }
and			{ return AND; }
or			{ return OR; }
not			{ return NOT; }
"+"			{ return PLUS; }
"-"			{ return MINUS; }
"*"			{ return MULTI; }
"%"			{ return MOD; }
[ \t\n]           	;
[0-9]+            	{ yylval.ival = atoi(yytext); return CONST; }
[a-zA-Z0-9]+      	{
				// we have to copy because we can't rely on yytext not changing underneath us:
				yylval.sval = strdup(yytext);
				return VAR;
			}
.           		; // by default
%%
