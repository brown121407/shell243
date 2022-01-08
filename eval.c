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
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <pwd.h>

#include "eval.h"
#include "debug.h"

pid_t *bg_pids;
int bg_pids_len, bg_pids_crnt;

void
eval_init ()
{
  bg_pids_len = 2;
  bg_pids = malloc (bg_pids_len * sizeof (pid_t));
}

static int
eval_command (int in_fd, int out_fd, ast_node *cmd)
{
  if (strcmp (cmd->children[0]->string, "cd") == 0)
    {
      if (cmd->len > 2)
	{
	  errno = EINVAL;
	  perror ("cd");
	  return -1;
	}
      else if (cmd->len == 1)
	{
	  struct passwd *pw = getpwuid (getuid ());
	  const char *homedir = pw->pw_dir;
	  if (chdir (homedir) != 0)
	    {
	      perror ("cd");
	      return -1;
	    }
	}
      else
	{
	  if (chdir (cmd->children[1]->string) != 0)
	    {
	      perror ("cd");
	      return -1;
	    }
	}

      return 0;
    }
  else if (strcmp (cmd->children[0]->string, "exit") == 0)
    {
      if (cmd->len == 1)
	exit (EXIT_SUCCESS);

      // TODO if 1 argument, turn into int and pass to exit

      errno = EINVAL;
      perror ("exit");
      return -1;
    }

  pid_t pid = fork ();

  if (pid == -1)
    {
      perror ("shell");
      return -1;
    }
  else if (pid == 0)
    {
      if (in_fd != STDIN_FILENO)
	{
	  dup2 (in_fd, STDIN_FILENO);
	  close (in_fd);
	}

      if (out_fd != STDOUT_FILENO)
	{
	  dup2 (out_fd, STDOUT_FILENO);
	  close (out_fd);
	}

      char **argv = (char **) malloc (sizeof (char *) * (cmd->len + 1));
      int i;
      for (i = 0; i < cmd->len && cmd->children[i]->type == AST_WORD; i++)
	argv[i] = cmd->children[i]->string;

      argv[i] = NULL;

      for (; i < cmd->len; i++)
	{
	  ast_node *node = cmd->children[i];
	  // TODO 0 1 2 inainte
	  ast_node *operator = node->children[0];
	  ast_node *file = node->children[1];
	  if (operator->type == AST_REDIR_OP)
	    {
	      if (strcmp (operator->string, ">") == 0)
		{
		  char *output = file->string;
		  int fd =
		    open (output, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
		  dup2 (fd, STDOUT_FILENO);
		  close (fd);
		}
	      else if (strcmp (operator->string, ">>") == 0)
		{
		  char *output = file->string;
		  int fd =
		    open (output, O_CREAT | O_WRONLY | O_APPEND,
			  S_IRUSR | S_IWUSR);
		  dup2 (fd, STDOUT_FILENO);
		  close (fd);
		}
	      else if (strcmp (operator->string, "<") == 0)
		{
		  char *input = file->string;
		  int fd = open (input, O_CREAT | O_RDONLY);
		  dup2 (fd, STDIN_FILENO);
		  close (fd);
		}
	    }
	}

      if (execvp (argv[0], argv))
	{
	  perror ("shell");
	  exit(errno);
	}
    }

  return pid;
}

static int
eval_pipe_seq (ast_node *ast)
{
  pid_t *pids = (pid_t *) malloc (ast->len * sizeof (pid_t));
  int in = STDIN_FILENO, fildes[2];
  int stdin_copy = dup (STDIN_FILENO);
  for (int i = 0; i < ast->len - 1; i++)
    {
      pipe (fildes);
      pids[i] = eval_command (in, fildes[1], ast->children[i]);
      close (fildes[1]);
      in = fildes[0];
    }

  if (in != STDIN_FILENO)
    dup2 (in, STDIN_FILENO);

  pids[ast->len - 1] =
    eval_command (STDIN_FILENO, STDOUT_FILENO, ast->children[ast->len - 1]);
  dup2 (stdin_copy, STDIN_FILENO);
  close (stdin_copy);

  int wstatus;
  for (int i = 0; i < ast->len; i++)
    {
      wstatus = 0;
      if (pids[i] > 0) // Negative PIDs mean something went wrong, so don't wait.
	waitpid (pids[i], &wstatus, 0);
    }

  free (pids);

  return wstatus;
}

static int
eval_and_or (ast_node *ast)
{
  int result;
  if (ast->type == AST_AND)
    {
      result = eval (ast->children[0]);
      if (result == 0)
	return eval (ast->children[1]);
    }
  else
    {
      result = eval (ast->children[0]);
      if (result != 0)
	return eval (ast->children[1]);
    }
  return 0;
}

int
eval_program (ast_node *ast)
{
  for (int i = 0; i < ast->len; i++)
    if (ast->children[i]->type != AST_SEMI
	&& ast->children[i]->type != AST_AMP)
      {
	if (ast->len > i + 1 && ast->children[i + 1]->type == AST_AMP)
	  {
	    pid_t pid;
	    bg_pids_crnt++;
	    if (bg_pids_crnt >= bg_pids_len)
	      {
		bg_pids_len *= 2;
		pid_t *new_pids = malloc (bg_pids_len * sizeof (pid_t));
		for (int j = 0; j < bg_pids_crnt; j++)
		  new_pids[j] = bg_pids[j];
		free (bg_pids);
		bg_pids = new_pids;
	      }

	    pid = fork ();
	    if (pid == -1)
	      {
		bg_pids_crnt--;
		perror ("shell");
		return errno;
	      }
	    else if (pid == 0)
	      exit (eval (ast->children[i]));
	    else
	      {
		printf ("[%d] %d\n", bg_pids_crnt, pid);
		bg_pids[bg_pids_crnt] = pid;
	      }
	  }
	else // foreground process
	  eval (ast->children[i]);

	check_bg_processes ();
      }
  return 0;
}

int
eval (ast_node *ast)
{
  switch (ast->type)
    {
    case AST_PROGRAM:
      return eval_program (ast);
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
}

void
check_bg_processes ()
{
  for (int j = 1; j <= bg_pids_crnt; j++)
    {
      int status;
      if (bg_pids[j] != -1)
	{
	  waitpid (bg_pids[j], &status, WNOHANG);
	  if (WIFEXITED (status))
	    {
	      printf ("[%d]+ Done\n", j);
	      bg_pids[j] = -1;
	    }
	}
    }

  for (; bg_pids_crnt >= 1 && bg_pids[bg_pids_crnt] == -1; bg_pids_crnt--) {}
}
