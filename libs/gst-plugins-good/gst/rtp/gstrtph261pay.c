/* GStreamer
 * Copyright (C) <2014> Stian Selnes <stian@pexip.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/**
 * SECTION:element-rtph261pay
 * @title: rtph261pay
 * @see_also: rtph261depay
 *
 * Payload encoded H.261 video frames into RTP packets according to RFC 4587.
 * For detailed information see: https://www.rfc-editor.org/rfc/rfc4587.txt
 *
 * The payloader takes a H.261 frame, parses it and splits it into fragments
 * on MB boundaries in order to match configured MTU size. For each fragment
 * an RTP packet is constructed with an RTP packet header followed by the
 * fragment. In addition the payloader will make sure the packetized H.261
 * stream appears as a continuous bit-stream after depacketization by shifting
 * the encoded bit-stream of a frame to align with the last significant bit of
 * the previous frame. This helps interoperability in the case where the
 * encoder does not produce a continuous bit-stream but the decoder requires
 * it.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 videotestsrc ! avenc_h261 ! rtph261pay ! udpsink
 * ]| This will encode a test video and payload it. Refer to the rtph261depay
 * example to depayload and play the RTP stream.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gstrtph261pay.h"
#include "gstrtputils.h"
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include <gst/base/gstbitreader.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (rtph261pay_debug);
#define GST_CAT_DEFAULT (rtph261pay_debug)

#define GST_RTP_HEADER_LEN 12
#define GST_RTP_H261_PAYLOAD_HEADER_LEN 4

static GstStaticPadTemplate gst_rtp_h261_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h261")
    );

static GstStaticPadTemplate gst_rtp_h261_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_H261_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H261\"; "
        "application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H261\"")
    );

G_DEFINE_TYPE (GstRtpH261Pay, gst_rtp_h261_pay, GST_TYPE_RTP_BASE_PAYLOAD);
#define parent_class gst_rtp_h261_pay_parent_class

typedef struct
{
  guint32 mba;
  guint32 mtype;
  guint32 quant;
  gint mvx;
  gint mvy;
  guint endpos;
  gint gobn;
} Macroblock;

typedef struct
{
  Macroblock last;
  guint startpos;
  guint endpos;
  guint32 gn;
  guint32 gquant;
} Gob;

#define PSC_LEN 20
#define TR_LEN 5
#define PTYPE_LEN 6
#define GBSC_LEN 16
#define GN_LEN 4
#define GQUANT_LEN 5
#define GEI_LEN 1
#define GSPARE_LEN 8
#define MQUANT_LEN 5
#define MAX_NUM_GOB 12

typedef enum
{
  PARSE_END_OF_BUFFER = -2,
  PARSE_ERROR = -1,
  PARSE_OK = 0,
  PARSE_END_OF_FRAME,
  PARSE_END_OF_GOB,
} ParseReturn;


#define SKIP_BITS(br,nbits) G_STMT_START {      \
    if (!gst_bit_reader_skip (br, nbits))       \
      return PARSE_END_OF_BUFFER;               \
  } G_STMT_END

#define GET_BITS(br,val,nbits) G_STMT_START {             \
    if (!gst_bit_reader_get_bits_uint32 (br, val, nbits)) \
      return PARSE_END_OF_BUFFER;                         \
  } G_STMT_END

/* Unchecked since we peek outside the buffer. Ok because of padding. */
#define PEEK_BITS(br,val,nbits) G_STMT_START {                    \
    *val = gst_bit_reader_peek_bits_uint16_unchecked (br, nbits); \
  } G_STMT_END


#define MBA_STUFFING 34
#define MBA_START_CODE 35
#define MBA_LEN 35
#define MBA_WID 4
/* [code, mask, nbits, mba] */
static const guint16 mba_table[MBA_LEN][MBA_WID] = {
  {0x8000, 0x8000, 1, 1},
  {0x6000, 0xe000, 3, 2},
  {0x4000, 0xe000, 3, 3},
  {0x3000, 0xf000, 4, 4},
  {0x2000, 0xf000, 4, 5},
  {0x1800, 0xf800, 5, 6},
  {0x1000, 0xf800, 5, 7},
  {0x0e00, 0xfe00, 7, 8},
  {0x0c00, 0xfe00, 7, 9},
  {0x0b00, 0xff00, 8, 10},
  {0x0a00, 0xff00, 8, 11},
  {0x0900, 0xff00, 8, 12},
  {0x0800, 0xff00, 8, 13},
  {0x0700, 0xff00, 8, 14},
  {0x0600, 0xff00, 8, 15},
  {0x05c0, 0xffc0, 10, 16},
  {0x0580, 0xffc0, 10, 17},
  {0x0540, 0xffc0, 10, 18},
  {0x0500, 0xffc0, 10, 19},
  {0x04c0, 0xffc0, 10, 20},
  {0x0480, 0xffc0, 10, 21},
  {0x0460, 0xffe0, 11, 22},
  {0x0440, 0xffe0, 11, 23},
  {0x0420, 0xffe0, 11, 24},
  {0x0400, 0xffe0, 11, 25},
  {0x03e0, 0xffe0, 11, 26},
  {0x03c0, 0xffe0, 11, 27},
  {0x03a0, 0xffe0, 11, 28},
  {0x0380, 0xffe0, 11, 29},
  {0x0360, 0xffe0, 11, 30},
  {0x0340, 0xffe0, 11, 31},
  {0x0320, 0xffe0, 11, 32},
  {0x0300, 0xffe0, 11, 33},
  {0x01e0, 0xffe0, 11, MBA_STUFFING},
  {0x0001, 0xffff, 16, MBA_START_CODE},
};

