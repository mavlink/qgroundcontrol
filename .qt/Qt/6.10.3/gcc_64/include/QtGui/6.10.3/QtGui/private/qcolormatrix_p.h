// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORMATRIX_H
#define QCOLORMATRIX_H

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

#include <QtGui/qtguiglobal.h>
#include <QtCore/qpoint.h>
#include <QtCore/private/qglobal_p.h>
#include <QtCore/private/qsimd_p.h>
#include <cmath>

QT_BEGIN_NAMESPACE

// An abstract 3 value color
class QColorVector
{
public:
    QColorVector() = default;
    constexpr QColorVector(float x, float y, float z, float w = 0.0f) noexcept : x(x), y(y), z(z), w(w) { }
    static constexpr QColorVector fromXYChromaticity(QPointF chr)
    { return {float(chr.x() / chr.y()), 1.0f, float((1.0f - chr.x() - chr.y()) / chr.y())}; }
    float x = 0.0f; // X, x, L, or red/cyan
    float y = 0.0f; // Y, y, a, or green/magenta
    float z = 0.0f; // Z, Y, b, or blue/yellow
    float w = 0.0f; // unused, or black

    constexpr bool isNull() const noexcept
    {
        return !x && !y && !z && !w;
    }
    bool isValid() const noexcept
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }

    static constexpr bool isValidChromaticity(const QPointF &chr)
    {
        if (chr.x() < qreal(0.0) || chr.x() > qreal(1.0))
            return false;
        if (chr.y() <= qreal(0.0) || chr.y() > qreal(1.0))
            return false;
        if (chr.x() + chr.y() > qreal(1.0))
            return false;
        return true;
    }

    constexpr QColorVector operator*(float f) const { return QColorVector(x * f, y * f, z * f, w * f); }
    constexpr QColorVector operator+(const QColorVector &v) const { return QColorVector(x + v.x, y + v.y, z + v.z, w + v.w); }
    constexpr QColorVector operator-(const QColorVector &v) const { return QColorVector(x - v.x, y - v.y, z - v.z, w - v.w); }
    void operator+=(const QColorVector &v) { x += v.x; y += v.y; z += v.z; w += v.w; }

    QPointF toChromaticity() const
    {
        if (isNull())
            return QPointF();
        float mag = 1.0f / (x + y + z);
        return QPointF(x * mag, y * mag);
    }

    // Common whitepoints:
    static constexpr QPointF D50Chromaticity() { return QPointF(0.34567, 0.35850); }
    static constexpr QPointF D65Chromaticity() { return QPointF(0.31271, 0.32902); }
    static constexpr QColorVector D50() { return fromXYChromaticity(D50Chromaticity()); }
    static constexpr QColorVector D65() { return fromXYChromaticity(D65Chromaticity()); }

    QColorVector xyzToLab() const
    {
        constexpr QColorVector ref = D50();
        constexpr float eps = 0.008856f;
        constexpr float kap = 903.3f;
#if defined(__SSE2__)
        const __m128 iref = _mm_setr_ps(1.f / ref.x, 1.f / ref.y, 1.f / ref.z, 0.f);
        __m128 v = _mm_loadu_ps(&x);
        v = _mm_mul_ps(v, iref);

        const __m128 f3 = _mm_set1_ps(3.f);
        __m128 est = _mm_add_ps(_mm_set1_ps(0.25f), _mm_mul_ps(v, _mm_set1_ps(0.75f))); // float est = 0.25f + (x * 0.75f);
        __m128 estsq = _mm_mul_ps(est, est);
        est = _mm_sub_ps(est, _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(estsq, est), v),
                                         _mm_rcp_ps(_mm_mul_ps(estsq, f3)))); // est -= ((est * est * est) - x) / (3.f * (est * est));
        estsq = _mm_mul_ps(est, est);
        est = _mm_sub_ps(est, _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(estsq, est), v),
                                         _mm_rcp_ps(_mm_mul_ps(estsq, f3)))); // est -= ((est * est * est) - x) / (3.f * (est * est));
        estsq = _mm_mul_ps(est, est);
        est = _mm_sub_ps(est, _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(estsq, est), v),
                                         _mm_rcp_ps(_mm_mul_ps(estsq, f3)))); // est -= ((est * est * est) - x) / (3.f * (est * est));
        estsq = _mm_mul_ps(est, est);
        est = _mm_sub_ps(est, _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(estsq, est), v),
                                         _mm_rcp_ps(_mm_mul_ps(estsq, f3)))); // est -= ((est * est * est) - x) / (3.f * (est * est));

        __m128 kapmul = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v, _mm_set1_ps(kap)), _mm_set1_ps(16.f)),
                                   _mm_set1_ps(1.f / 116.f)); // f_ = (kap * f_ + 16.f) * (1.f / 116.f);
        __m128 cmpgt = _mm_cmpgt_ps(v, _mm_set1_ps(eps)); // if (f_ > eps)
