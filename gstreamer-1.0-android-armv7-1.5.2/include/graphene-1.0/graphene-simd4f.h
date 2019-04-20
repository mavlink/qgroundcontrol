/* graphene-simd4f.h: SIMD wrappers and operations
 *
 * Copyright © 2014  Emmanuele Bassi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SH1_0 THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __GRAPHENE_SIMD4F_H__
#define __GRAPHENE_SIMD4F_H__

#if !defined(GRAPHENE_H_INSIDE) && !defined(GRAPHENE_COMPILATION)
#error "Only graphene.h can be included directly."
#endif

/* needed for memcpy() */
#include <string.h>
#include <math.h>

#include "graphene-config.h"
#include "graphene-macros.h"
#include "graphene-version-macros.h"

GRAPHENE_BEGIN_DECLS

/* Platform specific operations */

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_init            (float                   x,
                                                         float                   y,
                                                         float                   z,
                                                         float                   w);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_init_zero       (void);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_init_4f         (const float            *v);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_init_3f         (const float            *v);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_init_2f         (const float            *v);

GRAPHENE_AVAILABLE_IN_1_0
void                    graphene_simd4f_dup_4f          (const graphene_simd4f_t s,
                                                         float                  *v);
GRAPHENE_AVAILABLE_IN_1_0
void                    graphene_simd4f_dup_3f          (const graphene_simd4f_t s,
                                                         float                  *v);
GRAPHENE_AVAILABLE_IN_1_0
void                    graphene_simd4f_dup_2f          (const graphene_simd4f_t s,
                                                         float                  *v);

GRAPHENE_AVAILABLE_IN_1_2
float                   graphene_simd4f_get             (const graphene_simd4f_t s,
                                                         unsigned int            i);
GRAPHENE_AVAILABLE_IN_1_0
float                   graphene_simd4f_get_x           (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
float                   graphene_simd4f_get_y           (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
float                   graphene_simd4f_get_z           (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
float                   graphene_simd4f_get_w           (const graphene_simd4f_t s);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_splat           (float                   v);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_splat_x         (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_splat_y         (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_splat_z         (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_splat_w         (const graphene_simd4f_t s);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_add             (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_sub             (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_mul             (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_div             (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_sqrt            (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_reciprocal      (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_rsqrt           (const graphene_simd4f_t s);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_cross3          (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_min             (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_max             (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_shuffle_wxyz    (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_shuffle_zwxy    (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_shuffle_yzwx    (const graphene_simd4f_t s);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_zero_w          (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_zero_zw         (const graphene_simd4f_t s);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_merge_high      (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_merge_low       (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_merge_w         (const graphene_simd4f_t s,
                                                         float                   v);

GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_flip_sign_0101  (const graphene_simd4f_t s);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_flip_sign_1010  (const graphene_simd4f_t s);

GRAPHENE_AVAILABLE_IN_1_0
bool                    graphene_simd4f_cmp_eq          (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_0
bool                    graphene_simd4f_cmp_neq         (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_2
bool                    graphene_simd4f_cmp_lt          (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_2
bool                    graphene_simd4f_cmp_le          (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_2
bool                    graphene_simd4f_cmp_ge          (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_2
bool                    graphene_simd4f_cmp_gt          (const graphene_simd4f_t a,
                                                         const graphene_simd4f_t b);
GRAPHENE_AVAILABLE_IN_1_0
graphene_simd4f_t       graphene_simd4f_neg             (const graphene_simd4f_t s);

#if defined(GRAPHENE_USE_SSE)

/* SSE2 implementation of SIMD 4f */

# if defined(__GNUC__)

/* Use GCC statement __extension__ to inline all these functions */

#  define graphene_simd4f_init(x,y,z,w) \
  (__extension__ ({ \
    (graphene_simd4f_t) { (x), (y), (z), (w) }; \
  }))

#  define graphene_simd4f_init_zero() \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_setzero_ps(); \
  }))

#  define graphene_simd4f_init_4f(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_loadu_ps (v); \
  }))

#  define graphene_simd4f_init_3f(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) { (v)[0], (v)[1], (v)[2], 0.f }; \
  }))

#  define graphene_simd4f_init_2f(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) { (v)[0], (v)[1], 0.f, 0.f }; \
  }))

#  define graphene_simd4f_dup_4f(s,v) \
  (__extension__ ({ \
    _mm_storeu_ps ((v), (s)); \
  }))

#  define graphene_simd4f_dup_3f(s,v) \
  (__extension__ ({ \
    memcpy ((v), &(s), sizeof (float) * 3); \
  }))

#  define graphene_simd4f_dup_2f(s,v) \
  (__extension__ ({ \
    memcpy ((v), &(s), sizeof (float) * 2); \
  }))

typedef union {
  graphene_simd4f_t s;
  float f[4];
  unsigned int ui[4];
} graphene_simd4f_union_t;

#  define graphene_simd4f_get(s,i) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u = { (s) }; \
    (float) __u.f[(i)]; \
  }))

#  define graphene_simd4f_get_x(s)      graphene_simd4f_get (s, 0)
#  define graphene_simd4f_get_y(s)      graphene_simd4f_get (s, 1)
#  define graphene_simd4f_get_z(s)      graphene_simd4f_get (s, 2)
#  define graphene_simd4f_get_w(s)      graphene_simd4f_get (s, 3)

#  define graphene_simd4f_splat(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_set1_ps ((v)); \
  }))

#  define graphene_simd4f_splat_x(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_shuffle_ps ((v), (v), _MM_SHUFFLE (0, 0, 0, 0)); \
  }))

#  define graphene_simd4f_splat_y(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_shuffle_ps ((v), (v), _MM_SHUFFLE (1, 1, 1, 1)); \
  }))

#  define graphene_simd4f_splat_z(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_shuffle_ps ((v), (v), _MM_SHUFFLE (2, 2, 2, 2)); \
  }))

#  define graphene_simd4f_splat_w(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_shuffle_ps ((v), (v), _MM_SHUFFLE (3, 3, 3, 3)); \
  }))

#  define graphene_simd4f_add(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_add_ps ((a), (b)); \
  }))

#  define graphene_simd4f_sub(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_sub_ps ((a), (b)); \
  }))

#  define graphene_simd4f_mul(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_mul_ps ((a), (b)); \
  }))

#  define graphene_simd4f_div(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_div_ps ((a), (b)); \
  }))

#  define graphene_simd4f_sqrt(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_sqrt_ps ((v)); \
  }))

#  define graphene_simd4f_reciprocal(v) \
  (__extension__ ({ \
    const graphene_simd4f_t __two = graphene_simd4f_init (2.0f, 2.0f, 2.0f, 2.0f); \
    graphene_simd4f_t __s = _mm_rcp_ps ((v)); \
    graphene_simd4f_mul (__s, graphene_simd4f_sub (__two, graphene_simd4f_mul ((v), __s))); \
  }))

#  define graphene_simd4f_rsqrt(v) \
  (__extension__ ({ \
    const graphene_simd4f_t __half = graphene_simd4f_init (0.5f, 0.5f, 0.5f, 0.5f); \
    const graphene_simd4f_t __three = graphene_simd4f_init (3.0f, 3.0f, 3.0f, 3.0f); \
    graphene_simd4f_t __s = _mm_rsqrt_ps ((v)); \
    graphene_simd4f_mul (graphene_simd4f_mul (__s, __half), \
                         graphene_simd4f_sub (__three, \
                                              graphene_simd4f_mul (__s, graphene_simd4f_mul ((v), __s)))); \
  }))

