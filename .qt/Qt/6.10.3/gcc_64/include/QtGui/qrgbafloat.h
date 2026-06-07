// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRGBAFLOAT_H
#define QRGBAFLOAT_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qfloat16.h>

#include <algorithm>
#include <cmath>
#include <type_traits>

QT_BEGIN_NAMESPACE

template<typename F>
class alignas(sizeof(F) * 4) QRgbaFloat
{
    static_assert(std::is_same<F, qfloat16>::value || std::is_same<F, float>::value);
public:
    using Type = F;
#if defined(__AVX512FP16__) && QFLOAT16_IS_NATIVE
    // AVX512FP16 has multiplication instructions
    using FastType = F;
#else
    // use FP32 for multiplications
    using FastType = float;
#endif
    F r;
    F g;
    F b;
    F a;

    static constexpr
    QRgbaFloat fromRgba64(quint16 red, quint16 green, quint16 blue, quint16 alpha)
    {
        constexpr FastType scale = FastType(1.0f / 65535.0f);
        return QRgbaFloat{
            F(red    * scale),
            F(green  * scale),
            F(blue   * scale),
            F(alpha  * scale) };
    }

    static constexpr
    QRgbaFloat fromRgba(quint8 red, quint8 green, quint8 blue, quint8 alpha)
    {
        constexpr FastType scale = FastType(1.0f / 255.0f);
        return QRgbaFloat{
            F(red    * scale),
            F(green  * scale),
            F(blue   * scale),
            F(alpha  * scale) };
    }
    static constexpr
    QRgbaFloat fromArgb32(uint rgb)
    {
        return fromRgba(quint8(rgb >> 16), quint8(rgb >> 8), quint8(rgb), quint8(rgb >> 24));
    }

    constexpr bool isOpaque() const { return a >= FastType(1.0f); }
    constexpr bool isTransparent() const { return a <= FastType(0.0f); }

    constexpr float red()   const { return r; }
    constexpr float green() const { return g; }
    constexpr float blue()  const { return b; }
    constexpr float alpha() const { return a; }
    void setRed(float _red)     { r = F(_red); }
    void setGreen(float _green) { g = F(_green); }
    void setBlue(float _blue)   { b = F(_blue); }
    void setAlpha(float _alpha) { a = F(_alpha); }

    constexpr float redNormalized()   const { return clamp01(r); }
    constexpr float greenNormalized() const { return clamp01(g); }
    constexpr float blueNormalized()  const { return clamp01(b); }
    constexpr float alphaNormalized() const { return clamp01(a); }

    constexpr quint8 red8()   const { return qRound(redNormalized()   * FastType(255.0f)); }
    constexpr quint8 green8() const { return qRound(greenNormalized() * FastType(255.0f)); }
    constexpr quint8 blue8()  const { return qRound(blueNormalized()  * FastType(255.0f)); }
    constexpr quint8 alpha8() const { return qRound(alphaNormalized() * FastType(255.0f)); }
    constexpr uint toArgb32() const
    {
       return uint((alpha8() << 24) | (red8() << 16) | (green8() << 8) | blue8());
    }

    constexpr quint16 red16()   const { return qRound(redNormalized()   * FastType(65535.0f)); }
    constexpr quint16 green16() const { return qRound(greenNormalized() * FastType(65535.0f)); }
    constexpr quint16 blue16()  const { return qRound(blueNormalized()  * FastType(65535.0f)); }
    constexpr quint16 alpha16() const { return qRound(alphaNormalized() * FastType(65535.0f)); }

    Q_ALWAYS_INLINE constexpr QRgbaFloat premultiplied() const
    {
        return QRgbaFloat{r * a, g * a, b * a, a};
    }
    Q_ALWAYS_INLINE constexpr QRgbaFloat unpremultiplied() const
    {
        if (a <= F{0.0f})
            return QRgbaFloat{};    // default-initialization: zeroes
        if (a >= F{1.0f})
            return *this;
        const FastType ia = FastType(1.0f) / FastType(a);
        return QRgbaFloat{F(r * ia), F(g * ia), F(b * ia), F(a)};
    }
    constexpr bool operator==(QRgbaFloat f) const
    {
        return r == f.r && g == f.g && b == f.b && a == f.a;
    }
    constexpr bool operator!=(QRgbaFloat f) const
    {
        return !(*this == f);
    }

private:
    constexpr static FastType clamp01(Type f)
    {
        return std::clamp(FastType(f), FastType(0.0f), FastType(1.0f));
    }
};

typedef QRgbaFloat<qfloat16> QRgbaFloat16;
typedef QRgbaFloat<float> QRgbaFloat32;

QT_END_NAMESPACE

#endif // QRGBAFLOAT_H
