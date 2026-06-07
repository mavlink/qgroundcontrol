// Copyright (C) 2024 Loongson Technology Corporation Limited.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDRAWINGPRIMITIVE_LSX_P_H
#define QDRAWINGPRIMITIVE_LSX_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include <private/qsimd_p.h>
#include "qdrawhelper_loongarch64_p.h"
#include "qrgba64_p.h"

#ifdef __loongarch_sx

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

QT_BEGIN_NAMESPACE

/*
 * Multiply the components of pixelVector by alphaChannel
 * Each 32bits components of alphaChannel must be in the form 0x00AA00AA
 * colorMask must have 0x00ff00ff on each 32 bits component
 * half must have the value 128 (0x80) for each 32 bits component
 */
inline static void Q_DECL_VECTORCALL
BYTE_MUL_LSX(__m128i &pixelVector, __m128i alphaChannel, __m128i colorMask, __m128i half)
{
    /* 1. separate the colors in 2 vectors so each color is on 16 bits
       (in order to be multiplied by the alpha
       each 32 bit of dstVectorAG are in the form 0x00AA00GG
       each 32 bit of dstVectorRB are in the form 0x00RR00BB */
    __m128i pixelVectorAG = __lsx_vsrli_h(pixelVector, 8);
    __m128i pixelVectorRB = __lsx_vand_v(pixelVector, colorMask);

    /* 2. multiply the vectors by the alpha channel */
    pixelVectorAG = __lsx_vmul_h(pixelVectorAG, alphaChannel);
    pixelVectorRB = __lsx_vmul_h(pixelVectorRB, alphaChannel);

    /* 3. divide by 255, that's the tricky part.
       we do it like for BYTE_MUL(), with bit shift: X/255 ~= (X + X/256 + rounding)/256 */
    /** so first (X + X/256 + rounding) */
    pixelVectorRB = __lsx_vadd_h(pixelVectorRB, __lsx_vsrli_h(pixelVectorRB, 8));
    pixelVectorRB = __lsx_vadd_h(pixelVectorRB, half);
    pixelVectorAG = __lsx_vadd_h(pixelVectorAG, __lsx_vsrli_h(pixelVectorAG, 8));
    pixelVectorAG = __lsx_vadd_h(pixelVectorAG, half);

    /** second divide by 256 */
    pixelVectorRB = __lsx_vsrli_h(pixelVectorRB, 8);
    /** for AG, we could >> 8 to divide followed by << 8 to put the
        bytes in the correct position. By masking instead, we execute
        only one instruction */
    pixelVectorAG = __lsx_vandn_v(colorMask, pixelVectorAG);

    /* 4. combine the 2 pairs of colors */
    pixelVector = __lsx_vor_v(pixelVectorAG, pixelVectorRB);
}

/*
 * Each 32bits components of alphaChannel must be in the form 0x00AA00AA
 * oneMinusAlphaChannel must be 255 - alpha for each 32 bits component
 * colorMask must have 0x00ff00ff on each 32 bits component
 * half must have the value 128 (0x80) for each 32 bits component
 */
inline static void Q_DECL_VECTORCALL
INTERPOLATE_PIXEL_255_LSX(__m128i srcVector, __m128i &dstVector, __m128i alphaChannel,
                          __m128i oneMinusAlphaChannel, __m128i colorMask, __m128i half)
{
    /* interpolate AG */
    __m128i srcVectorAG = __lsx_vsrli_h(srcVector, 8);
    __m128i dstVectorAG = __lsx_vsrli_h(dstVector, 8);
    __m128i srcVectorAGalpha = __lsx_vmul_h(srcVectorAG, alphaChannel);
    __m128i dstVectorAGoneMinusAlphalpha = __lsx_vmul_h(dstVectorAG, oneMinusAlphaChannel);
    __m128i finalAG = __lsx_vadd_h(srcVectorAGalpha, dstVectorAGoneMinusAlphalpha);
    finalAG = __lsx_vadd_h(finalAG, __lsx_vsrli_h(finalAG, 8));
    finalAG = __lsx_vadd_h(finalAG, half);
    finalAG = __lsx_vandn_v(colorMask, finalAG);

    /* interpolate RB */
    __m128i srcVectorRB = __lsx_vand_v(srcVector, colorMask);
    __m128i dstVectorRB = __lsx_vand_v(dstVector, colorMask);
    __m128i srcVectorRBalpha = __lsx_vmul_h(srcVectorRB, alphaChannel);
    __m128i dstVectorRBoneMinusAlphalpha = __lsx_vmul_h(dstVectorRB, oneMinusAlphaChannel);
    __m128i finalRB = __lsx_vadd_h(srcVectorRBalpha, dstVectorRBoneMinusAlphalpha);
    finalRB = __lsx_vadd_h(finalRB, __lsx_vsrli_h(finalRB, 8));
    finalRB = __lsx_vadd_h(finalRB, half);
    finalRB = __lsx_vsrli_h(finalRB, 8);

    /* combine */
    dstVector = __lsx_vor_v(finalAG, finalRB);
}

