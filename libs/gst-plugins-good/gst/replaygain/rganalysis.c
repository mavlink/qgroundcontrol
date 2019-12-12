/* GStreamer ReplayGain analysis
 *
 * Copyright (C) 2006 Rene Stadler <mail@renestadler.de>
 * Copyright (C) 2001 David Robinson <David@Robinson.org>
 *                    Glen Sawyer <glensawyer@hotmail.com>
 *
 * rganalysis.c: Analyze raw audio data in accordance with ReplayGain
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

/* Based on code with Copyright (C) 2001 David Robinson
 * <David@Robinson.org> and Glen Sawyer <glensawyer@hotmail.com>,
 * which is distributed under the LGPL as part of the vorbisgain
 * program.  The original code also mentions Frank Klemm
 * (http://www.uni-jena.de/~pfk/mpp/) for having contributed lots of
 * good code.  Specifically, this is based on the file
 * "gain_analysis.c" from vorbisgain version 0.34.
 */

/* Room for future improvement: Mono data is currently in fact copied
 * to two channels which get processed normally.  This means that mono
 * input data is processed twice.
 */

/* Helpful information for understanding this code: The two IIR
 * filters depend on previous input _and_ previous output samples (up
 * to the filter's order number of samples).  This explains the whole
 * lot of memcpy'ing done in rg_analysis_analyze and why the context
 * holds so many buffers.
 */

#include <math.h>
#include <string.h>
#include <glib.h>

#include "rganalysis.h"

#define YULE_ORDER         10
#define BUTTER_ORDER        2
/* Percentile which is louder than the proposed level: */
#define RMS_PERCENTILE     95
/* Duration of RMS window in milliseconds: */
#define RMS_WINDOW_MSECS   50
/* Histogram array elements per dB: */
#define STEPS_PER_DB      100
/* Histogram upper bound in dB (normal max. values in the wild are
 * assumed to be around 70, 80 dB): */
#define MAX_DB            120
/* Calibration value: */
#define PINK_REF           64.82        /* 298640883795 */

#define MAX_ORDER         MAX (BUTTER_ORDER, YULE_ORDER)
#define MAX_SAMPLE_RATE   48000
/* The + 999 has the effect of ceil()ing: */
#define MAX_SAMPLE_WINDOW (guint) \
  ((MAX_SAMPLE_RATE * RMS_WINDOW_MSECS + 999) / 1000)

/* Analysis result accumulator. */

struct _RgAnalysisAcc
{
  guint32 histogram[STEPS_PER_DB * MAX_DB];
  gdouble peak;
};

typedef struct _RgAnalysisAcc RgAnalysisAcc;

/* Analysis context. */

struct _RgAnalysisCtx
{
  /* Filter buffers for left channel. */
  gfloat inprebuf_l[MAX_ORDER * 2];
  gfloat *inpre_l;
  gfloat stepbuf_l[MAX_SAMPLE_WINDOW + MAX_ORDER];
  gfloat *step_l;
  gfloat outbuf_l[MAX_SAMPLE_WINDOW + MAX_ORDER];
  gfloat *out_l;
  /* Filter buffers for right channel. */
  gfloat inprebuf_r[MAX_ORDER * 2];
  gfloat *inpre_r;
  gfloat stepbuf_r[MAX_SAMPLE_WINDOW + MAX_ORDER];
  gfloat *step_r;
  gfloat outbuf_r[MAX_SAMPLE_WINDOW + MAX_ORDER];
  gfloat *out_r;

  /* Number of samples to reach duration of the RMS window: */
  guint window_n_samples;
  /* Progress of the running window: */
  guint window_n_samples_done;
  gdouble window_square_sum;

  gint sample_rate;
  gint sample_rate_index;

