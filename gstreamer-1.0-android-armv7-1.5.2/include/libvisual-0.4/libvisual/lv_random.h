/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 * 	    Vitaly V. Bursov <vitalyvb@ukr.net>
 *
 * $Id: lv_random.h,v 1.15 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_RANDOM_H
#define _LV_RANDOM_H

#include <libvisual/lv_common.h>

VISUAL_BEGIN_DECLS

#define VISUAL_RANDOMCONTEXT(obj)			(VISUAL_CHECK_CAST ((obj), VisRandomContext))

/**
 * The highest random nummer.
 */
#define VISUAL_RANDOM_MAX	4294967295U

typedef struct _VisRandomContext VisRandomContext;

/**
 * The VisRandomContext data structure is used to keep track of
 * the randomizer it's state. When state tracking is used you need
 * to use the visual_random_context_* functions.
 */
struct _VisRandomContext {
	VisObject	object;		/**< The VisObject data. */
	uint32_t	seed;		/**< The initial random seed. */
	uint32_t	seed_state;	/**< The current state seed. */
};

/* Non context random macros */
extern VisRandomContext __lv_internal_random_context;
#define visual_random_set_seed(a) visual_random_context_set_seed(&__lv_internal_random_context, a)
#define visual_random_get_seed() visual_random_context_get_seed(&__lv_internal_random_context)
#define visual_random_int() visual_random_context_int(&__lv_internal_random_context)
#define visual_random_int_range(a, b) visual_random_context_int(&__lv_internal_random_context, a, b)
#define visual_random_double () visual_random_context_double(&__lv_internal_random_context);
#define visual_random_float () visual_random_context_float(&__lv_internal_random_context);
#define visual_random_decide(a) visual_random_int(&__lv_internal_random_context, a)

/* Context management */
VisRandomContext *visual_random_context_new (uint32_t seed);
int visual_random_context_init (VisRandomContext *rcontext, uint32_t seed);
int visual_random_context_set_seed (VisRandomContext *rcontext, uint32_t seed);
uint32_t visual_random_context_get_seed (VisRandomContext *rcontext);
uint32_t visual_random_context_get_seed_state (VisRandomContext *rcontext);

/* Context random functions */
uint32_t visual_random_context_int (VisRandomContext *rcontext);
uint32_t visual_random_context_int_range (VisRandomContext *rcontext, int min, int max);
double visual_random_context_double (VisRandomContext *rcontext);
float visual_random_context_float (VisRandomContext *rcontext);
int visual_random_context_decide (VisRandomContext *rcontext, float a);

VISUAL_END_DECLS

#endif /* _LV_RANDOM_H */
