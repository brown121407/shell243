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

#include "eval.h"
#include "debug.h"

pid_t *bg_pids;
int bg_pids_len;

void
eval_init ()
{
    bg_pids_len = 2;
    bg_pids = malloc(bg_pids_len * sizeof(pid_t));
}

static int
eval_command (int in_fd, int out_fd, ast_node *cmd)
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("shell");
        return errno;
    } else if (pid == 0) {
        if (in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }

        if (out_fd != STDOUT_FILENO) {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }

        char **argv = (char **) malloc (sizeof (char *) * cmd->len);
        int i;
        for (i = 0; i < cmd->len && cmd->children[i]->type == AST_WORD; i++)
            argv[i] = cmd->children[i]->string;

        for (; i < cmd->len; i++) {
            ast_node *node = cmd->children[i];
            // TODO 0 1 2 inainte
            ast_node *operator = node->children[0];
            ast_node *file = node->children[1];
            if (operator->type == AST_REDIR_OP) {
                if (strcmp(operator->string, ">") == 0) {
                    char* output = file->string;
                    int fd = open(output, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                    dup2 (fd, STDOUT_FILENO);
                    close(fd);
                } else if (strcmp(operator->string, ">>") == 0) {
                    char* output = file->string;
                    int fd = open(output, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
                    dup2 (fd, STDOUT_FILENO);
                    close(fd);
                } else if (strcmp(operator->string, "<") == 0) {
                    char* input = file->string;
                    int fd = open(input, O_CREAT | O_RDONLY);
                    dup2 (fd, STDIN_FILENO);
                    close(fd);
                }
            }
        }

        if (execvp(argv[0], argv)) {
            perror("shell");
            return errno;
        }
    }

    return pid;
}

static int
eval_pipe_seq (ast_node *ast)
{
    pid_t *pids = (pid_t *) malloc((ast->len - 1) * sizeof(pid_t));
    int in = STDIN_FILENO, fildes[2];
    int stdin_copy = dup(STDIN_FILENO);
    for (int i = 0; i < ast->len - 1; i++) {
        pipe(fildes);
        pids[i] = eval_command(in, fildes[1], ast->children[i]);
        close(fildes[1]);
        in = fildes[0];
    }

    if (in != 0)
        dup2(in, 0);

    pids[ast->len - 1] = eval_command(STDIN_FILENO, STDOUT_FILENO, ast->children[ast->len - 1]);
    dup2(stdin_copy, STDIN_FILENO);
    close(stdin_copy);

    int wstatus;
    for (int i = 0; i < ast->len; i++) {
        wstatus = 0;
        waitpid(pids[i], &wstatus, 0);
    }
    return wstatus;
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