  RgAnalysisAcc track;
  RgAnalysisAcc album;
  void (*post_message) (gpointer analysis,
      GstClockTime timestamp, GstClockTime duration, gdouble rglevel);
  gpointer analysis;
  /* The timestamp of the current incoming buffer. */
  GstClockTime buffer_timestamp;
  /* Number of samples processed in current buffer, during emit_signal,
     this will always be on an RMS window boundary. */
  guint buffer_n_samples_done;
};

/* Filter coefficients for the IIR filters that form the equal
 * loudness filter.  XFilter[ctx->sample_rate_index] gives the array
 * of the X coefficients (A or B) for the configured sample rate. */

#ifdef _MSC_VER
/* Disable double-to-float warning: */
/* A better solution would be to append 'f' to each constant, but that
 * makes the code ugly. */
#pragma warning ( disable : 4305 )
#endif

static const gfloat AYule[9][11] = {
  {1., -3.84664617118067, 7.81501653005538, -11.34170355132042,
        13.05504219327545, -12.28759895145294, 9.48293806319790,
        -5.87257861775999, 2.75465861874613, -0.86984376593551,
      0.13919314567432},
  {1., -3.47845948550071, 6.36317777566148, -8.54751527471874, 9.47693607801280,
        -8.81498681370155, 6.85401540936998, -4.39470996079559,
      2.19611684890774, -0.75104302451432, 0.13149317958808},
  {1., -2.37898834973084, 2.84868151156327, -2.64577170229825, 2.23697657451713,
        -1.67148153367602, 1.00595954808547, -0.45953458054983,
      0.16378164858596, -0.05032077717131, 0.02347897407020},
  {1., -1.61273165137247, 1.07977492259970, -0.25656257754070,
        -0.16276719120440, -0.22638893773906, 0.39120800788284,
        -0.22138138954925, 0.04500235387352, 0.02005851806501,
      0.00302439095741},
  {1., -1.49858979367799, 0.87350271418188, 0.12205022308084, -0.80774944671438,
        0.47854794562326, -0.12453458140019, -0.04067510197014,
      0.08333755284107, -0.04237348025746, 0.02977207319925},
  {1., -0.62820619233671, 0.29661783706366, -0.37256372942400, 0.00213767857124,
        -0.42029820170918, 0.22199650564824, 0.00613424350682, 0.06747620744683,
      0.05784820375801, 0.03222754072173},
  {1., -1.04800335126349, 0.29156311971249, -0.26806001042947, 0.00819999645858,
        0.45054734505008, -0.33032403314006, 0.06739368333110,
      -0.04784254229033, 0.01639907836189, 0.01807364323573},
  {1., -0.51035327095184, -0.31863563325245, -0.20256413484477,
        0.14728154134330, 0.38952639978999, -0.23313271880868,
        -0.05246019024463, -0.02505961724053, 0.02442357316099,
      0.01818801111503},
  {1., -0.25049871956020, -0.43193942311114, -0.03424681017675,
        -0.04678328784242, 0.26408300200955, 0.15113130533216,
        -0.17556493366449, -0.18823009262115, 0.05477720428674,
      0.04704409688120}
};

