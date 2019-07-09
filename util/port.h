/*

  port.h
  
  Copyright (C) 2001 Oliver Kraus (olikraus@yahoo.com)

  This file is part of dgc.

  dgc is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  dgc is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  

*/

#ifndef _PORT_H
#define _PORT_H

int port_strcasecmp(const char *s1, const char *s2);
int port_strncasecmp(const char *s1, const char *s2, size_t n);

#endif /* _PORT_H */

