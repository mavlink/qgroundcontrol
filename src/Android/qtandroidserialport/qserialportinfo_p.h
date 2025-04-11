// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2017 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSERIALPORTINFO_P_H
#define QSERIALPORTINFO_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QSerialPortInfoPrivate
{
public:
    static QString portNameToSystemLocation(const QString &source);
    static QString portNameFromSystemLocation(const QString &source);

    QString portName;
    QString device;
    QString description;
    QString manufacturer;
    QString serialNumber;

    quint16 vendorIdentifier = 0;
    quint16 productIdentifier = 0;

    bool hasVendorIdentifier = false;
    bool hasProductIdentifier = false;
};

QT_END_NAMESPACE

#endif // QSERIALPORTINFO_P_H
