/* Copyright (C) 1991, 94, 95, 97, 98 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <errno.h>
#include <hurd.h>
#include <unistd.h>

/* Extend the process's data space by INCREMENT.
   If INCREMENT is negative, shrink data space by - INCREMENT.
   Return the address of the start of the new data space, or -1 for errors.  */
void *
__sbrk (int increment)
{
  void *result;

  HURD_CRITICAL_BEGIN;
  __mutex_lock (&_hurd_brk_lock);
  result = (void *) _hurd_brk;
  if (increment != 0 && _hurd_set_brk (_hurd_brk + increment) < 0)
    result = (void *) -1;
  __mutex_unlock (&_hurd_brk_lock);
  HURD_CRITICAL_END;

  return result;
}

weak_alias (__sbrk, sbrk)
