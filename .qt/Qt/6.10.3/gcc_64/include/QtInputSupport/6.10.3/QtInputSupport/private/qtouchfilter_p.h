// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QTOUCHFILTER_P_H
#define QTOUCHFILTER_P_H

#include <private/qglobal_p.h>

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

struct QTouchFilter
{
    QTouchFilter();

    void initialize(float pos, float velocity);
    void update(float pos, float velocity, float timeDelta);

    float position() const { return x.x; }
    float velocity() const { return x.y; }

private:
    struct vec2 {
        vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) { }
        float x, y;

        vec2 operator-(vec2 v) {
            return vec2(x - v.x, y - v.y);
        }

        vec2 operator+(vec2 v) {
            return vec2(x + v.x, y + v.y);
        }
    };

    struct mat2 {
        float a, b, c, d;
        mat2(float a = 1.0f, float b = 0.0f, float c = 0.0f, float d = 1.0f)
            : a(a)
            , b(b)
            , c(c)
            , d(d)
        {
        }

        mat2 transposed() const {
            return mat2(a, c,
                        b, d);
        }

        mat2 inverted() const {
            float det = 1.0f / (a * d - b * c);
            return mat2( d * det, -b * det,
                        -c * det,  a * det);
        }

        mat2 operator+(mat2 m) const {
            return mat2(a + m.a, b + m.b,
                        c + m.c, d + m.d);
        }

        mat2 operator-(mat2 m) const {
            return mat2(a - m.a, b - m.b,
                        c - m.c, d - m.d);
        }

        vec2 operator*(vec2 v) const {
            return vec2(a * v.x + b * v.y,
                        c * v.x + d * v.y);
        }

        mat2 operator*(mat2 M) const {
            return mat2(a * M.a + b * M.c,
                        a * M.b + b * M.d,
                        c * M.a + d * M.c,
                        c * M.b + d * M.d);
        }
    };

    vec2 x;
    mat2 A;
    mat2 P;
    mat2 Q;
    mat2 R;
    mat2 H;
};

inline QTouchFilter::QTouchFilter()
{
}

inline void QTouchFilter::initialize(float pos, float velocity)
{
    x = vec2(pos, velocity);

    P = mat2(0.0f, 0.0f,
             0.0f, 0.0f);

    Q = mat2(0.0f, 0.0f,
             0.0f, 0.1f);
    R = mat2(0.1f, 0.0f,
             0.0f, 0.1f);
}

inline void QTouchFilter::update(float pos, float velocity, float dT)
{
    A.b = dT;

    // Prediction setp
    x = A * x;
    P = A * P * A.transposed() + Q;

    // Correction step (complete with H)
    // mat2 S = H * P * H.transposed() + R;
    // mat2 K = P * H.transposed() * S.inverted();
    // vec2 m(pos, velocity);
    // vec2 y = m - H * x;
    // x = x + K * y;
    // P = (mat2() - K * H) * P;

    // Correction step (without H as H is currently set to I, so we can ignore
    // it in the calculations...)
    mat2 S = P + R;
    mat2 K = P * S.inverted();
    vec2 m(pos, velocity);
    vec2 y = m - x;
    x = x + K * y;
    P = (mat2() - K) * P;

}

QT_END_NAMESPACE

#endif // QTOUCHFILTER_P_H