#  define graphene_simd4f_cross3(a,b) \
  (__extension__ ({ \
    const graphene_simd4f_t __a_yzx = _mm_shuffle_ps ((a), (a), _MM_SHUFFLE (3, 0, 2, 1)); \
    const graphene_simd4f_t __a_zxy = _mm_shuffle_ps ((a), (a), _MM_SHUFFLE (3, 1, 0, 2)); \
    const graphene_simd4f_t __b_yzx = _mm_shuffle_ps ((b), (b), _MM_SHUFFLE (3, 0, 2, 1)); \
    const graphene_simd4f_t __b_zxy = _mm_shuffle_ps ((b), (b), _MM_SHUFFLE (3, 1, 0, 2)); \
    (graphene_simd4f_t) _mm_sub_ps (_mm_mul_ps (__a_yzx, __b_zxy), _mm_mul_ps (__a_zxy, __b_yzx)); \
  }))

#  define graphene_simd4f_min(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_min_ps ((a), (b)); \
  }))

#  define graphene_simd4f_max(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_max_ps ((a), (b)); \
  }))

#  define graphene_simd4f_shuffle_wxyz(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_shuffle_ps ((v), (v), _MM_SHUFFLE (2, 1, 0, 3)); \
  }))

#  define graphene_simd4f_shuffle_zwxy(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_shuffle_ps ((v), (v), _MM_SHUFFLE (1, 0, 3, 2)); \
  }))

#  define graphene_simd4f_shuffle_yzwx(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_shuffle_ps ((v), (v), _MM_SHUFFLE (0, 3, 2, 1)); \
  }))

#  define graphene_simd4f_zero_w(v) \
  (__extension__ ({ \
    graphene_simd4f_t __s = _mm_unpackhi_ps ((v), _mm_setzero_ps ()); \
    (graphene_simd4f_t) _mm_movelh_ps ((v), __s); \
  }))

#  define graphene_simd4f_zero_zw(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_movelh_ps ((v), _mm_setzero_ps ()); \
  }))

#  define graphene_simd4f_merge_w(s,v) \
  (__extension__ ({ \
    graphene_simd4f_t __s = _mm_unpackhi_ps ((s), _mm_set1_ps ((v))); \
    (graphene_simd4f_t) _mm_movelh_ps ((s), __s); \
  }))

#  define graphene_simd4f_merge_high(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_movehl_ps ((b), (a)); \
  }))

#  define graphene_simd4f_merge_low(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) _mm_movelh_ps ((a), (b)); \
  }))

typedef GRAPHENE_ALIGN16 union {
  unsigned int ui[4];
  float f[4];
} graphene_simd4f_uif_t;

#  define graphene_simd4f_flip_sign_0101(v) \
  (__extension__ ({ \
    const graphene_simd4f_uif_t __pnpn = { { \
      0x00000000, \
      0x80000000, \
      0x00000000, \
      0x80000000  \
    } }; \
    (graphene_simd4f_t) _mm_xor_ps ((v), _mm_load_ps (__pnpn.f)); \
  }))

#  define graphene_simd4f_flip_sign_1010(v) \
  (__extension__ ({ \
    const graphene_simd4f_uif_t __npnp = { { \
      0x80000000, \
      0x00000000, \
      0x80000000, \
      0x00000000, \
    } }; \
    (graphene_simd4f_t) _mm_xor_ps ((v), _mm_load_ps (__npnp.f)); \
  }))

#  define graphene_simd4f_cmp_eq(a,b) \
  (__extension__ ({ \
    __m128i __res = (__m128i) _mm_cmpneq_ps ((a), (b)); \
    (bool) (_mm_movemask_epi8 (__res) == 0); \
  }))

#  define graphene_simd4f_cmp_neq(a,b) \
  (__extension__ ({ \
    __m128i __res = (__m128i) _mm_cmpneq_ps ((a), (b)); \
    (bool) (_mm_movemask_epi8 (__res) != 0); \
  }))

#  define graphene_simd4f_cmp_lt(a,b) \
  (__extension__ ({ \
    __m128i __res = (__m128i) _mm_cmplt_ps ((a), (b)); \
    (bool) (_mm_movemask_epi8 (__res) == 0xffff); \
  }))

#  define graphene_simd4f_cmp_le(a,b) \
  (__extension__ ({ \
    __m128i __res = (__m128i) _mm_cmple_ps ((a), (b)); \
    (bool) (_mm_movemask_epi8 (__res) == 0xffff); \
  }))

#  define graphene_simd4f_cmp_ge(a,b) \
  (__extension__ ({ \
    __m128i __res = (__m128i) _mm_cmpge_ps ((a), (b)); \
    (bool) (_mm_movemask_epi8 (__res) == 0xffff); \
  }))

#  define graphene_simd4f_cmp_gt(a,b) \
  (__extension__ ({ \
    __m128i __res = (__m128i) _mm_cmpgt_ps ((a), (b)); \
    (bool) (_mm_movemask_epi8 (__res) == 0xffff); \
  }))

#  define graphene_simd4f_neg(s) \
  (__extension__ ({ \
    const graphene_simd4f_uif_t __mask = { { \
      0x80000000, \
      0x80000000, \
      0x80000000, \
      0x80000000, \
    } }; \
    (graphene_simd4f_t) _mm_xor_ps ((s), _mm_load_ps (__mask.f)); \
  }))

# elif defined (_MSC_VER)

#include <emmintrin.h> /* for __m128i */

/* Use static inline to inline all these functions */

#define graphene_simd4f_init(x,y,z,w) _simd4f_init(x,y,z,w)

static inline graphene_simd4f_t
_simd4f_init (float x, float y, float z, float w)
{
  graphene_simd4f_t __s = { x, y, z, w };
  return __s;
}

#define graphene_simd4f_init_zero() \
  _mm_setzero_ps()

#define graphene_simd4f_init_4f(v) \
  _mm_loadu_ps(v)

#define graphene_simd4f_init_3f(v) \
  graphene_simd4f_init (v[0], v[1], v[2], 0.f)

#define graphene_simd4f_init_2f(v) \
  graphene_simd4f_init (v[0], v[1], 0.f, 0.f)

#define graphene_simd4f_dup_4f(s,v) \
  _mm_storeu_ps (v, s)

#define graphene_simd4f_dup_3f(s,v) \
  memcpy (v, &s, sizeof (float) * 3)

#define graphene_simd4f_dup_2f(s,v) \
  memcpy (v, &s, sizeof (float) * 2)

typedef union {
  graphene_simd4f_t s;
  float f[4];
  unsigned int ui[4];
} graphene_simd4f_union_t;

#define graphene_simd4f_get(s,i) _simd4f_get_xyzw(s, i)
#define graphene_simd4f_get_x(s) _simd4f_get_xyzw(s, 0)
#define graphene_simd4f_get_y(s) _simd4f_get_xyzw(s, 1)
#define graphene_simd4f_get_z(s) _simd4f_get_xyzw(s, 2)
#define graphene_simd4f_get_w(s) _simd4f_get_xyzw(s, 3)

static inline float
_simd4f_get_xyzw (graphene_simd4f_t s, int mode)
{
  /* mode: get_x=0
           get_y=1
           get_z=2
           get_z=3 */

  graphene_simd4f_union_t u;
  u.s = s;
  return u.f[mode];
}

#define graphene_simd4f_splat(v) \
  _mm_set1_ps (v)

#define graphene_simd4f_splat_x(v) \
  _mm_shuffle_ps (v, v, _MM_SHUFFLE (0, 0, 0, 0))

#define graphene_simd4f_splat_y(v) \
  _mm_shuffle_ps (v, v, _MM_SHUFFLE (1, 1, 1, 1))

#define graphene_simd4f_splat_z(v) \
  _mm_shuffle_ps (v, v, _MM_SHUFFLE (2, 2, 2, 2))

#define graphene_simd4f_splat_w(v) \
  _mm_shuffle_ps (v, v, _MM_SHUFFLE (3, 3, 3, 3))

