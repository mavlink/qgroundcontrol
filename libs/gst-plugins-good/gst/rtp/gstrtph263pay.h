/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
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
 * Author: Dejan Sakelsak sahel@kiberpipa.org
 */

#ifndef __GST_RTP_H263_PAY_H__
#define __GST_RTP_H263_PAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasepayload.h>

G_BEGIN_DECLS
#define GST_TYPE_RTP_H263_PAY \
  (gst_rtp_h263_pay_get_type())
#define GST_RTP_H263_PAY(obj) \
 (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_H263_PAY,GstRtpH263Pay))
#define GST_RTP_H263_PAY_CLASS(klass) \
 (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_H263_PAY,GstRtpH263PayClass))
#define GST_IS_RTP_H263_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_H263_PAY))
#define GST_IS_RTP_H263_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_H263_PAY))
// blocks per macroblock
#define N_BLOCKS 6
#define DEFAULT_MODE_A FALSE
#define MTU_SECURITY_OFFSET 50
    typedef enum _GstRtpH263PayHeaderMode
{
  GST_RTP_H263_PAYLOAD_HEADER_MODE_A = 4,
  GST_RTP_H263_PAYLOAD_HEADER_MODE_B = 8,
  GST_RTP_H263_PAYLOAD_HEADER_MODE_C = 12
} GstRtpH263PayHeaderMode;

typedef struct _GstRtpH263PayContext GstRtpH263PayContext;
typedef struct _GstRtpH263PayPic GstRtpH263PayPic;
typedef struct _GstRtpH263PayClass GstRtpH263PayClass;
typedef struct _GstRtpH263Pay GstRtpH263Pay;
typedef struct _GstRtpH263PayBoundry GstRtpH263PayBoundry;
typedef struct _GstRtpH263PayMB GstRtpH263PayMB;
typedef struct _GstRtpH263PayGob GstRtpH263PayGob;
typedef struct _GstRtpH263PayPackage GstRtpH263PayPackage;

//typedef enum _GstRtpH263PayHeaderMode GstRtpH263PayHeaderMode;

struct _GstRtpH263Pay
{
  GstRTPBasePayload payload;

  GstBuffer *current_buffer;
  GstMapInfo map;

  GstClockTime first_ts;
  gboolean prop_payload_mode;
  guint8 *data;
  guint available_data;

};

struct _GstRtpH263PayContext
{
  GstRtpH263PayPic *piclayer;

  guint mtu;
  guint window;
  guint8 *win_end;
  guint8 cpm;

  guint no_gobs;
  GstRtpH263PayGob **gobs;

};

struct _GstRtpH263PayClass
{
  GstRTPBasePayloadClass parent_class;
};

typedef struct _GstRtpH263PayAHeader
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  unsigned int ebit:3;          /* End position */
  unsigned int sbit:3;          /* Start position */
  unsigned int p:1;             /* PB-frames mode */
  unsigned int f:1;             /* flag bit */

  unsigned int r1:1;            /* Reserved */
  unsigned int a:1;             /* Advanced Prediction */
  unsigned int s:1;             /* syntax based arithmetic coding */
  unsigned int u:1;             /* Unrestricted motion vector */
  unsigned int i:1;             /* Picture coding type */
  unsigned int src:3;           /* Source format */

  unsigned int trb:3;           /* Temporal ref for B frame */
  unsigned int dbq:2;           /* Differential Quantisation parameter */
  unsigned int r2:3;            /* Reserved */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
  unsigned int f:1;             /* flag bit */
  unsigned int p:1;             /* PB-frames mode */
  unsigned int sbit:3;          /* Start position */
  unsigned int ebit:3;          /* End position */

  unsigned int src:3;           /* Source format */
  unsigned int i:1;             /* Picture coding type */
  unsigned int u:1;             /* Unrestricted motion vector */
  unsigned int s:1;             /* syntax based arithmetic coding */
  unsigned int a:1;             /* Advanced Prediction */
  unsigned int r1:1;            /* Reserved */

  unsigned int r2:3;            /* Reserved */
  unsigned int dbq:2;           /* Differential Quantisation parameter */
  unsigned int trb:3;           /* Temporal ref for B frame */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
  unsigned int tr:8;            /* Temporal ref for P frame */
} GstRtpH263PayAHeader;

