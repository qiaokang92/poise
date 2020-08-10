/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_POISE_TAB_H_INCLUDED
# define YY_YY_POISE_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    IF = 258,
    THEN = 259,
    ELSE = 260,
    MATCH = 261,
    COUNT = 262,
    MONITOR = 263,
    DROP = 264,
    FWD1 = 265,
    FWD2 = 266,
    FWD3 = 267,
    FWD4 = 268,
    FWD5 = 269,
    FWD6 = 270,
    FWD7 = 271,
    FWD8 = 272,
    FWD9 = 273,
    FWD10 = 274,
    FWD11 = 275,
    FWD12 = 276,
    FWD13 = 277,
    FWD14 = 278,
    LP = 279,
    RP = 280,
    LB = 281,
    RB = 282,
    LT = 283,
    GT = 284,
    GE = 285,
    LE = 286,
    EQ = 287,
    PLUS = 288,
    MINUS = 289,
    MULTI = 290,
    MOD = 291,
    AND = 292,
    OR = 293,
    NOT = 294,
    ASSIGN = 295,
    DEF = 296,
    IN = 297,
    NOTIN = 298,
    LISTVAR = 299,
    CONST = 300,
    VAR = 301
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 81 "poise.y" /* yacc.c:1909  */

	int ival;
	float fval;
	char *sval;
	int listval[100];

#line 108 "poise.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_POISE_TAB_H_INCLUDED  */
