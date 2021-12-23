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
#include <stdlib.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "parser.h"
#include "debug.h"
#include "eval.h"

int
main (int argc, char *argv[])
{
    eval_init ();
  while (true)
    {
      char *line = readline ("$ ");
      if (!line) break;
      if (*line)
	{
	  add_history (line);
	  init_lexer (line);
	  ast_node *ast = parse ();
	  print_ast (ast, 0);
          eval (ast);
	  ast_free (ast);
	}

      free (line);
    }
  return 0;
}