#define graphene_simd4f_add(a,b) \
  _mm_add_ps (a, b)

#define graphene_simd4f_sub(a,b) \
  _mm_sub_ps (a, b)

#define graphene_simd4f_mul(a,b) \
  _mm_mul_ps (a, b)

#define graphene_simd4f_div(a,b) \
  _mm_div_ps (a, b)

#define graphene_simd4f_sqrt(v) \
  _mm_sqrt_ps (v)

#define graphene_simd4f_reciprocal(v) _simd4f_reciprocal(v)

static inline graphene_simd4f_t
_simd4f_reciprocal(const graphene_simd4f_t v)
{
  const graphene_simd4f_t __two = graphene_simd4f_init (2.0f, 2.0f, 2.0f, 2.0f);
  graphene_simd4f_t __s = _mm_rcp_ps (v);
  return graphene_simd4f_mul (__s,
                              graphene_simd4f_sub (__two,
                                                   graphene_simd4f_mul (v, __s)));
}

#define graphene_simd4f_rsqrt(v) _simd4f_rsqrt(v)

static inline graphene_simd4f_t
_simd4f_rsqrt(const graphene_simd4f_t v)
{
    const graphene_simd4f_t __half = graphene_simd4f_init (0.5f, 0.5f, 0.5f, 0.5f);
    const graphene_simd4f_t __three = graphene_simd4f_init (3.0f, 3.0f, 3.0f, 3.0f);
    graphene_simd4f_t __s = _mm_rsqrt_ps (v);
    return graphene_simd4f_mul (graphene_simd4f_mul (__s, __half),
                                graphene_simd4f_sub (__three,
                                graphene_simd4f_mul (__s, graphene_simd4f_mul (v, __s))));
}

#define graphene_simd4f_cross3(a,b) \
  _simd4f_cross3(a,b)

static inline graphene_simd4f_t
_simd4f_cross3 (const graphene_simd4f_t a,
                const graphene_simd4f_t b)
{
  const graphene_simd4f_t __a_yzx = _mm_shuffle_ps (a, a, _MM_SHUFFLE (3, 0, 2, 1));
  const graphene_simd4f_t __a_zxy = _mm_shuffle_ps (a, a, _MM_SHUFFLE (3, 1, 0, 2));
  const graphene_simd4f_t __b_yzx = _mm_shuffle_ps (b, b, _MM_SHUFFLE (3, 0, 2, 1));
  const graphene_simd4f_t __b_zxy = _mm_shuffle_ps (b, b, _MM_SHUFFLE (3, 1, 0, 2));

  return _mm_sub_ps (_mm_mul_ps (__a_yzx, __b_zxy), _mm_mul_ps (__a_zxy, __b_yzx));
}

#define graphene_simd4f_min(a,b) \
  _mm_min_ps (a, b)

#define graphene_simd4f_max(a,b) \
  _mm_max_ps (a, b)


#define graphene_simd4f_shuffle_wxyz(v) \
  _mm_shuffle_ps (v, v, _MM_SHUFFLE (2, 1, 0, 3))

#define graphene_simd4f_shuffle_zwxy(v) \
  _mm_shuffle_ps (v, v, _MM_SHUFFLE (1, 0, 3, 2))

#define graphene_simd4f_shuffle_yzwx(v) \
  _mm_shuffle_ps (v, v, _MM_SHUFFLE (0, 3, 2, 1))

#define graphene_simd4f_zero_w(v) \
  _mm_movelh_ps (v, _mm_unpackhi_ps (v, _mm_setzero_ps ()))

#define graphene_simd4f_zero_zw(v) \
  _mm_movelh_ps (v, _mm_setzero_ps ())

#define graphene_simd4f_merge_w(s,v) \
  _mm_movelh_ps (s, _mm_unpackhi_ps (s, _mm_set1_ps (v)))

#define graphene_simd4f_merge_high(a,b) \
  _mm_movehl_ps (b, a)

#define graphene_simd4f_merge_low(a,b) \
  _mm_movelh_ps (a, b)

typedef GRAPHENE_ALIGN16 union {
  unsigned int ui[4];
  float f[4];
} graphene_simd4f_uif_t;

#define graphene_simd4f_flip_sign_0101(v) _simd4f_flip_sign_0101(v)

static inline graphene_simd4f_t
_simd4f_flip_sign_0101 (const graphene_simd4f_t v)
{
  const graphene_simd4f_uif_t __pnpn = { {
    0x00000000,
    0x80000000,
    0x00000000,
    0x80000000
  } };

  return _mm_xor_ps (v, _mm_load_ps (__pnpn.f));
}

#define graphene_simd4f_flip_sign_1010(v) _simd4f_flip_sign_1010(v)

static inline graphene_simd4f_t
_simd4f_flip_sign_1010(const graphene_simd4f_t v)
{
  const graphene_simd4f_uif_t __npnp = { {
    0x80000000,
    0x00000000,
    0x80000000,
    0x00000000,
  } };

  return _mm_xor_ps (v, _mm_load_ps (__npnp.f));
}

#define graphene_simd4f_cmp_eq(a,b) _simd4f_cmp_eq(a,b)

static inline bool
_simd4f_cmp_eq (const graphene_simd4f_t a,
                        const graphene_simd4f_t b)
{
  __m128i __res = _mm_castps_si128 (_mm_cmpneq_ps (a, b));
  return (_mm_movemask_epi8 (__res) == 0);
}

#define graphene_simd4f_cmp_neq(a,b) _simd4f_cmp_neq(a,b)

static inline bool
_simd4f_cmp_neq (const graphene_simd4f_t a,
                         const graphene_simd4f_t b)
{
  __m128i __res = _mm_castps_si128 (_mm_cmpneq_ps (a, b));
  return (_mm_movemask_epi8 (__res) != 0);
}

#define graphene_simd4f_cmp_lt(a,b) _simd4f_cmp_lt(a,b)

static inline bool
_simd4f_cmp_lt (const graphene_simd4f_t a,
                const graphene_simd4f_t b)
{
  __m128i __res = _mm_castps_si128 (_mm_cmplt_ps (a, b));
  return (_mm_movemask_epi8 (__res) == 0xffff);
}

#define graphene_simd4f_cmp_le(a,b) _simd4f_cmp_le(a,b)

static inline bool
_simd4f_cmp_le (const graphene_simd4f_t a,
                const graphene_simd4f_t b)
{
  __m128i __res = _mm_castps_si128 (_mm_cmple_ps (a, b));
  return (_mm_movemask_epi8 (__res) == 0xffff);
}

#define graphene_simd4f_cmp_ge(a,b) _simd4f_cmp_ge(a,b)

static inline bool
_simd4f_cmp_ge (const graphene_simd4f_t a,
                const graphene_simd4f_t b)
{
  __m128i __res = _mm_castps_si128 (_mm_cmpge_ps (a, b));
  return (_mm_movemask_epi8 (__res) == 0xffff);
}

#define graphene_simd4f_cmp_gt(a,b) _simd4f_cmp_gt(a,b)

static inline bool
_simd4f_cmp_gt (const graphene_simd4f_t a,
                const graphene_simd4f_t b)
{
  __m128i __res = _mm_castps_si128 (_mm_cmpgt_ps (a, b));
  return (_mm_movemask_epi8 (__res) == 0xffff);
}

#define graphene_simd4f_neg(s) _simd4f_neg(s)

static inline graphene_simd4f_t
_simd4f_neg (const graphene_simd4f_t s)
{
  const graphene_simd4f_uif_t __mask = { {
    0x80000000,
    0x80000000,
    0x80000000,
    0x80000000,
  } };

  return _mm_xor_ps (s, _mm_load_ps (__mask.f));
}

#else 

