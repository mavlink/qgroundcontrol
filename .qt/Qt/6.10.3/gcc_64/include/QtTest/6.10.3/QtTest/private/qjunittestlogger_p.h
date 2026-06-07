// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJUNITTESTLOGGER_P_H
#define QJUNITTESTLOGGER_P_H

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

#include <QtTest/qttestglobal.h>

#include <QtTest/private/qabstracttestlogger_p.h>
#include <QtTest/private/qtestelementattribute_p.h>
#include <QtCore/qmutex.h>

#include <vector>

QT_BEGIN_NAMESPACE

class QTestJUnitStreamer;
class QTestElement;

class QJUnitTestLogger : public QAbstractTestLogger
{
    public:
        QJUnitTestLogger(const char *filename);
        ~QJUnitTestLogger();

        void startLogging() override;
        void stopLogging() override;

        void enterTestFunction(const char *function) override;
        void leaveTestFunction() override;

        void enterTestData(QTestData *) override;

        void addIncident(IncidentTypes type, const char *description,
                     const char *file = nullptr, int line = 0) override;
        void addMessage(MessageTypes type, const QString &message,
                    const char *file = nullptr, int line = 0) override;

        void addBenchmarkResult(const QBenchmarkResult &) override {}

    private:
        void enterTestCase(const char *name);
        void leaveTestCase();

        void addFailure(QTest::LogElementType elementType,
            const char *failureType, const QString &failureDescription);

        QTestElement *currentTestSuite = nullptr;
        std::vector<QTestElement*> listOfTestcases;
        QTestElement *currentTestCase = nullptr;
        QTestElement *systemOutputElement = nullptr;
        QTestElement *systemErrorElement = nullptr;
        QTestJUnitStreamer *logFormatter = nullptr;
        // protects currentTestCase, systemOutputElement and systemErrorElement
        // in case of qDebug()/qWarning() etc. from threads
        QMutex mutex;

        int testCounter = 0;
        int failureCounter = 0;
        int errorCounter = 0;
};

QT_END_NAMESPACE

#endif // QJUNITTESTLOGGER_P_H
