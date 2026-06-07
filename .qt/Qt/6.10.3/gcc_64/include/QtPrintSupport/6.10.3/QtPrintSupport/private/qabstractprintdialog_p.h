// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTPRINTDIALOG_P_H
#define QABSTRACTPRINTDIALOG_P_H

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

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>

#include "private/qdialog_p.h"
#include "QtPrintSupport/qabstractprintdialog.h"

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(printdialog);

QT_BEGIN_NAMESPACE

class QPrinter;
class QPrinterPrivate;

class QAbstractPrintDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QAbstractPrintDialog)

public:
    QAbstractPrintDialogPrivate()
        : printer(nullptr), pd(nullptr)
        , options(QAbstractPrintDialog::PrintToFile | QAbstractPrintDialog::PrintPageRange |
                QAbstractPrintDialog::PrintCollateCopies | QAbstractPrintDialog::PrintShowPageSize),
          minPage(0), maxPage(INT_MAX), ownsPrinter(false)
    {
    }

    QPrinter *printer;
    QPrinterPrivate *pd;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;

    QAbstractPrintDialog::PrintDialogOptions options;

    virtual void setTabs(const QList<QWidget *> &) {}
    void setPrinter(QPrinter *newPrinter);
    int minPage;
    int maxPage;

    bool ownsPrinter;
};

QT_END_NAMESPACE

#endif // QABSTRACTPRINTDIALOG_P_H
