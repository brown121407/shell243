/* Copyright (C) 2021 by Alexandru-Sergiu Marton
   Copyright (C) 2021 by Daria Mihaela Broscoțeanu
   Copyright (C) 2021 by Andreea Diana Gherghescu

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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>

#include "eval.h"
#include "debug.h"

static int
eval_command (ast_node *ast)
{
  char **argv = (char **) malloc (sizeof (char *) * ast->len);
  int i;
  for (i = 0; i < ast->len && ast->children[i]->type == AST_WORD; i++)
    argv[i] = ast->children[i]->string;

  // TODO use redirects (next nodes starting with i) 

  pid_t pid = fork();
  if (pid == -1)
    perror("shell");
  else if (pid == 0)
    {
      if (execvp(argv[0], argv) == -1)
	perror ("shell");
    }
  else
    {
      int wstatus;
      waitpid(pid, &wstatus, 0);
      free(argv);

      return wstatus;
    }

  return -1;
}

static int
eval_pipe_seq (ast_node *ast)
{
  // TODO
  // needs custom AST_COMMAND evaluation here, to be able to pipe.
  // go through children in order and pipe n into n + 1
  return 0;
}

static int
eval_and_or (ast_node *ast)
{
  // TODO
  // if ast->type is AST_AND:
  //   eval first node
  //   if returns 0 eval second node
  // else is AST_OR:
  //   eval first node
  //   if returns != 0 eval second node
  return 0;
}

int
eval_program (ast_node *ast)
{
  // TODO
  // for each child in ast->children
  //   if child is not separator
  //     if next child is AST_AMP
  //       eval child, but run in background
  //     else eval child
  //   else skip child
  return 0;
}

int
eval (ast_node *ast)
{
  switch (ast->type)
    {
    case AST_PROGRAM:
      return eval_program (ast);
    case AST_COMMAND:
      return eval_command (ast);
    case AST_AND:
    case AST_OR:
      return eval_and_or (ast);
    case AST_PIPE_SEQ:
      return eval_pipe_seq (ast);
    default:
      fprintf (stderr, "Unexpected AST node: %s",
	       ast_type_to_string (ast->type));
      return -1;
    }

  return 0;
}
