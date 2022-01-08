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

// The function names in this file should parse according to the rule
// with the most similar name in grammar.ebnf. (e.g. parse_pipe_seq
// corresponds to the "pipe sequence" rule)

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "parser.h"
#include "lexer.h"
#include "debug.h"

static token current_token;

#define MATCH(ttype) (current_token.type == ttype)

static ast_node *
make_error (const char *message)
{
  ast_node *node = (ast_node *) malloc (sizeof (ast_node));
  node->type = AST_ERROR;
  node->len = 0;
  node->children = NULL;
  node->string = (char *) malloc (strlen (message) + 1);
  strcpy (node->string, message);

  return node;
}

static ast_node *
empty_node (ast_node_type type)
{
  ast_node *node = (ast_node *) malloc (sizeof (ast_node));
  node->cap = 0;
  node->len = 0;
  node->children = NULL;
  node->string = NULL;
  node->type = type;

  return node;
}

static ast_node *
from_token (token tok, ast_node_type type)
{
  ast_node *node = empty_node (type);
  node->string = (char *) malloc (tok.length + 1);
  strncpy (node->string, tok.start, tok.length);
  node->string[tok.length] = '\0';
  if (type == AST_NUMBER)
    {
      int number = atoi (node->string);
      free (node->string);
      node->string = NULL;
      node->number = number;
    }

  return node;
}

static void
add_child (ast_node *node, ast_node *child)
{
  node->len++;
  if (node->len > node->cap)
    {
      if (node->cap == 0)
	node->cap = 2;
      else
	node->cap *= 2;

      ast_node **new_kids =
	(ast_node **) malloc (sizeof (ast_node *) * node->cap);
      for (int i = 0; i < node->len - 1; i++)
	new_kids[i] = node->children[i];
      free (node->children);
      node->children = new_kids;
    }
  node->children[node->len - 1] = child;
}

void
ast_free (ast_node *node)
{
  if (node->string != NULL)
    free (node->string);
  if (node->cap > 0)
    {
      for (int i = 0; i < node->len; i++)
	ast_free (node->children[i]);
      free (node->children);
    }
  free (node);
}

static ast_node *
parse_redirect ()
{
  ast_node *fd_node = NULL, *redir_op_node = NULL, *file_node = NULL;

  if (MATCH (TOK_IONUM))
    {
      fd_node = from_token (current_token, AST_NUMBER);
      current_token = next_token ();
    }

  if (!(MATCH (TOK_LT) || MATCH (TOK_GT) || MATCH (TOK_DGT)))
    {
      char err[64];
      sprintf (err, "Expected TOK_LT or TOK_GT or TOK_DGT, got %s.",
	       token_type_to_string (current_token.type));
      return make_error (err);
    }

  redir_op_node = from_token (current_token, AST_REDIR_OP);

  current_token = next_token ();
  if (!MATCH (TOK_WORD))
    {
      char err[64];
      sprintf (err, "Expected TOK_WORD, got %s.",
	       token_type_to_string (current_token.type));
      return make_error (err);
    }

  file_node = from_token (current_token, AST_WORD);

  ast_node *redirect_node = empty_node (AST_REDIRECT);

  if (fd_node != NULL)
    add_child (redirect_node, fd_node);

  add_child (redirect_node, redir_op_node);
  add_child (redirect_node, file_node);

  return redirect_node;
}

static ast_node *
parse_command ()
{
  if (!MATCH (TOK_WORD))
    {
      char err[64];
      sprintf (err, "Expected TOK_WORD, got %s.",
	       token_type_to_string (current_token.type));
      current_token = next_token ();
      return make_error (err);
    }

  ast_node *command_node = empty_node (AST_COMMAND);
  while (MATCH (TOK_WORD)) // there will be at least 1 iteration
    {
      ast_node *word_node = from_token (current_token, AST_WORD);
      add_child (command_node, word_node);
      current_token = next_token ();
    }

  while (MATCH (TOK_IONUM) || MATCH (TOK_LT) || MATCH (TOK_GT)
	 || MATCH (TOK_DGT))
    {
      ast_node *redirect_node = parse_redirect ();
      add_child (command_node, redirect_node);
      current_token = next_token ();
    }

  return command_node;
}

static ast_node *
parse_pipe_seq ()
{
  ast_node *command_node = parse_command ();
  ast_node *pipe_seq_node = empty_node (AST_PIPE_SEQ);
  add_child (pipe_seq_node, command_node);
  while (MATCH (TOK_PIPE))
    {
      current_token = next_token ();
      command_node = parse_command ();
      add_child (pipe_seq_node, command_node);
    }

  return pipe_seq_node;
}

static ast_node *
parse_and_or_nested (ast_node *left)
{
  ast_node *and_or_node = empty_node (MATCH (TOK_AND) ? AST_AND : AST_OR);
  current_token = next_token ();
  ast_node *pipe_seq_node = parse_pipe_seq ();

  add_child (and_or_node, left);
  add_child (and_or_node, pipe_seq_node);

  if (MATCH (TOK_AND) || MATCH (TOK_OR))
    return parse_and_or_nested (and_or_node);
  else
    return and_or_node;
}

static ast_node *
parse_and_or ()
{
  ast_node *pipe_seq_node = parse_pipe_seq ();

  if (!MATCH (TOK_AND) && !MATCH (TOK_OR))
    return pipe_seq_node;

  ast_node *and_or_node = empty_node (MATCH (TOK_AND) ? AST_AND : AST_OR);
  add_child (and_or_node, pipe_seq_node);
  current_token = next_token ();

  pipe_seq_node = parse_pipe_seq ();
  add_child (and_or_node, pipe_seq_node);

  if (MATCH (TOK_AND) || MATCH (TOK_OR))
    return parse_and_or_nested (and_or_node);
  else
    return and_or_node;
}

ast_node *
parse ()
{
  ast_node *program_node = empty_node (AST_PROGRAM);
  current_token = next_token ();
  ast_node *and_or_node = parse_and_or ();
  add_child (program_node, and_or_node);

  while (MATCH (TOK_AMP) || MATCH (TOK_SEMI))
    {
      ast_node *sep_node = empty_node (MATCH (TOK_AMP) ? AST_AMP : AST_SEMI);
      add_child (program_node, sep_node);
      current_token = next_token ();

      if (!MATCH (TOK_AMP) && !MATCH (TOK_SEMI) && !MATCH (TOK_EOF))
	{
	  and_or_node = parse_and_or ();
	  add_child (program_node, and_or_node);
	}
    }

  if (!MATCH (TOK_EOF))
    {
      print_token (current_token);
      print_ast (program_node, 0);
      ast_free (program_node);
      char err[64];
      sprintf (err, "Expected TOK_EOF, got %s.",
	       token_type_to_string (current_token.type));
      return make_error (err);
    }

  return program_node;
}

bool
check_ast_error (ast_node *ast)
{
  if (ast->type == AST_ERROR)
    {
      fprintf (stdout, "%s\n", ast->string);
      return true;
    }
  else
    {
      bool found = false;
      for (int i = 0; i < ast->len; ++i)
	found = found || check_ast_error (ast->children[i]);
      return found;
    }
}

#undef MATCH