#define MTYPE_INTRA    (1 << 0)
#define MTYPE_INTER    (1 << 1)
#define MTYPE_MC       (1 << 2)
#define MTYPE_FIL      (1 << 3)
#define MTYPE_MQUANT   (1 << 4)
#define MTYPE_MVD      (1 << 5)
#define MTYPE_CBP      (1 << 6)
#define MTYPE_TCOEFF   (1 << 7)
#define MTYPE_LEN 10
#define MTYPE_WID 4
/* [code, mask, nbits, flags] */
static const guint16 mtype_table[MTYPE_LEN][MTYPE_WID] = {
  {0x8000, 0x8000, 1, MTYPE_INTER | MTYPE_CBP | MTYPE_TCOEFF},
  {0x4000, 0xc000, 2,
      MTYPE_INTER | MTYPE_MC | MTYPE_FIL | MTYPE_MVD | MTYPE_CBP |
        MTYPE_TCOEFF},
  {0x2000, 0xe000, 3, MTYPE_INTER | MTYPE_MC | MTYPE_FIL | MTYPE_MVD},
  {0x1000, 0xf000, 4, MTYPE_INTRA | MTYPE_TCOEFF},
  {0x0800, 0xf800, 5, MTYPE_INTER | MTYPE_MQUANT | MTYPE_CBP | MTYPE_TCOEFF},
  {0x0400, 0xfc00, 6,
      MTYPE_INTER | MTYPE_MC | MTYPE_FIL | MTYPE_MQUANT | MTYPE_MVD |
        MTYPE_CBP | MTYPE_TCOEFF},
  {0x0200, 0xfe00, 7, MTYPE_INTRA | MTYPE_MQUANT | MTYPE_TCOEFF},
  {0x0100, 0xff00, 8,
      MTYPE_INTER | MTYPE_MC | MTYPE_MVD | MTYPE_CBP | MTYPE_TCOEFF},
  {0x0080, 0xff80, 9, MTYPE_INTER | MTYPE_MC | MTYPE_MVD},
  {0x0040, 0xffc0, 10,
      MTYPE_INTER | MTYPE_MC | MTYPE_MQUANT | MTYPE_MVD | MTYPE_CBP |
        MTYPE_TCOEFF},
};

#define MVD_LEN 32
#define MVD_WID 5
/* [code, mask, nbits, mvd1, mvd2] */
static const guint16 mvd_table[MVD_LEN][MVD_WID] = {
  {0x8000, 0x8000, 1, 0, 0},
  {0x6000, 0xe000, 3, -1, -1},
  {0x4000, 0xe000, 3, 1, 1},
  {0x3000, 0xf000, 4, -2, 30},
  {0x2000, 0xf000, 4, 2, -30},
  {0x1800, 0xf800, 5, -3, 29},
  {0x1000, 0xf800, 5, 3, -29},
  {0x0e00, 0xfe00, 7, -4, 28},
  {0x0c00, 0xfe00, 7, 4, -28},
  {0x0700, 0xff00, 8, -7, 25},
  {0x0900, 0xff00, 8, -6, 26},
  {0x0b00, 0xff00, 8, -5, 27},
  {0x0a00, 0xff00, 8, 5, -27},
  {0x0800, 0xff00, 8, 6, -26},
  {0x0600, 0xff00, 8, 7, -25},
  {0x04c0, 0xffc0, 10, -10, 22},
  {0x0540, 0xffc0, 10, -9, 23},
  {0x05c0, 0xffc0, 10, -8, 24},
  {0x0580, 0xffc0, 10, 8, -24},
  {0x0500, 0xffc0, 10, 9, -23},
  {0x0480, 0xffc0, 10, 10, -22},
  {0x0320, 0xffe0, 11, -16, 16},
  {0x0360, 0xffe0, 11, -15, 17},
  {0x03a0, 0xffe0, 11, -14, 18},
  {0x03e0, 0xffe0, 11, -13, 19},
  {0x0420, 0xffe0, 11, -12, 20},
  {0x0460, 0xffe0, 11, -11, 21},
  {0x0440, 0xffe0, 11, 11, -21},
  {0x0400, 0xffe0, 11, 12, -20},
  {0x03c0, 0xffe0, 11, 13, -19},
  {0x0380, 0xffe0, 11, 14, -18},
  {0x0340, 0xffe0, 11, 15, -17},
};

