#include <gst/check/check.h>
#include <gst/check/gstharness.h>
#include <gst/rtp/gstrtpbuffer.h>

#define H261_RTP_CAPS_STR                                               \
  "application/x-rtp,media=video,encoding-name=H261,clock-rate=90000,payload=31"

typedef struct _GstRtpH261PayHeader
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  unsigned int v:1;             /* Motion vector flag */
  unsigned int i:1;             /* Intra encoded data */
  unsigned int ebit:3;          /* End position */
  unsigned int sbit:3;          /* Start position */

  unsigned int mbap1:4;         /* MB address predictor - part1 */
  unsigned int gobn:4;          /* GOB number */

  unsigned int hmvd1:2;         /* Horizontal motion vector data - part1 */
  unsigned int quant:5;         /* Quantizer */
  unsigned int mbap2:1;         /* MB address predictor - part2 */

  unsigned int vmvd:5;          /* Horizontal motion vector data - part1 */
  unsigned int hmvd2:3;         /* Vertical motion vector data */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
  unsigned int sbit:3;          /* Start position */
  unsigned int ebit:3;          /* End position */
  unsigned int i:1;             /* Intra encoded data */
  unsigned int v:1;             /* Motion vector flag */

  unsigned int gobn:4;          /* GOB number */
  unsigned int mbap1:4;         /* MB address predictor - part1 */

  unsigned int mbap2:1;         /* MB address predictor - part2 */
  unsigned int quant:5;         /* Quantizer */
  unsigned int hmvd1:2;         /* Horizontal motion vector data - part1 */

  unsigned int hmvd2:3;         /* Vertical motion vector data */
  unsigned int vmvd:5;          /* Horizontal motion vector data - part1 */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
} GstRtpH261PayHeader;

#define GST_RTP_H261_PAYLOAD_HEADER_LEN 4

static guint8 *
create_h261_payload (gint sbit, gint ebit, gint psc, gsize size)
{
  GstRtpH261PayHeader header;
  const gint header_len = 4;
  guint8 *data = g_malloc0 (size);

  memset (&header, 0x00, sizeof (header));

  header.sbit = sbit;
  header.ebit = ebit;

  memset (data, 0xff, size);
  memcpy (data, &header, header_len);

  if (psc) {
    guint32 word = 0x000100ff >> sbit;
    data[header_len + 0] = (word >> 24) & 0xff;
    data[header_len + 1] = (word >> 16) & 0xff;
    data[header_len + 2] = (word >> 8) & 0xff;
    data[header_len + 3] = (word >> 0) & 0xff;
  }

  return data;
}

static GstBuffer *
create_rtp_copy_payload (const guint8 * data, gsize size, guint ts, guint16 seq,
    gboolean marker, guint csrcs)
{
  GstBuffer *buf = gst_rtp_buffer_new_allocate (size, 0, csrcs);
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_rtp_buffer_map (buf, GST_MAP_WRITE, &rtp);

  gst_rtp_buffer_set_seq (&rtp, seq);
  gst_rtp_buffer_set_marker (&rtp, marker);
  memcpy (gst_rtp_buffer_get_payload (&rtp), data, size);

  GST_BUFFER_PTS (buf) = (ts) * (GST_SECOND / 30);
  GST_BUFFER_DURATION (buf) = (GST_SECOND / 30);

  gst_rtp_buffer_unmap (&rtp);

  return buf;
}

GST_START_TEST (test_h263depay_empty_payload)
{
  GstHarness *h = gst_harness_new ("rtph261depay");
  gint sbit = 4;
  gint ebit = 4;
  gsize size;
  guint8 *payload;
  guint seq = 0;

  gst_harness_set_src_caps_str (h, H261_RTP_CAPS_STR);

  /* First send a proper packet with picture start code */
  size = 100;
  payload = create_h261_payload (sbit, ebit, TRUE, size);
  gst_harness_push (h, create_rtp_copy_payload (payload, size, 0, seq++, FALSE,
          0));
  g_free (payload);

  /* Not a complete frame */
  fail_unless_equals_int (gst_harness_buffers_received (h), 0);

  /* Second buffer has invalid empty payload */
  size = GST_RTP_H261_PAYLOAD_HEADER_LEN;
  payload = create_h261_payload (sbit, ebit, FALSE, size);
  gst_harness_push (h, create_rtp_copy_payload (payload, size, 0, seq++, TRUE,
          0));
  g_free (payload);

  /* Invalid payload should be dropped */
  fail_unless_equals_int (gst_harness_buffers_received (h), 0);

  gst_harness_teardown (h);
}

GST_END_TEST;

static Suite *
rtph261_suite (void)
{
  Suite *s = suite_create ("rtph261");
  TCase *tc_chain;

  suite_add_tcase (s, (tc_chain = tcase_create ("h261depay")));
  tcase_add_test (tc_chain, test_h263depay_empty_payload);

  return s;
}

GST_CHECK_MAIN (rtph261);
