// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPLATFORMPRINTPLUGIN_H
#define QPLATFORMPRINTPLUGIN_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

#ifndef QT_NO_PRINTER

QT_BEGIN_NAMESPACE


class QPlatformPrinterSupport;

#define QPlatformPrinterSupportFactoryInterface_iid "org.qt-project.QPlatformPrinterSupportFactoryInterface.5.1"

class Q_PRINTSUPPORT_EXPORT QPlatformPrinterSupportPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QPlatformPrinterSupportPlugin(QObject *parent = nullptr);
    ~QPlatformPrinterSupportPlugin();

    virtual QPlatformPrinterSupport *create(const QString &key) = 0;

    static QPlatformPrinterSupport *get();
};

QT_END_NAMESPACE

#endif

#endif // QPLATFORMPRINTPLUGIN_H
