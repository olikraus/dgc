/*

  b_time.c
  
  Copyright (C) 2001 Oliver Kraus (olikraus@yahoo.com)

  This file is part of DGC.

  DGC is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  DGC is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DGC; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  

*/
#include <sys/types.h>
#include <time.h>
#include "b_time.h"

time_t g_b_time[16];
int g_b_time_stack_pos = 0;

const char *b_time_to_str(time_t t)
{
  static char buf[16];
  int s, m, h;
  s = t % 60;
  t = t / 60;
  m = t % 60;
  t = t / 60;
  h = t;
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  return buf;
}

void b_time_start()
{
  g_b_time[g_b_time_stack_pos] = time(NULL);
  g_b_time_stack_pos++;
}

const char *b_time_end()
{
  clock_t t;
  g_b_time_stack_pos--;
  t = time(NULL);
  return b_time_to_str(t-g_b_time[g_b_time_stack_pos]);
}

