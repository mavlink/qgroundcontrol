// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKTRANSFORMGROUP_P_H
#define QQUICKTRANSFORMGROUP_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtQml/qqmllist.h>
#include <QtQuick/qquickitem.h>
#include <QtQuickVectorImageHelpers/qtquickvectorimagehelpersexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEHELPERS_EXPORT QQuickTransformGroup : public QQuickTransform
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TransformGroup)

    Q_PROPERTY(QQmlListProperty<QQuickTransform> transformSequence READ transformSequence)
    Q_CLASSINFO("DefaultProperty", "transformSequence")

public:
    QQuickTransformGroup(QObject *parent = nullptr);
    ~QQuickTransformGroup();

    Q_INVOKABLE void activateOverride(QQuickTransform *);
    Q_INVOKABLE void deactivateOverride(QQuickTransform *);
    Q_INVOKABLE void deactivate(QQuickTransform *);

    void applyTo(QMatrix4x4 *matrix) const override;

    QQmlListProperty<QQuickTransform> transformSequence();

    void triggerUpdate();

private:
    Q_DECLARE_PRIVATE(QQuickTransform)

    static void transformSequence_append(QQmlListProperty<QQuickTransform> *, QQuickTransform *);
    static qsizetype transformSequence_count(QQmlListProperty<QQuickTransform> *);
    static QQuickTransform *transformSequence_at(QQmlListProperty<QQuickTransform> *, qsizetype);
    static void transformSequence_clear(QQmlListProperty<QQuickTransform> *);

    enum StateFlag {
        Disabled = 1,
        Override = 2
    };

    QQuickItem *m_dummyItem = nullptr;
    QList<QQuickTransform *> m_transforms;
    QHash<QQuickTransform *, int> m_transformFlags;
};

QT_END_NAMESPACE

#endif // QQUICKTRANSFORMGROUP_P_H
