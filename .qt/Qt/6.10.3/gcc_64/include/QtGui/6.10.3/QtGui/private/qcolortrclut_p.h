// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRCLUT_P_H
#define QCOLORTRCLUT_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qrgb.h>
#include <QtGui/qrgba64.h>
#include <QtCore/private/qsimd_p.h>

#include <cmath>
#include <memory>

#if defined(__SSE2__)
#include <emmintrin.h>
#elif defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

QT_BEGIN_NAMESPACE

class QColorTransferGenericFunction;
class QColorTransferFunction;
class QColorTransferTable;
class QColorTrc;

class Q_GUI_EXPORT QColorTrcLut
{
public:
    static constexpr uint32_t ShiftUp = 4;                         // Amount to shift up from 1->255
    static constexpr uint32_t ShiftDown = (8 - ShiftUp);           // Amount to shift down from 1->65280
    static constexpr qsizetype Resolution = (1 << ShiftUp) * 255;  // Number of entries in table

    enum Direction {
        ToLinear = 1,
        FromLinear = 2,
        BiLinear = ToLinear | FromLinear
    };

    static std::shared_ptr<QColorTrcLut> fromGamma(float gamma, Direction dir = BiLinear);
    static std::shared_ptr<QColorTrcLut> fromTrc(const QColorTrc &trc, Direction dir = BiLinear);
    void setFromGamma(float gamma, Direction dir = BiLinear);
    void setFromTransferFunction(const QColorTransferFunction &transFn, Direction dir = BiLinear);
    void setFromTransferTable(const QColorTransferTable &transTable, Direction dir = BiLinear);
    void setFromTransferGenericFunction(const QColorTransferGenericFunction &transfn, Direction dir);
    void setFromTrc(const QColorTrc &trc, Direction dir);

    // The following methods all convert opaque or unpremultiplied colors:

    QRgba64 toLinear64(QRgb rgb32) const
    {
#if defined(__SSE2__)
        __m128i v = _mm_cvtsi32_si128(rgb32);
        v = _mm_unpacklo_epi8(v, _mm_setzero_si128());
        const __m128i vidx = _mm_slli_epi16(v, ShiftUp);
        const int ridx = _mm_extract_epi16(vidx, 2);
        const int gidx = _mm_extract_epi16(vidx, 1);
        const int bidx = _mm_extract_epi16(vidx, 0);
        v = _mm_slli_epi16(v, 8); // a * 256
        v = _mm_insert_epi16(v, m_toLinear[ridx], 0);
        v = _mm_insert_epi16(v, m_toLinear[gidx], 1);
        v = _mm_insert_epi16(v, m_toLinear[bidx], 2);
        v = _mm_add_epi16(v, _mm_srli_epi16(v, 8));
        QRgba64 rgba64;
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&rgba64), v);
        return rgba64;
#elif defined(__ARM_NEON__) && Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        uint8x8_t v8 = vreinterpret_u8_u32(vmov_n_u32(rgb32));
        uint16x4_t v16 = vget_low_u16(vmovl_u8(v8));
        const uint16x4_t vidx = vshl_n_u16(v16, ShiftUp);
        const int ridx = vget_lane_u16(vidx, 2);
        const int gidx = vget_lane_u16(vidx, 1);
        const int bidx = vget_lane_u16(vidx, 0);
        v16 = vshl_n_u16(v16, 8); // a * 256
        v16 = vset_lane_u16(m_toLinear[ridx], v16, 0);
        v16 = vset_lane_u16(m_toLinear[gidx], v16, 1);
        v16 = vset_lane_u16(m_toLinear[bidx], v16, 2);
        v16 = vadd_u16(v16, vshr_n_u16(v16, 8));
        return QRgba64::fromRgba64(vget_lane_u64(vreinterpret_u64_u16(v16), 0));
