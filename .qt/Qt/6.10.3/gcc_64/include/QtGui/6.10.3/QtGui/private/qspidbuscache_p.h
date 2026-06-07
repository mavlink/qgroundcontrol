// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef Q_SPI_CACHE_H
#define Q_SPI_CACHE_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/QObject>
#include "qspi_struct_marshallers_p.h"

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

class QSpiDBusCache : public QObject
{
    Q_OBJECT

public:
    explicit QSpiDBusCache(QDBusConnection c, QObject* parent = nullptr);
    void emitAddAccessible(const QSpiAccessibleCacheItem& item);
    void emitRemoveAccessible(const QSpiObjectReference& item);

Q_SIGNALS:
    void AddAccessible(const QSpiAccessibleCacheItem &nodeAdded);
    void RemoveAccessible(const QSpiObjectReference &nodeRemoved);

public Q_SLOTS:
    QSpiAccessibleCacheArray GetItems();
};

QT_END_NAMESPACE

#endif /* Q_SPI_CACHE_H */