static const gfloat BYule[9][11] = {
  {0.03857599435200, -0.02160367184185, -0.00123395316851, -0.00009291677959,
        -0.01655260341619, 0.02161526843274, -0.02074045215285,
      0.00594298065125, 0.00306428023191, 0.00012025322027, 0.00288463683916},
  {0.05418656406430, -0.02911007808948, -0.00848709379851, -0.00851165645469,
        -0.00834990904936, 0.02245293253339, -0.02596338512915,
        0.01624864962975, -0.00240879051584, 0.00674613682247,
      -0.00187763777362},
  {0.15457299681924, -0.09331049056315, -0.06247880153653, 0.02163541888798,
        -0.05588393329856, 0.04781476674921, 0.00222312597743, 0.03174092540049,
      -0.01390589421898, 0.00651420667831, -0.00881362733839},
  {0.30296907319327, -0.22613988682123, -0.08587323730772, 0.03282930172664,
        -0.00915702933434, -0.02364141202522, -0.00584456039913,
        0.06276101321749, -0.00000828086748, 0.00205861885564,
      -0.02950134983287},
  {0.33642304856132, -0.25572241425570, -0.11828570177555, 0.11921148675203,
        -0.07834489609479, -0.00469977914380, -0.00589500224440,
        0.05724228140351, 0.00832043980773, -0.01635381384540,
      -0.01760176568150},
  {0.44915256608450, -0.14351757464547, -0.22784394429749, -0.01419140100551,
        0.04078262797139, -0.12398163381748, 0.04097565135648, 0.10478503600251,
      -0.01863887810927, -0.03193428438915, 0.00541907748707},
  {0.56619470757641, -0.75464456939302, 0.16242137742230, 0.16744243493672,
        -0.18901604199609, 0.30931782841830, -0.27562961986224,
        0.00647310677246, 0.08647503780351, -0.03788984554840,
      -0.00588215443421},
  {0.58100494960553, -0.53174909058578, -0.14289799034253, 0.17520704835522,
        0.02377945217615, 0.15558449135573, -0.25344790059353, 0.01628462406333,
      0.06920467763959, -0.03721611395801, -0.00749618797172},
  {0.53648789255105, -0.42163034350696, -0.00275953611929, 0.04267842219415,
        -0.10214864179676, 0.14590772289388, -0.02459864859345,
        -0.11202315195388, -0.04060034127000, 0.04788665548180,
      -0.02217936801134}
};

static const gfloat AButter[9][3] = {
  {1., -1.97223372919527, 0.97261396931306},
  {1., -1.96977855582618, 0.97022847566350},
  {1., -1.95835380975398, 0.95920349965459},
  {1., -1.95002759149878, 0.95124613669835},
  {1., -1.94561023566527, 0.94705070426118},
  {1., -1.92783286977036, 0.93034775234268},
  {1., -1.91858953033784, 0.92177618768381},
  {1., -1.91542108074780, 0.91885558323625},
  {1., -1.88903307939452, 0.89487434461664}
};

static const gfloat BButter[9][3] = {
  {0.98621192462708, -1.97242384925416, 0.98621192462708},
  {0.98500175787242, -1.97000351574484, 0.98500175787242},
  {0.97938932735214, -1.95877865470428, 0.97938932735214},
  {0.97531843204928, -1.95063686409857, 0.97531843204928},
  {0.97316523498161, -1.94633046996323, 0.97316523498161},
  {0.96454515552826, -1.92909031105652, 0.96454515552826},
  {0.96009142950541, -1.92018285901082, 0.96009142950541},
  {0.95856916599601, -1.91713833199203, 0.95856916599601},
  {0.94597685600279, -1.89195371200558, 0.94597685600279}
};

#ifdef _MSC_VER
#pragma warning ( default : 4305 )
#endif

/* Filter functions.  These access elements with negative indices of
 * the input and output arrays (up to the filter's order). */

/* For much better performance, the function below has been
 * implemented by unrolling the inner loop for our two use cases. */

/*
 * static inline void
 * apply_filter (const gfloat * input, gfloat * output, guint n_samples,
 *     const gfloat * a, const gfloat * b, guint order)
 * {
 *   gfloat y;
 *   gint i, k;
 * 
 *   for (i = 0; i < n_samples; i++) {
 *     y = input[i] * b[0];
 *     for (k = 1; k <= order; k++)
 *       y += input[i - k] * b[k] - output[i - k] * a[k];
 *     output[i] = y;
 *   }
 * }
 */