#else
        uint r = m_toLinear[qRed(rgb32) << ShiftUp];
        uint g = m_toLinear[qGreen(rgb32) << ShiftUp];
        uint b = m_toLinear[qBlue(rgb32) << ShiftUp];
        r = r + (r >> 8);
        g = g + (g >> 8);
        b = b + (b >> 8);
        return QRgba64::fromRgba64(r, g, b, qAlpha(rgb32) * 257);
#endif
    }
    QRgba64 toLinear64(QRgba64) const = delete;

    QRgb toLinear(QRgb rgb32) const
    {
        return convertWithTable(rgb32, m_toLinear.get());
    }

    QRgba64 toLinear(QRgba64 rgb64) const
    {
        return convertWithTable(rgb64, m_toLinear.get());
    }

    float u8ToLinearF32(int c) const
    {
        ushort v = m_toLinear[c << ShiftUp];
        return v * (1.0f / (255*256));
    }

    float u16ToLinearF32(int c) const
    {
        c -= (c >> 8);
        ushort v = m_toLinear[c >> ShiftDown];
        return v * (1.0f / (255*256));
    }

    float toLinear(float f) const
    {
        ushort v = m_toLinear[(int)(f * Resolution + 0.5f)];
        return v * (1.0f / (255*256));
    }

    QRgb fromLinear64(QRgba64 rgb64) const
    {
#if defined(__SSE2__)
        __m128i v = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&rgb64));
        v = _mm_sub_epi16(v, _mm_srli_epi16(v, 8));
        const __m128i vidx = _mm_srli_epi16(v, ShiftDown);
        const int ridx = _mm_extract_epi16(vidx, 0);
        const int gidx = _mm_extract_epi16(vidx, 1);
        const int bidx = _mm_extract_epi16(vidx, 2);
        v = _mm_insert_epi16(v, m_fromLinear[ridx], 2);
        v = _mm_insert_epi16(v, m_fromLinear[gidx], 1);
        v = _mm_insert_epi16(v, m_fromLinear[bidx], 0);
        v = _mm_add_epi16(v, _mm_set1_epi16(0x80));
        v = _mm_srli_epi16(v, 8);
        v = _mm_packus_epi16(v, v);
        return _mm_cvtsi128_si32(v);
#elif defined(__ARM_NEON__) && Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        uint16x4_t v = vreinterpret_u16_u64(vmov_n_u64(rgb64));
        v = vsub_u16(v, vshr_n_u16(v, 8));
        const uint16x4_t vidx = vshr_n_u16(v, ShiftDown);
        const int ridx = vget_lane_u16(vidx, 0);
        const int gidx = vget_lane_u16(vidx, 1);
        const int bidx = vget_lane_u16(vidx, 2);
        v = vset_lane_u16(m_fromLinear[ridx], v, 2);
        v = vset_lane_u16(m_fromLinear[gidx], v, 1);
        v = vset_lane_u16(m_fromLinear[bidx], v, 0);
        uint8x8_t v8 = vrshrn_n_u16(vcombine_u16(v, v), 8);
        return vget_lane_u32(vreinterpret_u32_u8(v8), 0);
#else
        uint a = rgb64.alpha();
        uint r = rgb64.red();
        uint g = rgb64.green();
        uint b = rgb64.blue();
        a = a - (a >> 8);
        r = r - (r >> 8);
        g = g - (g >> 8);
        b = b - (b >> 8);
        a = (a + 0x80) >> 8;
        r = (m_fromLinear[r >> ShiftDown] + 0x80) >> 8;
        g = (m_fromLinear[g >> ShiftDown] + 0x80) >> 8;
        b = (m_fromLinear[b >> ShiftDown] + 0x80) >> 8;
        return (a << 24) | (r << 16) | (g << 8) | b;
