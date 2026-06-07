// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINPUTCONTEXTPLUGIN_H
#define QPLATFORMINPUTCONTEXTPLUGIN_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE


class QPlatformInputContext;

#define QPlatformInputContextFactoryInterface_iid "org.qt-project.Qt.QPlatformInputContextFactoryInterface.5.1"

class Q_GUI_EXPORT QPlatformInputContextPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QPlatformInputContextPlugin(QObject *parent = nullptr);
    ~QPlatformInputContextPlugin();

    virtual QPlatformInputContext *create(const QString &key, const QStringList &paramList) = 0;
};

QT_END_NAMESPACE

#endif // QPLATFORMINPUTCONTEXTPLUGIN_H
