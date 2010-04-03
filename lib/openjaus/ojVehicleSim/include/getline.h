/* getline.c -- Replacement for GNU C library function getline

Copyright (C) 1993 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.  */

/* Written by Jan Brittenson, bson@gnu.ai.mit.edu.  */

/*
 * Modified for WinCvs/MacCVS : Alexandre Parenteau <aubonbeurre@hotmail.com> --- April 1998
 */

#ifndef _getline_h_
#define _getline_h_ 1

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__GNUC__) || (defined (__STDC__) && __STDC__) || defined(__cplusplus)
#define __PROTO(args) args
#else
#define __PROTO(args) ()
#endif  /* GCC.  */

#ifndef __ssize_t_defined
#define __ssize_t int
#endif
__ssize_t
  getline __PROTO ((char **_lineptr, size_t *_n, FILE *_stream));
int
  getstr __PROTO ((char **_lineptr, size_t *_n, FILE *_stream,
		   char _terminator, int _offset));

#ifdef __cplusplus
}
#endif

#endif /* _getline_h_ */

