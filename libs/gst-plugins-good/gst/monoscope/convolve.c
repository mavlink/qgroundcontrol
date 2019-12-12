/* Karatsuba convolution
 *
 *  Copyright (C) 1999 Ralph Loader <suckfish@ihug.co.nz>
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
 *
 * Note: 7th December 2004: This file used to be licensed under the GPL,
 *       but we got permission from Ralp Loader to relicense it to LGPL.
 *
 *  $Id$
 *
 */

/* The algorithm is based on the following.  For the convolution of a pair
 * of pairs, (a,b) * (c,d) = (0, a.c, a.d+b.c, b.d), we can reduce the four
 * multiplications to three, by the formulae a.d+b.c = (a+b).(c+d) - a.c -
 * b.d.  A similar relation enables us to compute a 2n by 2n convolution
 * using 3 n by n convolutions, and thus a 2^n by 2^n convolution using 3^n
 * multiplications (as opposed to the 4^n that the quadratic algorithm
 * takes. */

/* For large n, this is slower than the O(n log n) that the FFT method
 * takes, but we avoid using complex numbers, and we only have to compute
 * one convolution, as opposed to 3 FFTs.  We have good locality-of-
 * reference as well, which will help on CPUs with tiny caches.  */

/* E.g., for a 512 x 512 convolution, the FFT method takes 55 * 512 = 28160
 * (real) multiplications, as opposed to 3^9 = 19683 for the Karatsuba
 * algorithm.  We actually want 257 outputs of a 256 x 512 convolution;
 * that doesn't appear to give an easy advantage for the FFT algorithm, but
 * for the Karatsuba algorithm, it's easy to use two 256 x 256
 * convolutions, taking 2 x 3^8 = 12312 multiplications.  [This difference
 * is that the FFT method "wraps" the arrays, doing a 2^n x 2^n -> 2^n,
 * while the Karatsuba algorithm pads with zeros, doing 2^n x 2^n -> 2.2^n
 * - 1]. */

/* There's a big lie above, actually... for a 4x4 convolution, it's quicker
 * to do it using 16 multiplications than the more complex Karatsuba
 * algorithm...  So the recursion bottoms out at 4x4s.  This increases the
 * number of multiplications by a factor of 16/9, but reduces the overheads
 * dramatically. */

/* The convolution algorithm is implemented as a stack machine.  We have a
 * stack of commands, each in one of the forms "do a 2^n x 2^n
 * convolution", or "combine these three length 2^n outputs into one
 * 2^{n+1} output." */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "convolve.h"

typedef union stack_entry_s
{
  struct
  {
    const double *left, *right;
    double *out;
  }
  v;
  struct
  {
    double *main, *null;
  }
  b;

}
stack_entry;

struct _struct_convolve_state
{
  int depth, small, big, stack_size;
  double *left;
  double *right;
  double *scratch;
  stack_entry *stack;
};

/*
 * Initialisation routine - sets up tables and space to work in.
 * Returns a pointer to internal state, to be used when performing calls.
 * On error, returns NULL.
 * The pointer should be freed when it is finished with, by convolve_close().
 */
convolve_state *
convolve_init (int depth)
{
  convolve_state *state;

  state = malloc (sizeof (convolve_state));
  state->depth = depth;
  state->small = (1 << depth);
  state->big = (2 << depth);
  state->stack_size = depth * 3;
  state->left = calloc (state->big, sizeof (double));
  state->right = calloc (state->small * 3, sizeof (double));
  state->scratch = calloc (state->small * 3, sizeof (double));
  state->stack = calloc (state->stack_size + 1, sizeof (stack_entry));
  return state;
}

/*
 * Free the state allocated with convolve_init().
 */
void
convolve_close (convolve_state * state)
{
  free (state->left);
  free (state->right);
  free (state->scratch);
  free (state->stack);
  free (state);
}

static void
convolve_4 (double *out, const double *left, const double *right)
/* This does a 4x4 -> 7 convolution.  For what it's worth, the slightly odd
 * ordering gives about a 1% speed up on my Pentium II. */
{
  double l0, l1, l2, l3, r0, r1, r2, r3;
  double a;

  l0 = left[0];
  r0 = right[0];
  a = l0 * r0;
  l1 = left[1];
  r1 = right[1];
  out[0] = a;
  a = (l0 * r1) + (l1 * r0);
  l2 = left[2];
  r2 = right[2];
  out[1] = a;
  a = (l0 * r2) + (l1 * r1) + (l2 * r0);
  l3 = left[3];
  r3 = right[3];
  out[2] = a;

  out[3] = (l0 * r3) + (l1 * r2) + (l2 * r1) + (l3 * r0);
  out[4] = (l1 * r3) + (l2 * r2) + (l3 * r1);
  out[5] = (l2 * r3) + (l3 * r2);
  out[6] = l3 * r3;
}

