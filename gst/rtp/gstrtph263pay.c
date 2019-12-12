/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
 * Copyright (C) <2008> Dejan Sakelsak <dejan.sakelsak@marand.si>
 * Copyright (C) <2009> Janin Kolenc  <janin.kolenc@marand.si>
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
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>

#include "gstrtph263pay.h"
#include "gstrtputils.h"

typedef enum
{
  GST_H263_FRAME_TYPE_I = 0,
  GST_H263_FRAME_TYPE_P = 1,
  GST_H263_FRAME_TYPE_PB = 2
} GstRtpH263PayFrameType;

typedef enum
{
  GST_RTP_H263_PAYLOAD_PICTURE_FORMAT_RES1 = 0,
  GST_RTP_H263_PAYLOAD_PICTURE_FORMAT_SQCIF = 1,
  GST_RTP_H263_PAYLOAD_PICTURE_FORMAT_QCIF = 2,
  GST_RTP_H263_PAYLOAD_PICTURE_FORMAT_CIF = 3,
  GST_RTP_H263_PAYLOAD_PICTURE_FORMAT_4CIF = 4,
  GST_RTP_H263_PAYLOAD_PICTURE_FORMAT_16CIF = 5,
  GST_RTP_H263_PAYLOAD_PICTURE_FORMAT_RES2 = 6,
  GST_H263_PAYLOAD_PICTURE_FORMAT_PLUS = 7
} GstRtpH263PayPictureFormat;

static const guint format_props[8][2] = { {254, 254},
{6, 8},
{9, 11},
{18, 22},
{18, 88},
{18, 352},
{254, 254},
{255, 255}
};

/*
 * I-frame MCBPC table: code, mask, nbits, cb, cr, mb type -> 10 = undefined (because we have guint)
 */
#define MCBPC_I_LEN 9
#define MCBPC_I_WID 6
static const guint32 mcbpc_I[9][6] = {
  {0x8000, 0x8000, 1, 0, 0, 3},
  {0x2000, 0xe000, 3, 0, 1, 3},
  {0x4000, 0xe000, 3, 1, 0, 3},
  {0x6000, 0xe000, 3, 1, 1, 3},
  {0x1000, 0xf000, 4, 0, 0, 4},
  {0x0400, 0xfc00, 6, 0, 1, 4},
  {0x0800, 0xfc00, 6, 1, 0, 4},
  {0x0c00, 0xfc00, 6, 1, 1, 4},
  {0x0080, 0xff80, 9, 10, 10, 10}
};

/*
 * P-frame MCBPC table: code, mask, nbits, cb, cr, mb type -> 10 = undefined (because we have guint)
 */
#define MCBPC_P_LEN 21
#define MCBPC_P_WID 6
static const guint16 mcbpc_P[21][6] = {
  {0x8000, 0x8000, 1, 0, 0, 0},
  {0x3000, 0xf000, 4, 0, 1, 0},
  {0x2000, 0xf000, 4, 1, 0, 0},
  {0x1400, 0xfc00, 6, 1, 1, 0},
  {0x6000, 0xe000, 3, 0, 0, 1},
  {0x0e00, 0xfe00, 7, 0, 1, 1},
  {0x0c00, 0xfe00, 7, 1, 0, 1},
  {0x0280, 0xff80, 9, 1, 1, 1},
  {0x4000, 0xe000, 3, 0, 0, 2},
  {0x0a00, 0xfe00, 7, 0, 1, 2},
  {0x0800, 0xfe00, 7, 1, 0, 2},
  {0x0500, 0xff00, 8, 1, 1, 2},
  {0x1800, 0xf800, 5, 0, 0, 3},
  {0x0400, 0xff00, 8, 0, 1, 3},
  {0x0300, 0xff00, 8, 1, 0, 3},
  {0x0600, 0xfe00, 7, 1, 1, 3},
  {0x1000, 0xfc00, 6, 0, 0, 4},
  {0x0200, 0xff80, 9, 0, 1, 4},
  {0x0180, 0xff80, 9, 1, 0, 4},
  {0x0100, 0xff80, 9, 1, 1, 4},
  {0x0080, 0xff80, 9, 10, 10, 10}
};

/*
 * I-frame CBPY (code, mask, nbits, Y0, Y1, Y2, Y3)
 */
#define CBPY_LEN 16
#define CBPY_WID 7
static const guint8 cbpy_I[16][7] = {
  {0x30, 0xf0, 4, 0, 0, 0, 0},
  {0x28, 0xf8, 5, 0, 0, 0, 1},
  {0x20, 0xf8, 5, 0, 0, 1, 0},
  {0x90, 0xf0, 4, 0, 0, 1, 1},
  {0x18, 0xf8, 5, 0, 1, 0, 0},
  {0x70, 0xf0, 4, 0, 1, 0, 1},
  {0x08, 0xfc, 6, 0, 1, 1, 0},
  {0xb0, 0xf0, 4, 0, 1, 1, 1},
  {0x10, 0xf8, 5, 1, 0, 0, 0},
  {0x0c, 0xfc, 6, 1, 0, 0, 1},
  {0x50, 0xf0, 4, 1, 0, 1, 0},
  {0xa0, 0xf0, 4, 1, 0, 1, 1},
  {0x40, 0xf0, 4, 1, 1, 0, 0},
  {0x80, 0xf0, 4, 1, 1, 0, 1},
  {0x60, 0xf0, 4, 1, 1, 1, 0},
  {0xc0, 0xc0, 2, 1, 1, 1, 1}
};

/*
 * P-frame CBPY (code, mask, nbits, Y0, Y1, Y2, Y3)
 */
static const guint8 cbpy_P[16][7] = {
  {0x30, 0xf0, 4, 1, 1, 1, 1},
  {0x28, 0xf8, 5, 1, 1, 1, 0},
  {0x20, 0xf8, 5, 1, 1, 0, 1},
  {0x90, 0xf0, 4, 1, 1, 0, 0},
  {0x18, 0xf8, 5, 1, 0, 1, 1},
  {0x70, 0xf0, 4, 1, 0, 1, 0},
  {0x08, 0xfc, 6, 1, 0, 0, 1},
  {0xb0, 0xf0, 4, 1, 0, 0, 0},
  {0x10, 0xf8, 5, 0, 1, 1, 1},
  {0x0c, 0xfc, 6, 0, 1, 1, 0},
  {0x50, 0xf0, 4, 0, 1, 0, 1},
  {0xa0, 0xf0, 4, 0, 1, 0, 0},
  {0x40, 0xf0, 4, 0, 0, 1, 1},
  {0x80, 0xf0, 4, 0, 0, 1, 0},
  {0x60, 0xf0, 4, 0, 0, 0, 1},
  {0xc0, 0xc0, 2, 0, 0, 0, 0}
};

/*
 * Block TCOEF table (code, mask, nbits, LAST, RUN, LEVEL)
 */
