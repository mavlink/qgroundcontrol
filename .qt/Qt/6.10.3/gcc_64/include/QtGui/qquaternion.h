// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUATERNION_H
#define QQUATERNION_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qgenericmatrix.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_QUATERNION

class QMatrix4x4;
class QVariant;

class QT6_ONLY(Q_GUI_EXPORT) QQuaternion
{
public:
    constexpr QQuaternion() noexcept;
    explicit QQuaternion(Qt::Initialization) noexcept {}
    constexpr QQuaternion(float scalar, float xpos, float ypos, float zpos) noexcept;
#ifndef QT_NO_VECTOR3D
    constexpr QQuaternion(float scalar, const QVector3D &vector) noexcept;
#endif
#ifndef QT_NO_VECTOR4D
    constexpr explicit QQuaternion(const QVector4D &vector) noexcept;
#endif

    constexpr bool isNull() const noexcept;
    constexpr bool isIdentity() const noexcept;

#ifndef QT_NO_VECTOR3D
    constexpr QVector3D vector() const noexcept;
    constexpr void setVector(const QVector3D &vector) noexcept;
#endif
    constexpr void setVector(float x, float y, float z) noexcept;

    constexpr float x() const noexcept;
    constexpr float y() const noexcept;
    constexpr float z() const noexcept;
    constexpr float scalar() const noexcept;

    constexpr void setX(float x) noexcept;
    constexpr void setY(float y) noexcept;
    constexpr void setZ(float z) noexcept;
    constexpr void setScalar(float scalar) noexcept;

    constexpr static float dotProduct(const QQuaternion &q1, const QQuaternion &q2) noexcept;

    // ### Qt 7: make the next four constexpr
    // (perhaps using std::hypot, constexpr in C++26, or constexpr qHypot)
    QT7_ONLY(Q_GUI_EXPORT) float length() const;
    QT7_ONLY(Q_GUI_EXPORT) float lengthSquared() const;

    [[nodiscard]] QT7_ONLY(Q_GUI_EXPORT) QQuaternion normalized() const;
    QT7_ONLY(Q_GUI_EXPORT)  void normalize();

    constexpr QQuaternion inverted() const noexcept;

    [[nodiscard]] constexpr QQuaternion conjugated() const noexcept;

    QT7_ONLY(Q_GUI_EXPORT) QVector3D rotatedVector(const QVector3D &vector) const;

    constexpr QQuaternion &operator+=(const QQuaternion &quaternion) noexcept;
    constexpr QQuaternion &operator-=(const QQuaternion &quaternion) noexcept;
    constexpr QQuaternion &operator*=(float factor) noexcept;
    constexpr QQuaternion &operator*=(const QQuaternion &quaternion) noexcept;
    constexpr QQuaternion &operator/=(float divisor);

QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE
    friend constexpr bool operator==(const QQuaternion &q1, const QQuaternion &q2) noexcept
    {
        return q1.wp == q2.wp && q1.xp == q2.xp && q1.yp == q2.yp && q1.zp == q2.zp;
    }
    friend constexpr bool operator!=(const QQuaternion &q1, const QQuaternion &q2) noexcept
    {
        return !(q1 == q2);
    }
QT_WARNING_POP

    friend constexpr QQuaternion operator+(const QQuaternion &q1, const QQuaternion &q2) noexcept;
    friend constexpr QQuaternion operator-(const QQuaternion &q1, const QQuaternion &q2) noexcept;
    friend constexpr QQuaternion operator*(float factor, const QQuaternion &quaternion) noexcept;
    friend constexpr QQuaternion operator*(const QQuaternion &quaternion, float factor) noexcept;
    friend constexpr QQuaternion operator*(const QQuaternion &q1, const QQuaternion &q2) noexcept;
    friend constexpr QQuaternion operator-(const QQuaternion &quaternion) noexcept;
    friend constexpr QQuaternion operator/(const QQuaternion &quaternion, float divisor);

    friend constexpr bool qFuzzyCompare(const QQuaternion &q1, const QQuaternion &q2) noexcept;

#ifndef QT_NO_VECTOR4D
    constexpr QVector4D toVector4D() const noexcept;
#endif

    QT7_ONLY(Q_GUI_EXPORT) operator QVariant() const;

#ifndef QT_NO_VECTOR3D
    inline void getAxisAndAngle(QVector3D *axis, float *angle) const;
    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion fromAxisAndAngle(const QVector3D &axis, float angle);
#endif
    QT7_ONLY(Q_GUI_EXPORT) void getAxisAndAngle(float *x, float *y, float *z, float *angle) const;
    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion fromAxisAndAngle(float x, float y, float z,
                                                               float angle);

#ifndef QT_NO_VECTOR3D
    inline QVector3D toEulerAngles() const;
    static inline QQuaternion fromEulerAngles(const QVector3D &angles);
#endif
    QT7_ONLY(Q_GUI_EXPORT) void getEulerAngles(float *pitch, float *yaw, float *roll) const;
    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion fromEulerAngles(float pitch, float yaw, float roll);