#  error "Need GCC-compatible or Visual Studio compiler for SSE extensions."

/* Use static inline to inline all these functions */

# endif /* __GNUC__ */

#elif defined(GRAPHENE_USE_GCC)

/* GCC vector intrinsic implementation of SIMD 4f */

typedef int graphene_simd4i_t __attribute__((vector_size (16)));

# define graphene_simd4f_init(x,y,z,w) \
  (__extension__ ({ \
    (graphene_simd4f_t) { (x), (y), (z), (w) }; \
  }))

# define graphene_simd4f_init_zero() \
  (__extension__ ({ \
    (graphene_simd4f_t) { 0.f, 0.f, 0.f, 0.f }; \
  }))

# define graphene_simd4f_init_4f(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) { (v)[0], (v)[1], (v)[2], (v)[3] }; \
  }))

# define graphene_simd4f_init_3f(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) { (v)[0], (v)[1], (v)[2], 0.f }; \
  }))

# define graphene_simd4f_init_2f(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) { (v)[0], (v)[1], 0.f, 0.f }; \
  }))

# define graphene_simd4f_dup_4f(s,v) \
  (__extension__ ({ \
    memcpy ((v), &(s), sizeof (float) * 4); \
  }))

# define graphene_simd4f_dup_3f(s,v) \
  (__extension__ ({ \
    memcpy ((v), &(s), sizeof (float) * 3); \
  }))

# define graphene_simd4f_dup_2f(s,v) \
  (__extension__ ({ \
    memcpy ((v), &(s), sizeof (float) * 2); \
  }))

# define graphene_simd4f_get(s,i)       (__extension__ ({ (float) (s)[(i)]; }))
# define graphene_simd4f_get_x(s)       graphene_simd4f_get ((s), 0)
# define graphene_simd4f_get_y(s)       graphene_simd4f_get ((s), 1)
# define graphene_simd4f_get_z(s)       graphene_simd4f_get ((s), 2)
# define graphene_simd4f_get_w(s)       graphene_simd4f_get ((s), 3)

# define graphene_simd4f_splat(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) { (v), (v), (v), (v) }; \
  }))

# define graphene_simd4f_splat_x(v) \
  (__extension__ ({ \
    float __val = graphene_simd4f_get_x ((v)); \
    (graphene_simd4f_t) { __val, __val, __val, __val }; \
  }))

# define graphene_simd4f_splat_y(v) \
  (__extension__ ({ \
    float __val = graphene_simd4f_get_y ((v)); \
    (graphene_simd4f_t) { __val, __val, __val, __val }; \
  }))

# define graphene_simd4f_splat_z(v) \
  (__extension__ ({ \
    float __val = graphene_simd4f_get_z ((v)); \
    (graphene_simd4f_t) { __val, __val, __val, __val }; \
  }))

# define graphene_simd4f_splat_w(v) \
  (__extension__ ({ \
    float __val = graphene_simd4f_get_w ((v)); \
    (graphene_simd4f_t) { __val, __val, __val, __val }; \
  }))

# define graphene_simd4f_reciprocal(v) \
  (__extension__ ({ \
    graphene_simd4f_splat (1.0f) / (v); \
  }))

# define graphene_simd4f_sqrt(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) { \
      sqrt (graphene_simd4f_get_x (v)), \
      sqrt (graphene_simd4f_get_y (v)), \
      sqrt (graphene_simd4f_get_z (v)), \
      sqrt (graphene_simd4f_get_w (v)), \
    }; \
  }))

# define graphene_simd4f_rsqrt(v) \
  (__extension__ ({ \
    graphene_simd4f_splat (1.0f) / graphene_simd4f_sqrt ((v)); \
  }))

# define graphene_simd4f_add(a,b)       (__extension__ ({ (graphene_simd4f_t) ((a) + (b)); }))
# define graphene_simd4f_sub(a,b)       (__extension__ ({ (graphene_simd4f_t) ((a) - (b)); }))
# define graphene_simd4f_mul(a,b)       (__extension__ ({ (graphene_simd4f_t) ((a) * (b)); }))
# define graphene_simd4f_div(a,b)       (__extension__ ({ (graphene_simd4f_t) ((a) / (b)); }))

# define graphene_simd4f_cross3(a,b) \
  (__extension__ ({ \
    const graphene_simd4f_t __a = (a); \
    const graphene_simd4f_t __b = (b); \
    graphene_simd4f_init (__a[1] * __b[2] - __a[2] * __b[1], \
                          __a[2] * __b[0] - __a[0] * __b[2], \
                          __a[0] * __b[1] - __a[1] * __b[0], \
                          0.f); \
  }))

# define graphene_simd4f_min(a,b) \
  (__extension__ ({ \
    const graphene_simd4f_t __a = (a); \
    const graphene_simd4f_t __b = (b); \
    graphene_simd4f_init (__a[0] < __b[0] ? __a[0] : __b[0], \
                          __a[1] < __b[1] ? __a[1] : __b[1], \
                          __a[2] < __b[2] ? __a[2] : __b[2], \
                          __a[3] < __b[3] ? __a[3] : __b[3]); \
  }))

# define graphene_simd4f_max(a,b) \
  (__extension__ ({ \
    const graphene_simd4f_t __a = (a); \
    const graphene_simd4f_t __b = (b); \
    graphene_simd4f_init (__a[0] > __b[0] ? __a[0] : __b[0], \
                          __a[1] > __b[1] ? __a[1] : __b[1], \
                          __a[2] > __b[2] ? __a[2] : __b[2], \
                          __a[3] > __b[3] ? __a[3] : __b[3]); \
  }))

# define graphene_simd4f_shuffle_wxyz(v) \
  (__extension__ ({ \
    const graphene_simd4i_t __mask = { 3, 0, 1, 2 }; \
    (graphene_simd4f_t) __builtin_shuffle ((v), __mask); \
  }))

# define graphene_simd4f_shuffle_zwxy(v) \
  (__extension__ ({ \
    const graphene_simd4i_t __mask = { 2, 3, 0, 1 }; \
    (graphene_simd4f_t) __builtin_shuffle ((v), __mask); \
  }))

# define graphene_simd4f_shuffle_yzwx(v) \
  (__extension__ ({ \
    const graphene_simd4i_t __mask = { 1, 2, 3, 0 }; \
    (graphene_simd4f_t) __builtin_shuffle ((v), __mask); \
  }))

# define graphene_simd4f_zero_w(v) \
  (__extension__ ({ \
    const graphene_simd4i_t __mask = { 0, 1, 2, 4 }; \
    (graphene_simd4f_t) __builtin_shuffle ((v), graphene_simd4f_init_zero (), __mask); \
  }))

# define graphene_simd4f_zero_zw(v) \
  (__extension__ ({ \
    const graphene_simd4i_t __mask = { 0, 1, 4, 4 }; \
    (graphene_simd4f_t) __builtin_shuffle ((v), graphene_simd4f_init_zero (), __mask); \
  }))

# define graphene_simd4f_merge_w(s,v) \
  (__extension__ ({ \
    const graphene_simd4i_t __mask = { 0, 1, 2, 4 }; \
    (graphene_simd4f_t) __builtin_shuffle ((s), graphene_simd4f_splat ((v)), __mask); \
  }))

# define graphene_simd4f_merge_high(a,b) \
  (__extension__ ({ \
    const graphene_simd4i_t __mask = { 2, 3, 6, 7 }; \
    (graphene_simd4f_t) __builtin_shuffle ((a), (b), __mask); \
  }))

# define graphene_simd4f_merge_low(a,b) \
  (__extension__ ({ \
    const graphene_simd4i_t __mask = { 0, 1, 4, 5 }; \
    (graphene_simd4f_t) __builtin_shuffle ((a), (b), __mask); \
  }))