static inline void
yule_filter (const gfloat * input, gfloat * output,
    const gfloat * a, const gfloat * b)
{
  /* 1e-10 is added below to avoid running into denormals when operating on
   * near silence. */

  output[0] = 1e-10 + input[0] * b[0]
      + input[-1] * b[1] - output[-1] * a[1]
      + input[-2] * b[2] - output[-2] * a[2]
      + input[-3] * b[3] - output[-3] * a[3]
      + input[-4] * b[4] - output[-4] * a[4]
      + input[-5] * b[5] - output[-5] * a[5]
      + input[-6] * b[6] - output[-6] * a[6]
      + input[-7] * b[7] - output[-7] * a[7]
      + input[-8] * b[8] - output[-8] * a[8]
      + input[-9] * b[9] - output[-9] * a[9]
      + input[-10] * b[10] - output[-10] * a[10];
}

static inline void
butter_filter (const gfloat * input, gfloat * output,
    const gfloat * a, const gfloat * b)
{
  output[0] = input[0] * b[0]
      + input[-1] * b[1] - output[-1] * a[1]
      + input[-2] * b[2] - output[-2] * a[2];
}

/* Because butter_filter and yule_filter are inlined, this function is
 * a bit blown-up (code-size wise), but not inlining gives a ca. 40%
 * performance penalty. */

static inline void
apply_filters (const RgAnalysisCtx * ctx, const gfloat * input_l,
    const gfloat * input_r, guint n_samples)
{
  const gfloat *ayule = AYule[ctx->sample_rate_index];
  const gfloat *byule = BYule[ctx->sample_rate_index];
  const gfloat *abutter = AButter[ctx->sample_rate_index];
  const gfloat *bbutter = BButter[ctx->sample_rate_index];
  gint pos = ctx->window_n_samples_done;
  gint i;

  for (i = 0; i < n_samples; i++, pos++) {
    yule_filter (input_l + i, ctx->step_l + pos, ayule, byule);
    butter_filter (ctx->step_l + pos, ctx->out_l + pos, abutter, bbutter);

    yule_filter (input_r + i, ctx->step_r + pos, ayule, byule);
    butter_filter (ctx->step_r + pos, ctx->out_r + pos, abutter, bbutter);
  }
}

/* Clear filter buffer state and current RMS window. */

static void
reset_filters (RgAnalysisCtx * ctx)
{
  gint i;

  for (i = 0; i < MAX_ORDER; i++) {

    ctx->inprebuf_l[i] = 0.;
    ctx->stepbuf_l[i] = 0.;
    ctx->outbuf_l[i] = 0.;

    ctx->inprebuf_r[i] = 0.;
    ctx->stepbuf_r[i] = 0.;
    ctx->outbuf_r[i] = 0.;
  }

  ctx->window_square_sum = 0.;
  ctx->window_n_samples_done = 0;
}

/* Accumulator functions. */

/* Add two accumulators in-place.  The sum is defined as the result of
 * the vector sum of the histogram array and the maximum value of the
 * peak field.  Thus "adding" the accumulators for all tracks yields
 * the correct result for obtaining the album gain and peak. */

static void
accumulator_add (RgAnalysisAcc * acc, const RgAnalysisAcc * acc_other)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (acc->histogram); i++)
    acc->histogram[i] += acc_other->histogram[i];

  acc->peak = MAX (acc->peak, acc_other->peak);
}

/* Reset an accumulator to zero. */

static void
accumulator_clear (RgAnalysisAcc * acc)
{
  memset (acc->histogram, 0, sizeof (acc->histogram));
  acc->peak = 0.;
}

/* Obtain final analysis result from an accumulator.  Returns TRUE on
 * success, FALSE on error (if accumulator is still zero). */

static gboolean
accumulator_result (const RgAnalysisAcc * acc, gdouble * result_gain,
    gdouble * result_peak)
{
  guint32 sum = 0;
  guint32 upper;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (acc->histogram); i++)
    sum += acc->histogram[i];

  if (sum == 0)
    /* All entries are 0: We got less than 50ms of data. */
    return FALSE;

  upper = (guint32) ceil (sum * (1. - (gdouble) (RMS_PERCENTILE / 100.)));

  for (i = G_N_ELEMENTS (acc->histogram); i--;) {
    if (upper <= acc->histogram[i])
      break;
    upper -= acc->histogram[i];
  }

  if (result_peak != NULL)
    *result_peak = acc->peak;
  if (result_gain != NULL)
    *result_gain = PINK_REF - (gdouble) i / STEPS_PER_DB;

  return TRUE;
}

