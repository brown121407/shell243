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

#ifndef SH243_DEBUG_H
#define SH243_DEBUG_H

#include "lexer.h"
#include "parser.h"

const char *
token_type_to_string (token_type type);

const char *
ast_type_to_string (ast_node_type type);

void
print_token (token tok);

void
print_tokens ();

void
print_ast (ast_node *node, int indent_level);

#endif