#define CBP_LEN 63
/* [code, mask, nbits, cbp] */
static const guint16 cbp_table[CBP_LEN][4] = {
  {0xe000, 0xe000, 3, 60},
  {0xd000, 0xf000, 4, 4},
  {0xc000, 0xf000, 4, 8},
  {0xb000, 0xf000, 4, 16},
  {0xa000, 0xf000, 4, 32},
  {0x9800, 0xf800, 5, 12},
  {0x9000, 0xf800, 5, 48},
  {0x8800, 0xf800, 5, 20},
  {0x8000, 0xf800, 5, 40},
  {0x7800, 0xf800, 5, 28},
  {0x7000, 0xf800, 5, 44},
  {0x6800, 0xf800, 5, 52},
  {0x6000, 0xf800, 5, 56},
  {0x5800, 0xf800, 5, 1},
  {0x5000, 0xf800, 5, 61},
  {0x4800, 0xf800, 5, 2},
  {0x4000, 0xf800, 5, 62},
  {0x3c00, 0xfc00, 6, 24},
  {0x3800, 0xfc00, 6, 36},
  {0x3400, 0xfc00, 6, 3},
  {0x3000, 0xfc00, 6, 63},
  {0x2e00, 0xfe00, 7, 5},
  {0x2c00, 0xfe00, 7, 9},
  {0x2a00, 0xfe00, 7, 17},
  {0x2800, 0xfe00, 7, 33},
  {0x2600, 0xfe00, 7, 6},
  {0x2400, 0xfe00, 7, 10},
  {0x2200, 0xfe00, 7, 18},
  {0x2000, 0xfe00, 7, 34},
  {0x1f00, 0xff00, 8, 7},
  {0x1e00, 0xff00, 8, 11},
  {0x1d00, 0xff00, 8, 19},
  {0x1c00, 0xff00, 8, 35},
  {0x1b00, 0xff00, 8, 13},
  {0x1a00, 0xff00, 8, 49},
  {0x1900, 0xff00, 8, 21},
  {0x1800, 0xff00, 8, 41},
  {0x1700, 0xff00, 8, 14},
  {0x1600, 0xff00, 8, 50},
  {0x1500, 0xff00, 8, 22},
  {0x1400, 0xff00, 8, 42},
  {0x1300, 0xff00, 8, 15},
  {0x1200, 0xff00, 8, 51},
  {0x1100, 0xff00, 8, 23},
  {0x1000, 0xff00, 8, 43},
  {0x0f00, 0xff00, 8, 25},
  {0x0e00, 0xff00, 8, 37},
  {0x0d00, 0xff00, 8, 26},
  {0x0c00, 0xff00, 8, 38},
  {0x0b00, 0xff00, 8, 29},
  {0x0a00, 0xff00, 8, 45},
  {0x0900, 0xff00, 8, 53},
  {0x0800, 0xff00, 8, 57},
  {0x0700, 0xff00, 8, 30},
  {0x0600, 0xff00, 8, 46},
  {0x0500, 0xff00, 8, 54},
  {0x0400, 0xff00, 8, 58},
  {0x0380, 0xff80, 9, 31},
  {0x0300, 0xff80, 9, 47},
  {0x0280, 0xff80, 9, 55},
  {0x0200, 0xff80, 9, 59},
  {0x0180, 0xff80, 9, 27},
  {0x0100, 0xff80, 9, 39},
};