    QT7_ONLY(Q_GUI_EXPORT) QMatrix3x3 toRotationMatrix() const;
    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion fromRotationMatrix(const QMatrix3x3 &rot3x3);

#ifndef QT_NO_VECTOR3D
    QT7_ONLY(Q_GUI_EXPORT) void getAxes(QVector3D *xAxis, QVector3D *yAxis, QVector3D *zAxis) const;
    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion fromAxes(const QVector3D &xAxis,
                                                       const QVector3D &yAxis,
                                                       const QVector3D &zAxis);

    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion fromDirection(const QVector3D &direction,
                                                            const QVector3D &up);

    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion rotationTo(const QVector3D &from,
                                                         const QVector3D &to);
#endif // QT_NO_VECTOR3D

    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion slerp(const QQuaternion &q1, const QQuaternion &q2,
                                                    float t);
    QT7_ONLY(Q_GUI_EXPORT) static QQuaternion nlerp(const QQuaternion &q1, const QQuaternion &q2,
                                                    float t);

private:
    float wp, xp, yp, zp;
};

Q_DECLARE_TYPEINFO(QQuaternion, Q_PRIMITIVE_TYPE);

constexpr QQuaternion::QQuaternion() noexcept : wp(1.0f), xp(0.0f), yp(0.0f), zp(0.0f) {}

constexpr QQuaternion::QQuaternion(float aScalar, float xpos, float ypos, float zpos) noexcept
    : wp(aScalar), xp(xpos), yp(ypos), zp(zpos) {}

QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE

constexpr bool QQuaternion::isNull() const noexcept
{
    return wp == 0.0f && xp == 0.0f && yp == 0.0f && zp == 0.0f;
}

constexpr bool QQuaternion::isIdentity() const noexcept
{
    return wp == 1.0f && xp == 0.0f && yp == 0.0f && zp == 0.0f;
}
QT_WARNING_POP

constexpr float QQuaternion::x() const noexcept { return xp; }
constexpr float QQuaternion::y() const noexcept { return yp; }
constexpr float QQuaternion::z() const noexcept { return zp; }
constexpr float QQuaternion::scalar() const noexcept { return wp; }

constexpr void QQuaternion::setX(float aX) noexcept { xp = aX; }
constexpr void QQuaternion::setY(float aY) noexcept { yp = aY; }
constexpr void QQuaternion::setZ(float aZ) noexcept { zp = aZ; }
constexpr void QQuaternion::setScalar(float aScalar) noexcept { wp = aScalar; }

constexpr float QQuaternion::dotProduct(const QQuaternion &q1, const QQuaternion &q2) noexcept
{
    return q1.wp * q2.wp + q1.xp * q2.xp + q1.yp * q2.yp + q1.zp * q2.zp;
}

constexpr QQuaternion QQuaternion::inverted() const noexcept
{
    // Need some extra precision if the length is very small.
    double len = double(wp) * double(wp) +
                 double(xp) * double(xp) +
                 double(yp) * double(yp) +
                 double(zp) * double(zp);
    if (!qFuzzyIsNull(len))
        return QQuaternion(float(double(wp) / len), float(double(-xp) / len),
                           float(double(-yp) / len), float(double(-zp) / len));
    return QQuaternion(0.0f, 0.0f, 0.0f, 0.0f);
}

constexpr QQuaternion QQuaternion::conjugated() const noexcept
{
    return QQuaternion(wp, -xp, -yp, -zp);
}

constexpr QQuaternion &QQuaternion::operator+=(const QQuaternion &quaternion) noexcept
{
    wp += quaternion.wp;
    xp += quaternion.xp;
    yp += quaternion.yp;
    zp += quaternion.zp;
    return *this;
}

constexpr QQuaternion &QQuaternion::operator-=(const QQuaternion &quaternion) noexcept
{
    wp -= quaternion.wp;
    xp -= quaternion.xp;
    yp -= quaternion.yp;
    zp -= quaternion.zp;
    return *this;
}

constexpr QQuaternion &QQuaternion::operator*=(float factor) noexcept
{
    wp *= factor;
    xp *= factor;
    yp *= factor;
    zp *= factor;
    return *this;
}

constexpr QQuaternion operator*(const QQuaternion &q1, const QQuaternion &q2) noexcept
{
    float yy = (q1.wp - q1.yp) * (q2.wp + q2.zp);
    float zz = (q1.wp + q1.yp) * (q2.wp - q2.zp);
    float ww = (q1.zp + q1.xp) * (q2.xp + q2.yp);
    float xx = ww + yy + zz;
    float qq = 0.5f * (xx + (q1.zp - q1.xp) * (q2.xp - q2.yp));

    float w = qq - ww + (q1.zp - q1.yp) * (q2.yp - q2.zp);
    float x = qq - xx + (q1.xp + q1.wp) * (q2.xp + q2.wp);
    float y = qq - yy + (q1.wp - q1.xp) * (q2.yp + q2.zp);
    float z = qq - zz + (q1.zp + q1.yp) * (q2.wp - q2.xp);

    return QQuaternion(w, x, y, z);
}