#if defined(__SSE4_1__)
        v = _mm_blendv_ps(kapmul, est, cmpgt); // if (..) f_ =..  else f_ =..
#else
        v = _mm_or_ps(_mm_and_ps(cmpgt, est), _mm_andnot_ps(cmpgt, kapmul));
#endif
        alignas(16) float out[4];
        _mm_store_ps(out, v);
        const float L = 116.f * out[1] - 16.f;
        const float a = 500.f * (out[0] - out[1]);
        const float b = 200.f * (out[1] - out[2]);
#else
        float xr = x * (1.f / ref.x);
        float yr = y * (1.f / ref.y);
        float zr = z * (1.f / ref.z);

        float fx, fy, fz;
        if (xr > eps)
            fx = fastCbrt(xr);
        else
            fx = (kap * xr + 16.f) * (1.f / 116.f);
        if (yr > eps)
            fy = fastCbrt(yr);
        else
            fy = (kap * yr + 16.f) * (1.f / 116.f);
        if (zr > eps)
            fz = fastCbrt(zr);
        else
            fz = (kap * zr + 16.f) * (1.f / 116.f);

        const float L = 116.f * fy - 16.f;
        const float a = 500.f * (fx - fy);
        const float b = 200.f * (fy - fz);
#endif
        // We output Lab values that has been scaled to 0.0->1.0 values, see also labToXyz.
        return QColorVector(L * (1.f / 100.f), (a + 128.f) * (1.f / 255.f), (b + 128.f) * (1.f / 255.f));
    }

    QColorVector labToXyz() const
    {
        constexpr QColorVector ref = D50();
        constexpr float eps = 0.008856f;
        constexpr float kap = 903.3f;
        // This transform has been guessed from the ICC spec, but it is not stated
        // anywhere to be the one to use to map to and from 0.0->1.0 values:
        const float L = x * 100.f;
        const float a = (y * 255.f) - 128.f;
        const float b = (z * 255.f) - 128.f;
        // From here is official Lab->XYZ conversion:
        float fy = (L + 16.f) * (1.f / 116.f);
        float fx = fy + (a * (1.f / 500.f));
        float fz = fy - (b * (1.f / 200.f));

        float xr, yr, zr;
        if (fx * fx * fx > eps)
            xr = fx * fx * fx;
        else
            xr = (116.f * fx - 16) * (1.f / kap);
        if (L > (kap * eps))
            yr = fy * fy * fy;
        else
            yr = L * (1.f / kap);
        if (fz * fz * fz > eps)
            zr = fz * fz * fz;
        else
            zr = (116.f * fz - 16) * (1.f / kap);

        xr = xr * ref.x;
        yr = yr * ref.y;
        zr = zr * ref.z;
        return QColorVector(xr, yr, zr);
    }
    friend inline bool comparesEqual(const QColorVector &lhs, const QColorVector &rhs) noexcept;
    Q_DECLARE_EQUALITY_COMPARABLE(QColorVector)