#define TCOEF_LEN 103
#define TCOEF_WID 6
static const guint16 tcoef[103][6] = {
  {0x8000, 0xc000, 3, 0, 0, 1},
  {0xf000, 0xf000, 5, 0, 0, 2},
  {0x5400, 0xfc00, 7, 0, 0, 3},
  {0x2e00, 0xfe00, 8, 0, 0, 4},
  {0x1f00, 0xff00, 9, 0, 0, 5},
  {0x1280, 0xff80, 10, 0, 0, 6},
  {0x1200, 0xff80, 10, 0, 0, 7},
  {0x0840, 0xffc0, 11, 0, 0, 8},
  {0x0800, 0xffc0, 11, 0, 0, 9},
  {0x00e0, 0xffe0, 12, 0, 0, 10},       //10
  {0x00c0, 0xffe0, 12, 0, 0, 11},
  {0x0400, 0xffe0, 12, 0, 0, 12},
  {0xc000, 0xe000, 4, 0, 1, 1},
  {0x5000, 0xfc00, 7, 0, 1, 2},
  {0x1e00, 0xff00, 9, 0, 1, 3},
  {0x03c0, 0xffc0, 11, 0, 1, 4},
  {0x0420, 0xffe0, 12, 0, 1, 5},
  {0x0500, 0xfff0, 13, 0, 1, 6},
  {0xe000, 0xf000, 5, 0, 2, 1},
  {0x1d00, 0xff00, 9, 0, 2, 2}, //20
  {0x0380, 0xffc0, 11, 0, 2, 3},
  {0x0510, 0xfff0, 13, 0, 2, 4},
  {0x6800, 0xf800, 6, 0, 3, 1},
  {0x1180, 0xff80, 10, 0, 3, 2},
  {0x0340, 0xffc0, 11, 0, 3, 3},
  {0x6000, 0xf800, 6, 0, 4, 1},
  {0x1100, 0xff80, 10, 0, 4, 2},
  {0x0520, 0xfff0, 13, 0, 4, 3},
  {0x5800, 0xf800, 6, 0, 5, 1},
  {0x0300, 0xffc0, 11, 0, 5, 2},        // 30
  {0x0530, 0xfff0, 13, 0, 5, 3},
  {0x4c00, 0xfc00, 7, 0, 6, 1},
  {0x02c0, 0xffc0, 11, 0, 6, 2},
  {0x0540, 0xfff0, 13, 0, 6, 3},
  {0x4800, 0xfc00, 7, 0, 7, 1},
  {0x0280, 0xffc0, 11, 0, 7, 2},
  {0x4400, 0xfc00, 7, 0, 8, 1},
  {0x0240, 0xffc0, 11, 0, 8, 2},
  {0x4000, 0xfc00, 7, 0, 9, 1},
  {0x0200, 0xffc0, 11, 0, 9, 2},        // 40
  {0x2c00, 0xfe00, 8, 0, 10, 1},
  {0x0550, 0xfff0, 13, 0, 10, 2},
  {0x2a00, 0xfe00, 8, 0, 11, 1},
  {0x2800, 0xfe00, 8, 0, 12, 1},
  {0x1c00, 0xff00, 9, 0, 13, 1},
  {0x1b00, 0xff00, 9, 0, 14, 1},
  {0x1080, 0xff80, 10, 0, 15, 1},
  {0x1000, 0xff80, 10, 0, 16, 1},
  {0x0f80, 0xff80, 10, 0, 17, 1},
  {0x0f00, 0xff80, 10, 0, 18, 1},       // 50
  {0x0e80, 0xff80, 10, 0, 19, 1},
  {0x0e00, 0xff80, 10, 0, 20, 1},
  {0x0d80, 0xff80, 10, 0, 21, 1},
  {0x0d00, 0xff80, 10, 0, 22, 1},
  {0x0440, 0xffe0, 12, 0, 23, 1},
  {0x0460, 0xffe0, 12, 0, 24, 1},
  {0x0560, 0xfff0, 13, 0, 25, 1},
  {0x0570, 0xfff0, 13, 0, 26, 1},
  {0x7000, 0xf000, 5, 1, 0, 1},
  {0x0c80, 0xff80, 10, 1, 0, 2},        // 60
  {0x00a0, 0xffe0, 12, 1, 0, 3},
  {0x3c00, 0xfc00, 7, 1, 1, 1},
  {0x0080, 0xffe0, 12, 1, 1, 2},
  {0x3800, 0xfc00, 7, 1, 2, 1},
  {0x3400, 0xfc00, 7, 1, 3, 1},
  {0x3000, 0xfc00, 7, 1, 4, 1},
  {0x2600, 0xfe00, 8, 1, 5, 1},
  {0x2400, 0xfe00, 8, 1, 6, 1},
  {0x2200, 0xfe00, 8, 1, 7, 1},
  {0x2000, 0xfe00, 8, 1, 8, 1}, // 70
  {0x1a00, 0xff00, 9, 1, 9, 1},
  {0x1900, 0xff00, 9, 1, 10, 1},
  {0x1800, 0xff00, 9, 1, 11, 1},
  {0x1700, 0xff00, 9, 1, 12, 1},
  {0x1600, 0xff00, 9, 1, 13, 1},
  {0x1500, 0xff00, 9, 1, 14, 1},
  {0x1400, 0xff00, 9, 1, 15, 1},
  {0x1300, 0xff00, 9, 1, 16, 1},
  {0x0c00, 0xff80, 10, 1, 17, 1},
  {0x0b80, 0xff80, 10, 1, 18, 1},       // 80
  {0x0b00, 0xff80, 10, 1, 19, 1},
  {0x0a80, 0xff80, 10, 1, 20, 1},
  {0x0a00, 0xff80, 10, 1, 21, 1},
  {0x0980, 0xff80, 10, 1, 22, 1},
  {0x0900, 0xff80, 10, 1, 23, 1},
  {0x0880, 0xff80, 10, 1, 24, 1},
  {0x01c0, 0xffc0, 11, 1, 25, 1},
  {0x0180, 0xffc0, 11, 1, 26, 1},
  {0x0140, 0xffc0, 11, 1, 27, 1},
  {0x0100, 0xffc0, 11, 1, 28, 1},       // 90
  {0x0480, 0xffe0, 12, 1, 29, 1},
  {0x04a0, 0xffe0, 12, 1, 30, 1},
  {0x04c0, 0xffe0, 12, 1, 31, 1},
  {0x04e0, 0xffe0, 12, 1, 32, 1},
  {0x0580, 0xfff0, 13, 1, 33, 1},
  {0x0590, 0xfff0, 13, 1, 34, 1},
  {0x05a0, 0xfff0, 13, 1, 35, 1},
  {0x05b0, 0xfff0, 13, 1, 36, 1},
  {0x05c0, 0xfff0, 13, 1, 37, 1},
  {0x05d0, 0xfff0, 13, 1, 38, 1},       // 100
  {0x05e0, 0xfff0, 13, 1, 39, 1},
  {0x05f0, 0xfff0, 13, 1, 40, 1},
  {0x0600, 0xfe00, 7, 0, 0xffff, 0xffff}
};

/*
 * Motion vector code table (Code, mask, nbits, vector (halfpixel, two's complement), diff (halfpixel, two's complement))
 */
#define MVD_LEN 64
#define MVD_WID 5
static const guint16 mvd[64][5] = {
  {0x0028, 0xfff8, 13, 0x0060, 0x0020},
  {0x0038, 0xfff8, 13, 0x0061, 0x0021},
  {0x0050, 0xfff0, 12, 0x0062, 0x0022},
  {0x0070, 0xfff0, 12, 0x0063, 0x0023},
  {0x0090, 0xfff0, 12, 0x0064, 0x0024},
  {0x00b0, 0xfff0, 12, 0x0065, 0x0025},
  {0x00d0, 0xfff0, 12, 0x0066, 0x0026},
  {0x00f0, 0xfff0, 12, 0x0067, 0x0027},
  {0x0120, 0xffe0, 11, 0x0068, 0x0028},
  {0x0160, 0xffe0, 11, 0x0069, 0x0029},
  {0x01a0, 0xffe0, 11, 0x006a, 0x002a},
  {0x01e0, 0xffe0, 11, 0x006b, 0x002b},
  {0x0220, 0xffe0, 11, 0x006c, 0x002c},
  {0x0260, 0xffe0, 11, 0x006d, 0x002d},
  {0x02a0, 0xffe0, 11, 0x006e, 0x002e},
  {0x02e0, 0xffe0, 11, 0x006f, 0x002f},
  {0x0320, 0xffe0, 11, 0x0070, 0x0030},
  {0x0360, 0xffe0, 11, 0x0071, 0x0031},
  {0x03a0, 0xffe0, 11, 0x0072, 0x0032},
  {0x03e0, 0xffe0, 11, 0x0073, 0x0033},
  {0x0420, 0xffe0, 11, 0x0074, 0x0034},
  {0x0460, 0xffe0, 11, 0x0075, 0x0035},
  {0x04c0, 0xffc0, 10, 0x0076, 0x0036},
  {0x0540, 0xffc0, 10, 0x0077, 0x0037},
  {0x05c0, 0xffc0, 10, 0x0078, 0x0038},
  {0x0700, 0xff00, 8, 0x0079, 0x0039},
  {0x0900, 0xff00, 8, 0x007a, 0x003a},
  {0x0b00, 0xff00, 8, 0x007b, 0x003b},
  {0x0e00, 0xfe00, 7, 0x007c, 0x003c},
  {0x1800, 0xf800, 5, 0x007d, 0x003d},
  {0x3000, 0xf000, 4, 0x007e, 0x003e},
  {0x6000, 0xe000, 3, 0x007f, 0x003f},
  {0x8000, 0x8000, 1, 0x0000, 0x0000},
  {0x4000, 0xe000, 3, 0x0001, 0x0041},
  {0x2000, 0xf000, 4, 0x0002, 0x0042},
  {0x1000, 0xf800, 5, 0x0003, 0x0043},
  {0x0c00, 0xfe00, 7, 0x0004, 0x0044},
  {0x0a00, 0xff00, 8, 0x0005, 0x0045},
  {0x0800, 0xff00, 8, 0x0006, 0x0046},
  {0x0600, 0xff00, 8, 0x0007, 0x0047},
  {0x0580, 0xffc0, 10, 0x0008, 0x0048},
  {0x0500, 0xffc0, 10, 0x0009, 0x0049},
  {0x0480, 0xffc0, 10, 0x000a, 0x004a},
  {0x0440, 0xffe0, 11, 0x000b, 0x004b},
  {0x0400, 0xffe0, 11, 0x000c, 0x004c},
  {0x03c0, 0xffe0, 11, 0x000d, 0x004d},
  {0x0380, 0xffe0, 11, 0x000e, 0x004e},
  {0x0340, 0xffe0, 11, 0x000f, 0x004f},
  {0x0300, 0xffe0, 11, 0x0010, 0x0050},
  {0x02c0, 0xffe0, 11, 0x0011, 0x0051},
  {0x0280, 0xffe0, 11, 0x0012, 0x0052},
  {0x0240, 0xffe0, 11, 0x0013, 0x0053},
  {0x0200, 0xffe0, 11, 0x0014, 0x0054},
  {0x01c0, 0xffe0, 11, 0x0015, 0x0055},
  {0x0180, 0xffe0, 11, 0x0016, 0x0056},
  {0x0140, 0xffe0, 11, 0x0017, 0x0057},
  {0x0100, 0xffe0, 11, 0x0018, 0x0058},
  {0x00e0, 0xfff0, 12, 0x0019, 0x0059},
  {0x00c0, 0xfff0, 12, 0x001a, 0x005a},
  {0x00a0, 0xfff0, 12, 0x001b, 0x005b},
  {0x0080, 0xfff0, 12, 0x001c, 0x005c},
  {0x0060, 0xfff0, 12, 0x001d, 0x005d},
  {0x0040, 0xfff0, 12, 0x001e, 0x005e},
  {0x0030, 0xfff8, 13, 0x001f, 0x005f}
};

