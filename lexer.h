/* Copyright (C) 2021 by Alexandru-Sergiu Marton

   This file is part of shell243.
   
   shell243 is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   shell243 is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with shell243.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SH243_LEXER_H
#define SH243_LEXER_H

// Whenever adding new token types, remember to update debug.c!
typedef enum token_type
  {
    TOK_GT, TOK_DGT, TOK_LT, TOK_DLT,
    TOK_PIPE, TOK_AMP, TOK_OR, TOK_AND,
    TOK_IONUM,
    TOK_SEMI,
    TOK_WORD,
    TOK_ERR,
    TOK_EOF
  } token_type;

typedef struct stream
{
  char *start;
  char *current;
} stream;
extern stream stm;

typedef struct token
{
  token_type type;
  const char *start;
  int length;
} token;

void
init_lexer ();

token
next_token ();

#endif
