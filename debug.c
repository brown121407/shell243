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

#include <stdio.h>

#include "debug.h"

const char *token_names[] = {
  [TOK_GT]    = "TOK_GT",
  [TOK_DGT]   = "TOK_DGT",
  [TOK_LT]    = "TOK_LT",
  [TOK_DLT]   = "TOK_DLT",
  [TOK_PIPE]  = "TOK_PIPE",
  [TOK_AMP]   = "TOK_AMP",
  [TOK_OR]    = "TOK_OR",
  [TOK_AND]   = "TOK_AND",
  [TOK_IONUM] = "TOK_IONUM",
  [TOK_SEMI]  = "TOK_SEMI",
  [TOK_WORD]  = "TOK_WORD",
  [TOK_ERR]   = "TOK_ERR",
  [TOK_EOF]   = "TOK_EOF"
};

const char *ast_names[] = {
  [AST_PROGRAM] = "AST_PROGRAM",
  [AST_AMP] = "AST_AMP",
  [AST_SEMI] = "AST_SEMI",
  [AST_AND] = "AST_AND",
  [AST_OR] = "AST_OR",
  [AST_PIPE_SEQ] = "AST_PIPE_SEQ",
  [AST_COMMAND] = "AST_COMMAND",
  [AST_WORD] = "AST_WORD",
  [AST_REDIRECT] = "AST_REDIRECT",
  [AST_REDIR_OP] = "AST_REDIR_OP",
  [AST_NUMBER] = "AST_NUMBER",
  [AST_ERROR] = "AST_ERROR"
};

const char *
token_type_to_string (token_type type)
{
  return token_names[type];
}

void
print_token (token tok)
{
  printf ("token: { type: %s, content: %.*s }\n", token_names[tok.type],
	  tok.length, tok.start);
}

void
print_tokens ()
{
  for (;;)
    {
      token tok = next_token ();
      print_token (tok);
      if (tok.type == TOK_EOF)
	break;
    }
}

const char *
ast_type_to_string (ast_node_type type)
{
  return ast_names[type];
}

void
print_ast (ast_node *node, int indent_level)
{
  for (int i = 0; i < indent_level; i++)
    putchar ('\t');

  printf ("%s, %d children, content: ", ast_names[node->type], node->len);
  if (node->string != NULL)
    printf ("%s", node->string);
  else if (node->type == AST_NUMBER)
    printf ("%d", node->number);

  puts ("");

  if (node->len > 0)
    {
      for (int i = 0; i < node->len; i++)
	print_ast (node->children[i], indent_level + 1);
    }
}
