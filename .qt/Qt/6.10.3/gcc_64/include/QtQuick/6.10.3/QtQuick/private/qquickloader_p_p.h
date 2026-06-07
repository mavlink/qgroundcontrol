// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLOADER_P_P_H
#define QQUICKLOADER_P_P_H

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

#include "qquickloader_p.h"
#include "qquickimplicitsizeitem_p_p.h"
#include "qquickitemchangelistener_p.h"
#include <qqmlincubator.h>

#include <private/qv4staticvalue_p.h>
#include <private/qv4persistent_p.h>

QT_BEGIN_NAMESPACE


class QQuickLoaderPrivate;
class QQuickLoaderIncubator : public QQmlIncubator
{
public:
    QQuickLoaderIncubator(QQuickLoaderPrivate *l, IncubationMode mode) : QQmlIncubator(mode), loader(l) {}

protected:
    void statusChanged(Status) override;
    void setInitialState(QObject *) override;

private:
    QQuickLoaderPrivate *loader;
};

class QQmlContext;
class QQuickLoaderPrivate : public QQuickImplicitSizeItemPrivate,
                            public QSafeQuickItemChangeListener<QQuickLoaderPrivate>
{
    Q_DECLARE_PUBLIC(QQuickLoader)

public:
    QQuickLoaderPrivate();
    ~QQuickLoaderPrivate();

    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &oldGeometry) override;
    void itemImplicitWidthChanged(QQuickItem *) override;
    void itemImplicitHeightChanged(QQuickItem *) override;
    void clear();
    void initResize();
    void load();

    void incubatorStateChanged(QQmlIncubator::Status status);
    void setInitialState(QObject *o);
    void disposeInitialPropertyValues();
    QQuickLoader::Status computeStatus() const;
    void updateStatus();
    void createComponent();

    qreal getImplicitWidth() const override;
    qreal getImplicitHeight() const override;

    QUrl source;
    QQuickItem *item;
    QObject *object;
    QQmlStrongJSQObjectReference<QQmlComponent> component;
    QQmlContext *itemContext;
    QQuickLoaderIncubator *incubator;
    QV4::PersistentValue initialPropertyValues;
    QV4::PersistentValue qmlCallingContext;
    bool updatingSize: 1;
    bool active : 1;
    bool loadingFromSource : 1;
    bool asynchronous : 1;
    // We need to use char instead of QQuickLoader::Status
    // as otherwise the size of the class would increase
    // on 32-bit systems, as sizeof(Status) == sizeof(int)
    // and sizeof(int) > remaining padding on 32 bit
    char status;

    void _q_sourceLoaded();
    void _q_updateSize(bool loaderGeometryChanged = true);
};

QT_END_NAMESPACE

#endif // QQUICKLOADER_P_P_H