# define graphene_simd4f_flip_sign_0101(v) \
  (__extension__ ({ \
    const graphene_simd4f_t __v = (v); \
    graphene_simd4f_init (__v[0], -__v[1], __v[2], -__v[3]); \
  }))

# define graphene_simd4f_flip_sign_1010(v) \
  (__extension__ ({ \
    const graphene_simd4f_t __v = (v); \
    graphene_simd4f_init (-__v[0], __v[1], -__v[2], __v[3]); \
  }))

# define graphene_simd4f_cmp_eq(a,b) \
  (__extension__ ({ \
    const graphene_simd4i_t __res = (a) == (b); \
    (bool) (__res[0] != 0 && \
            __res[1] != 0 && \
            __res[2] != 0 && \
            __res[3] != 0); \
  }))

# define graphene_simd4f_cmp_neq(a,b) (!graphene_simd4f_cmp_eq (a,b))

# define graphene_simd4f_cmp_lt(a,b) \
  (__extension__ ({ \
    const graphene_simd4i_t __res = (a) < (b); \
    (bool) (__res[0] != 0 && \
            __res[1] != 0 && \
            __res[2] != 0 && \
            __res[3] != 0); \
  }))

# define graphene_simd4f_cmp_le(a,b) \
  (__extension__ ({ \
    const graphene_simd4i_t __res = (a) <= (b); \
    (bool) (__res[0] != 0 && \
            __res[1] != 0 && \
            __res[2] != 0 && \
            __res[3] != 0); \
  }))

# define graphene_simd4f_cmp_ge(a,b) \
  (__extension__ ({ \
    const graphene_simd4i_t __res = (a) >= (b); \
    (bool) (__res[0] != 0 && \
            __res[1] != 0 && \
            __res[2] != 0 && \
            __res[3] != 0); \
  }))

# define graphene_simd4f_cmp_gt(a,b) \
  (__extension__ ({ \
    const graphene_simd4i_t __res = (a) > (b); \
    (bool) (__res[0] != 0 && \
            __res[1] != 0 && \
            __res[2] != 0 && \
            __res[3] != 0); \
  }))

# define graphene_simd4f_neg(s) \
  (__extension__ ({ \
    const graphene_simd4f_t __s = (s); \
    graphene_simd4f_init (-__s[0], -__s[1], -__s[2], -__s[3]); \
  }))

#elif defined(GRAPHENE_USE_ARM_NEON)

/* ARM Neon implementation of SIMD4f */
# warning "The ARM Neon implementation of graphene_simd4f_t is experimental."

typedef union {
  graphene_simd4f_t s;
  float f[4];
} graphene_simd4f_union_t;

# define graphene_simd4f_init(x,y,z,w) \
  (__extension__ ({ \
    const float32_t __v[4] = { (x), (y), (z), (w) }; \
    (graphene_simd4f_t) vld1q_f32 (__v); \
  }))

# define graphene_simd4f_init_zero() \
  (__extension__ ({ \
    (graphene_simd4f_t) vdupq_n_f32 (0.f); \
  }))

# define graphene_simd4f_init_4f(v) \
  (__extension__ ({ \
    const float32_t *__v32 = (const float32_t *) (v); \
    (graphene_simd4f_t) vld1q_f32 (__v32); \
  }))

# define graphene_simd4f_init_3f(v) \
  (__extension__ ({ \
    graphene_simd4f_init (v[0], v[1], v[2], 0.f); \
  }))

# define graphene_simd4f_init_2f(v) \
  (__extension__ ({ \
    const float32_t *__v32 = (const float32_t *) (v); \
    float32x2_t __low = vld1_f32 (__v32); \
    const float32_t __zero = 0; \
    float32x2_t __high = vld1_dup_f32 (&__zero); \
    (graphene_simd4f_t) vcombine_f32 (__low, __high); \
  }))

# define graphene_simd4f_dup_4f(s,v) \
  (__extension__ ({ \
    vst1q_f32 ((float32_t *) (v), (s)); \
  }))

# define graphene_simd4f_dup_3f(s,v) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u = { (s) }; \
    (v)[0] = __u.f[0]; \
    (v)[1] = __u.f[1]; \
    (v)[2] = __u.f[2]; \
  }))

# define graphene_simd4f_dup_2f(s,v) \
  (__extension__ ({ \
    const float32x2_t __low = vget_low_f32 ((s)); \
    vst1_f32 ((float32_t *) (v), __low); \
  }))

# define graphene_simd4f_get(s,i) \
  (__extension__ ({ \
    (float) vgetq_lane_f32 ((s), (i)); \
  })

# define graphene_simd4f_get_x(s)       graphene_simd4f_get (s, 0)
# define graphene_simd4f_get_y(s)       graphene_simd4f_get (s, 1)
# define graphene_simd4f_get_z(s)       graphene_simd4f_get (s, 2)
# define graphene_simd4f_get_w(s)       graphene_simd4f_get (s, 3)

# define graphene_simd4f_splat(v) \
  (__extension__ ({ \
    (graphene_simd4f_t) vdupq_n_f32 ((v)); \
  }))

# define graphene_simd4f_splat_x(s) \
  (__extension__ ({ \
    graphene_simd4f_splat (graphene_simd4f_get_x ((s))); \
  }))

# define graphene_simd4f_splat_y(s) \
  (__extension__ ({ \
    graphene_simd4f_splat (graphene_simd4f_get_y ((s))); \
  }))

# define graphene_simd4f_splat_z(s) \
  (__extension__ ({ \
    graphene_simd4f_splat (graphene_simd4f_get_z ((s))); \
  }))

# define graphene_simd4f_splat_w(s) \
  (__extension__ ({ \
    graphene_simd4f_splat (graphene_simd4f_get_w ((s))); \
  }))

# define graphene_simd4f_reciprocal(s) \
  (__extension__ ({ \
    graphene_simd4f_t __est = vrecpeq_f32 ((s)); \
    __est = vmulq_f32 (vrecpsq_f32 (__est, (s)), __est); \
    (graphene_simd4f_t) vmulq_f32 (vrecpsq_f32 (__est, (s)), __est); \
  }))

# define graphene_simd4f_add(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) vaddq_f32 ((a), (b)); \
  }))

# define graphene_simd4f_sub(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) vsubq_f32 ((a), (b)); \
  }))

# define graphene_simd4f_mul(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) vmulq_f32 ((a), (b)); \
  }))

# define graphene_simd4f_div(a,b) \
  (__extension__ ({ \
    graphene_simd4f_t __rec = graphene_simd4f_reciprocal ((b)); \
    (graphene_simd4f_t) vmulq_f32 ((a), __rec); \
  }))

# define graphene_simd4f_rsqrt(s) \
  (__extension__ ({ \
    graphene_simd4f_t __est_1 = vrecpeq_f32 ((s)); \
    graphene_simd4f_t __est_2 = vmulq_f32 (__est_1, (s)); \
    __est_1 = vmulq_f32 (__est_1, vrsqrtsq_f32 (__est_2, __est_1)); \
    __est_2 = vmulq_f32 (__est_1, (s)); \
    __est_1 = vmulq_f32 (__est_1, vrsqrtsq_f32 (__est_2, __est_1)); \
    __est_2 = vmulq_f32 (__est_1, (s)); \
    (graphene_simd4f_t) vmulq_f32 (__est_1, vrsqrtsq_f32 (__est_2, __est_1)); \
  }))

# define graphene_simd4f_sqrt(s) \
  (__extension__ ({ \
    graphene_simd4f_t __rsq = graphene_simd4f_rsqrt ((s)); \
    graphene_simd4f_t __rrsq = graphene_simd4f_reciprocal (__rsq); \
    uint32x4_t __tmp = vreinterpretq_u32_f32 ((s)); \
    (graphene_simd4f_t) vreinterpretq_f32_u32 (vandq_u32 (vtstq_u32 (__tmp, __tmp), vreinterpretq_u32_f32 (__rrsq))); \
  }))

