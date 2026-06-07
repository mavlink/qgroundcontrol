// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMTHEMEPLUGIN_H
#define QPLATFORMTHEMEPLUGIN_H

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

class QPlatformTheme;

#define QPlatformThemeFactoryInterface_iid "org.qt-project.Qt.QPA.QPlatformThemeFactoryInterface.5.1"

class Q_GUI_EXPORT QPlatformThemePlugin : public QObject
{
    Q_OBJECT
public:
    explicit QPlatformThemePlugin(QObject *parent = nullptr);
    ~QPlatformThemePlugin();

    virtual QPlatformTheme *create(const QString &key, const QStringList &paramList) = 0;
};

QT_END_NAMESPACE

#endif // QPLATFORMTHEMEPLUGIN_H
