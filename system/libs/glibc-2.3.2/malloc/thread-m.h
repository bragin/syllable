/* Basic platform-independent macro definitions for mutexes and
   thread-specific data.
   Copyright (C) 1996,1997,1998,2000,2001,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Wolfram Gloger <wg@malloc.de>, 2001.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* $Id$
   One out of _LIBC, USE_PTHREADS, USE_THR or USE_SPROC should be
   defined, otherwise the token NO_THREADS and dummy implementations
   of the macros will be defined.  */

#ifndef _THREAD_M_H
#define _THREAD_M_H

#undef thread_atfork_static

#if defined(_LIBC) /* The GNU C library, a special case of Posix threads */

#include <bits/libc-lock.h>

#ifdef PTHREAD_MUTEX_INITIALIZER

__libc_lock_define (typedef, mutex_t)

#if (defined(LLL_LOCK_INITIALIZER) && !defined(NOT_IN_libc)) || defined __SYLLABLE__

/* Assume NPTL.  */

#define mutex_init(m)		__libc_lock_init (*(m))
#define mutex_lock(m)		__libc_lock_lock (*(m))
#define mutex_trylock(m)	__libc_lock_trylock (*(m))
#define mutex_unlock(m)		__libc_lock_unlock (*(m))

#elif defined(__libc_maybe_call2)

#define mutex_init(m)		\
  __libc_maybe_call2 (pthread_mutex_init, (m, NULL), (*(int *)(m) = 0))
#define mutex_lock(m)		\
  __libc_maybe_call2 (pthread_mutex_lock, (m), ((*(int *)(m) = 1), 0))
#define mutex_trylock(m)	\
  __libc_maybe_call2 (pthread_mutex_trylock, (m), \
		      (*(int *)(m) ? 1 : ((*(int *)(m) = 1), 0)))
#define mutex_unlock(m)		\
  __libc_maybe_call2 (pthread_mutex_unlock, (m), (*(int *)(m) = 0))

#else

#define mutex_init(m)		\
  __libc_maybe_call (__pthread_mutex_init, (m, NULL), (*(int *)(m) = 0))
#define mutex_lock(m)		\
  __libc_maybe_call (__pthread_mutex_lock, (m), ((*(int *)(m) = 1), 0))
#define mutex_trylock(m)	\
  __libc_maybe_call (__pthread_mutex_trylock, (m), \
		     (*(int *)(m) ? 1 : ((*(int *)(m) = 1), 0)))
#define mutex_unlock(m)		\
  __libc_maybe_call (__pthread_mutex_unlock, (m), (*(int *)(m) = 0))

#endif

/* This is defined by newer gcc version unique for each module.  */
extern void *__dso_handle __attribute__ ((__weak__));

#include <fork.h>

#ifdef SHARED
# define thread_atfork(prepare, parent, child) \
   __register_atfork (prepare, parent, child, __dso_handle)
#else
# define thread_atfork(prepare, parent, child) \
   __register_atfork (prepare, parent, child,				      \
		      &__dso_handle == NULL ? NULL : __dso_handle)
#endif

#elif defined(MUTEX_INITIALIZER)
/* Assume hurd, with cthreads */

/* Cthreads `mutex_t' is a pointer to a mutex, and malloc wants just the
   mutex itself.  */
#undef mutex_t
#define mutex_t struct mutex

#undef mutex_init
#define mutex_init(m) (__mutex_init(m), 0)

#undef mutex_lock
#define mutex_lock(m) (__mutex_lock(m), 0)

#undef mutex_unlock
#define mutex_unlock(m) (__mutex_unlock(m), 0)

#define mutex_trylock(m) (!__mutex_trylock(m))

#define thread_atfork(prepare, parent, child) do {} while(0)
#define thread_atfork_static(prepare, parent, child) \
 text_set_element(_hurd_fork_prepare_hook, prepare); \
 text_set_element(_hurd_fork_parent_hook, parent); \
 text_set_element(_hurd_fork_child_hook, child);

/* No we're *not* using pthreads.  */
#define __pthread_initialize ((void (*)(void))0)

#else

#define NO_THREADS

#endif /* MUTEX_INITIALIZER && PTHREAD_MUTEX_INITIALIZER */

#ifndef NO_THREADS

/* thread specific data for glibc */

#include <bits/libc-tsd.h>

typedef int tsd_key_t[1];	/* no key data structure, libc magic does it */
__libc_tsd_define (static, MALLOC)	/* declaration/common definition */
#define tsd_key_create(key, destr)	((void) (key))
#define tsd_setspecific(key, data)	__libc_tsd_set (MALLOC, (data))
#define tsd_getspecific(key, vptr)	((vptr) = __libc_tsd_get (MALLOC))

#endif

#elif defined(USE_PTHREADS) /* Posix threads */

#include <pthread.h>

/* mutex */
#if (defined __i386__ || defined __x86_64__) && defined __GNUC__ && \
    !defined USE_NO_SPINLOCKS

#include <time.h>

/* Use fast inline spinlocks.  */
typedef struct {
  volatile unsigned int lock;
  int pad0_;
} mutex_t;

