/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/tag/tag.h>

#include "gstrtputils.h"

#include "gstrtpac3depay.h"
#include "gstrtpac3pay.h"
#include "gstrtpbvdepay.h"
#include "gstrtpbvpay.h"
#include "gstrtpceltdepay.h"
#include "gstrtpceltpay.h"
#include "gstrtpdvdepay.h"
#include "gstrtpdvpay.h"
#include "gstrtpgstdepay.h"
#include "gstrtpgstpay.h"
#include "gstrtpilbcdepay.h"
#include "gstrtpilbcpay.h"
#include "gstrtppcmupay.h"
#include "gstrtppcmapay.h"
#include "gstrtppcmadepay.h"
#include "gstrtppcmudepay.h"
#include "gstrtpg722depay.h"
#include "gstrtpg722pay.h"
#include "gstrtpg723depay.h"
#include "gstrtpg723pay.h"
#include "gstrtpg726depay.h"
#include "gstrtpg726pay.h"
#include "gstrtpg729depay.h"
#include "gstrtpg729pay.h"
#include "gstrtpgsmpay.h"
#include "gstrtpgsmdepay.h"
#include "gstrtpamrpay.h"
#include "gstrtpamrdepay.h"
#include "gstrtpmpapay.h"
#include "gstrtpmpadepay.h"
#include "gstrtpmparobustdepay.h"
#include "gstrtpmpvdepay.h"
#include "gstrtpmpvpay.h"
#include "gstrtpopusdepay.h"
#include "gstrtpopuspay.h"
#include "gstrtph261pay.h"
#include "gstrtph261depay.h"
#include "gstrtph263pdepay.h"
#include "gstrtph263ppay.h"
#include "gstrtph263depay.h"
#include "gstrtph263pay.h"
#include "gstrtph264depay.h"
#include "gstrtph264pay.h"
#include "gstrtph265depay.h"
#include "gstrtph265pay.h"
#include "gstrtpj2kdepay.h"
#include "gstrtpj2kpay.h"
#include "gstrtpjpegdepay.h"
#include "gstrtpjpegpay.h"
#include "gstrtpklvdepay.h"
#include "gstrtpklvpay.h"
#include "gstrtpL8depay.h"
#include "gstrtpL8pay.h"
#include "gstrtpL16depay.h"
#include "gstrtpL16pay.h"
#include "gstrtpL24depay.h"
#include "gstrtpL24pay.h"
#include "gstasteriskh263.h"
#include "gstrtpmp1sdepay.h"
#include "gstrtpmp2tdepay.h"
#include "gstrtpmp2tpay.h"
#include "gstrtpmp4vdepay.h"
#include "gstrtpmp4vpay.h"
#include "gstrtpmp4adepay.h"
#include "gstrtpmp4apay.h"
#include "gstrtpmp4gdepay.h"
#include "gstrtpmp4gpay.h"
#include "gstrtpqcelpdepay.h"
#include "gstrtpqdmdepay.h"
#include "gstrtpsbcdepay.h"
#include "gstrtpsbcpay.h"
#include "gstrtpsirenpay.h"
#include "gstrtpsirendepay.h"
#include "gstrtpspeexpay.h"
#include "gstrtpspeexdepay.h"
#include "gstrtpsv3vdepay.h"
#include "gstrtptheoradepay.h"
#include "gstrtptheorapay.h"
#include "gstrtpvorbisdepay.h"
#include "gstrtpvorbispay.h"
#include "gstrtpvp8depay.h"
#include "gstrtpvp8pay.h"
#include "gstrtpvp9depay.h"
#include "gstrtpvp9pay.h"
#include "gstrtpvrawdepay.h"
#include "gstrtpvrawpay.h"
#include "gstrtpstreampay.h"
#include "gstrtpstreamdepay.h"
#include "gstrtpredenc.h"
#include "gstrtpreddec.h"
#include "gstrtpulpfecdec.h"
#include "gstrtpulpfecenc.h"
#include "gstrtpreddec.h"
#include "gstrtpredenc.h"
#include "gstrtpstorage.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gst_tag_image_type_get_type ();

  rtp_quark_meta_tag_video =
      g_quark_from_static_string (GST_META_TAG_VIDEO_STR);
  rtp_quark_meta_tag_audio =
      g_quark_from_static_string (GST_META_TAG_AUDIO_STR);

  if (!gst_rtp_ac3_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_ac3_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_bv_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_bv_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_celt_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_celt_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_dv_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_dv_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_gst_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_gst_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_ilbc_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_ilbc_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_g722_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_g722_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_g723_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_g723_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_g726_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_g726_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_g729_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_g729_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_gsm_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_gsm_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_amr_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_amr_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_pcma_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_pcmu_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_pcmu_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_pcma_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mpa_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mpa_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mpa_robust_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mpv_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mpv_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_opus_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_opus_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h261_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h261_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h263p_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h263p_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h263_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h263_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h264_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h264_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h265_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_h265_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_j2k_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_j2k_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_jpeg_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_jpeg_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_klv_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_klv_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_L8_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_L8_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_L16_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_L16_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_L24_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_L24_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_asteriskh263_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp1s_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp2t_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp2t_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp4v_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp4v_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp4a_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp4a_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp4g_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_mp4g_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_qcelp_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_qdm2_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_sbc_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_sbc_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_siren_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_siren_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_speex_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_speex_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_sv3v_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_theora_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_theora_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_vorbis_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_vorbis_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_vp8_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_vp8_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_vp9_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_vp9_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_vraw_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_vraw_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_stream_pay_plugin_init (plugin))
    return FALSE;

  if (!gst_rtp_stream_depay_plugin_init (plugin))
    return FALSE;

  if (!gst_element_register (plugin, "rtpredenc", GST_RANK_NONE,
          GST_TYPE_RTP_RED_ENC))
    return FALSE;

  if (!gst_element_register (plugin, "rtpreddec", GST_RANK_NONE,
          GST_TYPE_RTP_RED_DEC))
    return FALSE;

  if (!gst_element_register (plugin, "rtpulpfecdec", GST_RANK_NONE,
          GST_TYPE_RTP_ULPFEC_DEC))
    return FALSE;

  if (!gst_element_register (plugin, "rtpulpfecenc", GST_RANK_NONE,
          GST_TYPE_RTP_ULPFEC_ENC))
    return FALSE;

  if (!gst_element_register (plugin, "rtpreddec", GST_RANK_NONE,
          GST_TYPE_RTP_RED_DEC))
    return FALSE;

  if (!gst_element_register (plugin, "rtpredenc", GST_RANK_NONE,
          GST_TYPE_RTP_RED_ENC))
    return FALSE;

  if (!gst_element_register (plugin, "rtpstorage", GST_RANK_NONE,
          GST_TYPE_RTP_STORAGE))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rtp,
    "Real-time protocol plugins",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