/* Functions that operate on contexts, for external usage. */

/* Create a new context.  Before it can be used, a sample rate must be
 * configured using rg_analysis_set_sample_rate. */

RgAnalysisCtx *
rg_analysis_new (void)
{
  RgAnalysisCtx *ctx;

  ctx = g_new (RgAnalysisCtx, 1);

  ctx->inpre_l = ctx->inprebuf_l + MAX_ORDER;
  ctx->step_l = ctx->stepbuf_l + MAX_ORDER;
  ctx->out_l = ctx->outbuf_l + MAX_ORDER;

  ctx->inpre_r = ctx->inprebuf_r + MAX_ORDER;
  ctx->step_r = ctx->stepbuf_r + MAX_ORDER;
  ctx->out_r = ctx->outbuf_r + MAX_ORDER;

  ctx->sample_rate = 0;

  accumulator_clear (&ctx->track);
  accumulator_clear (&ctx->album);

  return ctx;
}

static void
reset_silence_detection (RgAnalysisCtx * ctx)
{
  ctx->buffer_timestamp = GST_CLOCK_TIME_NONE;
  ctx->buffer_n_samples_done = 0;
}

/* Adapt to given sample rate.  Does nothing if already the current
 * rate (returns TRUE then).  Returns FALSE only if given sample rate
 * is not supported.  If the configured rate changes, the last
 * unprocessed incomplete 50ms chunk of data is dropped because the
 * filters are reset. */

gboolean
rg_analysis_set_sample_rate (RgAnalysisCtx * ctx, gint sample_rate)
{
  g_return_val_if_fail (ctx != NULL, FALSE);

  if (ctx->sample_rate == sample_rate)
    return TRUE;

  switch (sample_rate) {
    case 48000:
      ctx->sample_rate_index = 0;
      break;
    case 44100:
      ctx->sample_rate_index = 1;
      break;
    case 32000:
      ctx->sample_rate_index = 2;
      break;
    case 24000:
      ctx->sample_rate_index = 3;
      break;
    case 22050:
      ctx->sample_rate_index = 4;
      break;
    case 16000:
      ctx->sample_rate_index = 5;
      break;
    case 12000:
      ctx->sample_rate_index = 6;
      break;
    case 11025:
      ctx->sample_rate_index = 7;
      break;
    case 8000:
      ctx->sample_rate_index = 8;
      break;
    default:
      return FALSE;
  }

  ctx->sample_rate = sample_rate;
  /* The + 999 has the effect of ceil()ing: */
  ctx->window_n_samples = (guint) ((sample_rate * RMS_WINDOW_MSECS + 999)
      / 1000);

  reset_filters (ctx);
  reset_silence_detection (ctx);

  return TRUE;
}

void
rg_analysis_init_silence_detection (RgAnalysisCtx * ctx,
    void (*post_message) (gpointer analysis, GstClockTime timestamp,
        GstClockTime duration, gdouble rglevel), gpointer analysis)
{
  ctx->post_message = post_message;
  ctx->analysis = analysis;
  reset_silence_detection (ctx);
}

void
rg_analysis_start_buffer (RgAnalysisCtx * ctx, GstClockTime buffer_timestamp)
{
  ctx->buffer_timestamp = buffer_timestamp;
  ctx->buffer_n_samples_done = 0;
}

void
rg_analysis_destroy (RgAnalysisCtx * ctx)
{
  g_free (ctx);
}

/* Entry points for analyzing sample data in common raw data formats.
 * The stereo format functions expect interleaved frames.  It is
 * possible to pass data in different formats for the same context,
 * there are no restrictions.  All functions have the same signature;
 * the depth argument for the float functions is not variable and must
 * be given the value 32. */