# define graphene_simd4f_cross3(a,b) \
  (__extension__ ({ \
    const graphene_simd4f_t __a = (a), __b = (b); \
    const graphene_simd4f_t a_yzx = graphene_simd4f_init (graphene_simd4f_get_y (__a), \
                                                          graphene_simd4f_get_z (__a), \
                                                          graphene_simd4f_get_x (__a), \
                                                          graphene_simd4f_get_w (__a)); \
    const graphene_simd4f_t a_zxy = graphene_simd4f_init (graphene_simd4f_get_z (__a), \
                                                          graphene_simd4f_get_x (__a), \
                                                          graphene_simd4f_get_y (__a), \
                                                          graphene_simd4f_get_w (__a)); \
    const graphene_simd4f_t b_yzx = graphene_simd4f_init (graphene_simd4f_get_y (__b), \
                                                          graphene_simd4f_get_z (__b), \
                                                          graphene_simd4f_get_x (__b), \
                                                          graphene_simd4f_get_w (__b)); \
    const graphene_simd4f_t b_zxy = graphene_simd4f_init (graphene_simd4f_get_z (__b), \
                                                          graphene_simd4f_get_x (__b), \
                                                          graphene_simd4f_get_y (__b), \
                                                          graphene_simd4f_get_w (__b)); \
    (graphene_simd4f_t) vmlsq_f32 (vmulq_f32 (a_yzx, b_zxy), a_zxy, b_yzx); \
  }))

# define graphene_simd4f_min(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) vminq_f32 ((a), (b)); \
  }))

# define graphene_simd4f_max(a,b) \
  (__extension__ ({ \
    (graphene_simd4f_t) vmaxq_f32 (a, b); \
  }))

# define graphene_simd4f_shuffle_wxyz(v) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u = { (v) }; \
    graphene_simd4f_init (__u.f[3], __u.f[0], __u.f[1], __u.f[2]); \
  }))

# define graphene_simd4f_shuffle_zwxy(v) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u = { (v) }; \
    graphene_simd4f_init (__u.f[2], __u.f[3], __u.f[0], __u.f[1]); \
  }))

# define graphene_simd4f_shuffle_yzwx(v) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u = { (v) }; \
    graphene_simd4f_init (__u.f[1], __u.f[2], __u.f[3], __u.f[0]); \
  }))

# define graphene_simd4f_zero_w(v) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u = { (v) }; \
    graphene_simd4f_init (__u.f[0], __u.f[1], __u.f[2], 0.f); \
  }))

# define graphene_simd4f_zero_zw(v) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u = { (v) }; \
    graphene_simd4f_init (__u.f[0], __u.f[1], 0.f, 0.f); \
  }))

# define graphene_simd4f_merge_w(s,v) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u = { (s) }; \
    graphene_simd4f_init (__u.f[0], __u.f[1], __u.f[2], (v)); \
  }))

# define graphene_simd4f_merge_high(a,b) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u_a = { (a) }; \
    graphene_simd4f_union_t __u_b = { (b) }; \
    graphene_simd4f_init (__u_a.f[2], __u_a.f[3], __u_b.f[2], __u_b.f[3]); \
  }))

# define graphene_simd4f_merge_low(a,b) \
  (__extension__ ({ \
    graphene_simd4f_union_t __u_a = { (a) }; \
    graphene_simd4f_union_t __u_b = { (b) }; \
    graphene_simd4f_init (__u_a.f[0], __u_a.f[1], __u_b.f[0], __u_b.f[1]); \
  }))

# define graphene_simd4f_flip_sign_0101(s) \
  (__extension__ ({ \
    const unsigned int __upnpn[4] = { \
      0x00000000, \
      0x80000000, \
      0x00000000, \
      0x80000000 \
    }; \
    const uint32x4_t __pnpn = vld1q_u32 (__upnpn); \
    (graphene_simd4f_t) vreinterpretq_f32_u32 (veorq_u32 (vreinterpretq_u32_f32 ((s)), __pnpn)); \
  }))

# define graphene_simd4f_flip_sign_1010(s) \
  (__extension__ ({ \
    const unsigned int __unpnp[4] = { \
      0x80000000, \
      0x00000000, \
      0x80000000, \
      0x00000000 \
    }; \
    const uint32x4_t __npnp = vld1q_u32 (__unpnp); \
    (graphene_simd4f_t) vreinterpretq_f32_u32 (veorq_u32 (vreinterpretq_u32_f32 ((s)), __npnp)); \
  }))

# define _graphene_movemask(a) \
  (__extension__ ({ \
    const int8_t __attribute__ ((aligned (16))) __xr[8] = { -7,-6,-5,-4,-3,-2,-1,0 }; \
    const uint8x8_t __mask_and = vdup_n_u8 (0x80); \
    const int8x8_t __mask_shift = vld1_s8 (__xr); \
    uint8x8_t __lo = vget_low_u8 ((a)); \
    uint8x8_t __hi = vget_high_u8 ((a)); \
    __lo = vand_u8 (__lo, __mask_and); \
    __lo = vshl_u8 (__lo, __mask_shift); \
    __hi = vand_u8 (__hi, __mask_and); \
    __h1 = vshl_u8 (__hi, __mask_shift); \
    __lo = vpadd_u8 (__lo, __lo); \
    __lo = vpadd_u8 (__lo, __lo); \
    __lo = vpadd_u8 (__lo, __lo); \
    __hi = vpadd_u8 (__hi, __hi); \
    __hi = vpadd_u8 (__hi, __hi); \
    __hi = vpadd_u8 (__hi, __hi); \
    return ((__hi[0] << 8) | (__lo[0] & 0xff)); \
  }))

# define graphene_simd4f_cmp_eq(a,b) \
  (__extension__ ({ \
    const graphene_simd4f_union_t __u_a = { (a) }; \
    const graphene_simd4f_union_t __u_b = { (b) }; \
    (bool) (__u_a.f[0] == __u_b.f[0] && \
            __u_a.f[1] == __u_b.f[1] && \
            __u_a.f[2] == __u_b.f[2] && \
            __u_a.f[3] == __u_b.f[3]); \
  }))

# define graphene_simd4f_cmp_neq(a,b) \
  (__extension__ ({ \
    const graphene_simd4f_union_t __u_a = { (a) }; \
    const graphene_simd4f_union_t __u_b = { (b) }; \
    (bool) (__u_a.f[0] != __u_b.f[0] && \
            __u_a.f[1] != __u_b.f[1] && \
            __u_a.f[2] != __u_b.f[2] && \
            __u_a.f[3] != __u_b.f[3]); \
  }))

# define graphene_simd4f_cmp_lt(a,b) \
  (__extension__ ({ \
    const uint32x4_t __mask = vcltq_f32 ((a), (b)); \
    (bool) (_graphene_movemask (__mask) != 0); \
  }))

# define graphene_simd4f_cmp_le(a,b) \
  (__extension__ ({ \
    const uint32x4_t __mask = vcleq_f32 ((a), (b)); \
    (bool) (_graphene_movemask (__mask) != 0); \
  }))

# define graphene_simd4f_cmp_ge(a,b) \
  (__extension__ ({ \
    const uint32x4_t __mask = vcgeq_f32 ((a), (b)); \
    (bool) (_graphene_movemask (__mask) != 0); \
  }))

# define graphene_simd4f_cmp_gt(a,b) \
  (__extension__ ({ \
    const uint32x4_t __mask = vcgeq_f32 ((a), (b)); \
    (bool) (_graphene_movemask (__mask) != 0); \
  }))