// same as BLEND_SOURCE_OVER_ARGB32_LSX, but for one vector srcVector
inline static void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_LSX_helper(quint32 *dst, int x, __m128i srcVector,
                                    __m128i nullVector, __m128i half, __m128i one,
                                    __m128i colorMask, __m128i alphaMask)
{
    const __m128i srcVectorAlpha = __lsx_vand_v(srcVector, alphaMask);
    __m128i vseq = __lsx_vseq_w(srcVectorAlpha, alphaMask);
    v4i32 vseq_res = (v4i32)__lsx_vmsknz_b(vseq);
    if (vseq_res[0] == (0x0000ffff)) {
        /* all opaque */
        __lsx_vst(srcVector, &dst[x], 0);
    } else {
        __m128i vseq_n = __lsx_vseq_w(srcVectorAlpha, nullVector);
        v4i32 vseq_n_res = (v4i32)__lsx_vmsknz_b(vseq_n);
        if (vseq_n_res[0] != (0x0000ffff)) {
            /* not fully transparent */
            /* extract the alpha channel on 2 x 16 bits */
            /* so we have room for the multiplication */
            /* each 32 bits will be in the form 0x00AA00AA */
            /* with A being the 1 - alpha */
            __m128i alphaChannel = __lsx_vsrli_w(srcVector, 24);
            alphaChannel = __lsx_vor_v(alphaChannel, __lsx_vslli_w(alphaChannel, 16));
            alphaChannel = __lsx_vsub_h(one, alphaChannel);

            __m128i dstVector = __lsx_vld(&dst[x], 0);
            BYTE_MUL_LSX(dstVector, alphaChannel, colorMask, half);

            /* result = s + d * (1-alpha) */
            const __m128i result = __lsx_vadd_b(srcVector, dstVector);
            __lsx_vst(result, &dst[x], 0);
        }
    }
}

// Basically blend src over dst with the const alpha defined as constAlphaVector.
// nullVector, half, one, colorMask are constant across the whole image/texture, and should be defined as:
//const __m128i nullVector = __lsx_vreplgr2vr_w(0);
//const __m128i half = __lsx_vreplgr2vr_h(0x80);
//const __m128i one = __lsx_vreplgr2vr_h(0xff);
//const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
//const __m128i alphaMask = __lsx_vreplgr2vr_w(0xff000000);
//
// The computation being done is:
// result = s + d * (1-alpha)
// with shortcuts if fully opaque or fully transparent.
inline static void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_LSX(quint32 *dst, const quint32 *src, int length)
{
    int x = 0;

    /* First, get dst aligned. */
    ALIGNMENT_PROLOGUE_16BYTES(dst, x, length) {
        blend_pixel(dst[x], src[x]);
    }

    const __m128i alphaMask = __lsx_vreplgr2vr_w(0xff000000);
    const __m128i nullVector = __lsx_vreplgr2vr_w(0);
    const __m128i half = __lsx_vreplgr2vr_h(0x80);
    const __m128i one = __lsx_vreplgr2vr_h(0xff);
    const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);

    for (; x < length-3; x += 4) {
        const __m128i srcVector = __lsx_vld((const __m128i *)&src[x], 0);
        BLEND_SOURCE_OVER_ARGB32_LSX_helper(dst, x, srcVector, nullVector, half, one, colorMask, alphaMask);
    }
    SIMD_EPILOGUE(x, length, 3) {
        blend_pixel(dst[x], src[x]);
    }
}

