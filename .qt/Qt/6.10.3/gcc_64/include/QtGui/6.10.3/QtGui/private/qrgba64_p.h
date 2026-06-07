// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRGBA64_P_H
#define QRGBA64_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qrgba64.h"
#include "qdrawhelper_p.h"

#include <QtCore/private/qsimd_p.h>
#include <QtGui/private/qtguiglobal_p.h>

QT_BEGIN_NAMESPACE

inline QRgba64 combineAlpha256(QRgba64 rgba64, uint alpha256)
{
    return QRgba64::fromRgba64(rgba64.red(), rgba64.green(), rgba64.blue(), (rgba64.alpha() * alpha256) >> 8);
}

#if defined(__SSE2__)
static inline __m128i Q_DECL_VECTORCALL multiplyAlpha65535(__m128i rgba64, __m128i va)
{
    __m128i vs = rgba64;
    vs = _mm_unpacklo_epi16(_mm_mullo_epi16(vs, va), _mm_mulhi_epu16(vs, va));
    vs = _mm_add_epi32(vs, _mm_srli_epi32(vs, 16));
    vs = _mm_add_epi32(vs, _mm_set1_epi32(0x8000));
    vs = _mm_srai_epi32(vs, 16);
    vs = _mm_packs_epi32(vs, vs);
    return vs;
}
static inline __m128i Q_DECL_VECTORCALL multiplyAlpha65535(__m128i rgba64, uint alpha65535)
{
    const __m128i va = _mm_shufflelo_epi16(_mm_cvtsi32_si128(alpha65535), _MM_SHUFFLE(0, 0, 0, 0));
    return multiplyAlpha65535(rgba64, va);
}
#elif defined(__ARM_NEON__)
static inline uint16x4_t multiplyAlpha65535(uint16x4_t rgba64, uint16x4_t alpha65535)
{
    uint32x4_t vs32 = vmull_u16(rgba64, alpha65535); // vs = vs * alpha
    vs32 = vsraq_n_u32(vs32, vs32, 16); // vs = vs + (vs >> 16)
    return vrshrn_n_u32(vs32, 16); // vs = (vs + 0x8000) >> 16
}
static inline uint16x4_t multiplyAlpha65535(uint16x4_t rgba64, uint alpha65535)
{
    uint32x4_t vs32 = vmull_n_u16(rgba64, alpha65535); // vs = vs * alpha
    vs32 = vsraq_n_u32(vs32, vs32, 16); // vs = vs + (vs >> 16)
    return vrshrn_n_u32(vs32, 16); // vs = (vs + 0x8000) >> 16
}
#elif defined(__loongarch_sx)
static inline __m128i Q_DECL_VECTORCALL multiplyAlpha65535(__m128i rgba64, __m128i va)
{
    __m128i vs = rgba64;
    vs = __lsx_vilvl_h(__lsx_vmuh_hu(vs, va), __lsx_vmul_h(vs, va));
    vs = __lsx_vadd_w(vs, __lsx_vsrli_w(vs, 16));
    vs = __lsx_vadd_w(vs, __lsx_vreplgr2vr_w(0x8000));
    vs = __lsx_vsrai_w(vs, 16);
    vs = __lsx_vpickev_h(__lsx_vsat_w(vs, 15), __lsx_vsat_w(vs, 15));
    return vs;
}
static inline __m128i Q_DECL_VECTORCALL multiplyAlpha65535(__m128i rgba64, uint alpha65535)
{
    const __m128i shuffleMask = (__m128i)(v8i16){0, 0, 0, 0, 4, 5, 6, 7};
    const __m128i va = __lsx_vshuf_h(shuffleMask, __lsx_vldi(0),
                                     __lsx_vinsgr2vr_w(__lsx_vldi(0), alpha65535, 0));
    return multiplyAlpha65535(rgba64, va);
}
#endif