typedef struct _GstRtpH263PayBHeader
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  unsigned int ebit:3;          /* End position */
  unsigned int sbit:3;          /* Start position */
  unsigned int p:1;             /* PB-frames mode */
  unsigned int f:1;             /* flag bit */

  unsigned int quant:5;         /* Quantization value for first MB */
  unsigned int src:3;           /* Source format */

  unsigned int mba1:3;          /* Address of first MB starting count from 0 - part1 */
  unsigned int gobn:5;          /* GOB number in effect at start of packet */

  unsigned int r:2;             /* Reserved */
  unsigned int mba2:6;          /* Address of first MB starting count from 0 - part2 */

  unsigned int hmv11:4;         /* horizontal motion vector predictor for MB 1 - part 1 */
  unsigned int a:1;             /* Advanced Prediction */
  unsigned int s:1;             /* syntax based arithmetic coding */
  unsigned int u:1;             /* Unrestricted motion vector */
  unsigned int i:1;             /* Picture coding type */

  unsigned int vmv11:5;         /* vertical motion vector predictor for MB 1 - part 1 */
  unsigned int hmv12:3;         /* horizontal motion vector predictor for MB 1 - part 2 */

  unsigned int hmv21:6;         /* horizontal motion vector predictor for MB 3 - part 1 */
  unsigned int vmv12:2;         /* vertical motion vector predictor for MB 1 - part 2 */

  unsigned int vmv21:7;         /* vertical motion vector predictor for MB 3 */
  unsigned int hmv22:1;         /* horizontal motion vector predictor for MB 3 - part 2 */

#elif G_BYTE_ORDER == G_BIG_ENDIAN
  unsigned int f:1;             /* flag bit */
  unsigned int p:1;             /* PB-frames mode */
  unsigned int sbit:3;          /* Start position */
  unsigned int ebit:3;          /* End position */

  unsigned int src:3;           /* Source format */
  unsigned int quant:5;         /* Quantization value for first MB */

  unsigned int gobn:5;          /* GOB number in effect at start of packet */
  unsigned int mba1:3;          /* Address of first MB starting count from 0 - part1 */

  unsigned int mba2:6;          /* Address of first MB starting count from 0 - part2 */
  unsigned int r:2;             /* Reserved */

  unsigned int i:1;             /* Picture coding type */
  unsigned int u:1;             /* Unrestricted motion vector */
  unsigned int s:1;             /* syntax based arithmetic coding */
  unsigned int a:1;             /* Advanced Prediction */
  unsigned int hmv11:4;         /* horizontal motion vector predictor for MB 1 - part 1 */

  unsigned int hmv12:3;         /* horizontal motion vector predictor for MB 1 - part 2 */
  unsigned int vmv11:5;         /* vertical motion vector predictor for MB 1 - part 1 */

  unsigned int vmv12:2;         /* vertical motion vector predictor for MB 1 - part 2 */
  unsigned int hmv21:6;         /* horizontal motion vector predictor for MB 3 - part 1 */

  unsigned int hmv22:1;         /* horizontal motion vector predictor for MB 3 - part 2 */
  unsigned int vmv21:7;          /* vertical motion vector predictor for MB 3 */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
} GstRtpH263PayBHeader;

typedef struct _GstRtpH263PayCHeader
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  unsigned int ebit:3;          /* End position */
  unsigned int sbit:3;          /* Start position */
  unsigned int p:1;             /* PB-frames mode */
  unsigned int f:1;             /* flag bit */

  unsigned int quant:5;         /* Quantization value for first MB */
  unsigned int src:3;           /* Source format */

  unsigned int mba1:3;          /* Address of first MB starting count from 0 - part1 */
  unsigned int gobn:5;          /* GOB number in effect at start of packet */

  unsigned int r:2;             /* Reserved */
  unsigned int mba2:6;          /* Address of first MB starting count from 0 - part2 */

  unsigned int hmv11:4;         /* horizontal motion vector predictor for MB 1 - part 1 */
  unsigned int a:1;             /* Advanced Prediction */
  unsigned int s:1;             /* syntax based arithmetic coding */
  unsigned int u:1;             /* Unrestricted motion vector */
  unsigned int i:1;             /* Picture coding type */

  unsigned int vmv11:5;         /* vertical motion vector predictor for MB 1 - part 1 */
  unsigned int hmv12:3;         /* horizontal motion vector predictor for MB 1 - part 2 */

  unsigned int hmv21:6;         /* horizontal motion vector predictor for MB 3 - part 1 */
  unsigned int vmv12:2;         /* vertical motion vector predictor for MB 1 - part 2 */

  unsigned int vmv21:7;         /* vertical motion vector predictor for MB 3 */
  unsigned int hmv22:1;         /* horizontal motion vector predictor for MB 3 - part 2 */

  unsigned int rr1:8;           /* reserved */

  unsigned int rr2:8;           /* reserved */

  unsigned int trb:3;           /* Temporal Reference for the B */
  unsigned int dbq:2;           /* Differential quantization parameter */
  unsigned int rr3:3;           /* reserved */

  unsigned int tr:8;            /* Temporal Reference for the P frame */

