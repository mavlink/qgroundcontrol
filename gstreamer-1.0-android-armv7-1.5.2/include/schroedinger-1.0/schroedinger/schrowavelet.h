
#ifndef _SCHRO_WAVELET_H_
#define _SCHRO_WAVELET_H_

#include <schroedinger/schroutils.h>
#include <schroedinger/schroframe.h>

SCHRO_BEGIN_DECLS

#ifdef SCHRO_ENABLE_UNSTABLE_API

void schro_wavelet_transform_2d (SchroFrameData *fd, int type, int16_t *tmp);
void schro_wavelet_inverse_transform_2d (SchroFrameData *fd_dest,
    SchroFrameData *fd_src, int type, int16_t *tmp);

#endif

SCHRO_END_DECLS

#endif