#define TCOEFF_EOB 0xffff
#define TCOEFF_ESC 0xfffe
#define TCOEFF_LEN 65
/* [code, mask, nbits, run, level] */
static const guint16 tcoeff_table[TCOEFF_LEN][5] = {
  {0x8000, 0xc000, 2, TCOEFF_EOB, 0},   /* Not available for first coeff */
  /* {0x8000, 0x8000,  2,  0,  1}, *//* Available only for first Inter coeff */
  {0xc000, 0xc000, 3, 0, 1},    /* Not available for first coeff */
  {0x6000, 0xe000, 4, 1, 1},
  {0x4000, 0xf000, 5, 0, 2},
  {0x5000, 0xf000, 5, 2, 1},
  {0x2800, 0xf800, 6, 0, 3},
  {0x3800, 0xf800, 6, 3, 1},
  {0x3000, 0xf800, 6, 4, 1},
  {0x0400, 0xfc00, 6, TCOEFF_ESC, 0},
  {0x1800, 0xfc00, 7, 1, 2},
  {0x1c00, 0xfc00, 7, 5, 1},
  {0x1400, 0xfc00, 7, 6, 1},
  {0x1000, 0xfc00, 7, 7, 1},
  {0x0c00, 0xfe00, 8, 0, 4},
  {0x0800, 0xfe00, 8, 2, 2},
  {0x0e00, 0xfe00, 8, 8, 1},
  {0x0a00, 0xfe00, 8, 9, 1},
  {0x2600, 0xff00, 9, 0, 5},
  {0x2100, 0xff00, 9, 0, 6},
  {0x2500, 0xff00, 9, 1, 3},
  {0x2400, 0xff00, 9, 3, 2},
  {0x2700, 0xff00, 9, 10, 1},
  {0x2300, 0xff00, 9, 11, 1},
  {0x2200, 0xff00, 9, 12, 1},
  {0x2000, 0xff00, 9, 13, 1},
  {0x0280, 0xffc0, 11, 0, 7},
  {0x0300, 0xffc0, 11, 1, 4},
  {0x02c0, 0xffc0, 11, 2, 3},
  {0x03c0, 0xffc0, 11, 4, 2},
  {0x0240, 0xffc0, 11, 5, 2},
  {0x0380, 0xffc0, 11, 14, 1},
  {0x0340, 0xffc0, 11, 15, 1},
  {0x0200, 0xffc0, 11, 16, 1},
  {0x01d0, 0xfff0, 13, 0, 8},
  {0x0180, 0xfff0, 13, 0, 9},
  {0x0130, 0xfff0, 13, 0, 10},
  {0x0100, 0xfff0, 13, 0, 11},
  {0x01b0, 0xfff0, 13, 1, 5},
  {0x0140, 0xfff0, 13, 2, 4},
  {0x01c0, 0xfff0, 13, 3, 3},
  {0x0120, 0xfff0, 13, 4, 3},
  {0x01e0, 0xfff0, 13, 6, 2},
  {0x0150, 0xfff0, 13, 7, 2},
  {0x0110, 0xfff0, 13, 8, 2},
  {0x01f0, 0xfff0, 13, 17, 1},
  {0x01a0, 0xfff0, 13, 18, 1},
  {0x0190, 0xfff0, 13, 19, 1},
  {0x0170, 0xfff0, 13, 20, 1},
  {0x0160, 0xfff0, 13, 21, 1},
  {0x00d0, 0xfff8, 14, 0, 12},
  {0x00c8, 0xfff8, 14, 0, 13},
  {0x00c0, 0xfff8, 14, 0, 14},
  {0x00b8, 0xfff8, 14, 0, 15},
  {0x00b0, 0xfff8, 14, 1, 6},
  {0x00a8, 0xfff8, 14, 1, 7},
  {0x00a0, 0xfff8, 14, 2, 5},
  {0x0098, 0xfff8, 14, 3, 4},
  {0x0090, 0xfff8, 14, 5, 3},
  {0x0088, 0xfff8, 14, 9, 2},
  {0x0080, 0xfff8, 14, 10, 2},
  {0x00f8, 0xfff8, 14, 22, 1},
  {0x00f0, 0xfff8, 14, 23, 1},
  {0x00e8, 0xfff8, 14, 24, 1},
  {0x00e0, 0xfff8, 14, 25, 1},
  {0x00d8, 0xfff8, 14, 26, 1},
};

static ParseReturn
decode_mba (GstBitReader * br, gint * mba)
{
  gint i;
  guint16 code;

  *mba = -1;
  do {
    PEEK_BITS (br, &code, 16);
    for (i = 0; i < MBA_LEN; i++) {
      if ((code & mba_table[i][1]) == mba_table[i][0]) {
        *mba = mba_table[i][3];

        if (*mba == MBA_START_CODE)
          return PARSE_END_OF_GOB;
        SKIP_BITS (br, mba_table[i][2]);
        if (*mba != MBA_STUFFING)
          return PARSE_OK;
      }
    }
  } while (*mba == MBA_STUFFING);

  /* 0x0 indicates end of frame since we appended 0-bytes */
  if (code == 0x0)
    return PARSE_END_OF_FRAME;

  return PARSE_ERROR;
}

static ParseReturn
decode_mtype (GstBitReader * br, guint * mtype)
{
  gint i;
  guint16 code;

  PEEK_BITS (br, &code, 16);
  for (i = 0; i < MTYPE_LEN; i++) {
    if ((code & mtype_table[i][1]) == mtype_table[i][0]) {
      SKIP_BITS (br, mtype_table[i][2]);
      *mtype = mtype_table[i][3];
      return PARSE_OK;
    }
  }

  return PARSE_ERROR;
}

static ParseReturn
decode_mvd (GstBitReader * br, gint * mvd1, gint * mvd2)
{
  gint i;
  guint16 code;

  PEEK_BITS (br, &code, 16);
  for (i = 0; i < MVD_LEN; i++) {
    if ((code & mvd_table[i][1]) == mvd_table[i][0]) {
      SKIP_BITS (br, mvd_table[i][2]);
      *mvd1 = (gint16) mvd_table[i][3];
      *mvd2 = (gint16) mvd_table[i][4];
      return PARSE_OK;
    }
  }

  return PARSE_ERROR;
}

static ParseReturn
decode_cbp (GstBitReader * br, guint * cbp)
{
  gint i;
  guint16 code;

  PEEK_BITS (br, &code, 16);
  for (i = 0; i < CBP_LEN; i++) {
    if ((code & cbp_table[i][1]) == cbp_table[i][0]) {
      SKIP_BITS (br, cbp_table[i][2]);
      *cbp = cbp_table[i][3];
      return PARSE_OK;
    }
  }

  return PARSE_ERROR;
}

