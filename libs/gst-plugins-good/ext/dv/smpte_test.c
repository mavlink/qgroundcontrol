
#include "config.h"

#include "gstsmptetimecode.h"

#include <glib.h>

#define NTSC_FRAMES_PER_10_MINS (10*60*30 - 10*2 + 2)
#define NTSC_FRAMES_PER_HOUR (6*NTSC_FRAMES_PER_10_MINS)


int
main (int argc, char *argv[])
{
  GstSMPTETimeCode tc;
  int i;
  int min;

  for (min = 0; min < 3; min++) {
    g_print ("--- minute %d ---\n", min);
    for (i = min * 60 * 30 - 5; i <= min * 60 * 30 + 5; i++) {
      gst_smpte_time_code_from_frame_number (GST_SMPTE_TIME_CODE_SYSTEM_30, &tc,
          i);
      g_print ("%d %02d:%02d:%02d:%02d\n", i, tc.hours, tc.minutes, tc.seconds,
          tc.frames);
    }
  }

  for (min = 9; min < 12; min++) {
    g_print ("--- minute %d ---\n", min);
    for (i = min * 60 * 30 - 5 - 18; i <= min * 60 * 30 + 5 - 18; i++) {
      gst_smpte_time_code_from_frame_number (GST_SMPTE_TIME_CODE_SYSTEM_30, &tc,
          i);
      g_print ("%d %02d:%02d:%02d:%02d\n", i, tc.hours, tc.minutes, tc.seconds,
          tc.frames);
    }
  }

  for (min = -1; min < 2; min++) {
    int offset = NTSC_FRAMES_PER_HOUR;

    g_print ("--- minute %d ---\n", min);
    for (i = offset + min * 60 * 30 - 5; i <= offset + min * 60 * 30 + 5; i++) {
      gst_smpte_time_code_from_frame_number (GST_SMPTE_TIME_CODE_SYSTEM_30, &tc,
          i);
      g_print ("%d %02d:%02d:%02d:%02d\n", i, tc.hours, tc.minutes, tc.seconds,
          tc.frames);
    }
  }

  for (min = 0; min < 1; min++) {
    int offset = NTSC_FRAMES_PER_HOUR;

    g_print ("--- minute %d ---\n", min);
    for (i = 24 * offset - 5; i <= 24 * offset + 5; i++) {
      gst_smpte_time_code_from_frame_number (GST_SMPTE_TIME_CODE_SYSTEM_30, &tc,
          i);
      g_print ("%d %02d:%02d:%02d:%02d\n", i, tc.hours, tc.minutes, tc.seconds,
          tc.frames);
    }
  }

  for (i = 0; i < 24 * NTSC_FRAMES_PER_HOUR; i++) {
    int fn;
    int ret;

    gst_smpte_time_code_from_frame_number (GST_SMPTE_TIME_CODE_SYSTEM_30, &tc,
        i);

    ret = gst_smpte_time_code_get_frame_number (GST_SMPTE_TIME_CODE_SYSTEM_30,
        &fn, &tc);
    if (!ret) {
      g_print ("bad valid at %d\n", i);
    }
    if (fn != i) {
      g_print ("index mismatch %d != %d\n", fn, i);
    }
  }

  return 0;
}