static inline QRgba64 multiplyAlpha65535(QRgba64 rgba64, uint alpha65535)
{
#if defined(__SSE2__)
    const __m128i v = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&rgba64));
    const __m128i vr = multiplyAlpha65535(v, alpha65535);
    QRgba64 r;
    _mm_storel_epi64(reinterpret_cast<__m128i *>(&r), vr);
    return r;
#elif defined(__ARM_NEON__)
    const uint16x4_t v = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&rgba64)));
    const uint16x4_t vr = multiplyAlpha65535(v, alpha65535);
    QRgba64 r;
    vst1_u64(reinterpret_cast<uint64_t *>(&r), vreinterpret_u64_u16(vr));
    return r;
#elif defined(__loongarch_sx)
    const __m128i v = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&rgba64), 0);
    const __m128i vr = multiplyAlpha65535(v, alpha65535);
    QRgba64 r;
    __lsx_vstelm_d(vr, reinterpret_cast<__m128i *>(&r), 0, 0);
    return r;
#else
    return QRgba64::fromRgba64(qt_div_65535(rgba64.red()   * alpha65535),
                               qt_div_65535(rgba64.green() * alpha65535),
                               qt_div_65535(rgba64.blue()  * alpha65535),
                               qt_div_65535(rgba64.alpha() * alpha65535));
#endif
}

#if defined(__SSE2__) || defined(__ARM_NEON__) || defined(__loongarch_sx)
template<typename T>
static inline T Q_DECL_VECTORCALL multiplyAlpha255(T rgba64, uint alpha255)
{
    return multiplyAlpha65535(rgba64, alpha255 * 257);
}
#else
template<typename T>
static inline T multiplyAlpha255(T rgba64, uint alpha255)
{
    return QRgba64::fromRgba64(qt_div_255(rgba64.red()   * alpha255),
                               qt_div_255(rgba64.green() * alpha255),
                               qt_div_255(rgba64.blue()  * alpha255),
                               qt_div_255(rgba64.alpha() * alpha255));
}
#endif

#if defined __SSE2__
static inline __m128i Q_DECL_VECTORCALL interpolate255(__m128i x, uint alpha1, __m128i y, uint alpha2)
{
    return _mm_add_epi16(multiplyAlpha255(x, alpha1), multiplyAlpha255(y, alpha2));
}
#endif

#if defined __ARM_NEON__
inline uint16x4_t interpolate255(uint16x4_t x, uint alpha1, uint16x4_t y, uint alpha2)
{
    return vadd_u16(multiplyAlpha255(x, alpha1), multiplyAlpha255(y, alpha2));
}
#endif

#if defined __loongarch_sx
static inline __m128i Q_DECL_VECTORCALL
interpolate255(__m128i x, uint alpha1, __m128i y, uint alpha2)
{
    return __lsx_vadd_h(multiplyAlpha255(x, alpha1), multiplyAlpha255(y, alpha2));
}
#endif

static inline QRgba64 interpolate255(QRgba64 x, uint alpha1, QRgba64 y, uint alpha2)
{
#if defined(__SSE2__)
    const __m128i vx = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&x));
    const __m128i vy = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&y));
    const __m128i vr = interpolate255(vx, alpha1, vy, alpha2);
    QRgba64 r;
    _mm_storel_epi64(reinterpret_cast<__m128i *>(&r), vr);
    return r;
#elif defined(__ARM_NEON__)
    const uint16x4_t vx = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&x)));
    const uint16x4_t vy = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&y)));
    const uint16x4_t vr = interpolate255(vx, alpha1, vy, alpha2);
    QRgba64 r;
    vst1_u64(reinterpret_cast<uint64_t *>(&r), vreinterpret_u64_u16(vr));
    return r;
#elif defined(__loongarch_sx)
    const __m128i vx = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&x), 0);
    const __m128i vy = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&y), 0);
    const __m128i vr = interpolate255(vx, alpha1, vy, alpha2);
    QRgba64 r;
    __lsx_vstelm_d(vr, reinterpret_cast<__m128i *>(&r), 0, 0);
    return r;