# define graphene_simd4f_neg(s) \
  (__extension__ ({ \
    const unsigned int __umask[4] = { \
      0x80000000, \
      0x80000000, \
      0x80000000, \
      0x80000000 \
    }; \
    const uint32x4_t __mask = vld1q_u32 (__umask); \
    (graphene_simd4f_t) vreinterpretq_f32_u32 (veorq_u32 (vreinterpretq_u32_f32 ((s)), __mask)); \
  }))

#elif defined(GRAPHENE_USE_SCALAR)

/* Fallback implementation using scalar types */

#define graphene_simd4f_init(x,y,z,w) \
  (graphene_simd4f_init ((x), (y), (z), (w)))
#define graphene_simd4f_init_zero() \
  (graphene_simd4f_init_zero ())
#define graphene_simd4f_init_4f(v) \
  (graphene_simd4f_init_4f ((const float *) (v)))
#define graphene_simd4f_init_3f(v) \
  (graphene_simd4f_init_3f ((const float *) (v)))
#define graphene_simd4f_init_2f(v) \
  (graphene_simd4f_init_2f ((const float *) (v)))
#define graphene_simd4f_dup_4f(s,v) \
  (graphene_simd4f_dup_4f ((s), (float *) (v)))
#define graphene_simd4f_dup_3f(s,v) \
  (graphene_simd4f_dup_3f ((s), (float *) (v)))
#define graphene_simd4f_dup_2f(s,v) \
  (graphene_simd4f_dup_2f ((s), (float *) (v)))
#define graphene_simd4f_get(s,i) \
  (graphene_simd4f_get ((s), (i)))
#define graphene_simd4f_get_x(s) \
  (graphene_simd4f_get_x ((s)))
#define graphene_simd4f_get_y(s) \
  (graphene_simd4f_get_y ((s)))
#define graphene_simd4f_get_z(s) \
  (graphene_simd4f_get_z ((s)))
#define graphene_simd4f_get_w(s) \
  (graphene_simd4f_get_w ((s)))
#define graphene_simd4f_splat(v) \
  (graphene_simd4f_splat ((v)))
#define graphene_simd4f_splat_x(s) \
  (graphene_simd4f_splat_x ((s)))
#define graphene_simd4f_splat_y(s) \
  (graphene_simd4f_splat_y ((s)))
#define graphene_simd4f_splat_z(s) \
  (graphene_simd4f_splat_z ((s)))
#define graphene_simd4f_splat_w(s) \
  (graphene_simd4f_splat_w ((s)))
#define graphene_simd4f_add(a,b) \
  (graphene_simd4f_add ((a), (b)))
#define graphene_simd4f_sub(a,b) \
  (graphene_simd4f_sub ((a), (b)))
#define graphene_simd4f_mul(a,b) \
  (graphene_simd4f_mul ((a), (b)))
#define graphene_simd4f_div(a,b) \
  (graphene_simd4f_div ((a), (b)))
#define graphene_simd4f_sqrt(s) \
  (graphene_simd4f_sqrt ((s)))
#define graphene_simd4f_rsqrt(s) \
  (graphene_simd4f_rsqrt ((s)))
#define graphene_simd4f_reciprocal(s) \
  (graphene_simd4f_reciprocal ((s)))
#define graphene_simd4f_cross3(a,b) \
  (graphene_simd4f_cross3 ((a), (b)))
#define graphene_simd4f_min(a,b) \
  (graphene_simd4f_min ((a), (b)))
#define graphene_simd4f_max(a,b) \
  (graphene_simd4f_max ((a), (b)))
#define graphene_simd4f_shuffle_wxyz(s) \
  (graphene_simd4f_shuffle_wxyz ((s)))
#define graphene_simd4f_shuffle_zwxy(s) \
  (graphene_simd4f_shuffle_zwxy ((s)))
#define graphene_simd4f_shuffle_yzwx(s) \
  (graphene_simd4f_shuffle_yzwx ((s)))
#define graphene_simd4f_flip_sign_0101(s) \
  (graphene_simd4f_flip_sign_0101 ((s)))
#define graphene_simd4f_flip_sign_1010(s) \
  (graphene_simd4f_flip_sign_1010 ((s)))
#define graphene_simd4f_zero_w(v) \
  (graphene_simd4f_zero_w ((v)))
#define graphene_simd4f_zero_zw(v) \
  (graphene_simd4f_zero_zw ((v)))
#define graphene_simd4f_merge_w(s,v) \
  (graphene_simd4f_merge_w ((s), (v)))
#define graphene_simd4f_merge_high(a,b) \
  (graphene_simd4f_merge_high ((a), (b)))
#define graphene_simd4f_merge_low(a,b) \
  (graphene_simd4f_merge_low ((a), (b)))
#define graphene_simd4f_cmp_eq(a,b) \
  (graphene_simd4f_cmp_eq ((a), (b)))
#define graphene_simd4f_cmp_neq(a,b) \
  (graphene_simd4f_cmp_neq ((a), (b)))
#define graphene_simd4f_cmp_lt(a,b) \
  (graphene_simd4f_cmp_lt ((a), (b)))
#define graphene_simd4f_cmp_le(a,b) \
  (graphene_simd4f_cmp_le ((a), (b)))
#define graphene_simd4f_cmp_ge(a,b) \
  (graphene_simd4f_cmp_ge ((a), (b)))
#define graphene_simd4f_cmp_gt(a,b) \
  (graphene_simd4f_cmp_gt ((a), (b)))
#define graphene_simd4f_neg(s) \
  (graphene_simd4f_neg ((s)))

#else
# error "Unsupported simd4f implementation."
#endif

/* Generic operations, inlined */

/**
 * graphene_simd4f_madd:
 * @m1: a #graphene_simd4f_t
 * @m2: a #graphene_simd4f_t
 * @a: a #graphene_simd4f_t
 *
 * Adds @a to the product of @m1 and @m2.
 *
 * Returns: the result vector
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_madd (const graphene_simd4f_t m1,
                      const graphene_simd4f_t m2,
                      const graphene_simd4f_t a)
{
  return graphene_simd4f_add (graphene_simd4f_mul (m1, m2), a);
}

/**
 * graphene_simd4f_sum:
 * @v: a #graphene_simd4f_t
 *
 * Sums all components of the given vector.
 *
 * Returns: a vector with all components set to be the
 *   sum of the passed #graphene_simd4f_t
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_sum (const graphene_simd4f_t v)
{
  const graphene_simd4f_t s0 = graphene_simd4f_splat_x (v);
  const graphene_simd4f_t s1 = graphene_simd4f_add (s0, graphene_simd4f_splat_y (v));
  const graphene_simd4f_t s2 = graphene_simd4f_add (s1, graphene_simd4f_splat_z (v));
  const graphene_simd4f_t s3 = graphene_simd4f_add (s2, graphene_simd4f_splat_w (v));
  return s3;
}

/**
 * graphene_simd4f_sum_scalar:
 * @v: a #graphene_simd4f_t
 *
 * Sums all the components of the given vector.
 *
 * Returns: a scalar value with the sum of the components
 *   of the given #graphene_simd4f_t
 *
 * Since: 1.0
 */
static inline float
graphene_simd4f_sum_scalar (const graphene_simd4f_t v)
{
  return graphene_simd4f_get_x (graphene_simd4f_sum (v));
}