private:
    static float fastCbrt(float x)
    {
        // This gives us cube root within the precision we need.
        float est = 0.25f + (x * 0.75f); // guessing a cube-root of numbers between 0.01 and 1.
        est -= ((est * est * est) - x) / (3.f * (est * est));
        est -= ((est * est * est) - x) / (3.f * (est * est));
        est -= ((est * est * est) - x) / (3.f * (est * est));
        est -= ((est * est * est) - x) / (3.f * (est * est));
        // Q_ASSERT(qAbs(est - std::cbrt(x)) < 0.0001f);
        return est;
    }
};

inline bool comparesEqual(const QColorVector &v1, const QColorVector &v2) noexcept
{
    return (std::abs(v1.x - v2.x) < (1.0f / 2048.0f))
        && (std::abs(v1.y - v2.y) < (1.0f / 2048.0f))
        && (std::abs(v1.z - v2.z) < (1.0f / 2048.0f))
        && (std::abs(v1.w - v2.w) < (1.0f / 2048.0f));
}

// A matrix mapping 3 value colors.
// Not using QTransform because only floats are needed and performance is critical.
class QColorMatrix
{
public:
    // We are storing the matrix transposed as that is more convenient:
    QColorVector r;
    QColorVector g;
    QColorVector b;

    constexpr bool isNull() const
    {
        return r.isNull() && g.isNull() && b.isNull();
    }
    constexpr float determinant() const
    {
        return r.x * (b.z * g.y - g.z * b.y) -
               r.y * (b.z * g.x - g.z * b.x) +
               r.z * (b.y * g.x - g.y * b.x);
    }
    bool isValid() const
    {
        // A color matrix must be invertible
        return std::isnormal(determinant());
    }
    bool isIdentity() const noexcept
    {
        return *this == identity();
    }

    QColorMatrix inverted() const
    {
        float det = determinant();
        det = 1.0f / det;
        QColorMatrix inv;
        inv.r.x = (g.y * b.z - b.y * g.z) * det;
        inv.r.y = (b.y * r.z - r.y * b.z) * det;
        inv.r.z = (r.y * g.z - g.y * r.z) * det;
        inv.g.x = (b.x * g.z - g.x * b.z) * det;
        inv.g.y = (r.x * b.z - b.x * r.z) * det;
        inv.g.z = (g.x * r.z - r.x * g.z) * det;
        inv.b.x = (g.x * b.y - b.x * g.y) * det;
        inv.b.y = (b.x * r.y - r.x * b.y) * det;
        inv.b.z = (r.x * g.y - g.x * r.y) * det;
        return inv;
    }
    friend inline constexpr QColorMatrix operator*(const QColorMatrix &a, const QColorMatrix &o)
    {
        QColorMatrix comb;
        comb.r.x = a.r.x * o.r.x + a.g.x * o.r.y + a.b.x * o.r.z;
        comb.g.x = a.r.x * o.g.x + a.g.x * o.g.y + a.b.x * o.g.z;
        comb.b.x = a.r.x * o.b.x + a.g.x * o.b.y + a.b.x * o.b.z;

        comb.r.y = a.r.y * o.r.x + a.g.y * o.r.y + a.b.y * o.r.z;
        comb.g.y = a.r.y * o.g.x + a.g.y * o.g.y + a.b.y * o.g.z;
        comb.b.y = a.r.y * o.b.x + a.g.y * o.b.y + a.b.y * o.b.z;

        comb.r.z = a.r.z * o.r.x + a.g.z * o.r.y + a.b.z * o.r.z;
        comb.g.z = a.r.z * o.g.x + a.g.z * o.g.y + a.b.z * o.g.z;
        comb.b.z = a.r.z * o.b.x + a.g.z * o.b.y + a.b.z * o.b.z;
        return comb;

    }
    QColorVector map(const QColorVector &c) const
    {
        return QColorVector { c.x * r.x + c.y * g.x + c.z * b.x,
                              c.x * r.y + c.y * g.y + c.z * b.y,
                              c.x * r.z + c.y * g.z + c.z * b.z };
    }
    QColorMatrix transposed() const
    {
        return QColorMatrix { { r.x, g.x, b.x },
                              { r.y, g.y, b.y },
                              { r.z, g.z, b.z } };
    }

