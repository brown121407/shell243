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

#define _POSIX_C_SOURCE 200809L

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
#include <signal.h>

#include "eval.h"
#include "job.h"
#include "debug.h"

static int
eval_builtin_cd (const ast_node *cmd)
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

static int
eval_builtin_jobs (const ast_node *cmd)
{
  if (cmd->len > 1)
    {
      errno = EINVAL;
      perror ("jobs");
      return -1;
    }

  job *j = jobs, *prev = NULL;
  while (j)
    {
      int wstatus;
      pid_t w = waitpid (j->pid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);

      if (w == -1)
	{
	  perror ("jobs: waitpid");
	  return -1;
	}
      printf ("[%d]+ ", j->jid);
      if (w == 0)
	printf ("Running\n");
      else
	{
	  if (WIFEXITED (wstatus))
	    {
	      puts ("Done");
	      job_remove (&j, &prev);
	    }
	  else if (WIFSIGNALED (wstatus))
	    printf ("Killed by signal %d\n", WTERMSIG (wstatus));
	  else if (WIFSTOPPED (wstatus))
	    printf ("Stopped by signal %d\n", WSTOPSIG (wstatus));
	  else if (WIFCONTINUED (wstatus))
	    puts ("Continued");
	}

      prev = j;
      if (j)
	j = j->next;
    }

  return 0; 
}

static int
eval_builtin_exit (const ast_node *cmd)
{
  if (cmd->len > 2)
    {
      errno = EINVAL;
      perror ("jobs");
      return -1;
    }

  if (cmd->len == 2)
    {
      errno = 0;
      char *endptr;
      long retval = strtol (cmd->children[1]->string, &endptr, 10);

      if (errno != 0)
	{
	  perror ("exit: strtol");
	  return errno;
	}
      
      if (endptr == cmd->children[1]->string)
	{
	  errno = EINVAL;
	  perror ("exit: strtol");
	  return errno;
	}

      exit (retval);
    }
  else
    exit (EXIT_SUCCESS);
}

static int
eval_command (int in_fd, int out_fd, const ast_node *cmd)
{
  if (strcmp (cmd->children[0]->string, "cd") == 0)
    return eval_builtin_cd (cmd);
  else if (strcmp (cmd->children[0]->string, "jobs") == 0)
    return eval_builtin_jobs (cmd);
  else if (strcmp (cmd->children[0]->string, "exit") == 0)
    return eval_builtin_exit (cmd);
  
  pid_t pid = fork ();

  if (pid == -1)
    {
      perror ("shell");
      return -1;
    }
  else if (pid == 0)
    {
      signal (SIGINT, SIG_DFL);
      signal (SIGTSTP, SIG_DFL);
      
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
	  const ast_node *node = cmd->children[i];
	  // TODO Redirects with specific fds (1> 2> etc.)
	  const ast_node *operator = node->children[0];
	  const ast_node *file = node->children[1];
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
eval_pipe_seq (const ast_node *ast)
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

  return WEXITSTATUS (wstatus);
}

static int
eval_and_or (const ast_node *ast)
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
eval_program (const ast_node *ast)
{
  int retval = EXIT_SUCCESS;

  for (int i = 0; i < ast->len; i++)
    if (ast->children[i]->type != AST_SEMI
	&& ast->children[i]->type != AST_AMP)
      {
	if (ast->len > i + 1 && ast->children[i + 1]->type == AST_AMP)
	  {
	    pid_t pid;

	    pid = fork ();
	    if (pid == -1)
	      {
		perror ("shell");
		return errno;
	      }
	    else if (pid == 0)
	      {
		free_jobs ();
		exit (eval (ast->children[i]));
	      }
	    else
	      {
		job_add (pid);
		printf ("[%d] %d\n", last_jid, pid);
		retval = EXIT_SUCCESS;
	      }
	  }
	else // foreground process
	  retval = eval (ast->children[i]);
      }

  check_bg_processes ();

  return retval;
}

int
eval (const ast_node *ast)
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
  job *j = jobs, *prev = NULL;
  while (j)
    {
      int wstatus = 0;
      pid_t pid = waitpid (j->pid, &wstatus, WNOHANG);

      if (pid == -1)
	{
	  printf ("fucked up with pid: %d, job %d\n", j->pid, j->jid);
	  perror ("check_bg_processes: waitpid");
	  return;
	}

      if (pid)
	{
	  printf ("[%d]+ Done\n", j->jid);
	  job_remove (&j, &prev);
	}
      prev = j;
      if (j)
	j = j->next;
    }
}
