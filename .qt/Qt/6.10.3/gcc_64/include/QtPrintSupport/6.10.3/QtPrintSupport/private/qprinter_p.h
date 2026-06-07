// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPRINTER_P_H
#define QPRINTER_P_H

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

#ifndef QT_NO_PRINTER

#include "QtPrintSupport/qprinter.h"
#include "QtPrintSupport/qprinterinfo.h"
#include "QtPrintSupport/qprintengine.h"
#include "QtCore/qpointer.h"
#include "QtCore/qset.h"

#include <limits.h>

QT_BEGIN_NAMESPACE

class QPrintEngine;
class QPreviewPaintEngine;
class QPicture;

class Q_PRINTSUPPORT_EXPORT QPrinterPrivate
{
    Q_DECLARE_PUBLIC(QPrinter)
public:
    QPrinterPrivate(QPrinter *printer)
        : pdfVersion(QPrinter::PdfVersion_1_4),
          printEngine(nullptr),
          paintEngine(nullptr),
          realPrintEngine(nullptr),
          realPaintEngine(nullptr),
#if QT_CONFIG(printpreviewwidget)
          previewEngine(nullptr),
#endif
          q_ptr(printer),
          printRange(QPrinter::AllPages),
          use_default_engine(true),
          validPrinter(false)
    {
    }

    ~QPrinterPrivate() {

    }

    static QPrinterPrivate *get(QPrinter *printer) {
        return printer->d_ptr.get();
    }

    void init(const QPrinterInfo &printer, QPrinter::PrinterMode mode);

    QPrinterInfo findValidPrinter(const QPrinterInfo &printer = QPrinterInfo());
    void initEngines(QPrinter::OutputFormat format, const QPrinterInfo &printer);
    void changeEngines(QPrinter::OutputFormat format, const QPrinterInfo &printer);
#if QT_CONFIG(printpreviewwidget)
    QList<const QPicture *> previewPages() const;
    void setPreviewMode(bool);
    bool previewMode() const;
#endif

    void setProperty(QPrintEngine::PrintEnginePropertyKey key, const QVariant &value);

    QPrinter::PrinterMode printerMode;
    QPrinter::OutputFormat outputFormat;
    QPrinter::PdfVersion pdfVersion;
    QPrintEngine *printEngine;
    QPaintEngine *paintEngine;

    QPrintEngine *realPrintEngine;
    QPaintEngine *realPaintEngine;
#if QT_CONFIG(printpreviewwidget)
    QPreviewPaintEngine *previewEngine;
#endif

    QPrinter *q_ptr;

    QPrinter::PrintRange printRange;

    uint use_default_engine : 1;
    uint had_default_engines : 1;

    uint validPrinter : 1;
    uint hasCustomPageMargins : 1;

    // Used to remember which properties have been manually set by the user.
    QSet<QPrintEngine::PrintEnginePropertyKey> m_properties;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTER_P_H