    static QColorMatrix identity()
    {
        return { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    }
    static QColorMatrix fromScale(QColorVector v)
    {
        return QColorMatrix { { v.x,  0.0f, 0.0f },
                              { 0.0f, v.y,  0.0f },
                              { 0.0f, 0.0f, v.z  } };
    }
    static QColorMatrix chromaticAdaptation(const QColorVector &whitePoint)
    {
        constexpr QColorVector whitePointD50 = QColorVector::D50();
        if (whitePoint != whitePointD50) {
            // A chromatic adaptation to map a white point to XYZ D50.

            // The Bradford method chromatic adaptation matrix:
            const QColorMatrix abrad = { {  0.8951f, -0.7502f,  0.0389f },
                                         {  0.2664f,  1.7135f, -0.0685f },
                                         { -0.1614f,  0.0367f,  1.0296f } };
            const QColorMatrix abradinv = { {  0.9869929f, 0.4323053f, -0.0085287f },
                                            { -0.1470543f, 0.5183603f,  0.0400428f },
                                            {  0.1599627f, 0.0492912f,  0.9684867f } };

            const QColorVector srcCone = abrad.map(whitePoint);
            if (srcCone.x && srcCone.y && srcCone.z) {
                const QColorVector dstCone = abrad.map(whitePointD50);
                const QColorMatrix wToD50 = { { dstCone.x / srcCone.x, 0, 0 },
                                              { 0, dstCone.y / srcCone.y, 0 },
                                              { 0, 0, dstCone.z / srcCone.z } };
                return abradinv * (wToD50 * abrad);
            }
        }
        return QColorMatrix::identity();
    }

    // These are used to recognize matrices from ICC profiles:
    static QColorMatrix toXyzFromSRgb()
    {
        return QColorMatrix { { 0.4360217452f, 0.2224751115f, 0.0139281144f },
                              { 0.3851087987f, 0.7169067264f, 0.0971015394f },
                              { 0.1430812478f, 0.0606181994f, 0.7141585946f } };
    }
    static QColorMatrix toXyzFromAdobeRgb()
    {
        return QColorMatrix { { 0.6097189188f, 0.3111021519f, 0.0194766335f },
                              { 0.2052682191f, 0.6256770492f, 0.0608891509f },
                              { 0.1492247432f, 0.0632209629f, 0.7448224425f } };
    }
    static QColorMatrix toXyzFromDciP3D65()
    {
        return QColorMatrix { { 0.5150973201f, 0.2411795557f, -0.0010491034f },
                              { 0.2919696569f, 0.6922441125f,  0.0418830328f },
                              { 0.1571449190f, 0.0665764511f,  0.7843542695f } };
    }
    static QColorMatrix toXyzFromProPhotoRgb()
    {
        return QColorMatrix { { 0.7976672649f, 0.2880374491f, 0.0000000000f },
                              { 0.1351922452f, 0.7118769884f, 0.0000000000f },
                              { 0.0313525312f, 0.0000856627f, 0.8251883388f } };
    }
    static QColorMatrix toXyzFromBt2020()
    {
        return QColorMatrix { { 0.673447f, 0.279037f, -0.00192261f },
                              { 0.165665f, 0.675339f,  0.0299835f  },
                              { 0.125092f, 0.0456238f, 0.797134f   } };
    }
    friend inline bool comparesEqual(const QColorMatrix &lhs, const QColorMatrix &rhs) noexcept;
    Q_DECLARE_EQUALITY_COMPARABLE(QColorMatrix)
};

inline bool comparesEqual(const QColorMatrix &m1, const QColorMatrix &m2) noexcept
{
    return (m1.r == m2.r) && (m1.g == m2.g) && (m1.b == m2.b);
}

QT_END_NAMESPACE

#endif // QCOLORMATRIX_P_H