static ParseReturn
decode_tcoeff (GstBitReader * br, guint mtype)
{
  gint i;
  guint16 code;
  gboolean eob;

  /* Special handling of first coeff */
  if (mtype & MTYPE_INTER) {
    /* Inter, different vlc since EOB is not allowed */
    PEEK_BITS (br, &code, 16);
    if (code & 0x8000) {
      SKIP_BITS (br, 2);
      GST_TRACE ("tcoeff first inter special");
    } else {
      /* Fallthrough. Let the first coeff be handled like other coeffs since
       * the vlc is the same as long as the first bit is not set. */
    }
  } else {
    /* Intra, first coeff is fixed 8-bit */
    GST_TRACE ("tcoeff first intra special");
    SKIP_BITS (br, 8);
  }

  /* Block must end with EOB. */
  eob = FALSE;
  while (!eob) {
    PEEK_BITS (br, &code, 16);
    for (i = 0; i < TCOEFF_LEN; i++) {
      if ((code & tcoeff_table[i][1]) == tcoeff_table[i][0]) {
        GST_TRACE ("tcoeff vlc[%d], run=%d, level=%d", i, tcoeff_table[i][3],
            tcoeff_table[i][4]);
        SKIP_BITS (br, tcoeff_table[i][2]);
        if (tcoeff_table[i][3] == TCOEFF_EOB) {
          eob = TRUE;
        } else if (tcoeff_table[i][3] == TCOEFF_ESC) {
#if 0
          guint16 val;
          val = gst_bit_reader_peek_bits_uint16_unchecked (br, 6 + 8);
          GST_TRACE ("esc run=%d, level=%d", val >> 8, (gint8) (val & 0xff));
#endif
          SKIP_BITS (br, 6 + 8);
        }
        break;
      }
    }
    if (i == TCOEFF_LEN)
      /* No matching VLC */
      return PARSE_ERROR;
  }

  return PARSE_OK;
}

static gint
find_picture_header_offset (const guint8 * data, gsize size)
{
  gint i;
  guint32 val;

  if (size < 4)
    return -1;

  val = GST_READ_UINT32_BE (data);
  for (i = 0; i < 8; i++) {
    if ((val >> (12 - i)) == 0x10)
      return i;
  }

  return -1;
}

static ParseReturn
parse_picture_header (GstRtpH261Pay * pay, GstBitReader * br, gint * num_gob)
{
  guint32 val;

  GET_BITS (br, &val, PSC_LEN);
  if (val != 0x10)
    return PARSE_ERROR;
  SKIP_BITS (br, TR_LEN);
  GET_BITS (br, &val, PTYPE_LEN);
  *num_gob = (val & 0x04) == 0 ? 3 : 12;

  return PARSE_OK;
}

static ParseReturn
parse_gob_header (GstRtpH261Pay * pay, GstBitReader * br, Gob * gob)
{
  guint32 val;

  GET_BITS (br, &val, GBSC_LEN);
  if (val != 0x01)
    return PARSE_ERROR;
  GET_BITS (br, &gob->gn, GN_LEN);
  GST_TRACE_OBJECT (pay, "Parsing GOB %d", gob->gn);

  GET_BITS (br, &gob->gquant, GQUANT_LEN);
  GST_TRACE_OBJECT (pay, "GQUANT %d", gob->gquant);
  GET_BITS (br, &val, GEI_LEN);
  while (val != 0) {
    SKIP_BITS (br, GSPARE_LEN);
    GET_BITS (br, &val, GEI_LEN);
  }

  return PARSE_OK;
}

static ParseReturn
parse_mb (GstRtpH261Pay * pay, GstBitReader * br, const Macroblock * prev,
    Macroblock * mb)
{
  gint mba_diff;
  guint cbp;
  ParseReturn ret;

  cbp = 0x3f;
  mb->quant = prev->quant;

  if ((ret = decode_mba (br, &mba_diff)) != PARSE_OK)
    return ret;
  mb->mba = prev->mba == 0 ? mba_diff : prev->mba + mba_diff;
  GST_TRACE_OBJECT (pay, "Parse MBA %d (mba_diff %d)", mb->mba, mba_diff);

  if ((ret = decode_mtype (br, &mb->mtype)) != PARSE_OK)
    return ret;
  GST_TRACE_OBJECT (pay,
      "MTYPE: inter %d, mc %d, fil %d, mquant %d, mvd %d, cbp %d, tcoeff %d",
      (mb->mtype & MTYPE_INTER) != 0, (mb->mtype & MTYPE_MC) != 0,
      (mb->mtype & MTYPE_FIL) != 0, (mb->mtype & MTYPE_MQUANT) != 0,
      (mb->mtype & MTYPE_MVD) != 0, (mb->mtype & MTYPE_CBP) != 0,
      (mb->mtype & MTYPE_TCOEFF) != 0);

  if (mb->mtype & MTYPE_MQUANT) {
    GET_BITS (br, &mb->quant, MQUANT_LEN);
    GST_TRACE_OBJECT (pay, "MQUANT: %d", mb->quant);
  }

  if (mb->mtype & MTYPE_MVD) {
    gint i, pmv[2], mv[2];

    if (mb->mba == 1 || mb->mba == 12 || mb->mba == 23 || mba_diff != 1 ||
        (prev->mtype & MTYPE_INTER) == 0) {
      pmv[0] = 0;
      pmv[1] = 0;
    } else {
      pmv[0] = prev->mvx;
      pmv[1] = prev->mvy;
    }
    for (i = 0; i < 2; i++) {
      gint mvd1, mvd2;
      if ((ret = decode_mvd (br, &mvd1, &mvd2)) != PARSE_OK)
        return ret;
      if (ABS (pmv[i] + mvd1) <= 15)
        mv[i] = pmv[i] + mvd1;
      else
        mv[i] = pmv[i] + mvd2;
    }
    mb->mvx = mv[0];
    mb->mvy = mv[1];
  } else {
    mb->mvx = 0;
    mb->mvy = 0;
  }

  if (mb->mtype & MTYPE_CBP) {
    if ((ret = decode_cbp (br, &cbp)) != PARSE_OK)
      return ret;
  }

  /* Block layer */
  if (mb->mtype & MTYPE_TCOEFF) {
    gint block;
    for (block = 0; block < 6; block++) {
      if (cbp & (1 << (5 - block))) {
        GST_TRACE_OBJECT (pay, "Decode TCOEFF for block %d", block);
        if ((ret = decode_tcoeff (br, mb->mtype)) != PARSE_OK)
          return ret;
      }
    }
  }

  mb->endpos = gst_bit_reader_get_pos (br);

  return ret;
}