#else
    return QRgba64::fromRgba64(multiplyAlpha255(x, alpha1) + multiplyAlpha255(y, alpha2));
#endif
}

#if defined __SSE2__
static inline __m128i Q_DECL_VECTORCALL interpolate65535(__m128i x, uint alpha1, __m128i y, uint alpha2)
{
    return _mm_add_epi16(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}

static inline __m128i Q_DECL_VECTORCALL interpolate65535(__m128i x, __m128i alpha1, __m128i y, __m128i alpha2)
{
    return _mm_add_epi16(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}
#endif

#if defined __ARM_NEON__
inline uint16x4_t interpolate65535(uint16x4_t x, uint alpha1, uint16x4_t y, uint alpha2)
{
    return vadd_u16(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}
inline uint16x4_t interpolate65535(uint16x4_t x, uint16x4_t alpha1, uint16x4_t y, uint16x4_t alpha2)
{
    return vadd_u16(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}
#endif

#if defined __loongarch_sx
static inline __m128i Q_DECL_VECTORCALL interpolate65535(__m128i x, uint alpha1, __m128i y, uint alpha2)
{
    return __lsx_vadd_h(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}

static inline __m128i Q_DECL_VECTORCALL interpolate65535(__m128i x, __m128i alpha1, __m128i y, __m128i alpha2)
{
    return __lsx_vadd_h(multiplyAlpha65535(x, alpha1), multiplyAlpha65535(y, alpha2));
}
#endif

static inline QRgba64 interpolate65535(QRgba64 x, uint alpha1, QRgba64 y, uint alpha2)
{
#if defined(__SSE2__)
    const __m128i vx = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&x));
    const __m128i vy = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&y));
    const __m128i vr = interpolate65535(vx, alpha1, vy, alpha2);
    QRgba64 r;
    _mm_storel_epi64(reinterpret_cast<__m128i *>(&r), vr);
    return r;
#elif defined(__ARM_NEON__)
    const uint16x4_t vx = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&x)));
    const uint16x4_t vy = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&y)));
    const uint16x4_t vr = interpolate65535(vx, alpha1, vy, alpha2);
    QRgba64 r;
    vst1_u64(reinterpret_cast<uint64_t *>(&r), vreinterpret_u64_u16(vr));
    return r;
#elif defined(__loongarch_sx)
    const __m128i vx = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&x), 0);
    const __m128i vy = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&y), 0);
    const __m128i vr = interpolate65535(vx, alpha1, vy, alpha2);
    QRgba64 r;
    __lsx_vstelm_d(vr, reinterpret_cast<__m128i *>(&r), 0, 0);
    return r;
#else
    return QRgba64::fromRgba64(multiplyAlpha65535(x, alpha1) + multiplyAlpha65535(y, alpha2));
#endif
}

static inline QRgba64 addWithSaturation(QRgba64 a, QRgba64 b)
{
#if defined(__SSE2__)
    const __m128i va = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&a));
    const __m128i vb = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&b));
    const __m128i vr = _mm_adds_epu16(va, vb);
    QRgba64 r;
    _mm_storel_epi64(reinterpret_cast<__m128i *>(&r), vr);
    return r;
#elif defined(__ARM_NEON__)
    const uint16x4_t va = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&a)));
    const uint16x4_t vb = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&b)));
    QRgba64 r;
    vst1_u64(reinterpret_cast<uint64_t *>(&r), vreinterpret_u64_u16(vqadd_u16(va, vb)));
    return r;
#elif defined(__loongarch_sx)
    const __m128i va = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&a), 0);
    const __m128i vb = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&b), 0);
    const __m128i vr = __lsx_vsadd_hu(va, vb);
    QRgba64 r;
    __lsx_vstelm_d(vr, reinterpret_cast<__m128i *>(&r), 0, 0);
    return r;
