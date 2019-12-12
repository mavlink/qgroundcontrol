/* GStreamer
* Copyright (C) 2009 Wim Taymans <wim.taymans@gmail.com>
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


#ifndef __GST_RTP_J2K_COMMON_H__
#define __GST_RTP_J2K_COMMON_H__



/* Sampling values from RFC 5371 for JPEG 2000 over RTP : https://datatracker.ietf.org/doc/rfc5371/C

RGB:  standard Red, Green, Blue color space.

BGR:  standard Blue, Green, Red color space.

RGBA:  standard Red, Green, Blue, Alpha color space.

BGRA:  standard Blue, Green, Red, Alpha color space.

YCbCr-4:4:4:  standard YCbCr color space; no subsampling.

YCbCr-4:2:2:  standard YCbCr color space; Cb and Cr are subsampled horizontally by 1/2.

YCbCr-4:2:0:  standard YCbCr color space; Cb and Cr are subsampled horizontally and vertically by 1/2.

YCbCr-4:1:1:  standard YCbCr color space; Cb and Cr are subsampled vertically by 1/4.

GRAYSCALE:  basically, a single component image of just multilevels of grey.
*/


#define GST_RTP_J2K_RGB       "RGB"
#define GST_RTP_J2K_BGR       "BGR"
#define GST_RTP_J2K_RGBA      "RGBA"
#define GST_RTP_J2K_BGRA      "BGRA"
#define GST_RTP_J2K_YBRA   	  "YCbCrA"
#define GST_RTP_J2K_YBR444    "YCbCr-4:4:4"
#define GST_RTP_J2K_YBR422    "YCbCr-4:2:2"
#define GST_RTP_J2K_YBR420    "YCbCr-4:2:0"
#define GST_RTP_J2K_YBR410    "YCbCr-4:1:0"
#define GST_RTP_J2K_GRAYSCALE "GRAYSCALE"

#define GST_RTP_J2K_SAMPLING_LIST "sampling = (string) {\"RGB\", \"BGR\", \"RGBA\", \"BGRA\", \"YCbCrA\", \"YCbCr-4:4:4\", \"YCbCr-4:2:2\", \"YCbCr-4:2:0\", \"YCbCr-4:1:1\", \"GRAYSCALE\"}"

typedef enum
{

  GST_RTP_SAMPLING_NONE,
  GST_RTP_SAMPLING_RGB,
  GST_RTP_SAMPLING_BGR,
  GST_RTP_SAMPLING_RGBA,
  GST_RTP_SAMPLING_BGRA,
  GST_RTP_SAMPLING_YBRA,
  GST_RTP_SAMPLING_YBR444,
  GST_RTP_SAMPLING_YBR422,
  GST_RTP_SAMPLING_YBR420,
  GST_RTP_SAMPLING_YBR410,
  GST_RTP_SAMPLING_GRAYSCALE
} GstRtpSampling;


/*
* GstRtpJ2KMarker:
* @GST_J2K_MARKER: Prefix for JPEG 2000 marker
* @GST_J2K_MARKER_SOC: Start of Codestream
* @GST_J2K_MARKER_SOT: Start of tile
* @GST_J2K_MARKER_EOC: End of Codestream
*
* Identifiers for markers in JPEG 2000 code streams
*/
typedef enum
{
  GST_J2K_MARKER = 0xFF,
  GST_J2K_MARKER_SOC = 0x4F,
  GST_J2K_MARKER_SOT = 0x90,
  GST_J2K_MARKER_SOP = 0x91,
  GST_J2K_MARKER_EPH = 0x92,
  GST_J2K_MARKER_SOD = 0x93,
  GST_J2K_MARKER_EOC = 0xD9
} GstRtpJ2KMarker;


#define GST_RTP_J2K_HEADER_SIZE 8


#endif /* __GST_RTP_J2K_COMMON_H__ */