GST_DEBUG_CATEGORY_STATIC (rtph263pay_debug);
#define GST_CAT_DEFAULT (rtph263pay_debug)

#define GST_RTP_HEADER_LEN 12

enum
{
  PROP_0,
  PROP_MODE_A_ONLY
};

static GstStaticPadTemplate gst_rtp_h263_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h263, "
        "variant = (string) \"itu\", " "h263version = (string) \"h263\"")
    );

static GstStaticPadTemplate gst_rtp_h263_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_H263_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H263\"; "
        "application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H263\"")
    );

static void gst_rtp_h263_pay_finalize (GObject * object);

static gboolean gst_rtp_h263_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static void gst_rtp_h263_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_h263_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_rtp_h263_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);

static void gst_rtp_h263_pay_boundry_init (GstRtpH263PayBoundry * boundry,
    guint8 * start, guint8 * end, guint8 sbit, guint8 ebit);
static GstRtpH263PayGob *gst_rtp_h263_pay_gob_new (GstRtpH263PayBoundry *
    boundry, guint gobn);
static GstRtpH263PayMB *gst_rtp_h263_pay_mb_new (GstRtpH263PayBoundry * boundry,
    guint mba);
static GstRtpH263PayPackage *gst_rtp_h263_pay_package_new_empty ();
static GstRtpH263PayPackage *gst_rtp_h263_pay_package_new (guint8 * start,
    guint8 * end, guint length, guint8 sbit, guint8 ebit, GstBuffer * outbuf,
    gboolean marker);

static void gst_rtp_h263_pay_mb_destroy (GstRtpH263PayMB * mb);
static void gst_rtp_h263_pay_gob_destroy (GstRtpH263PayGob * gob, guint ind);
static void gst_rtp_h263_pay_context_destroy (GstRtpH263PayContext * context,
    guint ind);
static void gst_rtp_h263_pay_package_destroy (GstRtpH263PayPackage * pack);

#define gst_rtp_h263_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpH263Pay, gst_rtp_h263_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_h263_pay_class_init (GstRtpH263PayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->finalize = gst_rtp_h263_pay_finalize;

  gstrtpbasepayload_class->set_caps = gst_rtp_h263_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_h263_pay_handle_buffer;
  gobject_class->set_property = gst_rtp_h263_pay_set_property;
  gobject_class->get_property = gst_rtp_h263_pay_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_MODE_A_ONLY, g_param_spec_boolean ("modea-only",
          "Fragment packets in mode A Only",
          "Disable packetization modes B and C", DEFAULT_MODE_A,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h263_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h263_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP H263 packet payloader", "Codec/Payloader/Network/RTP",
      "Payload-encodes H263 video in RTP packets (RFC 2190)",
      "Neil Stratford <neils@vipadia.com>"
      "Dejan Sakelsak <dejan.sakelsak@marand.si>");

  GST_DEBUG_CATEGORY_INIT (rtph263pay_debug, "rtph263pay", 0,
      "H263 RTP Payloader");
}

static void
gst_rtp_h263_pay_init (GstRtpH263Pay * rtph263pay)
{
  GST_RTP_BASE_PAYLOAD_PT (rtph263pay) = GST_RTP_PAYLOAD_H263;
  rtph263pay->prop_payload_mode = DEFAULT_MODE_A;
}