#else

    return QRgba64::fromRgba64(qMin(a.red() + b.red(), 65535),
                               qMin(a.green() + b.green(), 65535),
                               qMin(a.blue() + b.blue(), 65535),
                               qMin(a.alpha() + b.alpha(), 65535));
#endif
}

#if QT_COMPILER_SUPPORTS_HERE(SSE2)
QT_FUNCTION_TARGET(SSE2)
static inline uint Q_DECL_VECTORCALL toArgb32(__m128i v)
{
    v = _mm_unpacklo_epi16(v, _mm_setzero_si128());
    v = _mm_add_epi32(v, _mm_set1_epi32(128));
    v = _mm_sub_epi32(v, _mm_srli_epi32(v, 8));
    v = _mm_srli_epi32(v, 8);
    v = _mm_packs_epi32(v, v);
    v = _mm_packus_epi16(v, v);
    return _mm_cvtsi128_si32(v);
}
#elif defined __ARM_NEON__
static inline uint toArgb32(uint16x4_t v)
{
    v = vsub_u16(v, vrshr_n_u16(v, 8));
    v = vrshr_n_u16(v, 8);
    uint8x8_t v8 = vmovn_u16(vcombine_u16(v, v));
    return vget_lane_u32(vreinterpret_u32_u8(v8), 0);
}
#elif defined __loongarch_sx
static inline uint Q_DECL_VECTORCALL toArgb32(__m128i v)
{
    v = __lsx_vilvl_h(__lsx_vldi(0), v);
    v = __lsx_vadd_w(v, __lsx_vreplgr2vr_w(128));
    v = __lsx_vsub_w(v, __lsx_vsrli_w(v, 8));
    v = __lsx_vsrli_w(v, 8);
    v = __lsx_vpickev_h(__lsx_vsat_w(v, 15), __lsx_vsat_w(v, 15));
    __m128i tmp = __lsx_vmaxi_h(v, 0);
    v = __lsx_vpickev_b(__lsx_vsat_hu(tmp, 7), __lsx_vsat_hu(tmp, 7));
    return __lsx_vpickve2gr_w(v, 0);
}
#endif

static inline uint toArgb32(QRgba64 rgba64)
{
#if defined __SSE2__
    __m128i v = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&rgba64));
    v = _mm_shufflelo_epi16(v, _MM_SHUFFLE(3, 0, 1, 2));
    return toArgb32(v);
#elif defined __ARM_NEON__
    uint16x4_t v = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&rgba64)));
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    const uint8x8_t shuffleMask = qvset_n_u8(4, 5, 2, 3, 0, 1, 6, 7);
    v = vreinterpret_u16_u8(vtbl1_u8(vreinterpret_u8_u16(v), shuffleMask));
#else
    v = vext_u16(v, v, 3);
#endif
    return toArgb32(v);
#elif defined __loongarch_sx
    __m128i v = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&rgba64), 0);
    const __m128i shuffleMask = (__m128i)(v8i16){2, 1, 0, 3, 4, 5, 6, 7};
    v = __lsx_vshuf_h(shuffleMask, __lsx_vldi(0), v);
    return toArgb32(v);
#else
    return rgba64.toArgb32();
#endif
}

static inline uint toRgba8888(QRgba64 rgba64)
{
#if defined __SSE2__
    __m128i v = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&rgba64));
    return toArgb32(v);
#elif defined __ARM_NEON__
    uint16x4_t v = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&rgba64)));
    return toArgb32(v);
#elif defined __loongarch_sx
    __m128i v = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&rgba64), 0);
    return toArgb32(v);
#else
    return ARGB2RGBA(toArgb32(rgba64));
#endif
}