void
rg_analysis_analyze_mono_float (RgAnalysisCtx * ctx, gconstpointer data,
    gsize size, guint depth)
{
  gfloat conv_samples[512];
  const gfloat *samples = (gfloat *) data;
  guint n_samples = size / sizeof (gfloat);
  gint i;

  g_return_if_fail (depth == 32);
  g_return_if_fail (size % sizeof (gfloat) == 0);

  while (n_samples) {
    gint n = MIN (n_samples, G_N_ELEMENTS (conv_samples));

    n_samples -= n;
    memcpy (conv_samples, samples, n * sizeof (gfloat));
    for (i = 0; i < n; i++) {
      ctx->track.peak = MAX (ctx->track.peak, fabs (conv_samples[i]));
      conv_samples[i] *= 32768.;
    }
    samples += n;
    rg_analysis_analyze (ctx, conv_samples, NULL, n);
  }
}

void
rg_analysis_analyze_stereo_float (RgAnalysisCtx * ctx, gconstpointer data,
    gsize size, guint depth)
{
  gfloat conv_samples_l[256];
  gfloat conv_samples_r[256];
  const gfloat *samples = (gfloat *) data;
  guint n_frames = size / (sizeof (gfloat) * 2);
  gint i;

  g_return_if_fail (depth == 32);
  g_return_if_fail (size % (sizeof (gfloat) * 2) == 0);

  while (n_frames) {
    gint n = MIN (n_frames, G_N_ELEMENTS (conv_samples_l));

    n_frames -= n;
    for (i = 0; i < n; i++) {
      gfloat old_sample;

      old_sample = samples[2 * i];
      ctx->track.peak = MAX (ctx->track.peak, fabs (old_sample));
      conv_samples_l[i] = old_sample * 32768.;

      old_sample = samples[2 * i + 1];
      ctx->track.peak = MAX (ctx->track.peak, fabs (old_sample));
      conv_samples_r[i] = old_sample * 32768.;
    }
    samples += 2 * n;
    rg_analysis_analyze (ctx, conv_samples_l, conv_samples_r, n);
  }
}

void
rg_analysis_analyze_mono_int16 (RgAnalysisCtx * ctx, gconstpointer data,
    gsize size, guint depth)
{
  gfloat conv_samples[512];
  gint32 peak_sample = 0;
  const gint16 *samples = (gint16 *) data;
  guint n_samples = size / sizeof (gint16);
  gint shift = 1 << (sizeof (gint16) * 8 - depth);
  gint i;

  g_return_if_fail (depth <= (sizeof (gint16) * 8));
  g_return_if_fail (size % sizeof (gint16) == 0);

  while (n_samples) {
    gint n = MIN (n_samples, G_N_ELEMENTS (conv_samples));

    n_samples -= n;
    for (i = 0; i < n; i++) {
      gint16 old_sample = samples[i] * shift;

      peak_sample = MAX (peak_sample, ABS ((gint32) old_sample));
      conv_samples[i] = (gfloat) old_sample;
    }
    samples += n;
    rg_analysis_analyze (ctx, conv_samples, NULL, n);
  }
  ctx->track.peak = MAX (ctx->track.peak,
      (gdouble) peak_sample / ((gdouble) (1u << 15)));
}

