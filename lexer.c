/* Copyright (C) 2021 by Alexandru-Sergiu Marton 
   
   This file contains code derived from the source of the "Crafting
   Interpreters" book, Copyright (c) 2015 Robert Nystrom, originally
   licensed under the MIT license:

   > Permission is hereby granted, free of charge, to any person obtaining a copy
   > of this software and associated documentation files (the "Software"), to
   > deal in the Software without restriction, including without limitation the
   > rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   > sell copies of the Software, and to permit persons to whom the Software is
   > furnished to do so, subject to the following conditions:
   >
   > The above copyright notice and this permission notice shall be included in
   > all copies or substantial portions of the Software.
   >
   > THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   > IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   > FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   > AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   > LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   > FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   > IN THE SOFTWARE.

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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "lexer.h"

stream stm;

typedef enum { Q_SINGLE, Q_DOUBLE, Q_BACKSLASH, Q_NONE } quote_type;

static token
make_token (token_type type)
{
  return (token) { .type = type, .start = stm.start, .length = stm.current - stm.start };
}

static token
error_token (const char *msg)
{
  return (token) { .type = TOK_ERR, .start = msg, .length = strlen (msg) };
}

static bool
is_at_end ()
{
  return *stm.current == '\0';
}

static char
advance ()
{
  stm.current++;
  return stm.current[-1];
}

static bool
match (char expected)
{
  if (is_at_end ())
    return false;
  if (*stm.current != expected)
    return false;
  stm.current++;
  return true;
}

static char
peek ()
{
  return *stm.current;
}

static token
word (quote_type qtype)
{
  int quote_offset = 0;
  bool can_be_ionum = true;
  char c = peek ();
  while (!is_at_end ())
    {
      if (qtype == Q_NONE
	  && (c == '|' || c == '&' || c == '<' || c == '>' || c == ';' || isspace (c)))
	break;

      if (c == '\\' && qtype == Q_NONE)
	qtype = Q_BACKSLASH, quote_offset = stm.current - stm.start;
      else if (c == '\'' && qtype == Q_NONE)
	qtype = Q_SINGLE, quote_offset = stm.current - stm.start;
      else if (c == '"' && qtype == Q_NONE)
	qtype = Q_DOUBLE, quote_offset = stm.current - stm.start;
      else if (qtype == Q_BACKSLASH)
	{
	  qtype = Q_NONE;
	  memmove (stm.current - 1, stm.current, strlen (stm.current) + 1);
	  --stm.current;
	}
      else if ((c == '\'' && qtype == Q_SINGLE)
	       || (c == '"' && qtype == Q_DOUBLE))
	{
	  qtype = Q_NONE;
	  // Since this means we found both opening and closing
	  // quotes, we can safely shift our input two chars leftward.
	  memmove (stm.start + quote_offset, stm.start + quote_offset + 1,
		   strlen (stm.start + quote_offset + 1) + 1);
	  --stm.current;
	  memmove (stm.current, stm.current + 1,
		   strlen (stm.current + 1) + 1);
	  --stm.current;
	}

      if (can_be_ionum && !isdigit (c))
	can_be_ionum = false;

      advance ();
      c = peek ();
    }

  if (qtype != Q_NONE && is_at_end ())
    return error_token ("Reached EOF before closing quote.");

  if (can_be_ionum && (c == '<' || c == '>'))
    return make_token (TOK_IONUM);

  return make_token (TOK_WORD);
}

static void
skip_whitespace ()
{
  while (!is_at_end () && isspace (peek ()))
    advance ();
}

void
init_lexer (char *cmd)
{
  stm = (stream) { .start = cmd, .current = cmd };
}

token
next_token ()
{
  stm.start = stm.current;
  if (is_at_end ())
    return make_token (TOK_EOF);

  char c = advance ();

  switch (c)
    {
    case ';':
      return make_token (TOK_SEMI);
    case '|':
      if (match ('|'))
	return make_token (TOK_OR);
      return make_token (TOK_PIPE);
    case '&':
      if (match ('&'))
	return make_token (TOK_AND);
      return make_token (TOK_AMP);
    case '<':
      if (match ('<'))
	return make_token (TOK_DLT);
      return make_token (TOK_LT);
    case '>':
      if (match ('>'))
	return make_token (TOK_DGT);
      return make_token (TOK_GT);
    case '\'':
      return word (Q_SINGLE);
    case '"':
      return word (Q_DOUBLE);
    case '\\':
      return word (Q_BACKSLASH);
    default:
      if (isspace (c))
	{
	  skip_whitespace ();
	  return next_token ();
	}
      return word (Q_NONE);
    }

  return error_token ("Unexpected character.");
}
