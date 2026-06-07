// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTITEMMODELTESTER_H
#define QABSTRACTITEMMODELTESTER_H

#include <QtCore/QObject>
#include <QtTest/qttestglobal.h>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QVariant>

#ifdef QT_GUI_LIB
#include <QtGui/QFont>
#include <QtGui/QColor>
#include <QtGui/QBrush>
#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtGui/QIcon>
#endif

QT_REQUIRE_CONFIG(itemmodeltester);

QT_BEGIN_NAMESPACE

class QAbstractItemModel;
class QAbstractItemModelTester;
class QAbstractItemModelTesterPrivate;

namespace QTestPrivate {
inline bool testDataGuiRoles(QAbstractItemModelTester *tester);
}

class Q_TESTLIB_EXPORT QAbstractItemModelTester : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstractItemModelTester)

public:
    enum class FailureReportingMode {
        QtTest,
        Warning,
        Fatal
    };

    QAbstractItemModelTester(QAbstractItemModel *model, QObject *parent = nullptr);
    QAbstractItemModelTester(QAbstractItemModel *model, FailureReportingMode mode, QObject *parent = nullptr);

    QAbstractItemModel *model() const;
    FailureReportingMode failureReportingMode() const;
    void setUseFetchMore(bool value);

private:
    bool verify(bool statement, const char *statementStr, const char *description, const char *file, int line);
};

QT_END_NAMESPACE

#endif // QABSTRACTITEMMODELTESTER_H