void
rg_analysis_analyze_stereo_int16 (RgAnalysisCtx * ctx, gconstpointer data,
    gsize size, guint depth)
{
  gfloat conv_samples_l[256];
  gfloat conv_samples_r[256];
  gint32 peak_sample = 0;
  const gint16 *samples = (gint16 *) data;
  guint n_frames = size / (sizeof (gint16) * 2);
  gint shift = 1 << (sizeof (gint16) * 8 - depth);
  gint i;

  g_return_if_fail (depth <= (sizeof (gint16) * 8));
  g_return_if_fail (size % (sizeof (gint16) * 2) == 0);

  while (n_frames) {
    gint n = MIN (n_frames, G_N_ELEMENTS (conv_samples_l));

    n_frames -= n;
    for (i = 0; i < n; i++) {
      gint16 old_sample;

      old_sample = samples[2 * i] * shift;
      peak_sample = MAX (peak_sample, ABS ((gint32) old_sample));
      conv_samples_l[i] = (gfloat) old_sample;

      old_sample = samples[2 * i + 1] * shift;
      peak_sample = MAX (peak_sample, ABS ((gint32) old_sample));
      conv_samples_r[i] = (gfloat) old_sample;
    }
    samples += 2 * n;
    rg_analysis_analyze (ctx, conv_samples_l, conv_samples_r, n);
  }
  ctx->track.peak = MAX (ctx->track.peak,
      (gdouble) peak_sample / ((gdouble) (1u << 15)));
}

/* Analyze the given chunk of samples.  The sample data is given in
 * floating point format but should be scaled such that the values
 * +/-32768.0 correspond to the -0dBFS reference amplitude.
 *
 * samples_l: Buffer with sample data for the left channel or of the
 * mono channel.
 *
 * samples_r: Buffer with sample data for the right channel or NULL
 * for mono.
 *
 * n_samples: Number of samples passed in each buffer.
 */

void
rg_analysis_analyze (RgAnalysisCtx * ctx, const gfloat * samples_l,
    const gfloat * samples_r, guint n_samples)
{
  const gfloat *input_l, *input_r;
  guint n_samples_done;
  gint i;

  g_return_if_fail (ctx != NULL);
  g_return_if_fail (samples_l != NULL);
  g_return_if_fail (ctx->sample_rate != 0);

  if (n_samples == 0)
    return;

  if (samples_r == NULL)
    /* Mono. */
    samples_r = samples_l;

  memcpy (ctx->inpre_l, samples_l,
      MIN (n_samples, MAX_ORDER) * sizeof (gfloat));
  memcpy (ctx->inpre_r, samples_r,
      MIN (n_samples, MAX_ORDER) * sizeof (gfloat));

  n_samples_done = 0;
  while (n_samples_done < n_samples) {
    /* Limit number of samples to be processed in this iteration to
     * the number needed to complete the next window: */
    guint n_samples_current = MIN (n_samples - n_samples_done,
        ctx->window_n_samples - ctx->window_n_samples_done);

    if (n_samples_done < MAX_ORDER) {
      input_l = ctx->inpre_l + n_samples_done;
      input_r = ctx->inpre_r + n_samples_done;
      n_samples_current = MIN (n_samples_current, MAX_ORDER - n_samples_done);
    } else {
      input_l = samples_l + n_samples_done;
      input_r = samples_r + n_samples_done;
    }

    apply_filters (ctx, input_l, input_r, n_samples_current);

    /* Update the square sum. */
    for (i = 0; i < n_samples_current; i++)
      ctx->window_square_sum += ctx->out_l[ctx->window_n_samples_done + i]
          * ctx->out_l[ctx->window_n_samples_done + i]
          + ctx->out_r[ctx->window_n_samples_done + i]
          * ctx->out_r[ctx->window_n_samples_done + i];

    ctx->window_n_samples_done += n_samples_current;
    ctx->buffer_n_samples_done += n_samples_current;

    g_return_if_fail (ctx->window_n_samples_done <= ctx->window_n_samples);

    if (ctx->window_n_samples_done == ctx->window_n_samples) {
      /* Get the Root Mean Square (RMS) for this set of samples. */
      gdouble val = STEPS_PER_DB * 10. * log10 (ctx->window_square_sum /
          ctx->window_n_samples * 0.5 + 1.e-37);
      gint ival = CLAMP ((gint) val, 0,
          (gint) G_N_ELEMENTS (ctx->track.histogram) - 1);
      /* Compute the per-window gain */
      const gdouble gain = PINK_REF - (gdouble) ival / STEPS_PER_DB;
      const GstClockTime timestamp = ctx->buffer_timestamp
          + gst_util_uint64_scale_int_ceil (GST_SECOND,
          ctx->buffer_n_samples_done,
          ctx->sample_rate)
          - RMS_WINDOW_MSECS * GST_MSECOND;

      ctx->post_message (ctx->analysis, timestamp,
          RMS_WINDOW_MSECS * GST_MSECOND, -gain);


      ctx->track.histogram[ival]++;
      ctx->window_square_sum = 0.;
      ctx->window_n_samples_done = 0;

      /* No need for memmove here, the areas never overlap: Even for
       * the smallest sample rate, the number of samples needed for
       * the window is greater than MAX_ORDER. */

      memcpy (ctx->stepbuf_l, ctx->stepbuf_l + ctx->window_n_samples,
          MAX_ORDER * sizeof (gfloat));
      memcpy (ctx->outbuf_l, ctx->outbuf_l + ctx->window_n_samples,
          MAX_ORDER * sizeof (gfloat));

      memcpy (ctx->stepbuf_r, ctx->stepbuf_r + ctx->window_n_samples,
          MAX_ORDER * sizeof (gfloat));
      memcpy (ctx->outbuf_r, ctx->outbuf_r + ctx->window_n_samples,
          MAX_ORDER * sizeof (gfloat));
    }

    n_samples_done += n_samples_current;
  }

  if (n_samples >= MAX_ORDER) {

    memcpy (ctx->inprebuf_l, samples_l + n_samples - MAX_ORDER,
        MAX_ORDER * sizeof (gfloat));

    memcpy (ctx->inprebuf_r, samples_r + n_samples - MAX_ORDER,
        MAX_ORDER * sizeof (gfloat));

  } else {

    memmove (ctx->inprebuf_l, ctx->inprebuf_l + n_samples,
        (MAX_ORDER - n_samples) * sizeof (gfloat));
    memcpy (ctx->inprebuf_l + MAX_ORDER - n_samples, samples_l,
        n_samples * sizeof (gfloat));

    memmove (ctx->inprebuf_r, ctx->inprebuf_r + n_samples,
        (MAX_ORDER - n_samples) * sizeof (gfloat));
    memcpy (ctx->inprebuf_r + MAX_ORDER - n_samples, samples_r,
        n_samples * sizeof (gfloat));

  }
}