static void
gst_rtp_h263_pay_finalize (GObject * object)
{
  GstRtpH263Pay *rtph263pay;

  rtph263pay = GST_RTP_H263_PAY (object);

  gst_buffer_replace (&rtph263pay->current_buffer, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_h263_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  GstStructure *s = gst_caps_get_structure (caps, 0);
  gint width, height;
  gchar *framesize = NULL;
  gboolean res;

  if (gst_structure_has_field (s, "width") &&
      gst_structure_has_field (s, "height")) {
    if (!gst_structure_get_int (s, "width", &width) || width <= 0) {
      goto invalid_dimension;
    }

    if (!gst_structure_get_int (s, "height", &height) || height <= 0) {
      goto invalid_dimension;
    }

    framesize = g_strdup_printf ("%d-%d", width, height);
  }

  gst_rtp_base_payload_set_options (payload, "video",
      payload->pt != GST_RTP_PAYLOAD_H263, "H263", 90000);

  if (framesize != NULL) {
    res = gst_rtp_base_payload_set_outcaps (payload,
        "a-framesize", G_TYPE_STRING, framesize, NULL);
  } else {
    res = gst_rtp_base_payload_set_outcaps (payload, NULL);
  }
  g_free (framesize);

  return res;

  /* ERRORS */
invalid_dimension:
  {
    GST_ERROR_OBJECT (payload, "Invalid width/height from caps");
    return FALSE;
  }
}

static void
gst_rtp_h263_pay_context_destroy (GstRtpH263PayContext * context, guint ind)
{
  if (!context)
    return;

  if (context->gobs) {
    guint i;

    for (i = 0; i < format_props[ind][0]; i++) {
      if (context->gobs[i]) {
        gst_rtp_h263_pay_gob_destroy (context->gobs[i], ind);
      }
    }

    g_free (context->gobs);
  }

  g_free (context);
}

static void
gst_rtp_h263_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpH263Pay *rtph263pay;

  rtph263pay = GST_RTP_H263_PAY (object);

  switch (prop_id) {
    case PROP_MODE_A_ONLY:
      rtph263pay->prop_payload_mode = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_h263_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpH263Pay *rtph263pay;

  rtph263pay = GST_RTP_H263_PAY (object);

  switch (prop_id) {
    case PROP_MODE_A_ONLY:
      g_value_set_boolean (value, rtph263pay->prop_payload_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstRtpH263PayPackage *
gst_rtp_h263_pay_package_new_empty (void)
{
  return (GstRtpH263PayPackage *) g_malloc0 (sizeof (GstRtpH263PayPackage));
}

static GstRtpH263PayPackage *
gst_rtp_h263_pay_package_new (guint8 * start, guint8 * end, guint length,
    guint8 sbit, guint8 ebit, GstBuffer * outbuf, gboolean marker)
{

  GstRtpH263PayPackage *package;

  package = gst_rtp_h263_pay_package_new_empty ();

  package->payload_start = start;
  package->payload_end = end;
  package->payload_len = length;
  package->sbit = sbit;
  package->ebit = ebit;
  package->outbuf = outbuf;
  package->marker = marker;

  return package;
}

static void
gst_rtp_h263_pay_package_destroy (GstRtpH263PayPackage * pack)
{
  if (pack)
    g_free (pack);
}

static void
gst_rtp_h263_pay_boundry_init (GstRtpH263PayBoundry * boundry,
    guint8 * start, guint8 * end, guint8 sbit, guint8 ebit)
{
  boundry->start = start;
  boundry->end = end;
  boundry->sbit = sbit;
  boundry->ebit = ebit;
}

static void
gst_rtp_h263_pay_splat_header_A (guint8 * header,
    GstRtpH263PayPackage * package, GstRtpH263PayPic * piclayer)
{

  GstRtpH263PayAHeader *a_header;

  a_header = (GstRtpH263PayAHeader *) header;

  a_header->f = 0;
  a_header->p = 0;
  a_header->sbit = package->sbit;
  a_header->ebit = package->ebit;
  a_header->src = GST_H263_PICTURELAYER_PLSRC (piclayer);
  a_header->i = GST_H263_PICTURELAYER_PLTYPE (piclayer);
  a_header->u = GST_H263_PICTURELAYER_PLUMV (piclayer);
  a_header->s = GST_H263_PICTURELAYER_PLSAC (piclayer);
  a_header->a = GST_H263_PICTURELAYER_PLAP (piclayer);
  a_header->r1 = 0;
  a_header->r2 = 0;
  a_header->dbq = 0;
  a_header->trb = 0;
  a_header->tr = 0;

}

static void
gst_rtp_h263_pay_splat_header_B (guint8 * header,
    GstRtpH263PayPackage * package, GstRtpH263PayPic * piclayer)
{

  GstRtpH263PayBHeader *b_header;

  b_header = (GstRtpH263PayBHeader *) header;

  b_header->f = 1;
  b_header->p = 0;
  b_header->sbit = package->sbit;
  b_header->ebit = package->ebit;
  b_header->src = GST_H263_PICTURELAYER_PLSRC (piclayer);
  b_header->quant = package->quant;
  b_header->gobn = package->gobn;
  b_header->mba1 = package->mba >> 6;
  b_header->mba2 = package->mba & 0x003f;
  b_header->r = 0;
  b_header->i = GST_H263_PICTURELAYER_PLTYPE (piclayer);
  b_header->u = GST_H263_PICTURELAYER_PLUMV (piclayer);
  b_header->s = GST_H263_PICTURELAYER_PLSAC (piclayer);
  b_header->a = GST_H263_PICTURELAYER_PLAP (piclayer);

  b_header->hmv11 = 0;
  b_header->hmv12 = 0;
  b_header->vmv11 = 0;
  b_header->vmv12 = 0;
  b_header->hmv21 = 0;
  b_header->hmv22 = 0;
  b_header->vmv21 = 0;

  if (package->nmvd > 0) {
    //mvd[0]
    b_header->hmv11 = (package->mvd[0] & 0x7f) >> 3;
    b_header->hmv12 = (package->mvd[0] & 0x07);
    //mvd[1]
    b_header->vmv11 = (package->mvd[1] & 0x07f) >> 2;
    b_header->vmv12 = (package->mvd[1] & 0x03);

    if (package->nmvd == 8) {
      //mvd[4]
      b_header->hmv21 = (package->mvd[4] & 0x7f) >> 1;
      b_header->hmv22 = (package->mvd[4] & 0x01);
      //mvd[5]
      b_header->vmv21 = (package->mvd[5] & 0x7f);
    }
  }

}

static gboolean
gst_rtp_h263_pay_gobfinder (GstRtpH263Pay * rtph263pay,
    GstRtpH263PayBoundry * boundry)
{
  guint8 *current;
  guint range;
  guint i;

  current = boundry->end + 1;
  range = (rtph263pay->data - current) + rtph263pay->available_data;


  GST_DEBUG_OBJECT (rtph263pay,
      "Searching for next GOB, data:%p, len:%u, payload_len:%p,"
      " current:%p, range:%u", rtph263pay->data, rtph263pay->available_data,
      boundry->end + 1, current, range);

  /* If we are past the end, stop */
  if (current >= rtph263pay->data + rtph263pay->available_data)
    return FALSE;

  for (i = 3; i < range - 3; i++) {
    if ((current[i] == 0x0) &&
        (current[i + 1] == 0x0) && (current[i + 2] >> 7 == 0x1)) {
      GST_LOG_OBJECT (rtph263pay, "GOB end found at: %p start: %p len: %u",
          current + i - 1, boundry->end + 1,
          (guint) (current + i - boundry->end + 2));
      gst_rtp_h263_pay_boundry_init (boundry, boundry->end + 1, current + i - 1,
          0, 0);

      return TRUE;
    }
  }

  GST_DEBUG_OBJECT (rtph263pay,
      "Couldn't find any new GBSC in this frame, range:%u", range);

  gst_rtp_h263_pay_boundry_init (boundry, boundry->end + 1,
      (guint8 *) (rtph263pay->data + rtph263pay->available_data - 1), 0, 0);

  return TRUE;
}

static GstRtpH263PayGob *
gst_rtp_h263_pay_gob_new (GstRtpH263PayBoundry * boundry, guint gobn)
{
  GstRtpH263PayGob *gob;

  gob = (GstRtpH263PayGob *) g_malloc0 (sizeof (GstRtpH263PayGob));

  gob->start = boundry->start;
  gob->end = boundry->end;
  gob->length = boundry->end - boundry->start + 1;
  gob->ebit = boundry->ebit;
  gob->sbit = boundry->sbit;
  gob->gobn = gobn;
  gob->quant = 0;
  gob->macroblocks = NULL;
  gob->nmacroblocs = 0;

  return gob;
}

static void
gst_rtp_h263_pay_gob_destroy (GstRtpH263PayGob * gob, guint ind)
{

  if (!gob)
    return;

  if (gob->macroblocks) {

    guint i;

    for (i = 0; i < gob->nmacroblocs; i++) {
      gst_rtp_h263_pay_mb_destroy (gob->macroblocks[i]);
    }

    g_free (gob->macroblocks);
  }

  g_free (gob);
}

/*
 * decode MCBPC for I frames and return index in table or -1 if not found
 */
static gint
gst_rtp_h263_pay_decode_mcbpc_I (GstRtpH263Pay * rtph263pay, guint32 value)
{

  gint i;
  guint16 code;

  code = value >> 16;

  GST_TRACE_OBJECT (rtph263pay, "value:0x%08x, code:0x%04x", value, code);

  for (i = 0; i < MCBPC_I_LEN; i++) {
    if ((code & mcbpc_I[i][1]) == mcbpc_I[i][0]) {
      return i;
    }
  }

  GST_WARNING_OBJECT (rtph263pay, "Couldn't find code, returning -1");

  return -1;
}

/*
 * decode MCBPC for P frames and return index in table or -1 if not found
 */
static gint
gst_rtp_h263_pay_decode_mcbpc_P (GstRtpH263Pay * rtph263pay, guint32 value)
{

  gint i;
  guint16 code;

  code = value >> 16;

  GST_TRACE_OBJECT (rtph263pay, "value:0x%08x, code:0x%04x", value, code);

  for (i = 0; i < MCBPC_P_LEN; i++) {
    if ((code & mcbpc_P[i][1]) == mcbpc_P[i][0]) {
      return i;
    }
  }

  GST_WARNING_OBJECT (rtph263pay, "Couldn't find code, returning -1");

  return -1;
}

/*
 * decode CBPY and return index in table or -1 if not found
 */
static gint
gst_rtp_h263_pay_decode_cbpy (GstRtpH263Pay * rtph263pay, guint32 value,
    const guint8 cbpy_table[16][7])
{

  gint i;
  guint8 code;

  code = value >> 24;

  GST_TRACE_OBJECT (rtph263pay, "value:0x%08x, code:0x%04x", value, code);

  for (i = 0; i < CBPY_LEN; i++) {
    if ((code & cbpy_table[i][1]) == cbpy_table[i][0]) {
      return i;
    }
  }

  GST_WARNING_OBJECT (rtph263pay, "Couldn't find code, returning -1");

  return -1;
}

/*
 * decode MVD and return index in table or -1 if not found
 */
static gint
gst_rtp_h263_pay_decode_mvd (GstRtpH263Pay * rtph263pay, guint32 value)
{

  gint i;
  guint16 code;

  code = value >> 16;

  GST_TRACE_OBJECT (rtph263pay, "value:0x%08x, code:0x%04x", value, code);

  for (i = 0; i < MVD_LEN; i++) {
    if ((code & mvd[i][1]) == mvd[i][0]) {
      return i;
    }
  }

  GST_WARNING_OBJECT (rtph263pay, "Couldn't find code, returning -1");

  return -1;
}

/*
 * decode TCOEF and return index in table or -1 if not found
 */
static gint
gst_rtp_h263_pay_decode_tcoef (GstRtpH263Pay * rtph263pay, guint32 value)
{

  gint i;
  guint16 code;

  code = value >> 16;

  GST_TRACE_OBJECT (rtph263pay, "value:0x%08x, code:0x%04x", value, code);

  for (i = 0; i < TCOEF_LEN; i++) {
    if ((code & tcoef[i][1]) == tcoef[i][0]) {
      GST_TRACE_OBJECT (rtph263pay, "tcoef is %d", i);
      return i;
    }
  }

  GST_WARNING_OBJECT (rtph263pay, "Couldn't find code, returning -1");

  return -1;
}

/*
 * the 32-bit register is like a window that we move right for "move_bits" to get the next v "data" h263 field
 * "rest_bits" tells how many bits in the "data" byte address are still not used
 */

static gint
gst_rtp_h263_pay_move_window_right (GstRtpH263Pay * rtph263pay,
    GstRtpH263PayContext * context, guint n, guint rest_bits,
    guint8 ** orig_data, guint8 ** data_end)
{

  GST_TRACE_OBJECT (rtph263pay,
      "Moving window: 0x%08x from: %p for %d bits, rest_bits: %d, data_end %p",
      context->window, context->win_end, n, rest_bits, *data_end);

  if (n == 0)
    return rest_bits;

  while (n != 0 || context->win_end == ((*data_end) + 1)) {
    guint8 b = context->win_end <= *data_end ? *context->win_end : 0;
    if (rest_bits == 0) {
      if (n > 8) {
        context->window = (context->window << 8) | b;
        n -= 8;
      } else {
        context->window = (context->window << n) | (b >> (8 - n));
        rest_bits = 8 - n;
        if (rest_bits == 0)
          context->win_end++;
        break;
      }
    } else {
      if (n > rest_bits) {
        context->window = (context->window << rest_bits) |
            (b & (((guint) pow (2.0, (double) rest_bits)) - 1));
        n -= rest_bits;
        rest_bits = 0;
      } else {
        context->window = (context->window << n) |
            ((b & (((guint) pow (2.0, (double) rest_bits)) - 1)) >>
            (rest_bits - n));
        rest_bits -= n;
        if (rest_bits == 0)
          context->win_end++;
        break;
      }
    }

    context->win_end++;
  }

  *orig_data = context->win_end - 4;

  GST_TRACE_OBJECT (rtph263pay,
      "Window moved to %p with value: 0x%08x and orig_data: %p rest_bits: %d",
      context->win_end, context->window, *orig_data, rest_bits);
  return rest_bits;
}

/*
 * Find the start of the next MB (end of the current MB)
 * returns the number of excess bits and stores the end of the MB in end
 * data must be placed on first MB byte
 */
static GstRtpH263PayMB *
gst_rtp_h263_pay_B_mbfinder (GstRtpH263Pay * rtph263pay,
    GstRtpH263PayContext * context, GstRtpH263PayGob * gob,
    GstRtpH263PayMB * macroblock, guint mba)
{
  guint mb_type_index;
  guint cbpy_type_index;
  guint tcoef_type_index;
  GstRtpH263PayMB *mac;
  GstRtpH263PayBoundry boundry;

  gst_rtp_h263_pay_boundry_init (&boundry, macroblock->end,
      macroblock->end, 8 - macroblock->ebit, macroblock->ebit);
  mac = gst_rtp_h263_pay_mb_new (&boundry, mba);

  if (mac->sbit == 8) {
    mac->start++;
//     mac->end++;
    mac->sbit = 0;
  }

  GST_LOG_OBJECT (rtph263pay,
      "current_pos:%p, end:%p, rest_bits:%d, window:0x%08x", mac->start,
      mac->end, macroblock->ebit, context->window);

  if (context->piclayer->ptype_pictype == 0) {
    //We have an I frame
    gint i;
    guint last;
    guint ind;

    //Step 2 decode MCBPC I
    mb_type_index =
        gst_rtp_h263_pay_decode_mcbpc_I (rtph263pay, context->window);

    GST_TRACE_OBJECT (rtph263pay, "MCBPC index: %d", mb_type_index);
    if (mb_type_index == -1) {
      GST_ERROR_OBJECT (rtph263pay, "MB index shouldn't be -1 in window: %08x",
          context->window);
      goto beach;
    }

    mac->ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context,
        mcbpc_I[mb_type_index][2], mac->ebit, &mac->end, &gob->end);

    mac->mb_type = mcbpc_I[mb_type_index][5];

    if (mb_type_index == 8) {
      GST_TRACE_OBJECT (rtph263pay, "Stuffing skipping rest of MB header");
      return mac;
    }
    //Step 3 decode CBPY I
    cbpy_type_index =
        gst_rtp_h263_pay_decode_cbpy (rtph263pay, context->window, cbpy_I);

    GST_TRACE_OBJECT (rtph263pay, "CBPY index: %d", cbpy_type_index);
    if (cbpy_type_index == -1) {
      GST_ERROR_OBJECT (rtph263pay,
          "CBPY index shouldn't be -1 in window: %08x", context->window);
      goto beach;
    }

    mac->ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context,
        cbpy_I[cbpy_type_index][2], mac->ebit, &mac->end, &gob->end);

    //Step 4 decode rest of MB
    //MB type 1 and 4 have DQUANT - we store it for packaging purposes
    if (mcbpc_I[mb_type_index][5] == 4) {
      GST_TRACE_OBJECT (rtph263pay, "Shifting DQUANT");

      mac->quant = (context->window >> 30);

      mac->ebit =
          gst_rtp_h263_pay_move_window_right (rtph263pay, context, 2, mac->ebit,
          &mac->end, &gob->end);
    }
    //Step 5 go trough the blocks - decode DC and TCOEF
    last = 0;
    for (i = 0; i < N_BLOCKS; i++) {

      GST_TRACE_OBJECT (rtph263pay, "Decoding INTRADC and TCOEF, i:%d", i);
      mac->ebit =
          gst_rtp_h263_pay_move_window_right (rtph263pay, context, 8, mac->ebit,
          &mac->end, &gob->end);

      if (i > 3) {
        ind = mcbpc_I[mb_type_index][i - 1];
      } else {
        ind = cbpy_I[cbpy_type_index][i + 3];
      }

      if (ind == 1) {
        while (last == 0) {
          tcoef_type_index =
              gst_rtp_h263_pay_decode_tcoef (rtph263pay, context->window);

          GST_TRACE_OBJECT (rtph263pay, "TCOEF index: %d", tcoef_type_index);
          if (tcoef_type_index == -1) {
            GST_ERROR_OBJECT (rtph263pay,
                "TCOEF index shouldn't be -1 in window: %08x", context->window);
            goto beach;
          }
          mac->ebit =
              gst_rtp_h263_pay_move_window_right (rtph263pay, context,
              tcoef[tcoef_type_index][2], mac->ebit, &mac->end, &gob->end);

          last = tcoef[tcoef_type_index][3];
          if (tcoef_type_index == 102) {
            if ((context->window & 0x80000000) == 0x80000000)
              last = 1;
            else
              last = 0;

            mac->ebit =
                gst_rtp_h263_pay_move_window_right (rtph263pay, context, 15,
                mac->ebit, &mac->end, &gob->end);
          }
        }
        last = 0;
      }
    }

  } else {
    //We have a P frame
    guint i;
    guint last;
    guint ind;

    //Step 1 check COD
    GST_TRACE_OBJECT (rtph263pay, "Checking for COD");
    if ((context->window & 0x80000000) == 0x80000000) {
      //The MB is not coded
      mac->ebit =
          gst_rtp_h263_pay_move_window_right (rtph263pay, context, 1, mac->ebit,
          &mac->end, &gob->end);
      GST_TRACE_OBJECT (rtph263pay, "COOOOOOOOOOOD = 1");

      return mac;
    } else {
      //The MB is coded
      mac->ebit =
          gst_rtp_h263_pay_move_window_right (rtph263pay, context, 1, mac->ebit,
          &mac->end, &gob->end);
    }

    //Step 2 decode MCBPC P
    mb_type_index =
        gst_rtp_h263_pay_decode_mcbpc_P (rtph263pay, context->window);

    GST_TRACE_OBJECT (rtph263pay, "MCBPC index: %d", mb_type_index);
    if (mb_type_index == -1) {
      GST_ERROR_OBJECT (rtph263pay, "MB index shouldn't be -1 in window: %08x",
          context->window);
      goto beach;
    }
    mac->ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context,
        mcbpc_P[mb_type_index][2], mac->ebit, &mac->end, &gob->end);

    mac->mb_type = mcbpc_P[mb_type_index][5];

    if (mb_type_index == 20) {
      GST_TRACE_OBJECT (rtph263pay, "Stuffing skipping rest of MB header");
      return mac;
    }
    //Step 3 decode CBPY P
    cbpy_type_index =
        gst_rtp_h263_pay_decode_cbpy (rtph263pay, context->window, cbpy_P);

    GST_TRACE_OBJECT (rtph263pay, "CBPY index: %d", cbpy_type_index);
    if (cbpy_type_index == -1) {
      GST_ERROR_OBJECT (rtph263pay,
          "CBPY index shouldn't be -1 in window: %08x", context->window);
      goto beach;
    }
    mac->ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context,
        cbpy_P[cbpy_type_index][2], mac->ebit, &mac->end, &gob->end);

    //MB type 1 and 4 have DQUANT - we add it to MB object and jump over
    if (mcbpc_P[mb_type_index][5] == 4 || mcbpc_P[mb_type_index][5] == 1) {
      GST_TRACE_OBJECT (rtph263pay, "Shifting DQUANT");

      mac->quant = context->window >> 30;

      mac->ebit =
          gst_rtp_h263_pay_move_window_right (rtph263pay, context, 2, mac->ebit,
          &mac->end, &gob->end);
    }
    //MB types < 3 have MVD1-4
    if (mcbpc_P[mb_type_index][5] < 3) {

      guint nmvd;
      gint j;

      nmvd = 2;
      if (mcbpc_P[mb_type_index][5] == 2)
        nmvd = 8;

      for (j = 0; j < nmvd; j++) {
        guint mvd_type;

        mvd_type = gst_rtp_h263_pay_decode_mvd (rtph263pay, context->window);

        if (mvd_type == -1) {
          GST_ERROR_OBJECT (rtph263pay,
              "MVD1-4 index shouldn't be -1 in window: %08x", context->window);
          goto beach;
        }
        //set the MB mvd values
        mac->mvd[j] = mvd[mvd_type][3];

        mac->ebit =
            gst_rtp_h263_pay_move_window_right (rtph263pay, context,
            mvd[mvd_type][2], mac->ebit, &mac->end, &gob->end);
      }


    }
    //Step 5 go trough the blocks - decode DC and TCOEF
    last = 0;
    for (i = 0; i < N_BLOCKS; i++) {

      //if MB type 3 or 4 then INTRADC coef are present in blocks
      if (mcbpc_P[mb_type_index][5] > 2) {
        GST_TRACE_OBJECT (rtph263pay, "INTRADC coef: %d", i);
        mac->ebit =
            gst_rtp_h263_pay_move_window_right (rtph263pay, context, 8,
            mac->ebit, &mac->end, &gob->end);
      } else {
        GST_TRACE_OBJECT (rtph263pay, "INTRADC coef is not present");
      }

      //check if the block has TCOEF
      if (i > 3) {
        ind = mcbpc_P[mb_type_index][i - 1];
      } else {
        if (mcbpc_P[mb_type_index][5] > 2) {
          ind = cbpy_I[cbpy_type_index][i + 3];
        } else {
          ind = cbpy_P[cbpy_type_index][i + 3];
        }
      }

      if (ind == 1) {
        while (last == 0) {
          tcoef_type_index =
              gst_rtp_h263_pay_decode_tcoef (rtph263pay, context->window);

          GST_TRACE_OBJECT (rtph263pay, "TCOEF index: %d", tcoef_type_index);
          if (tcoef_type_index == -1) {
            GST_ERROR_OBJECT (rtph263pay,
                "TCOEF index shouldn't be -1 in window: %08x", context->window);
            goto beach;
          }

          mac->ebit =
              gst_rtp_h263_pay_move_window_right (rtph263pay, context,
              tcoef[tcoef_type_index][2], mac->ebit, &mac->end, &gob->end);

          last = tcoef[tcoef_type_index][3];
          if (tcoef_type_index == 102) {
            if ((context->window & 0x80000000) == 0x80000000)
              last = 1;
            else
              last = 0;

            mac->ebit =
                gst_rtp_h263_pay_move_window_right (rtph263pay, context, 15,
                mac->ebit, &mac->end, &gob->end);
          }
        }
        last = 0;
      }
    }
  }

  mac->length = mac->end - mac->start + 1;

  return mac;

beach:
  gst_rtp_h263_pay_mb_destroy (mac);
  return NULL;
}

