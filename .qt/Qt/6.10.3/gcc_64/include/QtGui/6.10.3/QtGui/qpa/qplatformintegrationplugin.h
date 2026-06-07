// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATIONPLUGIN_H
#define QPLATFORMINTEGRATIONPLUGIN_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE


class QPlatformIntegration;

#define QPlatformIntegrationFactoryInterface_iid "org.qt-project.Qt.QPA.QPlatformIntegrationFactoryInterface.5.3"

class Q_GUI_EXPORT QPlatformIntegrationPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QPlatformIntegrationPlugin(QObject *parent = nullptr);
    ~QPlatformIntegrationPlugin();

    virtual QPlatformIntegration *create(const QString &key, const QStringList &paramList);
    virtual QPlatformIntegration *create(const QString &key, const QStringList &paramList, int &argc, char **argv);
};

QT_END_NAMESPACE

#endif // QPLATFORMINTEGRATIONPLUGIN_H
