/* Libvisual - The audio visualisation framework.
 *
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *	    Chong Kai Xiong <descender@phreaker.net>
 *
 * $Id: lv_fourier.h,v 1.8 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_FOURIER_H
#define _LV_FOURIER_H

#include <libvisual/lv_common.h>

VISUAL_BEGIN_DECLS

#define VISUAL_DFT(obj)					(VISUAL_CHECK_CAST ((obj), VisDFT))

typedef struct _VisDFT VisDFT;

/**
 * Private structure to embed Fourier Transform states in.
 */
struct _VisDFT {
	VisObject	 object;			/**< The VisObject data. */
	unsigned int	 samples_in;			/**< The number of input samples. */
	unsigned int	 spectrum_size;			/**< The size of the spectrum (power of two). */
	float		*real;				/**< Private data that is used by the fourier engine. */
	float		*imag;				/**< Private data that is used by the fourier engine. */
	int		 brute_force;			/**< Private data that is used by the fourier engine. */
};

/* prototypes */
int visual_fourier_initialize (void);
int visual_fourier_is_initialized (void);
int visual_fourier_deinitialize (void);

VisDFT *visual_dft_new (unsigned int samples_out, unsigned int samples_in);
int visual_dft_init (VisDFT *fourier, unsigned int samples_out, unsigned int samples_in);

int visual_dft_perform (VisDFT *fourier, float *output, float *input);
int visual_dft_log_scale (float *output, float *input, int size);
int visual_dft_log_scale_standard (float *output, float *input, int size);
int visual_dft_log_scale_custom (float *output, float *input, int size, float log_scale_divisor);

VISUAL_END_DECLS

#endif /* _LV_FOURIER_H */