#endif
    }

    QRgb fromLinear(QRgb rgb32) const
    {
        return convertWithTable(rgb32, m_fromLinear.get());
    }

    QRgba64 fromLinear(QRgba64 rgb64) const
    {
        return convertWithTable(rgb64, m_fromLinear.get());
    }

    int u8FromLinearF32(float f) const
    {
        ushort v = m_fromLinear[(int)(f * Resolution + 0.5f)];
        return (v + 0x80) >> 8;
    }
    int u16FromLinearF32(float f) const
    {
        ushort v = m_fromLinear[(int)(f * Resolution + 0.5f)];
        return v + (v >> 8);
    }
    float fromLinear(float f) const
    {
        ushort v = m_fromLinear[(int)(f * Resolution + 0.5f)];
        return v * (1.0f / (255*256));
    }

    // We translate to 0-65280 (255*256) instead to 0-65535 to make simple
    // shifting an accurate conversion.
    // We translate from 0->Resolution (4080 = 255*16) for the same speed up,
    // and to keep the tables small enough to fit in most inner caches.
    std::unique_ptr<ushort[]> m_toLinear; // [0->Resolution] -> [0-65280]
    std::unique_ptr<ushort[]> m_fromLinear; // [0->Resolution] -> [0-65280]
    ushort m_unclampedToLinear = Resolution;

private:
    QColorTrcLut() = default;

    static std::shared_ptr<QColorTrcLut> create();

    Q_ALWAYS_INLINE static QRgb convertWithTable(QRgb rgb32, const ushort *table)
    {
        const int r = (table[qRed(rgb32) << ShiftUp] + 0x80) >> 8;
        const int g = (table[qGreen(rgb32) << ShiftUp] + 0x80) >> 8;
        const int b = (table[qBlue(rgb32) << ShiftUp] + 0x80) >> 8;
        return (rgb32 & 0xff000000) | (r << 16) | (g << 8) | b;
    }
    Q_ALWAYS_INLINE static QRgba64 convertWithTable(QRgba64 rgb64, const ushort *table)
    {
#if defined(__SSE2__)
        __m128i v = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(&rgb64));
        v = _mm_sub_epi16(v, _mm_srli_epi16(v, 8));
        const __m128i vidx = _mm_srli_epi16(v, ShiftDown);
        const int ridx = _mm_extract_epi16(vidx, 2);
        const int gidx = _mm_extract_epi16(vidx, 1);
        const int bidx = _mm_extract_epi16(vidx, 0);
        v = _mm_insert_epi16(v, table[ridx], 2);
        v = _mm_insert_epi16(v, table[gidx], 1);
        v = _mm_insert_epi16(v, table[bidx], 0);
        v = _mm_add_epi16(v, _mm_srli_epi16(v, 8));
        QRgba64 rgba64;
        _mm_storel_epi64(reinterpret_cast<__m128i *>(&rgba64), v);
        return rgba64;
#elif defined(__ARM_NEON__) && Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        uint16x4_t v = vreinterpret_u16_u64(vmov_n_u64(rgb64));
        v = vsub_u16(v, vshr_n_u16(v, 8));
        const uint16x4_t vidx = vshr_n_u16(v, ShiftDown);
        const int ridx = vget_lane_u16(vidx, 2);
        const int gidx = vget_lane_u16(vidx, 1);
        const int bidx = vget_lane_u16(vidx, 0);
        v = vset_lane_u16(table[ridx], v, 2);
        v = vset_lane_u16(table[gidx], v, 1);
        v = vset_lane_u16(table[bidx], v, 0);
        v = vadd_u16(v, vshr_n_u16(v, 8));
        return QRgba64::fromRgba64(vget_lane_u64(vreinterpret_u64_u16(v), 0));
#else
        ushort r = rgb64.red();
        ushort g = rgb64.green();
        ushort b = rgb64.blue();
        r = r - (r >> 8);
        g = g - (g >> 8);
        b = b - (b >> 8);
        r = table[r >> ShiftDown];
        g = table[g >> ShiftDown];
        b = table[b >> ShiftDown];
        r = r + (r >> 8);
        g = g + (g >> 8);
        b = b + (b >> 8);
        return QRgba64::fromRgba64(r, g, b, rgb64.alpha());
#endif
    }
};

QT_END_NAMESPACE

#endif // QCOLORTRCLUT_P_H
