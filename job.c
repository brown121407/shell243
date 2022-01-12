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

#include <stdlib.h>

#include "job.h"

int last_jid = 0;
job *jobs = NULL;

void
job_add (pid_t pid)
{
  job *j = (job *) malloc (sizeof (job));
  j->pid = pid;
  j->jid = ++last_jid;
  j->next = NULL;
  
  if (jobs == NULL)
    {
      jobs = j;
    }
  else
    {
      job *last = jobs;
      while (last->next != NULL)
	last = last->next;
      last->next = j;
    }
}

void
job_remove (job **j, job **prev)
{
  if (!j || !prev) // crash if bad parameters (future me won't like that)
    exit (EXIT_FAILURE);
  
  job *next = (*j)->next;

  free (*j);

  if (*prev)
    (*prev)->next = next;
  else
    jobs = next;
  *j = next;

  if (!next)
    {
      if (*prev)
	last_jid = (*prev)->jid;
      else
	{
	  last_jid = 0;
	  jobs = NULL;
	}
    }
}

void
free_jobs ()
{
  job *j = jobs;
  while (j != NULL)
    {
      job *aux = j->next;
      free (j);
      j = aux;
    }

  jobs = NULL;
}
