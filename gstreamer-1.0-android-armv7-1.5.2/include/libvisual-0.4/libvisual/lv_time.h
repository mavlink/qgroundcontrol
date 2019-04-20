/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_time.h,v 1.21 2006/02/05 18:45:57 synap Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_TIME_H
#define _LV_TIME_H

#include <sys/time.h>
#include <time.h>

#include <libvisual/lv_common.h>

VISUAL_BEGIN_DECLS

#define VISUAL_USEC_PER_SEC	1000000
#define VISUAL_MSEC_PER_SEC	1000

#define VISUAL_TIME(obj)				(VISUAL_CHECK_CAST ((obj), VisTime))
#define VISUAL_TIMER(obj)				(VISUAL_CHECK_CAST ((obj), VisTimer))

typedef struct _VisTime VisTime;
typedef struct _VisTimer VisTimer;

/**
 * The VisTime structure can contain seconds and microseconds for timing purpose.
 */
struct _VisTime {
	VisObject	object;		/**< The VisObject data. */
	long		tv_sec;		/**< seconds. */
	long		tv_usec;	/**< microseconds. */
};

/**
 * The VisTimer structure is used for timing using the visual_timer_* methods.
 */
struct _VisTimer {
	VisObject	object;		/**< The VisObject data. */
	VisTime		start;		/**< Private entry to indicate the starting point,
					 * Shouldn't be read for reliable starting point because
					 * the visual_timer_continue method changes it's value. */
	VisTime		stop;		/**< Private entry to indicate the stopping point. */

	int		active;		/**< Private entry to indicate if the timer is currently active or inactive. */
};

/* prototypes */
VisTime *visual_time_new (void);
int visual_time_init (VisTime *time_);
int visual_time_get (VisTime *time_);
int visual_time_set (VisTime *time_, long sec, long usec);
int visual_time_difference (VisTime *dest, VisTime *time1, VisTime *time2);
int visual_time_past (VisTime *time_, VisTime *past);
int visual_time_copy (VisTime *dest, VisTime *src);
int visual_time_usleep (unsigned long microseconds);

VisTimer *visual_timer_new (void);
int visual_timer_init (VisTimer *timer);
int visual_timer_reset (VisTimer *timer);
int visual_timer_is_active (VisTimer *timer);
int visual_timer_start (VisTimer *timer);
int visual_timer_stop (VisTimer *timer);
int visual_timer_continue (VisTimer *timer);
int visual_timer_elapsed (VisTimer *timer, VisTime *time_);
int visual_timer_elapsed_msecs (VisTimer *timer);
int visual_timer_elapsed_usecs (VisTimer *timer);
int visual_timer_has_passed (VisTimer *timer, VisTime *time_);
int visual_timer_has_passed_by_values (VisTimer *timer, long sec, long usec);

/* FIXME: does this work everywhere (x86) ? Check the cycle.h that can be found in FFTW,
 * also check liboil it's profile header: add powerpc support */

/**
 * This function can be used to retrieve the real time stamp counter. This function
 * will not check if timestamping is around, the reason for this is because it will make
 * the timing less reliable since checking for timestamping gives overhead. You
 * must make arrangments for this.
 *
 * @see visual_cpu_get_tsc
 *
 * @param lo The lower 32 bits of the timestmap.
 * @param hi The higher 32 bits of the timestamp.
 *
 * @return Nothing.
 */
static inline void visual_timer_tsc_get (uint32_t *lo, uint32_t *hi)
{
#if defined(VISUAL_ARCH_X86) || defined(VISUAL_ARCH_X86_64)
	__asm __volatile
		("\n\t cpuid"
		 "\n\t rdtsc"
		 "\n\t movl %%edx, %0"
		 "\n\t movl %%eax, %1"
		 : "=r" (*hi), "=r" (*lo)
		 :: "memory");
#endif
}

/* FIXME use uint64_t here, make sure type exists */
static inline unsigned long long visual_timer_tsc_get_returned ()
{
	uint32_t lo, hi;

	visual_timer_tsc_get (&lo, &hi);

	return ((unsigned long long) hi << 32) | lo;
}

VISUAL_END_DECLS

#endif /* _LV_TIME_H */
