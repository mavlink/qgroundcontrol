// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXMLTESTLOGGER_P_H
#define QXMLTESTLOGGER_P_H

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


#include <QtTest/private/qabstracttestlogger_p.h>

QT_BEGIN_NAMESPACE

class QXmlTestLogger : public QAbstractTestLogger
{
public:
    enum XmlMode { Complete = 0, Light };

    QXmlTestLogger(XmlMode mode, const char *filename);
    ~QXmlTestLogger();

    void startLogging() override;
    void stopLogging() override;

    void enterTestFunction(const char *function) override;
    void leaveTestFunction() override;

    void addIncident(IncidentTypes type, const char *description,
                     const char *file = nullptr, int line = 0) override;
    void addBenchmarkResult(const QBenchmarkResult &result) override;

    void addMessage(MessageTypes type, const QString &message,
                    const char *file = nullptr, int line = 0) override;

    [[nodiscard]] static bool xmlCdata(QTestCharBuffer *dest, char const *src);
    [[nodiscard]] static bool xmlQuote(QTestCharBuffer *dest, char const *src);
private:
    [[nodiscard]] static int xmlCdata(QTestCharBuffer *dest, char const *src, qsizetype n);
    [[nodiscard]] static int xmlQuote(QTestCharBuffer *dest, char const *src, qsizetype n);

    XmlMode xmlmode;
};

QT_END_NAMESPACE

#endif
