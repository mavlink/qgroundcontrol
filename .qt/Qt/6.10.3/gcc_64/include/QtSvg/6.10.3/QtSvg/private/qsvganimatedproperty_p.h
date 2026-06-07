// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGANIMATEDPROPERTY_P_H
#define QSVGANIMATEDPROPERTY_P_H

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

#include <QtSvg/private/qtsvgglobal_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qstring.h>
#include <QtCore/qpoint.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgAbstractAnimatedProperty
{
public:
    enum Type
    {
        Int,
        Float,
        Color,
        Transform,
    };

    QSvgAbstractAnimatedProperty(const QString &name, Type type);
    virtual ~QSvgAbstractAnimatedProperty();

    void setKeyFrames(const QList<qreal> &keyFrames);
    void appendKeyFrame(qreal keyFrame);
    QList<qreal> keyFrames() const;
    void setPropertyName(const QString &name);
    QStringView propertyName() const;
    Type type() const;
    QVariant interpolatedValue() const;
    virtual void interpolate(uint index, qreal t) const = 0;

    static QSvgAbstractAnimatedProperty *createAnimatedProperty(const QString &name);
protected:
    QList<qreal> m_keyFrames;
    mutable QVariant m_interpolatedValue;

private:
    QString m_propertyName;
    Type m_type;
};

class Q_SVG_EXPORT QSvgAnimatedPropertyColor : public QSvgAbstractAnimatedProperty
{
public:
    QSvgAnimatedPropertyColor(const QString &name);

    void setColors(const QList<QColor> &colors);
    void appendColor(const QColor &color);
    QList<QColor> colors() const;

    void interpolate(uint index, qreal t) const override;

private:
    QList<QColor> m_colors;
};

class Q_SVG_EXPORT QSvgAnimatedPropertyFloat : public QSvgAbstractAnimatedProperty
{
public:
    QSvgAnimatedPropertyFloat(const QString &name);

    void setValues(const QList<qreal> &values);
    void appendValue(const qreal value);
    QList<qreal> values() const;

    void interpolate(uint index, qreal t) const override;

private:
    QList<qreal> m_values;
};

class Q_SVG_EXPORT QSvgAnimatedPropertyTransform : public QSvgAbstractAnimatedProperty
{
public:
    struct TransformComponent {
        enum Type {
            Translate,
            Scale,
            Rotate,
            Skew,
            Matrix
        };
        Type type;
        QVarLengthArray<qreal, 16> values;
    };

public:
    QSvgAnimatedPropertyTransform(const QString &name);

    void setTransformCount(quint32 count);
    quint32 transformCount() const;
    void appendComponents(const QList<TransformComponent> &components);
    QList<TransformComponent> components() const;

    void interpolate(uint index, qreal t) const override;

private:
    QList<TransformComponent> m_components;
    quint32 m_transformCount = 0;
};

QT_END_NAMESPACE

#endif // QSVGANIMATEDPROPERTY_P_H