static GstRtpH263PayMB *
gst_rtp_h263_pay_mb_new (GstRtpH263PayBoundry * boundry, guint mba)
{
  GstRtpH263PayMB *mb;
  gint i;

  mb = (GstRtpH263PayMB *) g_malloc0 (sizeof (GstRtpH263PayMB));

  mb->start = boundry->start;
  mb->end = boundry->end;
  mb->length = boundry->end - boundry->start + 1;
  mb->sbit = boundry->sbit;
  mb->ebit = boundry->ebit;
  mb->mba = mba;

  for (i = 0; i < 5; i++)
    mb->mvd[i] = 0;

  return mb;
}

static void
gst_rtp_h263_pay_mb_destroy (GstRtpH263PayMB * mb)
{
  if (!mb)
    return;

  g_free (mb);
}

static GstFlowReturn
gst_rtp_h263_pay_push (GstRtpH263Pay * rtph263pay,
    GstRtpH263PayContext * context, GstRtpH263PayPackage * package)
{

  /*
   * Splat the payload header values
   */
  guint8 *header;
  GstFlowReturn ret;
  GstRTPBuffer rtp = { NULL };

  gst_rtp_buffer_map (package->outbuf, GST_MAP_WRITE, &rtp);

  header = gst_rtp_buffer_get_payload (&rtp);

  switch (package->mode) {
    case GST_RTP_H263_PAYLOAD_HEADER_MODE_A:
      GST_LOG_OBJECT (rtph263pay, "Pushing A packet");
      gst_rtp_h263_pay_splat_header_A (header, package, context->piclayer);
      break;
    case GST_RTP_H263_PAYLOAD_HEADER_MODE_B:
      GST_LOG_OBJECT (rtph263pay, "Pushing B packet");
      gst_rtp_h263_pay_splat_header_B (header, package, context->piclayer);
      break;
    case GST_RTP_H263_PAYLOAD_HEADER_MODE_C:
      //gst_rtp_h263_pay_splat_header_C(header, package, context->piclayer);
      //break;
    default:
      return GST_FLOW_ERROR;
  }

  /*
   * timestamp the buffer
   */
  GST_BUFFER_PTS (package->outbuf) = rtph263pay->first_ts;

  gst_rtp_buffer_set_marker (&rtp, package->marker);
  if (package->marker)
    GST_DEBUG_OBJECT (rtph263pay, "Marker set!");

  gst_rtp_buffer_unmap (&rtp);

  /*
   * Copy the payload data in the buffer
   */
  GST_DEBUG_OBJECT (rtph263pay, "Copying memory");
  gst_buffer_copy_into (package->outbuf, rtph263pay->current_buffer,
      GST_BUFFER_COPY_MEMORY, package->payload_start - rtph263pay->map.data,
      package->payload_len);
  gst_rtp_copy_video_meta (rtph263pay, package->outbuf,
      rtph263pay->current_buffer);

  ret =
      gst_rtp_base_payload_push (GST_RTP_BASE_PAYLOAD (rtph263pay),
      package->outbuf);
  GST_DEBUG_OBJECT (rtph263pay, "Package pushed, returning");

  gst_rtp_h263_pay_package_destroy (package);

  return ret;
}

