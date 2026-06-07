// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPLATFORMPRINTERSUPPORT_H
#define QPLATFORMPRINTERSUPPORT_H
//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QtPrintSupport/qprinter.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

typedef QHash<QString, QString> PrinterOptions;

class QPageSize;
class QPlatformPrintDevice;
class QPrintDevice;
class QPrintEngine;

class Q_PRINTSUPPORT_EXPORT QPlatformPrinterSupport
{
public:
    QPlatformPrinterSupport();
    virtual ~QPlatformPrinterSupport();

    virtual QPrintEngine *createNativePrintEngine(QPrinter::PrinterMode printerMode, const QString &deviceId = QString());
    virtual QPaintEngine *createPaintEngine(QPrintEngine *, QPrinter::PrinterMode printerMode);

    virtual QPrintDevice createPrintDevice(const QString &id);
    virtual QStringList availablePrintDeviceIds() const;
    virtual QString defaultPrintDeviceId() const;

protected:
    static QPrintDevice createPrintDevice(QPlatformPrintDevice *device);
    static QPageSize createPageSize(const QString &id, QSize size, const QString &localizedName);
};

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

#endif // QPLATFORMPRINTERSUPPORT_H
