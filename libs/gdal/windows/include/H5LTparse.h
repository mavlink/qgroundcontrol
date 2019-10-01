/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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

#ifndef YY_H5LTYY_HL_SRC_H5LTPARSE_H_INCLUDED
# define YY_H5LTYY_HL_SRC_H5LTPARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int H5LTyydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    H5T_STD_I8BE_TOKEN = 258,
    H5T_STD_I8LE_TOKEN = 259,
    H5T_STD_I16BE_TOKEN = 260,
    H5T_STD_I16LE_TOKEN = 261,
    H5T_STD_I32BE_TOKEN = 262,
    H5T_STD_I32LE_TOKEN = 263,
    H5T_STD_I64BE_TOKEN = 264,
    H5T_STD_I64LE_TOKEN = 265,
    H5T_STD_U8BE_TOKEN = 266,
    H5T_STD_U8LE_TOKEN = 267,
    H5T_STD_U16BE_TOKEN = 268,
    H5T_STD_U16LE_TOKEN = 269,
    H5T_STD_U32BE_TOKEN = 270,
    H5T_STD_U32LE_TOKEN = 271,
    H5T_STD_U64BE_TOKEN = 272,
    H5T_STD_U64LE_TOKEN = 273,
    H5T_NATIVE_CHAR_TOKEN = 274,
    H5T_NATIVE_SCHAR_TOKEN = 275,
    H5T_NATIVE_UCHAR_TOKEN = 276,
    H5T_NATIVE_SHORT_TOKEN = 277,
    H5T_NATIVE_USHORT_TOKEN = 278,
    H5T_NATIVE_INT_TOKEN = 279,
    H5T_NATIVE_UINT_TOKEN = 280,
    H5T_NATIVE_LONG_TOKEN = 281,
    H5T_NATIVE_ULONG_TOKEN = 282,
    H5T_NATIVE_LLONG_TOKEN = 283,
    H5T_NATIVE_ULLONG_TOKEN = 284,
    H5T_IEEE_F32BE_TOKEN = 285,
    H5T_IEEE_F32LE_TOKEN = 286,
    H5T_IEEE_F64BE_TOKEN = 287,
    H5T_IEEE_F64LE_TOKEN = 288,
    H5T_NATIVE_FLOAT_TOKEN = 289,
    H5T_NATIVE_DOUBLE_TOKEN = 290,
    H5T_NATIVE_LDOUBLE_TOKEN = 291,
    H5T_STRING_TOKEN = 292,
    STRSIZE_TOKEN = 293,
    STRPAD_TOKEN = 294,
    CSET_TOKEN = 295,
    CTYPE_TOKEN = 296,
    H5T_VARIABLE_TOKEN = 297,
    H5T_STR_NULLTERM_TOKEN = 298,
    H5T_STR_NULLPAD_TOKEN = 299,
    H5T_STR_SPACEPAD_TOKEN = 300,
    H5T_CSET_ASCII_TOKEN = 301,
    H5T_CSET_UTF8_TOKEN = 302,
    H5T_C_S1_TOKEN = 303,
    H5T_FORTRAN_S1_TOKEN = 304,
    H5T_OPAQUE_TOKEN = 305,
    OPQ_SIZE_TOKEN = 306,
    OPQ_TAG_TOKEN = 307,
    H5T_COMPOUND_TOKEN = 308,
    H5T_ENUM_TOKEN = 309,
    H5T_ARRAY_TOKEN = 310,
    H5T_VLEN_TOKEN = 311,
    STRING = 312,
    NUMBER = 313
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 74 "hl/src/H5LTparse.y" /* yacc.c:1909  */

    int     ival;         /*for integer token*/
    char    *sval;        /*for name string*/
    hid_t   hid;          /*for hid_t token*/

#line 119 "hl/src/H5LTparse.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE H5LTyylval;

int H5LTyyparse (void);

#endif /* !YY_H5LTYY_HL_SRC_H5LTPARSE_H_INCLUDED  */
