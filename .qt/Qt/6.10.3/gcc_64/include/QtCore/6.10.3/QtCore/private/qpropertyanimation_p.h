// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPROPERTYANIMATION_P_H
#define QPROPERTYANIMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qpropertyanimation.h"

#include "private/qvariantanimation_p.h"
#include "private/qproperty_p.h"

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QPropertyAnimationPrivate : public QVariantAnimationPrivate
{
   Q_DECLARE_PUBLIC(QPropertyAnimation)
public:
    QPropertyAnimationPrivate() : propertyType(0), propertyIndex(-1) { }
    ~QPropertyAnimationPrivate() override;

    void setTargetObjectForwarder(QObject *target) { q_func()->setTargetObject(target); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QPropertyAnimationPrivate, QObject *, targetObject,
                                       &QPropertyAnimationPrivate::setTargetObjectForwarder,
                                       nullptr)
    void targetObjectDestroyed()
    {
        // stop() has to be called before targetObject is set to nullptr.
        // targetObject must not be nullptr in states unequal to "Stopped".
        q_func()->stop();
        targetObject.setValueBypassingBindings(nullptr);
        targetObject.notify();
    }

    //for the QProperty
    int propertyType;
    int propertyIndex;

    void setPropertyName(const QByteArray &propertyName)
    {
        q_func()->setPropertyName(propertyName);
    }
    Q_OBJECT_COMPAT_PROPERTY(QPropertyAnimationPrivate, QByteArray, propertyName,
                             &QPropertyAnimationPrivate::setPropertyName)
    void updateProperty(const QVariant &);
    void updateMetaProperty();
};

QT_END_NAMESPACE

#endif //QPROPERTYANIMATION_P_H
