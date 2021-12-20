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

#ifndef SH243_PARSER_H
#define SH243_PARSER_H

typedef enum ast_node_type
  {
    AST_PROGRAM,
    AST_SEMI,
    AST_AMP,
    AST_AND,
    AST_OR,
    AST_PIPE_SEQ,
    AST_COMMAND,
    AST_REDIRECT,
    AST_REDIR_OP,
    AST_NUMBER,
    AST_WORD,
    AST_ERROR
  } ast_node_type;

typedef struct ast_node
{
  ast_node_type type;
  union { char *string; int number; };
  int len, cap;
  struct ast_node **children;
} ast_node;

ast_node *
parse ();

void
ast_free (ast_node *node);

#endif
