/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *	    Chong Kai Xiong <descender@phreaker.net>
 *
 * $Id: lv_cpu.h,v 1.16 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_CPU_H
#define _LV_CPU_H

#include <libvisual/lvconfig.h>
#include <libvisual/lv_common.h>

VISUAL_BEGIN_DECLS;

/**
 * Enumerate containing the different architectual types.
 */
typedef enum {
	VISUAL_CPU_TYPE_MIPS,		/**< Running on the mips architecture. */
	VISUAL_CPU_TYPE_ALPHA,		/**< Running on the alpha architecture. */
	VISUAL_CPU_TYPE_SPARC,		/**< Running on the sparc architecture. */ 
	VISUAL_CPU_TYPE_X86,		/**< Running on the X86 architecture. */
	VISUAL_CPU_TYPE_POWERPC,	/**< Running on the PowerPC architecture. */
	VISUAL_CPU_TYPE_OTHER		/**< Running on an architecture that is not specified. */
} VisCPUType;

typedef struct _VisCPU VisCPU;

/**
 * This VisCPU structure contains information regarding the processor.
 *
 * @see visual_cpu_get_info
 */
struct _VisCPU {
	VisObject	object;			/**< The VisObject data. */
	VisCPUType	type;			/**< Contains the CPU/arch type. */
	int		nrcpu;			/**< Contains the number of CPUs in the system. */

	/* Feature flags */
	int		x86cpuType;		/**< The x86 cpu family type. */
	int		cacheline;		/**< The size of the cacheline. */

	int		hasTSC;			/**< The CPU has the tsc feature. */
	int		hasMMX;			/**< The CPU has the mmx feature. */
	int		hasMMX2;		/**< The CPU has the mmx2 feature. */
	int		hasSSE;			/**< The CPU has the sse feature. */
	int		hasSSE2;		/**< The CPU has the sse2 feature. */
	int		has3DNow;		/**< The CPU has the 3dnow feature. */
	int		has3DNowExt;		/**< The CPU has the 3dnowext feature. */
	int		hasAltiVec;		/**< The CPU has the altivec feature. */	

	int		enabledTSC;		/**< The tsc feature is enabled. */
	int		enabledMMX;		/**< The mmx feature is enabled. */ 
	int		enabledMMX2;		/**< The tsc feature is enabled. */
	int		enabledSSE;		/**< The sse feature is enabled. */
	int		enabledSSE2;		/**< The sse2 feature is enabled. */
	int		enabled3DNow;		/**< The 3dnow feature is enabled. */
	int		enabled3DNowExt;	/**< The 3dnowext feature is enabled. */ 
	int		enabledAltiVec;		/**< The altivec feature is enabled. */  
};

void visual_cpu_initialize (void);
VisCPU *visual_cpu_get_caps (void);

int visual_cpu_get_tsc (void);
int visual_cpu_get_mmx (void);
int visual_cpu_get_mmx2 (void);
int visual_cpu_get_sse (void);
int visual_cpu_get_sse2 (void);
int visual_cpu_get_3dnow (void);
int visual_cpu_get_3dnow2 (void);
int visual_cpu_get_altivec (void);

int visual_cpu_set_tsc (int enabled);
int visual_cpu_set_mmx (int enabled);
int visual_cpu_set_mmx2 (int enabled);
int visual_cpu_set_sse (int enabled);
int visual_cpu_set_sse2 (int enabled);
int visual_cpu_set_3dnow (int enabled);
int visual_cpu_set_3dnow2 (int enabled);
int visual_cpu_set_altivec (int enabled);

VISUAL_END_DECLS

#endif /* _LV_CPU_H */