static GstFlowReturn
gst_rtp_h263_pay_A_fragment_push (GstRtpH263Pay * rtph263pay,
    GstRtpH263PayContext * context, guint first, guint last)
{

  GstRtpH263PayPackage *pack;

  pack = gst_rtp_h263_pay_package_new_empty ();

  pack->payload_start = context->gobs[first]->start;
  pack->sbit = context->gobs[first]->sbit;
  pack->ebit = context->gobs[last]->ebit;
  pack->payload_len =
      (context->gobs[last]->end - context->gobs[first]->start) + 1;
  pack->marker = FALSE;

  if (last == context->no_gobs - 1) {
    pack->marker = TRUE;
  }

  pack->gobn = context->gobs[first]->gobn;
  pack->mode = GST_RTP_H263_PAYLOAD_HEADER_MODE_A;
  pack->outbuf = gst_rtp_buffer_new_allocate (pack->mode, 0, 0);

  GST_DEBUG_OBJECT (rtph263pay, "Sending len:%d data to push function",
      pack->payload_len);

  return gst_rtp_h263_pay_push (rtph263pay, context, pack);
}

static GstFlowReturn
gst_rtp_h263_pay_B_fragment_push (GstRtpH263Pay * rtph263pay,
    GstRtpH263PayContext * context, GstRtpH263PayGob * gob, guint first,
    guint last, GstRtpH263PayBoundry * boundry)
{

  GstRtpH263PayPackage *pack;
  guint mv;

  pack = gst_rtp_h263_pay_package_new_empty ();

  pack->payload_start = gob->macroblocks[first]->start;
  pack->sbit = gob->macroblocks[first]->sbit;
  if (first == 0) {
    pack->payload_start = boundry->start;
    pack->sbit = boundry->sbit;
    pack->quant = gob->quant;
  } else {
    pack->quant = gob->macroblocks[first]->quant;
  }
  pack->payload_end = gob->macroblocks[last]->end;

  pack->ebit = gob->macroblocks[last]->ebit;
  pack->mba = gob->macroblocks[first]->mba;
  pack->gobn = gob->gobn;
  pack->mode = GST_RTP_H263_PAYLOAD_HEADER_MODE_B;
  pack->nmvd = 0;

  if (gob->macroblocks[first]->mb_type < 3) {
    if (gob->macroblocks[first]->mb_type == 2)
      pack->nmvd = 8;
    else if (gob->macroblocks[first]->mb_type < 2)
      pack->nmvd = 2;

    for (mv = 0; mv < pack->nmvd; mv++)
      pack->mvd[mv] = gob->macroblocks[first]->mvd[mv];
  }

  pack->marker = FALSE;
  if (last == gob->nmacroblocs - 1) {
    pack->ebit = 0;
  }

  if ((format_props[context->piclayer->ptype_srcformat][0] - 1 == gob->gobn)
      && (last == gob->nmacroblocs - 1)) {
    pack->marker = TRUE;
  }

  pack->payload_len = pack->payload_end - pack->payload_start + 1;
  pack->outbuf = gst_rtp_buffer_new_allocate (pack->mode, 0, 0);

  return gst_rtp_h263_pay_push (rtph263pay, context, pack);
}