/* Parse macroblocks until the next MB that exceeds maxpos. At least one MB is
 * included even if it exceeds maxpos. Returns endpos of last included MB. */
static ParseReturn
parse_mb_until_pos (GstRtpH261Pay * pay, GstBitReader * br, Gob * gob,
    guint * endpos)
{
  ParseReturn ret;
  gint count = 0;
  gboolean stop = FALSE;
  guint maxpos = *endpos;
  Macroblock mb;

  GST_LOG_OBJECT (pay, "Parse until pos %u, start at pos %u, gobn %d, mba %d",
      maxpos, gst_bit_reader_get_pos (br), gob->gn, gob->last.mba);

  while (!stop) {
    ret = parse_mb (pay, br, &gob->last, &mb);

    switch (ret) {
      case PARSE_OK:
        if (mb.endpos > maxpos && count > 0) {
          /* Don't include current MB */
          stop = TRUE;
        } else {
          /* Update to include current MB */
          *endpos = mb.endpos;
          gob->last = mb;
          count++;
        }
        break;

      case PARSE_END_OF_FRAME:
        *endpos = gst_bit_reader_get_pos (br);
        GST_DEBUG_OBJECT (pay, "End of frame at pos %u (last GOBN %d MBA %d)",
            *endpos, gob->gn, gob->last.mba);
        stop = TRUE;
        break;

      case PARSE_END_OF_GOB:
        /* Note that a GOB can contain nothing, so we may get here on the first
         * iteration. */
        *endpos = gob->last.mba == 0 ?
            gob->startpos : gst_bit_reader_get_pos (br);
        GST_DEBUG_OBJECT (pay, "End of gob at pos %u (last GOBN %d MBA %d)",
            *endpos, gob->gn, gob->last.mba);
        stop = TRUE;
        break;

      case PARSE_END_OF_BUFFER:
      case PARSE_ERROR:
        GST_WARNING_OBJECT (pay, "Failed to parse stream (reason %d)", ret);
        return ret;
        break;

      default:
        g_assert_not_reached ();
        break;
    }
  }
  gob->last.gobn = gob->gn;

  if (ret == PARSE_OK) {
    GST_DEBUG_OBJECT (pay,
        "Split GOBN %d after MBA %d (endpos %u, maxpos %u, nextpos %u)",
        gob->gn, gob->last.mba, *endpos, maxpos, mb.endpos);
    gst_bit_reader_set_pos (br, *endpos);
  }

  return ret;
}

static guint
bitrange_to_bytes (guint first, guint last)
{
  return (GST_ROUND_UP_8 (last) - GST_ROUND_DOWN_8 (first)) / 8;
}

/* Find next 16-bit GOB start code (0x0001), which may not be byte aligned.
 * Returns the bit offset of the first bit of GBSC. */
static gssize
find_gob (GstRtpH261Pay * pay, const guint8 * data, guint size, guint pos)
{
  gssize ret = -1;
  guint offset;

  GST_LOG_OBJECT (pay, "Search for GOB from pos %u", pos);

  for (offset = pos / 8; offset < size - 1; offset++) {
    if (data[offset] == 0x0) {
      gint msb = g_bit_nth_msf (data[offset + 1], 8);
      gint lsb = offset > 0 ? g_bit_nth_lsf (data[offset - 1], -1) : 0;
      if (lsb == -1)
        lsb = 8;
      if (msb >= 0 && lsb >= msb) {
        ret = offset * 8 - msb;
        GST_LOG_OBJECT (pay, "Found GOB start code at bitpos %"
            G_GSSIZE_FORMAT " (%02x %02x %02x)", ret,
            offset > 0 ? data[offset - 1] : 0, data[offset], data[offset + 1]);
        break;
      }
    }
  }

  return ret;
}

/* Scans after all GOB start codes and initializes the GOB structure with start
 * and end positions. */
