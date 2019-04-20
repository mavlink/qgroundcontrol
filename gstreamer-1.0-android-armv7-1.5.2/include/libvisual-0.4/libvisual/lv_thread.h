#ifndef _LV_THREAD_H
#define _LV_THREAD_H

#include <libvisual/lvconfig.h>
#include <libvisual/lv_common.h>

#include "lvconfig.h"

#if defined(VISUAL_OS_WIN32)
#include <windows.h>
#endif

#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
#include <pthread.h>
#elif defined(VISUAL_THREAD_MODEL_GTHREAD2) /* !VISUAL_THREAD_MODEL_POSIX */
#include <glib/gthread.h>
#else /* !VISUAL_THREAD_MODEL_GTHREAD2 */

#endif
#endif /* VISUAL_HAVE_THREADS */

VISUAL_BEGIN_DECLS

typedef struct _VisThread VisThread;
typedef struct _VisMutex VisMutex;

/**
 * The function defination for a function that forms the base of a new VisThread when
 * visual_thread_create is used.
 *
 * @see visual_thread_create
 *
 * @arg data Pointer that can contains the private data from the visual_thread_create function.
 *
 * @return Pointer to the data when a thread is joined.
 */
typedef void *(*VisThreadFunc)(void *data);

/**
 * The VisThread data structure and the VisThread subsystem is a wrapper system for native
 * threading implementations.
 */
struct _VisThread {
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	pthread_t thread;		/**< Private used for the pthread implementation. */
#elif defined(VISUAL_THREAD_MODEL_WIN32) /* !VISUAL_THREAD_MODEL_POSIX */
	HANDLE thread;
	DWORD threadId;
#elif defined(VISUAL_THREAD_MODEL_GTHREAD) /* !VISUAL_THREAD_MODEL_WIN32 */
	GThread *thread;
#endif
#endif /* VISUAL_HAVE_THREADS */
};

/**
 * The VisMutex data structure and the VisMutex subsystem is a wrapper system for native
 * thread locking implementations.
 */
struct _VisMutex {
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	pthread_mutex_t mutex;		/**< Private used for the pthreads implementation. */
#elif defined(VISUAL_THREAD_MODEL_WIN32) /* !VISUAL_THREAD_MODEL_POSIX */

#elif defined(VISUAL_THREAD_MODEL_GTHREAD) /* !VISUAL_THREAD_MODEL_WIN32 */
	GMutex *mutex;

	GStaticMutex static_mutex;
	int static_mutex_used;
#endif
#endif /* VISUAL_HAVE_THREADS */
};

int visual_thread_initialize (void);
int visual_thread_is_initialized (void);

void visual_thread_enable (int enabled);
int visual_thread_is_enabled (void);

int visual_thread_is_supported (void);

VisThread *visual_thread_create (VisThreadFunc func, void *data, int joinable);
int visual_thread_free (VisThread *thread);
void *visual_thread_join (VisThread *thread);
void visual_thread_exit (void *retval);
void visual_thread_yield (void);

VisMutex *visual_mutex_new (void);
int visual_mutex_free (VisMutex *mutex);
int visual_mutex_init (VisMutex *mutex);
int visual_mutex_lock (VisMutex *mutex);
int visual_mutex_trylock (VisMutex *mutex);
int visual_mutex_unlock (VisMutex *mutex);

VISUAL_END_DECLS

#endif /* _LV_THREAD_H */