// Basically blend src over dst with the const alpha defined as constAlphaVector.
// The computation being done is:
// dest = (s + d * sia) * ca + d * cia
//      = s * ca + d * (sia * ca + cia)
//      = s * ca + d * (1 - sa*ca)
inline static void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_LSX(quint32 *dst, const quint32 *src, int length, uint const_alpha)
{
    int x = 0;

    ALIGNMENT_PROLOGUE_16BYTES(dst, x, length) {
        blend_pixel(dst[x], src[x], const_alpha);
    }

    const __m128i nullVector = __lsx_vreplgr2vr_w(0);
    const __m128i half = __lsx_vreplgr2vr_h(0x80);
    const __m128i one = __lsx_vreplgr2vr_h(0xff);
    const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
    const __m128i constAlphaVector = __lsx_vreplgr2vr_h(const_alpha);

    for (; x < length-3; x += 4) {
        __m128i srcVector = __lsx_vld((const __m128i *)&src[x], 0);
        __m128i vseq = __lsx_vseq_w(srcVector, nullVector);
        v4i32 vseq_res = (v4i32)__lsx_vmsknz_b(vseq);
        if (vseq_res[0] != 0x0000ffff) {
            BYTE_MUL_LSX(srcVector, constAlphaVector, colorMask, half);

            __m128i alphaChannel = __lsx_vsrli_w(srcVector, 24);
            alphaChannel = __lsx_vor_v(alphaChannel, __lsx_vslli_w(alphaChannel, 16));
            alphaChannel = __lsx_vsub_h(one, alphaChannel);

            __m128i dstVector = __lsx_vld((__m128i *)&dst[x], 0);
            BYTE_MUL_LSX(dstVector, alphaChannel, colorMask, half);

            const __m128i result = __lsx_vadd_b(srcVector, dstVector);
            __lsx_vst(result, &dst[x], 0);
        }
    }
    SIMD_EPILOGUE(x, length, 3) {
        blend_pixel(dst[x], src[x], const_alpha);
    }
}

typedef union
{
    int i;
    float f;
} FloatInt;

/* float type data load instructions */
static __m128 __lsx_vreplfr2vr_s(float val)
{
    FloatInt fi_tmpval = {.f = val};
    return (__m128)__lsx_vreplgr2vr_w(fi_tmpval.i);
}

Q_ALWAYS_INLINE __m128 Q_DECL_VECTORCALL reciprocal_mul_ps(const __m128 a, float mul)
{
    __m128 ia = __lsx_vfrecip_s(a); // Approximate 1/a
    // Improve precision of ia using Newton-Raphson
    ia = __lsx_vfsub_s(__lsx_vfadd_s(ia, ia), __lsx_vfmul_s(ia, __lsx_vfmul_s(ia, a)));
    ia = __lsx_vfmul_s(ia, __lsx_vreplfr2vr_s(mul));
    return ia;
}

inline QRgb qUnpremultiply_lsx(QRgb p)
{
    const uint alpha = qAlpha(p);
    if (alpha == 255)
        return p;
    if (alpha == 0)
        return 0;
    const __m128 va = __lsx_vffint_s_w(__lsx_vreplgr2vr_w(alpha));
    __m128 via = reciprocal_mul_ps(va, 255.0f); // Approximate 1/a
    const __m128i shuffleMask = (__m128i)(v16i8){0,16,16,16,1,16,16,16,2,16,16,16,3,16,16,16};
    __m128i vl = __lsx_vshuf_b(__lsx_vldi(0), __lsx_vreplgr2vr_w(p), shuffleMask);
    vl = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(vl), via));
    vl = __lsx_vmaxi_w(vl, 0);
    vl = __lsx_vpickev_h(__lsx_vsat_wu(vl, 15), __lsx_vsat_wu(vl, 15));
    vl = __lsx_vinsgr2vr_h(vl, alpha, 3);
    vl = __lsx_vpickev_b(__lsx_vsat_hu(vl, 7), __lsx_vsat_hu(vl, 7));
    return __lsx_vpickve2gr_w(vl, 0);
}

