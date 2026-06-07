// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQuickV8PARTICLEDATA_H
#define QQuickV8PARTICLEDATA_H

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

#include <private/qquickparticlesystem_p.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickV4ParticleData
{
    Q_GADGET
    QML_VALUE_TYPE(particle)
    QML_ADDED_IN_VERSION(6, 7)

#define Q_QUICK_PARTICLE_ACCESSOR(TYPE, VARIABLE, NAME) \
    Q_PROPERTY(TYPE NAME READ NAME WRITE set_ ## NAME FINAL) \
    TYPE NAME() const { return datum ? datum->VARIABLE : TYPE(); } \
    void set_ ## NAME(TYPE a) { if (datum) datum->VARIABLE = a; }

    Q_QUICK_PARTICLE_ACCESSOR(float, x, initialX)
    Q_QUICK_PARTICLE_ACCESSOR(float, vx, initialVX)
    Q_QUICK_PARTICLE_ACCESSOR(float, ax, initialAX)
    Q_QUICK_PARTICLE_ACCESSOR(float, y, initialY)
    Q_QUICK_PARTICLE_ACCESSOR(float, vy, initialVY)
    Q_QUICK_PARTICLE_ACCESSOR(float, ay, initialAY)
    Q_QUICK_PARTICLE_ACCESSOR(float, t, t)
    Q_QUICK_PARTICLE_ACCESSOR(float, size, startSize)
    Q_QUICK_PARTICLE_ACCESSOR(float, endSize, endSize)
    Q_QUICK_PARTICLE_ACCESSOR(float, lifeSpan, lifeSpan)
    Q_QUICK_PARTICLE_ACCESSOR(float, rotation, rotation)
    Q_QUICK_PARTICLE_ACCESSOR(float, rotationVelocity, rotationVelocity)
    Q_QUICK_PARTICLE_ACCESSOR(bool, autoRotate, autoRotate)
    Q_QUICK_PARTICLE_ACCESSOR(bool, update, update)
    Q_QUICK_PARTICLE_ACCESSOR(float, xx, xDeformationVectorX)
    Q_QUICK_PARTICLE_ACCESSOR(float, yx, yDeformationVectorX)
    Q_QUICK_PARTICLE_ACCESSOR(float, xy, xDeformationVectorY)
    Q_QUICK_PARTICLE_ACCESSOR(float, yy, yDeformationVectorY)

    // Undocumented?
    Q_QUICK_PARTICLE_ACCESSOR(float, animIdx, animationIndex)
    Q_QUICK_PARTICLE_ACCESSOR(float, frameDuration, frameDuration)
    Q_QUICK_PARTICLE_ACCESSOR(float, frameAt, frameAt)
    Q_QUICK_PARTICLE_ACCESSOR(float, frameCount, frameCount)
    Q_QUICK_PARTICLE_ACCESSOR(float, animT, animationT)

#undef Q_QUICK_PARTICLE_ACCESSOR

#define Q_QUICK_PARTICLE_SYSTEM_ACCESSOR(GETTER, SETTER, NAME) \
    Q_PROPERTY(float NAME READ NAME WRITE set_ ## NAME) \
    float NAME() const { return (datum && particleSystem) ? datum->GETTER(particleSystem) : 0; } \
    void set_ ## NAME(float a) { if (datum && particleSystem) datum->SETTER(a, particleSystem); }

    Q_QUICK_PARTICLE_SYSTEM_ACCESSOR(curX, setInstantaneousX, x)
    Q_QUICK_PARTICLE_SYSTEM_ACCESSOR(curVX, setInstantaneousVX, vx)
    Q_QUICK_PARTICLE_SYSTEM_ACCESSOR(curAX, setInstantaneousAX, ax)
    Q_QUICK_PARTICLE_SYSTEM_ACCESSOR(curY, setInstantaneousY, y)
    Q_QUICK_PARTICLE_SYSTEM_ACCESSOR(curVY, setInstantaneousVY, vy)
    Q_QUICK_PARTICLE_SYSTEM_ACCESSOR(curAY, setInstantaneousAY, ay)

#undef Q_QUICK_PARTICLE_SYSTEM_ACCESSOR

#define Q_QUICK_PARTICLE_COLOR_ACCESSOR(VAR, NAME) \
    Q_PROPERTY(float NAME READ NAME WRITE set_ ## NAME) \
    float NAME() const { return datum ? datum->color.VAR / 255.0 : 0.0; } \
    void set_ ## NAME(float a)\
    {\
        if (datum)\
            datum->color.VAR = qMin(255, qMax(0, (int)::floor(a * 255.0)));\
    }

    Q_QUICK_PARTICLE_COLOR_ACCESSOR(r, red)
    Q_QUICK_PARTICLE_COLOR_ACCESSOR(g, green)
    Q_QUICK_PARTICLE_COLOR_ACCESSOR(b, blue)
    Q_QUICK_PARTICLE_COLOR_ACCESSOR(a, alpha)

#undef Q_QUICK_PARTICLE_COLOR_ACCESSOR

    Q_PROPERTY(float lifeLeft READ lifeLeft)
    Q_PROPERTY(float currentSize READ currentSize)

public:
    QQuickV4ParticleData() = default;
    QQuickV4ParticleData(QQuickParticleData *datum, QQuickParticleSystem *system)
        : datum(datum)
        , particleSystem(system)
    {}

    Q_INVOKABLE void discard()
    {
        if (datum)
            datum->lifeSpan = 0;
    }

    float lifeLeft() const
    {
        return (datum && particleSystem) ? datum->lifeLeft(particleSystem) : 0.0;
    }

    float currentSize() const
    {
        return (datum && particleSystem) ? datum->curSize(particleSystem) : 0.0;
    }

private:
    QQuickParticleData *datum = nullptr;
    QQuickParticleSystem *particleSystem = nullptr;
};


QT_END_NAMESPACE


#endif