constexpr QQuaternion &QQuaternion::operator*=(const QQuaternion &quaternion) noexcept
{
    *this = *this * quaternion;
    return *this;
}

constexpr QQuaternion &QQuaternion::operator/=(float divisor)
{
    wp /= divisor;
    xp /= divisor;
    yp /= divisor;
    zp /= divisor;
    return *this;
}

constexpr QQuaternion operator+(const QQuaternion &q1, const QQuaternion &q2) noexcept
{
    return QQuaternion(q1.wp + q2.wp, q1.xp + q2.xp, q1.yp + q2.yp, q1.zp + q2.zp);
}

constexpr QQuaternion operator-(const QQuaternion &q1, const QQuaternion &q2) noexcept
{
    return QQuaternion(q1.wp - q2.wp, q1.xp - q2.xp, q1.yp - q2.yp, q1.zp - q2.zp);
}

constexpr QQuaternion operator*(float factor, const QQuaternion &quaternion) noexcept
{
    return QQuaternion(quaternion.wp * factor, quaternion.xp * factor, quaternion.yp * factor, quaternion.zp * factor);
}

constexpr QQuaternion operator*(const QQuaternion &quaternion, float factor) noexcept
{
    return QQuaternion(quaternion.wp * factor, quaternion.xp * factor, quaternion.yp * factor, quaternion.zp * factor);
}

constexpr QQuaternion operator-(const QQuaternion &quaternion) noexcept
{
    return QQuaternion(-quaternion.wp, -quaternion.xp, -quaternion.yp, -quaternion.zp);
}

constexpr QQuaternion operator/(const QQuaternion &quaternion, float divisor)
{
    return QQuaternion(quaternion.wp / divisor, quaternion.xp / divisor, quaternion.yp / divisor, quaternion.zp / divisor);
}

constexpr bool qFuzzyCompare(const QQuaternion &q1, const QQuaternion &q2) noexcept
{
    return QtPrivate::fuzzyCompare(q1.wp, q2.wp)
        && QtPrivate::fuzzyCompare(q1.xp, q2.xp)
        && QtPrivate::fuzzyCompare(q1.yp, q2.yp)
        && QtPrivate::fuzzyCompare(q1.zp, q2.zp);
}

#ifndef QT_NO_VECTOR3D

constexpr QQuaternion::QQuaternion(float aScalar, const QVector3D &aVector) noexcept
    : wp(aScalar), xp(aVector.x()), yp(aVector.y()), zp(aVector.z()) {}

constexpr void QQuaternion::setVector(const QVector3D &aVector) noexcept
{
    xp = aVector.x();
    yp = aVector.y();
    zp = aVector.z();
}

constexpr QVector3D QQuaternion::vector() const noexcept
{
    return QVector3D(xp, yp, zp);
}

inline QVector3D operator*(const QQuaternion &quaternion, const QVector3D &vec)
{
    return quaternion.rotatedVector(vec);
}

void QQuaternion::getAxisAndAngle(QVector3D *axis, float *angle) const
{
    float aX, aY, aZ;
    getAxisAndAngle(&aX, &aY, &aZ, angle);
    *axis = QVector3D(aX, aY, aZ);
}

QVector3D QQuaternion::toEulerAngles() const
{
    float pitch, yaw, roll;
    getEulerAngles(&pitch, &yaw, &roll);
    return QVector3D(pitch, yaw, roll);
}

QQuaternion QQuaternion::fromEulerAngles(const QVector3D &angles)
{
    return QQuaternion::fromEulerAngles(angles.x(), angles.y(), angles.z());
}

#endif // QT_NO_VECTOR3D

constexpr void QQuaternion::setVector(float aX, float aY, float aZ) noexcept
{
    xp = aX;
    yp = aY;
    zp = aZ;
}

#ifndef QT_NO_VECTOR4D

constexpr QQuaternion::QQuaternion(const QVector4D &aVector) noexcept
    : wp(aVector.w()), xp(aVector.x()), yp(aVector.y()), zp(aVector.z()) {}

constexpr QVector4D QQuaternion::toVector4D() const noexcept
{
    return QVector4D(xp, yp, zp, wp);
}

#endif // QT_NO_VECTOR4D

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QQuaternion &q);
#endif

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QQuaternion &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QQuaternion &);
#endif

#endif // QT_NO_QUATERNION

QT_END_NAMESPACE

#endif // QQUATERNION_H