template<enum QtPixelOrder PixelOrder>
inline uint qConvertArgb32ToA2rgb30_lsx(QRgb p)
{
    const uint alpha = qAlpha(p);
    if (alpha == 255)
        return qConvertRgb32ToRgb30<PixelOrder>(p);
    if (alpha == 0)
        return 0;
    Q_CONSTEXPR float mult = 1023.0f / (255 >> 6);
    const uint newalpha = (alpha >> 6);
    const __m128 va = __lsx_vffint_s_w(__lsx_vreplgr2vr_w(alpha));
    __m128 via = reciprocal_mul_ps(va, mult * newalpha);
    const __m128i shuffleMask = (__m128i)(v16i8){0,16,16,16,1,16,16,16,2,16,16,16,3,16,16,16};
    __m128i vl = __lsx_vshuf_b(__lsx_vldi(0), __lsx_vreplgr2vr_w(p), shuffleMask);
    vl = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(vl), via));
    vl = __lsx_vmaxi_w(vl, 0);
    vl = __lsx_vpickev_h(__lsx_vsat_wu(vl, 15), __lsx_vsat_wu(vl, 15));
    uint rgb30 = (newalpha << 30);
    rgb30 |= ((uint)__lsx_vpickve2gr_h(vl, 1)) << 10;
    if (PixelOrder == PixelOrderRGB) {
        rgb30 |= ((uint)__lsx_vpickve2gr_h(vl, 2)) << 20;
        rgb30 |= ((uint)__lsx_vpickve2gr_h(vl, 0));
    } else {
        rgb30 |= ((uint)__lsx_vpickve2gr_h(vl, 0)) << 20;
        rgb30 |= ((uint)__lsx_vpickve2gr_h(vl, 2));
    }
    return rgb30;
}

template<enum QtPixelOrder PixelOrder>
inline uint qConvertRgba64ToRgb32_lsx(QRgba64 p)
{
    if (p.isTransparent())
        return 0;
    __m128i vl = __lsx_vilvl_d(__lsx_vldi(0), __lsx_vldrepl_d(&p, 0));
    if (!p.isOpaque()) {
        const __m128 va = __lsx_vffint_s_w(__lsx_vreplgr2vr_w(p.alpha()));
        __m128 via = reciprocal_mul_ps(va, 65535.0f);
        vl = __lsx_vilvl_h(__lsx_vldi(0), vl);
        vl = __lsx_vftintrne_w_s(__lsx_vfmul_s(__lsx_vffint_s_w(vl) , via));
        vl = __lsx_vmaxi_w(vl, 0);
        vl = __lsx_vpickev_h(__lsx_vsat_wu(vl, 15), __lsx_vsat_wu(vl, 15));
        vl = __lsx_vinsgr2vr_h(vl, p.alpha(), 3);
    }
    if (PixelOrder == PixelOrderBGR){
        const __m128i shuffleMask = (__m128i)(v8i16){2, 1, 0, 3, 4, 5, 6, 7};
        vl = __lsx_vshuf_h(shuffleMask, __lsx_vldi(0), vl);
    }
    vl = __lsx_vilvl_h(__lsx_vldi(0), vl);
    vl = __lsx_vadd_w(vl, __lsx_vreplgr2vr_w(128));
    vl = __lsx_vsub_w(vl, __lsx_vsrli_w(vl, 8));
    vl = __lsx_vsrli_w(vl, 8);
    vl = __lsx_vpickev_h(__lsx_vsat_w(vl, 15), __lsx_vsat_w(vl, 15));
    __m128i tmp = __lsx_vmaxi_h(vl, 0);
    vl = __lsx_vpickev_b(__lsx_vsat_hu(tmp, 7), __lsx_vsat_hu(tmp, 7));
    return __lsx_vpickve2gr_w(vl, 0);
}

QT_END_NAMESPACE

#endif // __loongarch_sx

#endif // QDRAWINGPRIMITIVE_LSX_P_H