/* Obtain track gain and peak.  Returns TRUE on success.  Can fail if
 * not enough samples have been processed.  Updates album accumulator.
 * Resets track accumulator. */

gboolean
rg_analysis_track_result (RgAnalysisCtx * ctx, gdouble * gain, gdouble * peak)
{
  gboolean result;

  g_return_val_if_fail (ctx != NULL, FALSE);

  accumulator_add (&ctx->album, &ctx->track);
  result = accumulator_result (&ctx->track, gain, peak);
  accumulator_clear (&ctx->track);

  reset_filters (ctx);
  reset_silence_detection (ctx);

  return result;
}

/* Obtain album gain and peak.  Returns TRUE on success.  Can fail if
 * not enough samples have been processed.  Resets album
 * accumulator. */

gboolean
rg_analysis_album_result (RgAnalysisCtx * ctx, gdouble * gain, gdouble * peak)
{
  gboolean result;

  g_return_val_if_fail (ctx != NULL, FALSE);

  result = accumulator_result (&ctx->album, gain, peak);
  accumulator_clear (&ctx->album);

  return result;
}

void
rg_analysis_reset_album (RgAnalysisCtx * ctx)
{
  accumulator_clear (&ctx->album);
}

/* Reset internal buffers as well as track and album accumulators.
 * Configured sample rate is kept intact. */

void
rg_analysis_reset (RgAnalysisCtx * ctx)
{
  g_return_if_fail (ctx != NULL);

  reset_filters (ctx);
  accumulator_clear (&ctx->track);
  accumulator_clear (&ctx->album);
  reset_silence_detection (ctx);
}
