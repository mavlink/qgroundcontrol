/* graphene-config.h
 *
 * This is a generated file. Please modify 'configure.ac'.
 */

#ifndef __GRAPHENE_CONFIG_H__
#define __GRAPHENE_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GRAPHENE_SIMD_BENCHMARK
# define GRAPHENE_HAS_SCALAR 1
#endif /* GRAPHENE_SIMD_BENCHMARK */

#if defined(GRAPHENE_HAS_SSE)
# define GRAPHENE_USE_SSE
# define GRAPHENE_SIMD_S "sse"
#elif defined(GRAPHENE_HAS_ARM_NEON)
# define GRAPHENE_USE_ARM_NEON
# define GRAPHENE_SIMD_S "neon"
#elif defined(GRAPHENE_HAS_GCC)
# define GRAPHENE_USE_GCC
# define GRAPHENE_SIMD_S "gcc"
#elif defined(GRAPHENE_HAS_SCALAR)
# define GRAPHENE_USE_SCALAR
# define GRAPHENE_SIMD_S "scalar"
#else
# error "Unsupported platform."
#endif

#if defined(GRAPHENE_USE_SSE)
#include <xmmintrin.h>
typedef __m128 graphene_simd4f_t;
#elif defined(GRAPHENE_USE_ARM_NEON)
#include <arm_neon.h>
typedef float32x4_t graphene_simd4f_t;
#elif defined(GRAPHENE_USE_GCC)
typedef float graphene_simd4f_t __attribute__((vector_size(16)));
#elif defined(GRAPHENE_USE_SCALAR)
typedef struct {
  /*< private >*/
  float x, y, z, w;
} graphene_simd4f_t;
#else
# error "Unsupported platform."
#endif

typedef struct {
  /*< private >*/
  graphene_simd4f_t x, y, z, w;
} graphene_simd4x4f_t;

#ifdef __cplusplus
}
#endif

#endif /* __GRAPHENE_CONFIG_H__ */