#define mutex_init(m)              ((m)->lock = 0)
static inline int mutex_lock(mutex_t *m) {
  int cnt = 0, r;
  struct timespec tm;

  for(;;) {
    __asm__ __volatile__
      ("xchgl %0, %1"
       : "=r"(r), "=m"(m->lock)
       : "0"(1), "m"(m->lock)
       : "memory");
    if(!r)
      return 0;
    if(cnt < 50) {
      sched_yield();
      cnt++;
    } else {
      tm.tv_sec = 0;
      tm.tv_nsec = 2000001;
      nanosleep(&tm, NULL);
      cnt = 0;
    }
  }
}
static inline int mutex_trylock(mutex_t *m) {
  int r;

  __asm__ __volatile__
    ("xchgl %0, %1"
     : "=r"(r), "=m"(m->lock)
     : "0"(1), "m"(m->lock)
     : "memory");
  return r;
}
static inline int mutex_unlock(mutex_t *m) {
  m->lock = 0;
  __asm __volatile ("" : "=m" (m->lock) : "0" (m->lock));
  return 0;
}

#else

/* Normal pthread mutex.  */
typedef pthread_mutex_t mutex_t;

#define mutex_init(m)              pthread_mutex_init(m, NULL)
#define mutex_lock(m)              pthread_mutex_lock(m)
#define mutex_trylock(m)           pthread_mutex_trylock(m)
#define mutex_unlock(m)            pthread_mutex_unlock(m)

#endif /* (__i386__ || __x86_64__) && __GNUC__ && !USE_NO_SPINLOCKS */

/* thread specific data */
#if defined(__sgi) || defined(USE_TSD_DATA_HACK)

/* Hack for thread-specific data, e.g. on Irix 6.x.  We can't use
   pthread_setspecific because that function calls malloc() itself.
   The hack only works when pthread_t can be converted to an integral
   type. */

typedef void *tsd_key_t[256];
#define tsd_key_create(key, destr) do { \
  int i; \
  for(i=0; i<256; i++) (*key)[i] = 0; \
} while(0)
#define tsd_setspecific(key, data) \
 (key[(unsigned)pthread_self() % 256] = (data))
#define tsd_getspecific(key, vptr) \
 (vptr = key[(unsigned)pthread_self() % 256])

#else

typedef pthread_key_t tsd_key_t;

#define tsd_key_create(key, destr) pthread_key_create(key, destr)
#define tsd_setspecific(key, data) pthread_setspecific(key, data)
#define tsd_getspecific(key, vptr) (vptr = pthread_getspecific(key))

#endif

/* at fork */
#define thread_atfork(prepare, parent, child) \
                                   pthread_atfork(prepare, parent, child)

#elif USE_THR /* Solaris threads */

#include <thread.h>

#define mutex_init(m)              mutex_init(m, USYNC_THREAD, NULL)

/*
 * Hack for thread-specific data on Solaris.  We can't use thr_setspecific
 * because that function calls malloc() itself.
 */
typedef void *tsd_key_t[256];
#define tsd_key_create(key, destr) do { \
  int i; \
  for(i=0; i<256; i++) (*key)[i] = 0; \
} while(0)
#define tsd_setspecific(key, data) (key[(unsigned)thr_self() % 256] = (data))
#define tsd_getspecific(key, vptr) (vptr = key[(unsigned)thr_self() % 256])

#define thread_atfork(prepare, parent, child) do {} while(0)

#elif USE_SPROC /* SGI sproc() threads */

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <abi_mutex.h>

typedef abilock_t mutex_t;

#define mutex_init(m)              init_lock(m)
#define mutex_lock(m)              (spin_lock(m), 0)
#define mutex_trylock(m)           acquire_lock(m)
#define mutex_unlock(m)            release_lock(m)

typedef int tsd_key_t;
int tsd_key_next;
#define tsd_key_create(key, destr) ((*key) = tsd_key_next++)
#define tsd_setspecific(key, data) (((void **)(&PRDA->usr_prda))[key] = data)
#define tsd_getspecific(key, vptr) (vptr = ((void **)(&PRDA->usr_prda))[key])

#define thread_atfork(prepare, parent, child) do {} while(0)

#else /* no _LIBC or USE_... are defined */

#define NO_THREADS

#endif /* defined(_LIBC) */

#ifdef NO_THREADS /* No threads, provide dummy macros */

/* The mutex functions used to do absolutely nothing, i.e. lock,
   trylock and unlock would always just return 0.  However, even
   without any concurrently active threads, a mutex can be used
   legitimately as an `in use' flag.  To make the code that is
   protected by a mutex async-signal safe, these macros would have to
   be based on atomic test-and-set operations, for example. */
typedef int mutex_t;

#define mutex_init(m)              (*(m) = 0)
#define mutex_lock(m)              ((*(m) = 1), 0)
#define mutex_trylock(m)           (*(m) ? 1 : ((*(m) = 1), 0))
#define mutex_unlock(m)            (*(m) = 0)

typedef void *tsd_key_t;
#define tsd_key_create(key, destr) do {} while(0)
#define tsd_setspecific(key, data) ((key) = (data))
#define tsd_getspecific(key, vptr) (vptr = (key))

#define thread_atfork(prepare, parent, child) do {} while(0)

#endif /* defined(NO_THREADS) */

#endif /* !defined(_THREAD_M_H) */
