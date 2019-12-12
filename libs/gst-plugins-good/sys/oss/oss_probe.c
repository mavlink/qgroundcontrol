
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <glib.h>

#ifdef HAVE_OSS_INCLUDE_IN_SYS
# include <sys/soundcard.h>
#else
# ifdef HAVE_OSS_INCLUDE_IN_ROOT
#  include <soundcard.h>
# else
#  ifdef HAVE_OSS_INCLUDE_IN_MACHINE
#   include <machine/soundcard.h>
#  else
#   error "What to include?"
#  endif /* HAVE_OSS_INCLUDE_IN_MACHINE */
# endif /* HAVE_OSS_INCLUDE_IN_ROOT */
#endif /* HAVE_OSS_INCLUDE_IN_SYS */

typedef struct _Probe Probe;
struct _Probe
{
  int fd;
  int format;
  int n_channels;
  GArray *rates;
  int min;
  int max;
};

typedef struct _Range Range;
struct _Range
{
  int min;
  int max;
};

static gboolean probe_check (Probe * probe);
static int check_rate (Probe * probe, int irate);
static void add_range (GQueue * queue, int min, int max);
static void add_rate (GArray * array, int rate);
static int int_compare (gconstpointer a, gconstpointer b);

int
main (int argc, char *argv[])
{
  int fd;
  int i;
  Probe *probe;

  fd = open ("/dev/dsp", O_RDWR);
  if (fd < 0) {
    perror ("/dev/dsp");
    exit (1);
  }

  probe = g_new0 (Probe, 1);
  probe->fd = fd;
  probe->format = AFMT_S16_LE;
  probe->n_channels = 2;

  probe_check (probe);
  g_array_sort (probe->rates, int_compare);
  for (i = 0; i < probe->rates->len; i++) {
    g_print ("%d\n", g_array_index (probe->rates, int, i));
  }

  g_array_free (probe->rates, TRUE);
  g_free (probe);

#if 0
  probe = g_new0 (Probe, 1);
  probe->fd = fd;
  probe->format = AFMT_S16_LE;
  probe->n_channels = 1;

  probe_check (probe);
  for (i = 0; i < probe->rates->len; i++) {
    g_print ("%d\n", g_array_index (probe->rates, int, i));
  }

  probe = g_new0 (Probe, 1);
  probe->fd = fd;
  probe->format = AFMT_U8;
  probe->n_channels = 2;

  probe_check (probe);
  for (i = 0; i < probe->rates->len; i++) {
    g_print ("%d\n", g_array_index (probe->rates, int, i));
  }

  probe = g_new0 (Probe, 1);
  probe->fd = fd;
  probe->format = AFMT_U8;
  probe->n_channels = 1;

  probe_check (probe);
  for (i = 0; i < probe->rates->len; i++) {
    g_print ("%d\n", g_array_index (probe->rates, int, i));
  }
#endif

  return 0;
}

static gboolean
probe_check (Probe * probe)
{
  Range *range;
  GQueue *ranges;
  int exact_rates = 0;
  gboolean checking_exact_rates = TRUE;
  int n_checks = 0;
  gboolean result = TRUE;

  ranges = g_queue_new ();

  probe->rates = g_array_new (FALSE, FALSE, sizeof (int));

  probe->min = check_rate (probe, 1000);
  n_checks++;
  probe->max = check_rate (probe, 100000);
  n_checks++;
  add_range (ranges, probe->min + 1, probe->max - 1);

  while ((range = g_queue_pop_head (ranges))) {
    int min1;
    int max1;
    int mid;
    int mid_ret;

    g_print ("checking [%d,%d]\n", range->min, range->max);

    mid = (range->min + range->max) / 2;
    mid_ret = check_rate (probe, mid);
    n_checks++;

    if (mid == mid_ret && checking_exact_rates) {
      int max_exact_matches = 100;

      exact_rates++;
      if (exact_rates > max_exact_matches) {
        g_print ("got %d exact rates, assuming all are exact\n",
            max_exact_matches);
        result = FALSE;
        g_free (range);
        break;
      }
    } else {
      checking_exact_rates = FALSE;
    }

    /* Assume that the rate is arithmetically rounded to the nearest
     * supported rate. */
    if (mid == mid_ret) {
      min1 = mid - 1;
      max1 = mid + 1;
    } else {
      if (mid < mid_ret) {
        min1 = mid - (mid_ret - mid);
        max1 = mid_ret + 1;
      } else {
        min1 = mid_ret - 1;
        max1 = mid + (mid - mid_ret);
      }
    }

    add_range (ranges, range->min, min1);
    add_range (ranges, max1, range->max);

    g_free (range);
  }

  while ((range = g_queue_pop_head (ranges))) {
    g_free (range);
  }
  g_queue_free (ranges);

  return result;
}

static void
add_range (GQueue * queue, int min, int max)
{
  g_print ("trying to add [%d,%d]\n", min, max);
  if (min <= max) {
    Range *range = g_new0 (Range, 1);

    range->min = min;
    range->max = max;

    g_queue_push_tail (queue, range);
    //g_queue_push_head (queue, range);
  }
}

static int
check_rate (Probe * probe, int irate)
{
  int rate;
  int format;
  int n_channels;

  rate = irate;
  format = probe->format;
  n_channels = probe->n_channels;

  ioctl (probe->fd, SNDCTL_DSP_SETFMT, &format);
  ioctl (probe->fd, SNDCTL_DSP_CHANNELS, &n_channels);
  ioctl (probe->fd, SNDCTL_DSP_SPEED, &rate);

  g_print ("rate %d -> %d\n", irate, rate);

  if (rate == irate - 1 || rate == irate + 1) {
    rate = irate;
  }
  add_rate (probe->rates, rate);
  return rate;
}

static void
add_rate (GArray * array, int rate)
{
  int i;
  int val;

  for (i = 0; i < array->len; i++) {
    val = g_array_index (array, int, i);

    if (val == rate)
      return;
  }
  g_print ("supported rate: %d\n", rate);
  g_array_append_val (array, rate);
}

static int
int_compare (gconstpointer a, gconstpointer b)
{
  const int *va = (const int *) a;
  const int *vb = (const int *) b;

  if (*va < *vb)
    return -1;
  if (*va > *vb)
    return 1;
  return 0;
}