static inline QRgba64 rgbBlend(QRgba64 d, QRgba64 s, uint rgbAlpha)
{
    QRgba64 blend;
#if defined(__SSE2__)
    __m128i vd = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&d));
    __m128i vs = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&s));
    __m128i va =  _mm_cvtsi32_si128(rgbAlpha);
    va = _mm_unpacklo_epi8(va, va);
    va = _mm_shufflelo_epi16(va, _MM_SHUFFLE(3, 0, 1, 2));
    __m128i vb = _mm_xor_si128(_mm_set1_epi16(-1), va);

    vs = _mm_unpacklo_epi16(_mm_mullo_epi16(vs, va), _mm_mulhi_epu16(vs, va));
    vd = _mm_unpacklo_epi16(_mm_mullo_epi16(vd, vb), _mm_mulhi_epu16(vd, vb));
    vd = _mm_add_epi32(vd, vs);
    vd = _mm_add_epi32(vd, _mm_srli_epi32(vd, 16));
    vd = _mm_add_epi32(vd, _mm_set1_epi32(0x8000));
    vd = _mm_srai_epi32(vd, 16);
    vd = _mm_packs_epi32(vd, vd);

    _mm_storel_epi64(reinterpret_cast<__m128i *>(&blend), vd);
#elif defined(__ARM_NEON__)
    uint16x4_t vd = vreinterpret_u16_u64(vmov_n_u64(d));
    uint16x4_t vs = vreinterpret_u16_u64(vmov_n_u64(s));
    uint8x8_t va8 = vreinterpret_u8_u32(vmov_n_u32(ARGB2RGBA(rgbAlpha)));
    uint16x4_t va = vreinterpret_u16_u8(vzip_u8(va8, va8).val[0]);
    uint16x4_t vb = veor_u16(vdup_n_u16(0xffff), va);

    uint32x4_t vs32 = vmull_u16(vs, va);
    uint32x4_t vd32 = vmull_u16(vd, vb);
    vd32 = vaddq_u32(vd32, vs32);
    vd32 = vsraq_n_u32(vd32, vd32, 16);
    vd = vrshrn_n_u32(vd32, 16);
    vst1_u64(reinterpret_cast<uint64_t *>(&blend), vreinterpret_u64_u16(vd));
#elif defined(__loongarch_sx)
    __m128i vd = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&d), 0);
    __m128i vs = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&s), 0);
    __m128i va = __lsx_vinsgr2vr_w(__lsx_vldi(0), rgbAlpha, 0);
    va = __lsx_vilvl_b(va, va);
    const __m128i shuffleMask = (__m128i)(v8i16){2, 1, 0, 3, 4, 5, 6, 7};
    va = __lsx_vshuf_h(shuffleMask, __lsx_vldi(0), va);
    __m128i vb = __lsx_vxor_v(__lsx_vreplgr2vr_h(-1), va);

    vs = __lsx_vilvl_h(__lsx_vmuh_hu(vs, va), __lsx_vmul_h(vs, va));
    vd = __lsx_vilvl_h(__lsx_vmuh_hu(vd, vb), __lsx_vmul_h(vd, vb));
    vd = __lsx_vadd_w(vd, vs);
    vd = __lsx_vadd_w(vd, __lsx_vsrli_w(vd, 16));
    vd = __lsx_vadd_w(vd, __lsx_vreplgr2vr_w(0x8000));
    vd = __lsx_vsrai_w(vd, 16);
    vd = __lsx_vpickev_h(__lsx_vsat_w(vd, 15), __lsx_vsat_w(vd, 15));
    __lsx_vstelm_d(vd, reinterpret_cast<__m128i *>(&blend), 0, 0);
#else
    const int mr = qRed(rgbAlpha);
    const int mg = qGreen(rgbAlpha);
    const int mb = qBlue(rgbAlpha);
    blend = qRgba64(qt_div_255(s.red()   * mr + d.red()   * (255 - mr)),
                    qt_div_255(s.green() * mg + d.green() * (255 - mg)),
                    qt_div_255(s.blue()  * mb + d.blue()  * (255 - mb)),
                    s.alpha());
#endif
    return blend;
}