/**
 * graphene_simd4f_dot4:
 * @a: a #graphene_simd4f_t
 * @b: a #graphene_simd4f_t
 *
 * Computes the dot product of all the components of the two
 * given #graphene_simd4f_t.
 *
 * Returns: a vector whose components are all set to be the
 *   dot product of the components of the two operands
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_dot4 (const graphene_simd4f_t a,
                      const graphene_simd4f_t b)
{
  return graphene_simd4f_sum (graphene_simd4f_mul (a, b));
}

/**
 * graphene_simd4f_dot3:
 * @a: a #graphene_simd4f_t
 * @b: a #graphene_simd4f_t
 *
 * Computes the dot product of the first three components of the
 * two given #graphene_simd4f_t.
 *
 * Returns: a vector whose components are all set to the the
 *   dot product of the componets of the two operands
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_dot3 (graphene_simd4f_t a,
                      graphene_simd4f_t b)
{
  const graphene_simd4f_t m = graphene_simd4f_mul (a, b);
  const graphene_simd4f_t x = graphene_simd4f_splat_x (m);
  const graphene_simd4f_t y = graphene_simd4f_splat_y (m);
  const graphene_simd4f_t z = graphene_simd4f_splat_z (m);

  return graphene_simd4f_add (graphene_simd4f_add (x, y), z);
}

/**
 * graphene_simd4f_dot2:
 * @a: a #graphene_simd4f_t
 * @b: a #graphene_simd4f_t
 *
 * Computes the dot product of the first two components of the
 * two given #graphene_simd4f_t.
 *
 * Returns: a vector whose components are all set to the the
 *   dot product of the componets of the two operands
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_dot2 (const graphene_simd4f_t a,
                      const graphene_simd4f_t b)
{
  const graphene_simd4f_t m = graphene_simd4f_mul (a, b);
  const graphene_simd4f_t x = graphene_simd4f_splat_x (m);
  const graphene_simd4f_t y = graphene_simd4f_splat_y (m);

  return graphene_simd4f_add (x, y);
}

/**
 * graphene_simd4f_length4:
 * @v: a #graphene_simd4f_t
 *
 * Computes the length of the given #graphene_simd4f_t vector,
 * using all four of its components.
 *
 * Returns: the length vector
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_length4 (const graphene_simd4f_t v)
{
  return graphene_simd4f_sqrt (graphene_simd4f_dot4 (v, v));
}

/**
 * graphene_simd4f_length3:
 * @v: a #graphene_simd4f_t
 *
 * Computes the length of the given #graphene_simd4f_t vector,
 * using the first three of its components.
 *
 * Returns: the length vector
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_length3 (const graphene_simd4f_t v)
{
  return graphene_simd4f_sqrt (graphene_simd4f_dot3 (v, v));
}

/**
 * graphene_simd4f_length2:
 * @v: a #graphene_simd4f_t
 *
 * Computes the length of the given #graphene_simd4f_t vector,
 * using the first two of its components.
 *
 * Returns: the length vector
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_length2 (const graphene_simd4f_t v)
{
  return graphene_simd4f_sqrt (graphene_simd4f_dot2 (v, v));
}

/**
 * graphene_simd4f_normalize4:
 * @v: a #graphene_simd4f_t
 *
 * Computes the normalization of the given #graphene_simd4f_t vector,
 * using all of its components.
 *
 * Returns: the normalized vector
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_normalize4 (const graphene_simd4f_t v)
{
  graphene_simd4f_t invlen = graphene_simd4f_rsqrt (graphene_simd4f_dot4 (v, v));
  return graphene_simd4f_mul (v, invlen);
}

/**
 * graphene_simd4f_normalize3:
 * @v: a #graphene_simd4f_t
 *
 * Computes the normalization of the given #graphene_simd4f_t vector,
 * using the first three of its components.
 *
 * Returns: the normalized vector
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_normalize3 (const graphene_simd4f_t v)
{
  graphene_simd4f_t invlen = graphene_simd4f_rsqrt (graphene_simd4f_dot3 (v, v));
  return graphene_simd4f_mul (v, invlen);
}

/**
 * graphene_simd4f_normalize2:
 * @v: a #graphene_simd4f_t
 *
 * Computes the normalization of the given #graphene_simd4f_t vector,
 * using the first two of its components.
 *
 * Returns: the normalized vector
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_normalize2 (const graphene_simd4f_t v)
{
  graphene_simd4f_t invlen = graphene_simd4f_rsqrt (graphene_simd4f_dot2 (v, v));
  return graphene_simd4f_mul (v, invlen);
}

/**
 * graphene_simd4f_is_zero4:
 * @v: a #graphene_simd4f_t
 *
 * Checks whether the given #graphene_simd4f_t has all its components
 * set to 0.
 *
 * Returns: `true` if all the vector components are zero
 *
 * Since: 1.0
 */
static inline bool
graphene_simd4f_is_zero4 (const graphene_simd4f_t v)
{
  graphene_simd4f_t zero = graphene_simd4f_init_zero ();
  return graphene_simd4f_cmp_eq (v, zero);
}

/**
 * graphene_simd4f_is_zero3:
 * @v: a #graphene_simd4f_t
 *
 * Checks whether the given #graphene_simd4f_t has the first three of
 * its components set to 0.
 *
 * Returns: `true` if the vector's components are zero
 *
 * Since: 1.0
 */
static inline bool
graphene_simd4f_is_zero3 (const graphene_simd4f_t v)
{
  return graphene_simd4f_get_x (v) == 0.f &&
         graphene_simd4f_get_y (v) == 0.f &&
         graphene_simd4f_get_z (v) == 0.f;
}

/**
 * graphene_simd4f_is_zero2:
 * @v: a #graphene_simd4f_t
 *
 * Checks whether the given #graphene_simd4f_t has the first two of
 * its components set to 0.
 *
 * Returns: `true` if the vector's components are zero
 *
 * Since: 1.0
 */
static inline bool
graphene_simd4f_is_zero2 (const graphene_simd4f_t v)
{
  return graphene_simd4f_get_x (v) == 0.f &&
         graphene_simd4f_get_y (v) == 0.f;
}

/**
 * graphene_simd4f_interpolate:
 * @a: a #graphene_simd4f_t
 * @b: a #graphene_simd4f_t
 * @f: the interpolation factor
 *
 * Linearly interpolates all components of the two given
 * #graphene_simd4f_t vectors using the given factor @f.
 *
 * Returns: the intrerpolated vector
 *
 * Since: 1.0
 */
static inline graphene_simd4f_t
graphene_simd4f_interpolate (const graphene_simd4f_t a,
                             const graphene_simd4f_t b,
                             float                   f)
{
  return graphene_simd4f_madd (graphene_simd4f_sub (b, a),
                               graphene_simd4f_splat (f),
                               a);
}

/**
 * graphene_simd4f_clamp:
 * @v: a #graphene_simd4f_t
 * @min: the lower boundary
 * @max: the upper boundary
 *
 * Ensures that all components of the vector @v are within
 * the components of the @lower and @upper boundaries.
 *
 * Returns: the clamped vector
 *
 * Since: 1.2
 */
static inline graphene_simd4f_t
graphene_simd4f_clamp (const graphene_simd4f_t v,
                       const graphene_simd4f_t min,
                       const graphene_simd4f_t max)
{
  const graphene_simd4f_t tmp = graphene_simd4f_max (min, v);

  return graphene_simd4f_min (tmp, max);
}

/**
 * graphene_simd4f_clamp_scalar:
 * @v: a #graphene_simd4f_t
 * @min: the lower boundary
 * @max: the upper boundary
 *
 * Ensures that all components of the vector @v are within
 * the @lower and @upper boundary scalar values.
 *
 * Returns: the clamped vector
 *
 * Since: 1.2
 */
static inline graphene_simd4f_t
graphene_simd4f_clamp_scalar (const graphene_simd4f_t v,
                              float                   min,
                              float                   max)
{
  return graphene_simd4f_clamp (v,
                                graphene_simd4f_splat (min),
                                graphene_simd4f_splat (max));
}

GRAPHENE_END_DECLS

#endif /* __GRAPHENE_SIMD4F_H__ */