static gboolean
gst_rtp_h263_pay_mode_B_fragment (GstRtpH263Pay * rtph263pay,
    GstRtpH263PayContext * context, GstRtpH263PayGob * gob)
{


    /*---------- MODE B MODE FRAGMENTATION ----------*/
  GstRtpH263PayMB *mac, *mac0;
  guint max_payload_size;
  GstRtpH263PayBoundry boundry;
  guint mb;
  guint8 ebit;

  guint first = 0;
  guint payload_len;

  max_payload_size =
      context->mtu - GST_RTP_H263_PAYLOAD_HEADER_MODE_B - GST_RTP_HEADER_LEN;

  gst_rtp_h263_pay_boundry_init (&boundry, gob->start, gob->start, gob->sbit,
      0);

  gob->macroblocks =
      (GstRtpH263PayMB **) g_malloc0 (sizeof (GstRtpH263PayMB *) *
      format_props[context->piclayer->ptype_srcformat][1]);

  GST_LOG_OBJECT (rtph263pay, "GOB isn't PB frame, applying mode B");

  //initializing window
  context->win_end = boundry.end;
  if (gst_rtp_h263_pay_move_window_right (rtph263pay, context, 32, boundry.ebit,
          &boundry.end, &gob->end) != 0) {
    GST_ERROR_OBJECT (rtph263pay,
        "The rest of the bits should be 0, exiting, because something bad happend");
    goto decode_error;
  }
  //The first GOB of a frame "has no" actual header - PICTURE header is his header
  if (gob->gobn == 0) {
    guint shift;
    GST_LOG_OBJECT (rtph263pay, "Initial GOB");
    shift = 43;

    boundry.ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context, shift,
        boundry.ebit, &boundry.end, &gob->end);

    //We need PQUANT for mode B packages - so we store it
    gob->quant = context->window >> 27;

    //if PCM == 1, then PSBI is present - header has 51 bits
    //shift for PQUANT (5) and PCM (1) = 6 bits
    shift = 6;
    if (context->cpm == 1)
      shift += 2;
    boundry.ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context, shift,
        boundry.ebit, &boundry.end, &gob->end);

    GST_TRACE_OBJECT (rtph263pay, "window: 0x%08x", context->window);

    //Shifting the PEI and PSPARE fields
    while ((context->window & 0x80000000) == 0x80000000) {
      boundry.ebit =
          gst_rtp_h263_pay_move_window_right (rtph263pay, context, 9,
          boundry.ebit, &boundry.end, &gob->end);
      GST_TRACE_OBJECT (rtph263pay, "window: 0x%08x", context->window);
    }

    //shift the last PEI field
    boundry.ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context, 1,
        boundry.ebit, &boundry.end, &gob->end);

  } else {
    //skipping GOBs 24 header bits + 5 GQUANT
    guint shift = 24;

    GST_TRACE_OBJECT (rtph263pay, "INTER GOB");

    //if CPM == 1, there are 2 more bits in the header - GSBI header is 31 bits long
    if (context->cpm == 1)
      shift += 2;

    GST_TRACE_OBJECT (rtph263pay, "window: 0x%08x", context->window);
    boundry.ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context, shift,
        boundry.ebit, &boundry.end, &gob->end);

    //We need GQUANT for mode B packages - so we store it
    gob->quant = context->window >> 27;

    shift = 5;
    boundry.ebit =
        gst_rtp_h263_pay_move_window_right (rtph263pay, context, shift,
        boundry.ebit, &boundry.end, &gob->end);

    GST_TRACE_OBJECT (rtph263pay, "window: 0x%08x", context->window);
  }

  GST_TRACE_OBJECT (rtph263pay, "GQUANT IS: %08x", gob->quant);

  // We are on MB layer

  mac = mac0 = gst_rtp_h263_pay_mb_new (&boundry, 0);
  for (mb = 0; mb < format_props[context->piclayer->ptype_srcformat][1]; mb++) {

    GST_TRACE_OBJECT (rtph263pay,
        "================ START MB %d =================", mb);

    //Find next macroblock boundaries
    ebit = mac->ebit;
    if (!(mac =
            gst_rtp_h263_pay_B_mbfinder (rtph263pay, context, gob, mac, mb))) {

      GST_LOG_OBJECT (rtph263pay, "Error decoding MB - sbit: %d", 8 - ebit);
      GST_ERROR_OBJECT (rtph263pay, "Error decoding in GOB");

      gst_rtp_h263_pay_mb_destroy (mac0);
      goto decode_error;
    }

    /* Store macroblock for further processing and delete old MB if any */
    gst_rtp_h263_pay_mb_destroy (gob->macroblocks[mb]);
    gob->macroblocks[mb] = mac;

    //If mb_type == stuffing, don't increment the mb address
    if (mac->mb_type == 10) {
      mb--;
      continue;
    } else {
      gob->nmacroblocs++;
    }

    if (mac->end >= gob->end) {
      GST_LOG_OBJECT (rtph263pay, "No more MBs in this GOB");
      if (!mac->ebit) {
        mac->end--;
      }
      gob->end = mac->end;
      break;
    }
    GST_DEBUG_OBJECT (rtph263pay,
        "Found MB: mba: %d start: %p end: %p len: %d sbit: %d ebit: %d",
        mac->mba, mac->start, mac->end, mac->length, mac->sbit, mac->ebit);
    GST_TRACE_OBJECT (rtph263pay,
        "================ END MB %d =================", mb);
  }
  gst_rtp_h263_pay_mb_destroy (mac0);

  mb = 0;
  first = 0;
  payload_len = boundry.end - boundry.start + 1;
  GST_DEBUG_OBJECT (rtph263pay,
      "------------------------- NEW PACKAGE ----------------------");
  while (mb < gob->nmacroblocs) {
    if (payload_len + gob->macroblocks[mb]->length < max_payload_size) {

      //FIXME: payload_len is not the real length -> ignoring sbit/ebit
      payload_len += gob->macroblocks[mb]->length;
      mb++;

    } else {
      //FIXME: we should include the last few bits of the GOB in the package - do we do that now?
      //GST_DEBUG_OBJECT (rtph263pay, "Pushing GOBS %d to %d because payload size is %d", first,
      //    first == mb - 1, payload_len);

      // FIXME: segfault if mb == 0 (first MB is larger than max_payload_size)
      GST_DEBUG_OBJECT (rtph263pay, "Push B mode fragment from mb %d to %d",
          first, mb - 1);
      if (gst_rtp_h263_pay_B_fragment_push (rtph263pay, context, gob, first,
              mb - 1, &boundry)) {
        GST_ERROR_OBJECT (rtph263pay, "Oooops, there was an error sending");
        goto decode_error;
      }

      payload_len = 0;
      first = mb;
      GST_DEBUG_OBJECT (rtph263pay,
          "------------------------- END PACKAGE ----------------------");
      GST_DEBUG_OBJECT (rtph263pay,
          "------------------------- NEW PACKAGE ----------------------");
    }
  }

  /* Push rest */
  GST_DEBUG_OBJECT (rtph263pay, "Remainder first: %d, MB: %d", first, mb);
  if (payload_len != 0) {
    GST_DEBUG_OBJECT (rtph263pay, "Push B mode fragment from mb %d to %d",
        first, mb - 1);
    if (gst_rtp_h263_pay_B_fragment_push (rtph263pay, context, gob, first,
            mb - 1, &boundry)) {
      GST_ERROR_OBJECT (rtph263pay, "Oooops, there was an error sending!");
      goto decode_error;
    }
  }

    /*---------- END OF MODE B FRAGMENTATION ----------*/

  return TRUE;

decode_error:
  return FALSE;
}

static GstFlowReturn
gst_rtp_h263_send_entire_frame (GstRtpH263Pay * rtph263pay,
    GstRtpH263PayContext * context)
{
  GstRtpH263PayPackage *pack;
  pack =
      gst_rtp_h263_pay_package_new (rtph263pay->data,
      rtph263pay->data + rtph263pay->available_data,
      rtph263pay->available_data, 0, 0, NULL, TRUE);
  pack->mode = GST_RTP_H263_PAYLOAD_HEADER_MODE_A;

  GST_DEBUG_OBJECT (rtph263pay, "Available data: %d",
      rtph263pay->available_data);

  pack->outbuf =
      gst_rtp_buffer_new_allocate (GST_RTP_H263_PAYLOAD_HEADER_MODE_A, 0, 0);

  return gst_rtp_h263_pay_push (rtph263pay, context, pack);
}