static ParseReturn
gst_rtp_h261_pay_init_gobs (GstRtpH261Pay * pay, Gob * gobs, gint num_gobs,
    const guint8 * bits, gint len, guint pos)
{
  gint i;

  for (i = 0; i < num_gobs; i++) {
    gssize gobpos = find_gob (pay, bits, len, pos);
    if (gobpos == -1) {
      GST_WARNING_OBJECT (pay, "Found only %d of %d GOBs", i, num_gobs);
      return PARSE_ERROR;
    }
    GST_LOG_OBJECT (pay, "Found GOB %d at pos %" G_GSSIZE_FORMAT, i, gobpos);
    pos = gobpos + GBSC_LEN;

    gobs[i].startpos = gobpos;
    if (i > 0)
      gobs[i - 1].endpos = gobpos;
  }
  gobs[num_gobs - 1].endpos = len * 8;

  return PARSE_OK;
}

static GstFlowReturn
gst_rtp_h261_pay_fragment_push (GstRtpH261Pay * pay, GstBuffer * buffer,
    const guint8 * bits, guint start, guint end,
    const Macroblock * last_mb_in_previous_packet, gboolean marker)
{
  GstBuffer *outbuf;
  guint8 *payload;
  GstRtpH261PayHeader *header;
  gint nbytes;
  const Macroblock *last = last_mb_in_previous_packet;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  nbytes = bitrange_to_bytes (start, end);

  outbuf = gst_rtp_buffer_new_allocate (nbytes +
      GST_RTP_H261_PAYLOAD_HEADER_LEN, 0, 0);
  gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);
  payload = gst_rtp_buffer_get_payload (&rtp);
  header = (GstRtpH261PayHeader *) payload;

  memset (header, 0, GST_RTP_H261_PAYLOAD_HEADER_LEN);
  header->v = 1;
  header->sbit = start & 7;
  header->ebit = (8 - (end & 7)) & 7;

  if (last != NULL && last->mba != 0 && last->mba != 33) {
    /* NOTE: MVD assumes that we're running on 2's complement architecture */
    guint mbap = last->mba - 1;
    header->gobn = last->gobn;
    header->mbap1 = mbap >> 1;
    header->mbap2 = mbap & 1;
    header->quant = last->quant;
    header->hmvd1 = last->mvx >> 3;
    header->hmvd2 = last->mvx & 7;
    header->vmvd = last->mvy;
  }

  memcpy (payload + GST_RTP_H261_PAYLOAD_HEADER_LEN,
      bits + GST_ROUND_DOWN_8 (start) / 8, nbytes);

  GST_BUFFER_TIMESTAMP (outbuf) = pay->timestamp;
  gst_rtp_buffer_set_marker (&rtp, marker);
  pay->offset = end & 7;

  GST_DEBUG_OBJECT (pay,
      "Push fragment, bytes %d, sbit %d, ebit %d, gobn %d, mbap %d, marker %d",
      nbytes, header->sbit, header->ebit, last != NULL ? last->gobn : 0,
      last != NULL ? MAX (last->mba - 1, 0) : 0, marker);

  gst_rtp_buffer_unmap (&rtp);

  gst_rtp_copy_video_meta (pay, outbuf, buffer);

  return gst_rtp_base_payload_push (GST_RTP_BASE_PAYLOAD_CAST (pay), outbuf);
}

static GstFlowReturn
gst_rtp_h261_packetize_and_push (GstRtpH261Pay * pay, GstBuffer * buffer,
    const guint8 * bits, gsize len)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstBitReader br_;
  GstBitReader *br = &br_;
  guint max_payload_size =
      gst_rtp_buffer_calc_payload_len (GST_RTP_BASE_PAYLOAD_MTU (pay) -
      GST_RTP_H261_PAYLOAD_HEADER_LEN, 0, 0);
  guint startpos;
  gint num_gobs = 0;
  Gob gobs[MAX_NUM_GOB];
  Gob *gob;
  Macroblock last_mb_in_previous_packet = { 0 };
  gboolean marker;
  ParseReturn result;

  gst_bit_reader_init (br, bits, len);
  gst_bit_reader_set_pos (br, pay->offset);
  startpos = pay->offset;

  if (parse_picture_header (pay, br, &num_gobs) < PARSE_OK) {
    GST_WARNING_OBJECT (pay, "Failed to parse picture header");
    goto beach;
  }

  if (gst_rtp_h261_pay_init_gobs (pay, gobs, num_gobs, bits, len,
          gst_bit_reader_get_pos (br)) < PARSE_OK)
    goto beach;

  /* Split, create and push packets */
  gob = gobs;
  marker = FALSE;
  while (marker == FALSE && ret == GST_FLOW_OK) {
    guint endpos;

    /* Check if there is wrap around because of extremely high MTU */
    endpos = GST_ROUND_DOWN_8 (startpos) + max_payload_size * 8;
    if (endpos < startpos)
      endpos = G_MAXUINT;

    GST_LOG_OBJECT (pay, "Next packet startpos %u maxpos %u", startpos, endpos);

    /* Find the last GOB that does not completely fit in packet */
    for (; gob < &gobs[num_gobs - 1]; gob++) {
      if (bitrange_to_bytes (startpos, gob->endpos) > max_payload_size) {
        GST_LOG_OBJECT (pay, "Split gob (start %u, end %u)",
            gob->startpos, gob->endpos);
        break;
      }
    }

    if (startpos <= gob->startpos) {
      /* Fast-forward until start of GOB */
      gst_bit_reader_set_pos (br, gob->startpos);
      if (parse_gob_header (pay, br, gob) < PARSE_OK) {
        GST_WARNING_OBJECT (pay, "Failed to parse GOB header");
        goto beach;
      }
      gob->last.mba = 0;
      gob->last.gobn = gob->gn;
      gob->last.quant = gob->gquant;
    }

    /* Parse MBs to find position where to split. Can only be done on after MB
     * or at GOB boundary. */
    result = parse_mb_until_pos (pay, br, gob, &endpos);
    if (result < PARSE_OK)
      goto beach;

    marker = result == PARSE_END_OF_FRAME;
    ret = gst_rtp_h261_pay_fragment_push (pay, buffer, bits, startpos, endpos,
        &last_mb_in_previous_packet, marker);

    last_mb_in_previous_packet = gob->last;
    if (endpos == gob->endpos)
      gob++;
    startpos = endpos;
  }