#elif G_BYTE_ORDER == G_BIG_ENDIAN
  unsigned int f:1;             /* flag bit */
  unsigned int p:1;             /* PB-frames mode */
  unsigned int sbit:3;          /* Start position */
  unsigned int ebit:3;          /* End position */

  unsigned int src:3;           /* Source format */
  unsigned int quant:5;         /* Quantization value for first MB */

  unsigned int gobn:5;          /* GOB number in effect at start of packet */
  unsigned int mba1:3;          /* Address of first MB starting count from 0 - part1 */

  unsigned int mba2:6;          /* Address of first MB starting count from 0 - part2 */
  unsigned int r:2;             /* Reserved */

  unsigned int i:1;             /* Picture coding type */
  unsigned int u:1;             /* Unrestricted motion vector */
  unsigned int s:1;             /* syntax based arithmetic coding */
  unsigned int a:1;             /* Advanced Prediction */
  unsigned int hmv11:4;         /* horizontal motion vector predictor for MB 1 - part 1 */

  unsigned int hmv12:3;         /* horizontal motion vector predictor for MB 1 - part 2 */
  unsigned int vmv11:5;         /* vertical motion vector predictor for MB 1 - part 1 */

  unsigned int vmv12:2;         /* vertical motion vector predictor for MB 1 - part 2 */
  unsigned int hmv21:6;         /* horizontal motion vector predictor for MB 3 - part 1 */

  unsigned int hmv22:1;         /* horizontal motion vector predictor for MB 3 - part 2 */
  unsigned int vmv21:7;          /* vertical motion vector predictor for MB 3 */
  unsigned int rr1:8;           /* reserved */
  unsigned int rr2:8;           /* reserved */

  unsigned int rr3:3;           /* reserved */
  unsigned int dbq:2;           /* Differential quantization parameter */
  unsigned int trb:3;           /* Temporal Reference for the B */

  unsigned int tr:8;            /* Temporal Reference for the P frame */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
} GstRtpH263PayCHeader;

struct _GstRtpH263PayPic
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  unsigned int psc1:16;

  unsigned int tr1:2;
  unsigned int psc2:6;

  unsigned int ptype_263:1;
  unsigned int ptype_start:1;
  unsigned int tr2:6;

  unsigned int ptype_umvmode:1;
  unsigned int ptype_pictype:1;
  unsigned int ptype_srcformat:3;
  unsigned int ptype_freeze:1;
  unsigned int ptype_camera:1;
  unsigned int ptype_split:1;

  unsigned int pquant:5;
  unsigned int ptype_pbmode:1;
  unsigned int ptype_apmode:1;
  unsigned int ptype_sacmode:1;

#elif G_BYTE_ORDER == G_BIG_ENDIAN
  unsigned int psc1:16;

  unsigned int psc2:6;
  unsigned int tr1:2;

  unsigned int tr2:6;
  unsigned int ptype_start:2;

  unsigned int ptype_split:1;
  unsigned int ptype_camera:1;
  unsigned int ptype_freeze:1;
  unsigned int ptype_srcformat:3;
  unsigned int ptype_pictype:1;
  unsigned int ptype_umvmode:1;

  unsigned int ptype_sacmode:1;
  unsigned int ptype_apmode:1;
  unsigned int ptype_pbmode:1;
  unsigned int pquant:5;

#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
};

struct _GstRtpH263PayBoundry
{

  guint8 *start;
  guint8 *end;
  guint8 sbit;
  guint8 ebit;

};

struct _GstRtpH263PayMB
{
  guint8 *start;
  guint8 *end;
  guint8 sbit;
  guint8 ebit;
  guint length;

  guint8 mb_type;
  guint quant;

  guint mba;
  guint8 mvd[10];
};

struct _GstRtpH263PayGob
{
  guint8 *start;
  guint8 *end;
  guint length;
  guint8 sbit;
  guint8 ebit;

  guint gobn;
  guint quant;

  GstRtpH263PayMB **macroblocks;
  guint nmacroblocs;
};

struct _GstRtpH263PayPackage
{
  guint8 *payload_start;
  guint8 *payload_end;
  guint payload_len;
  guint8 sbit;
  guint8 ebit;
  GstBuffer *outbuf;
  gboolean marker;

  GstRtpH263PayHeaderMode mode;

  /*
   *  mode B,C data
   */

  guint16 mba;
  guint nmvd;
  guint8 mvd[10];
  guint gobn;
  guint quant;
};

#define GST_H263_PICTURELAYER_PLSRC(buf) (((GstRtpH263PayPic *)(buf))->ptype_srcformat)
#define GST_H263_PICTURELAYER_PLTYPE(buf) (((GstRtpH263PayPic *)(buf))->ptype_pictype)
#define GST_H263_PICTURELAYER_PLUMV(buf) (((GstRtpH263PayPic *)(buf))->ptype_umvmode)
#define GST_H263_PICTURELAYER_PLSAC(buf) (((GstRtpH263PayPic *)(buf))->ptype_sacmode)
#define GST_H263_PICTURELAYER_PLAP(buf) (((GstRtpH263PayPic *)(buf))->ptype_apmode)

/*
 * TODO: PB frame relevant tables
 */

#define GST_RTP_H263_PAY_END(start, len) (((guint8 *)start) + ((guint)len))
#define GST_RTP_H263_PAY_GOBN(gob) (((((guint8 *) gob)[2] >> 2) & 0x1f)

GType gst_rtp_h263_pay_get_type (void);

gboolean gst_rtp_h263_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_RTP_H263_PAY_H__ */
