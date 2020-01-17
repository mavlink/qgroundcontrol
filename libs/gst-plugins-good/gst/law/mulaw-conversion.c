/*
 * This routine converts from linear to ulaw
 * 29 September 1989
 *
 * Craig Reese: IDA/Supercomputing Research Center
 * Joe Campbell: Department of Defense
 *
 * References:
 * 1) CCITT Recommendation G.711  (very difficult to follow)
 * 2) "A New Digital Technique for Implementation of Any 
 *     Continuous PCM Companding Law," Villeret, Michel,
 *     et al. 1973 IEEE Int. Conf. on Communications, Vol 1,
 *     1973, pg. 11.12-11.17
 * 3) MIL-STD-188-113,"Interoperability and Performance Standards
 *     for Analog-to_Digital Conversion Techniques,"
 *     17 February 1987
 *
 * Input: Signed 16 bit linear sample
 * Output: 8 bit ulaw sample
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#include "mulaw-conversion.h"

#undef ZEROTRAP                 /* turn on the trap as per the MIL-STD */
#define BIAS 0x84               /* define the add-in bias for 16 bit samples */
#define CLIP 32635

void
mulaw_encode (gint16 * in, guint8 * out, gint numsamples)
{
  static const gint16 exp_lut[256] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
  };
  gint16 sign, exponent, mantissa;
  gint16 sample;
  guint8 ulawbyte;
  gint i;

  for (i = 0; i < numsamples; i++) {
    sample = in[i];
    /* get the sample into sign-magnitude */
    sign = (sample >> 8) & 0x80;        /* set aside the sign */
    if (sign != 0) {
      sample = -sample;         /* get magnitude */
    }
    /* sample can be zero because we can overflow in the inversion,
     * checking against the unsigned version solves this */
    if (((guint16) sample) > CLIP)
      sample = CLIP;            /* clip the magnitude */

    /* convert from 16 bit linear to ulaw */
    sample = sample + BIAS;
    exponent = exp_lut[(sample >> 7) & 0xFF];
    mantissa = (sample >> (exponent + 3)) & 0x0F;
    ulawbyte = ~(sign | (exponent << 4) | mantissa);
#ifdef ZEROTRAP
    if (ulawbyte == 0)
      ulawbyte = 0x02;          /* optional CCITT trap */
#endif
    out[i] = ulawbyte;
  }
}

/*
 * This routine converts from ulaw to 16 bit linear
 * 29 September 1989
 *
 * Craig Reese: IDA/Supercomputing Research Center
 *
 * References:
 * 1) CCITT Recommendation G.711  (very difficult to follow)
 * 2) MIL-STD-188-113,"Interoperability and Performance Standards
 *     for Analog-to_Digital Conversion Techniques,"
 *     17 February 1987
 *
 * Input: 8 bit ulaw sample
 * Output: signed 16 bit linear sample
 */

void
mulaw_decode (guint8 * in, gint16 * out, gint numsamples)
{
  static const gint16 exp_lut[8] =
      { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
  gint16 sign, exponent, mantissa;
  guint8 ulawbyte;
  gint16 linear;
  gint i;

  for (i = 0; i < numsamples; i++) {
    ulawbyte = in[i];
    ulawbyte = ~ulawbyte;
    sign = (ulawbyte & 0x80);
    exponent = (ulawbyte >> 4) & 0x07;
    mantissa = ulawbyte & 0x0F;
    linear = exp_lut[exponent] + (mantissa << (exponent + 3));
    if (sign != 0)
      linear = -linear;
    out[i] = linear;
  }
}
