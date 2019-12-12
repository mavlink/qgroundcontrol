/* Smoke Codec
 * Copyright (C) <2004> Wim Taymans <wim@fluendo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef __SMOKECODEC_H__
#define __SMOKECODEC_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _SmokeCodecInfo SmokeCodecInfo;

typedef enum {
  SMOKECODEC_WRONGVERSION       = -5,
  SMOKECODEC_WRONGSIZE          = -4,
  SMOKECODEC_ERROR              = -3,
  SMOKECODEC_NOMEM              = -2,
  SMOKECODEC_NULLPTR            = -1,
  SMOKECODEC_OK                 =  0 
} SmokeCodecResult;
        
typedef enum {
  SMOKECODEC_KEYFRAME           = (1<<0),
  SMOKECODEC_MOTION_VECTORS     = (1<<1)
} SmokeCodecFlags;
        
#define SMOKECODEC_ID_STRING "smoke"

typedef enum {
  SMOKECODEC_TYPE_ID            = 0x80,
  SMOKECODEC_TYPE_COMMENT       = 0x81,
  SMOKECODEC_TYPE_EXTRA         = 0x83,
  SMOKECODEC_TYPE_DATA          = 0x40 
} SmokePacketType;

/* init */
int                     smokecodec_encode_new   (SmokeCodecInfo **info,
                                                 const unsigned int width,
                                                 const unsigned int height,
                                                 const unsigned int fps_num,
                                                 const unsigned int fps_denom);

int                     smokecodec_decode_new   (SmokeCodecInfo **info);

int                     smokecodec_info_free    (SmokeCodecInfo * info);

/* config */
SmokeCodecResult        smokecodec_set_quality  (SmokeCodecInfo *info,
                                                 const unsigned int min,
                                                 const unsigned int max);
SmokeCodecResult        smokecodec_get_quality  (SmokeCodecInfo *info,
                                                 unsigned int *min,
                                                 unsigned int *max);

SmokeCodecResult        smokecodec_set_threshold (SmokeCodecInfo *info,
                                                 const unsigned int threshold);
SmokeCodecResult        smokecodec_get_threshold (SmokeCodecInfo *info,
                                                 unsigned int *threshold);

SmokeCodecResult        smokecodec_set_bitrate  (SmokeCodecInfo *info,
                                                 const unsigned int bitrate);
SmokeCodecResult        smokecodec_get_bitrate  (SmokeCodecInfo *info,
                                                 unsigned int *bitrate);

/* encoding */
SmokeCodecResult        smokecodec_encode_id    (SmokeCodecInfo *info,
                                                 unsigned char *out,
                                                 unsigned int *outsize);

SmokeCodecResult        smokecodec_encode       (SmokeCodecInfo *info,
                                                 const unsigned char *in,
                                                 SmokeCodecFlags flags,
                                                 unsigned char *out,
                                                 unsigned int *outsize);

/* decoding */
SmokeCodecResult        smokecodec_parse_id     (SmokeCodecInfo *info,
                                                 const unsigned char *in,
                                                 const unsigned int insize);

SmokeCodecResult        smokecodec_parse_header (SmokeCodecInfo *info,
                                                 const unsigned char *in,
                                                 const unsigned int insize,
                                                 SmokeCodecFlags *flags,
                                                 unsigned int *width,
                                                 unsigned int *height,
                                                 unsigned int *fps_num,
                                                 unsigned int *fps_denom);

SmokeCodecResult        smokecodec_decode       (SmokeCodecInfo *info,
                                                 const unsigned char *in,
                                                 const unsigned int insize,
                                                 unsigned char *out);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __SMOKECODEC_H__ */