static inline void blend_pixel(QRgba64 &dst, QRgba64 src)
{
    if (src.isOpaque())
        dst = src;
    else if (!src.isTransparent()) {
#if defined(__SSE2__)
        const __m128i vd = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&dst));
        const __m128i vs = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&src));
        const __m128i via = _mm_xor_si128(_mm_set1_epi16(-1), _mm_shufflelo_epi16(vs, _MM_SHUFFLE(3, 3, 3, 3)));
        const __m128i vr = _mm_add_epi16(vs, multiplyAlpha65535(vd, via));
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&dst), vr);
#elif defined(__ARM_NEON__)
        const uint16x4_t vd = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&dst)));
        const uint16x4_t vs = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&src)));
        const uint16x4_t via = veor_u16(vdup_n_u16(0xffff), vdup_lane_u16(vs, 3));
        const uint16x4_t vr = vadd_u16(vs, multiplyAlpha65535(vd, via));
        vst1_u64(reinterpret_cast<uint64_t *>(&dst), vreinterpret_u64_u16(vr));
#elif defined(__loongarch_sx)
        const __m128i vd = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&dst), 0);
        const __m128i vs = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&src), 0);
        const __m128i shuffleMask = (__m128i)(v8i16){3, 3, 3, 3, 4, 5, 6, 7};
        const __m128i via = __lsx_vxor_v(__lsx_vreplgr2vr_h(-1), __lsx_vshuf_h(shuffleMask, __lsx_vldi(0), vs));
        const __m128i vr = __lsx_vadd_h(vs, multiplyAlpha65535(vd, via));
        __lsx_vstelm_d(vr, reinterpret_cast<__m128i *>(&dst), 0, 0);
#else
        dst = src + multiplyAlpha65535(dst, 65535 - src.alpha());
#endif
    }
}

static inline void blend_pixel(QRgba64 &dst, QRgba64 src, const int const_alpha)
{
    if (const_alpha == 255)
        return blend_pixel(dst, src);
    if (!src.isTransparent()) {
#if defined(__SSE2__)
        const __m128i vd = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&dst));
        __m128i vs = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&src));
        vs = multiplyAlpha255(vs, const_alpha);
        const __m128i via = _mm_xor_si128(_mm_set1_epi16(-1), _mm_shufflelo_epi16(vs, _MM_SHUFFLE(3, 3, 3, 3)));
        const __m128i vr = _mm_add_epi16(vs, multiplyAlpha65535(vd, via));
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&dst), vr);
#elif defined(__ARM_NEON__)
        const uint16x4_t vd = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&dst)));
        uint16x4_t vs = vreinterpret_u16_u64(vld1_u64(reinterpret_cast<const uint64_t *>(&src)));
        vs = multiplyAlpha255(vs, const_alpha);
        const uint16x4_t via = veor_u16(vdup_n_u16(0xffff), vdup_lane_u16(vs, 3));
        const uint16x4_t vr = vadd_u16(vs, multiplyAlpha65535(vd, via));
        vst1_u64(reinterpret_cast<uint64_t *>(&dst), vreinterpret_u64_u16(vr));
#elif defined(__loongarch_sx)
        const __m128i vd = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&dst), 0);
        __m128i vs = __lsx_vldrepl_d(reinterpret_cast<const __m128i *>(&src), 0);
        vs = multiplyAlpha255(vs, const_alpha);
        const __m128i shuffleMask = (__m128i)(v8i16){3, 3, 3, 3, 4, 5, 6, 7};
        const __m128i via = __lsx_vxor_v(__lsx_vreplgr2vr_h(-1), __lsx_vshuf_h(shuffleMask, __lsx_vldi(0), vs));
        const __m128i vr = __lsx_vadd_h(vs, multiplyAlpha65535(vd, via));
        __lsx_vstelm_d(vr, reinterpret_cast<__m128i *>(&dst), 0, 0);
#else
        src = multiplyAlpha255(src, const_alpha);
        dst = src + multiplyAlpha65535(dst, 65535 - src.alpha());
#endif
    }
}

QT_END_NAMESPACE

#endif // QRGBA64_P_H
