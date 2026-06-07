// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPAGESETUPDIALOG_P_H
#define QPAGESETUPDIALOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>

#include "private/qdialog_p.h"

#include "qbytearray.h"
#include "qpagesetupdialog.h"
#include "qpointer.h"

QT_REQUIRE_CONFIG(printdialog);

QT_BEGIN_NAMESPACE

class QPrinter;

class QPageSetupDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QPageSetupDialog)

public:
    explicit QPageSetupDialogPrivate(QPrinter *printer);

    void setPrinter(QPrinter *newPrinter);

    QPrinter *printer;
    bool ownsPrinter;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
};

QT_END_NAMESPACE

#endif // QPAGESETUPDIALOG_P_H