static GstFlowReturn
gst_rtp_h263_pay_flush (GstRtpH263Pay * rtph263pay)
{

  /*
   * FIXME: GSTUF bits are ignored right now,
   *  - not using EBIT/SBIT payload header fields in mode A fragmentation - ffmpeg doesn't need them, but others?
   */

  GstFlowReturn ret;
  GstRtpH263PayContext *context;
  gint i;

  ret = 0;
  context = (GstRtpH263PayContext *) g_malloc0 (sizeof (GstRtpH263PayContext));
  context->mtu =
      rtph263pay->payload.mtu - (MTU_SECURITY_OFFSET + GST_RTP_HEADER_LEN +
      GST_RTP_H263_PAYLOAD_HEADER_MODE_C);

  GST_DEBUG_OBJECT (rtph263pay, "MTU: %d", context->mtu);
  rtph263pay->available_data = gst_buffer_get_size (rtph263pay->current_buffer);
  if (rtph263pay->available_data == 0) {
    ret = GST_FLOW_OK;
    goto end;
  }

  /* Get a pointer to all the data for the frame */
  gst_buffer_map (rtph263pay->current_buffer, &rtph263pay->map, GST_MAP_READ);
  rtph263pay->data = (guint8 *) rtph263pay->map.data;

  /* Picture header */
  context->piclayer = (GstRtpH263PayPic *) rtph263pay->data;

  if (context->piclayer->ptype_pictype == 0)
    GST_DEBUG_OBJECT (rtph263pay, "We got an I-frame");
  else
    GST_DEBUG_OBJECT (rtph263pay, "We got a P-frame");

  context->cpm = rtph263pay->data[6] >> 7;

  GST_DEBUG_OBJECT (rtph263pay, "CPM: %d", context->cpm);

  GST_DEBUG_OBJECT (rtph263pay, "Payload length is: %d",
      rtph263pay->available_data);

  /*
   * - MODE A - If normal, I and P frames,  -> mode A
   *   - GOB layer fragmentation
   * - MODE B - If normal, I and P frames, but GOBs > mtu
   *   - MB layer fragmentation
   * - MODE C - For P frames with PB option, but GOBs > mtu
   *   - MB layer fragmentation
   */
  if (rtph263pay->available_data + GST_RTP_H263_PAYLOAD_HEADER_MODE_A +
      GST_RTP_HEADER_LEN < context->mtu) {
    ret = gst_rtp_h263_send_entire_frame (rtph263pay, context);
  } else {

      /*---------- FRAGMENTING THE FRAME BECAUSE TOO LARGE TO FIT IN MTU ----------*/
    GstRtpH263PayBoundry bound;
    gint first;
    guint payload_len;
    gboolean forcea = FALSE;

    GST_DEBUG_OBJECT (rtph263pay, "Frame too large for MTU");
    /*
     * Let's go trough all the data and fragment it until end is reached
     */

    gst_rtp_h263_pay_boundry_init (&bound, NULL, rtph263pay->data - 1, 0, 0);
    context->gobs =
        (GstRtpH263PayGob **) g_malloc0 (format_props[context->piclayer->
            ptype_srcformat][0] * sizeof (GstRtpH263PayGob *));


    for (i = 0; i < format_props[context->piclayer->ptype_srcformat][0]; i++) {
      GST_DEBUG_OBJECT (rtph263pay, "Searching for gob %d", i);
      if (!gst_rtp_h263_pay_gobfinder (rtph263pay, &bound)) {
        if (i <= 1) {
          GST_WARNING_OBJECT (rtph263pay,
              "No GOB's were found in data stream! Please enable RTP mode in encoder. Forcing mode A for now.");
          ret = gst_rtp_h263_send_entire_frame (rtph263pay, context);
          goto end;
        } else {
          /* try to send fragments corresponding to found GOBs */
          forcea = TRUE;
          break;
        }
      }

      context->gobs[i] = gst_rtp_h263_pay_gob_new (&bound, i);
      //FIXME - encoders may generate an EOS gob that has to be processed
      GST_DEBUG_OBJECT (rtph263pay,
          "Gob values are: gobn: %d, start: %p len: %d ebit: %d sbit: %d", i,
          context->gobs[i]->start, context->gobs[i]->length,
          context->gobs[i]->ebit, context->gobs[i]->sbit);
    }
    /* NOTE some places may still assume this to be the max possible */
    context->no_gobs = i;
    GST_DEBUG_OBJECT (rtph263pay, "Found %d GOBS of maximum %d",
        context->no_gobs, format_props[context->piclayer->ptype_srcformat][0]);

    // Make packages smaller than MTU
    //   A mode
    // - if ( GOB > MTU) -> B mode || C mode
    //   Push packages

    first = 0;
    payload_len = 0;
    i = 0;
    while (i < context->no_gobs) {

      if (context->gobs[i]->length >= context->mtu) {
        if (payload_len == 0) {

          GST_DEBUG_OBJECT (rtph263pay, "GOB len > MTU");
          if (rtph263pay->prop_payload_mode || forcea) {
            payload_len = context->gobs[i]->length;
            goto force_a;
          }
          if (!context->piclayer->ptype_pbmode) {
            GST_DEBUG_OBJECT (rtph263pay, "MODE B on GOB %d needed", i);
            if (!gst_rtp_h263_pay_mode_B_fragment (rtph263pay, context,
                    context->gobs[i])) {
              GST_ERROR_OBJECT (rtph263pay,
                  "There was an error fragmenting in mode B");
              ret = GST_FLOW_ERROR;
              goto end;
            }
          } else {
            //IMPLEMENT C mode
            GST_ERROR_OBJECT (rtph263pay,
                "MODE C on GOB %d needed, but not supported yet", i);
            /*if(!gst_rtp_h263_pay_mode_C_fragment(rtph263pay, context, context->gobs[i])) {
               ret = GST_FLOW_OK;
               GST_ERROR("There was an error fragmenting in mode C");
               goto decode_error;
               } */
            goto decode_error;
          }
        decode_error:
          i++;
          first = i;
          continue;

        } else {
          goto payload_a_push;
        }
      }

      if (payload_len + context->gobs[i]->length < context->mtu) {
        GST_DEBUG_OBJECT (rtph263pay, "GOB %d fills mtu", i);
        payload_len += context->gobs[i]->length;
        i++;
        if (i == context->no_gobs) {
          GST_DEBUG_OBJECT (rtph263pay, "LAST GOB %d", i);
          goto payload_a_push;
        }

      } else {
      payload_a_push:
        GST_DEBUG_OBJECT (rtph263pay,
            "Pushing GOBS %d to %d because payload size is %d", first,
            first == i ? i : i - 1, payload_len);
        gst_rtp_h263_pay_A_fragment_push (rtph263pay, context, first,
            first == i ? i : i - 1);
        payload_len = 0;
        first = i;
      }
      continue;

    force_a:
      GST_DEBUG_OBJECT (rtph263pay,
          "Pushing GOBS %d to %d because payload size is %d", first, i,
          payload_len);
      gst_rtp_h263_pay_A_fragment_push (rtph263pay, context, first, i);
      payload_len = 0;
      i++;
      first = i;
    }


  }/*---------- END OF FRAGMENTATION ----------*/

  /* Flush the input buffer data */

end:
  gst_rtp_h263_pay_context_destroy (context,
      context->piclayer->ptype_srcformat);
  gst_buffer_unmap (rtph263pay->current_buffer, &rtph263pay->map);
  gst_buffer_replace (&rtph263pay->current_buffer, NULL);

  return ret;
}

static GstFlowReturn
gst_rtp_h263_pay_handle_buffer (GstRTPBasePayload * payload, GstBuffer * buffer)
{

  GstRtpH263Pay *rtph263pay;
  GstFlowReturn ret;

  rtph263pay = GST_RTP_H263_PAY (payload);
  GST_DEBUG_OBJECT (rtph263pay,
      "-------------------- NEW FRAME ---------------");

  rtph263pay->first_ts = GST_BUFFER_PTS (buffer);

  gst_buffer_replace (&rtph263pay->current_buffer, buffer);
  gst_buffer_unref (buffer);

  /* we always encode and flush a full picture */
  ret = gst_rtp_h263_pay_flush (rtph263pay);
  GST_DEBUG_OBJECT (rtph263pay,
      "-------------------- END FRAME ---------------");

  return ret;
}

gboolean
gst_rtp_h263_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtph263pay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_H263_PAY);
}