beach:
  return ret;
}

/* Shift buffer to packetize a continuous stream of bits (not bytes). Some
 * payloaders/decoders are very picky about correct sbit/ebit for frames. */
static guint8 *
gst_rtp_h261_pay_shift_buffer (GstRtpH261Pay * pay, const guint8 * data,
    gsize size, gint offset, gsize * newsize)
{
  /* In order to read variable length codes at the very end of the buffer
   * without peeking into possibly unallocated data, we pad with extra 0's
   * which will generate an invalid code at the end of the buffer. */
  guint pad = 4;
  gsize allocsize = size + pad;
  guint8 *bits = g_malloc (allocsize);
  gint i;

  if (offset == 0) {
    memcpy (bits, data, size);
    *newsize = size;
  } else if (offset > 0) {
    bits[0] = 0;
    for (i = 0; i < size; i++) {
      bits[i] |= data[i] >> offset;
      bits[i + 1] = data[i] << (8 - offset);
    }
    *newsize = size + 1;
  } else {
    gint shift = -offset;
    for (i = 0; i < size - 1; i++)
      bits[i] = (data[i] << shift) | (data[i + 1] >> (8 - shift));
    bits[i] = data[i] << shift;
    *newsize = size;
  }
  for (i = *newsize; i < allocsize; i++)
    bits[i] = 0;

  return bits;
}

static GstFlowReturn
gst_rtp_h261_pay_handle_buffer (GstRTPBasePayload * payload, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstRtpH261Pay *pay = GST_RTP_H261_PAY (payload);
  gsize len;
  guint8 *bits;
  gint psc_offset, shift;
  GstMapInfo map;

  GST_DEBUG_OBJECT (pay, "Handle buffer of size %" G_GSIZE_FORMAT,
      gst_buffer_get_size (buffer));

  pay->timestamp = GST_BUFFER_TIMESTAMP (buffer);

  if (!gst_buffer_map (buffer, &map, GST_MAP_READ) || !map.data) {
    GST_WARNING_OBJECT (pay, "Failed to map buffer");
    return GST_FLOW_ERROR;
  }

  psc_offset = find_picture_header_offset (map.data, map.size);
  if (psc_offset < 0) {
    GST_WARNING_OBJECT (pay, "Failed to find picture header offset");
    goto beach;
  } else {
    GST_DEBUG_OBJECT (pay, "Picture header offset: %d", psc_offset);
  }

  shift = pay->offset - psc_offset;
  bits = gst_rtp_h261_pay_shift_buffer (pay, map.data, map.size, shift, &len);
  ret = gst_rtp_h261_packetize_and_push (pay, buffer, bits, len);
  g_free (bits);

beach:
  gst_buffer_unmap (buffer, &map);
  gst_buffer_unref (buffer);
  return ret;
}


static gboolean
gst_rtp_h261_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  gboolean res;

  gst_rtp_base_payload_set_options (payload, "video",
      payload->pt != GST_RTP_PAYLOAD_H261, "H261", 90000);
  res = gst_rtp_base_payload_set_outcaps (payload, NULL);

  return res;
}

static void
gst_rtp_h261_pay_init (GstRtpH261Pay * pay)
{
  GstRTPBasePayload *payload = GST_RTP_BASE_PAYLOAD (pay);
  payload->pt = GST_RTP_PAYLOAD_H261;
  pay->offset = 0;
}

static void
gst_rtp_h261_pay_class_init (GstRtpH261PayClass * klass)
{
  GstElementClass *element_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  element_class = GST_ELEMENT_CLASS (klass);
  gstrtpbasepayload_class = GST_RTP_BASE_PAYLOAD_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_h261_pay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_h261_pay_sink_template);

  gst_element_class_set_static_metadata (element_class,
      "RTP H261 packet payloader", "Codec/Payloader/Network/RTP",
      "Payload-encodes H261 video in RTP packets (RFC 4587)",
      "Stian Selnes <stian@pexip.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_h261_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_h261_pay_handle_buffer;

  GST_DEBUG_CATEGORY_INIT (rtph261pay_debug, "rtph261pay", 0,
      "H261 RTP Payloader");
}

gboolean
gst_rtp_h261_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtph261pay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_H261_PAY);
}