static void
convolve_run (stack_entry * top, unsigned size, double *scratch)
/* Interpret a stack of commands.  The stack starts with two entries; the
 * convolution to do, and an illegal entry used to mark the stack top.  The
 * size is the number of entries in each input, and must be a power of 2,
 * and at least 8.  It is OK to have out equal to left and/or right.
 * scratch must have length 3*size.  The number of stack entries needed is
 * 3n-4 where size=2^n. */
{
  do {
    const double *left;
    const double *right;
    double *out;

    /* When we get here, the stack top is always a convolve,
     * with size > 4.  So we will split it.  We repeatedly split
     * the top entry until we get to size = 4. */

    left = top->v.left;
    right = top->v.right;
    out = top->v.out;
    top++;

    do {
      double *s_left, *s_right;
      int i;

      /* Halve the size. */
      size >>= 1;

      /* Allocate the scratch areas. */
      s_left = scratch + size * 3;
      /* s_right is a length 2*size buffer also used for
       * intermediate output. */
      s_right = scratch + size * 4;

      /* Create the intermediate factors. */
      for (i = 0; i < size; i++) {
        double l = left[i] + left[i + size];
        double r = right[i] + right[i + size];

        s_left[i + size] = r;
        s_left[i] = l;
      }

      /* Push the combine entry onto the stack. */
      top -= 3;
      top[2].b.main = out;
      top[2].b.null = NULL;

      /* Push the low entry onto the stack.  This must be
       * the last of the three sub-convolutions, because
       * it may overwrite the arguments. */
      top[1].v.left = left;
      top[1].v.right = right;
      top[1].v.out = out;

      /* Push the mid entry onto the stack. */
      top[0].v.left = s_left;
      top[0].v.right = s_right;
      top[0].v.out = s_right;

      /* Leave the high entry in variables. */
      left += size;
      right += size;
      out += size * 2;

    } while (size > 4);

    /* When we get here, the stack top is a group of 3
     * convolves, with size = 4, followed by some combines.  */
    convolve_4 (out, left, right);
    convolve_4 (top[0].v.out, top[0].v.left, top[0].v.right);
    convolve_4 (top[1].v.out, top[1].v.left, top[1].v.right);
    top += 2;

    /* Now process combines. */
    do {
      /* b.main is the output buffer, mid is the middle
       * part which needs to be adjusted in place, and
       * then folded back into the output.  We do this in
       * a slightly strange way, so as to avoid having
       * two loops. */
      double *out = top->b.main;
      double *mid = scratch + size * 4;
      unsigned int i;

      top++;
      out[size * 2 - 1] = 0;
      for (i = 0; i < size - 1; i++) {
        double lo;
        double hi;

        lo = mid[0] - (out[0] + out[2 * size]) + out[size];
        hi = mid[size] - (out[size] + out[3 * size]) + out[2 * size];
        out[size] = lo;
        out[2 * size] = hi;
        out++;
        mid++;
      }
      size <<= 1;
    } while (top->b.null == NULL);
  } while (top->b.main != NULL);
}

/*
 * convolve_match:
 * @lastchoice: an array of size SMALL.
 * @input: an array of size BIG (2*SMALL)
 * @state: a (non-NULL) pointer returned by convolve_init.
 *
 * We find the contiguous SMALL-size sub-array of input that best matches
 * lastchoice. A measure of how good a sub-array is compared with the lastchoice
 * is given by the sum of the products of each pair of entries.  We maximise
 * that, by taking an appropriate convolution, and then finding the maximum
 * entry in the convolutions.
 *
 * Return: the position of the best match
 */
int
convolve_match (const int *lastchoice, const short *input,
    convolve_state * state)
{
  double avg = 0;
  double best;
  int p = 0;
  int i;
  double *left = state->left;
  double *right = state->right;
  double *scratch = state->scratch;
  stack_entry *top = state->stack + (state->stack_size - 1);

  for (i = 0; i < state->big; i++)
    left[i] = input[i];

  for (i = 0; i < state->small; i++) {
    double a = lastchoice[(state->small - 1) - i];

    right[i] = a;
    avg += a;
  }

  /* We adjust the smaller of the two input arrays to have average
   * value 0.  This makes the eventual result insensitive to both
   * constant offsets and positive multipliers of the inputs. */
  avg /= state->small;
  for (i = 0; i < state->small; i++)
    right[i] -= avg;
  /* End-of-stack marker. */
  top[1].b.null = scratch;
  top[1].b.main = NULL;
  /* The low (small x small) part, of which we want the high outputs. */
  top->v.left = left;
  top->v.right = right;
  top->v.out = right + state->small;
  convolve_run (top, state->small, scratch);

  /* The high (small x small) part, of which we want the low outputs. */
  top->v.left = left + state->small;
  top->v.right = right;
  top->v.out = right;
  convolve_run (top, state->small, scratch);

  /* Now find the best position amoungs this.  Apart from the first
   * and last, the required convolution outputs are formed by adding
   * outputs from the two convolutions above. */
  best = right[state->big - 1];
  right[state->big + state->small - 1] = 0;
  p = -1;
  for (i = 0; i < state->small; i++) {
    double a = right[i] + right[i + state->big];

    if (a > best) {
      best = a;
      p = i;
    }
  }
  p++;

#if 0
  {
    /* This is some debugging code... */
    best = 0;
    for (i = 0; i < state->small; i++)
      best += ((double) input[i + p]) * ((double) lastchoice[i] - avg);

    for (i = 0; i <= state->small; i++) {
      double tot = 0;
      unsigned int j;

      for (j = 0; j < state->small; j++)
        tot += ((double) input[i + j]) * ((double) lastchoice[j] - avg);
      if (tot > best)
        printf ("(%i)", i);
      if (tot != left[i + (state->small - 1)])
        printf ("!");
    }

    printf ("%i\n", p);
  }
#endif

  return p;
}
