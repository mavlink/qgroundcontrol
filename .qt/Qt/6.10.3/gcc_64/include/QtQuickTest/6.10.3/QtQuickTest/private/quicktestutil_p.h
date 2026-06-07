// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QUICKTESTUTIL_P_H
#define QUICKTESTUTIL_P_H

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

#include <QtQuickTest/private/quicktestglobal_p.h>

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>
#include <QtQml/private/qqmlsignalnames_p.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

class Q_QMLTEST_EXPORT QuickTestUtil : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool printAvailableFunctions READ printAvailableFunctions NOTIFY printAvailableFunctionsChanged)
    Q_PROPERTY(int dragThreshold READ dragThreshold NOTIFY dragThresholdChanged)
    QML_NAMED_ELEMENT(TestUtil)
    QML_ADDED_IN_VERSION(1, 0)
public:
    QuickTestUtil(QObject *parent = nullptr) :QObject(parent) {}
    ~QuickTestUtil() override {}

    bool printAvailableFunctions() const;
    int dragThreshold() const;

    Q_INVOKABLE void populateClipboardText(int lineCount);

Q_SIGNALS:
    void printAvailableFunctionsChanged();
    void dragThresholdChanged();

public Q_SLOTS:

    QJSValue typeName(const QVariant& v) const;
    bool compare(const QVariant& act, const QVariant& exp) const;

    QJSValue callerFile(int frameIndex = 0) const;
    int callerLine(int frameIndex = 0) const;

    Q_REVISION(6, 7) QString signalHandlerName(const QString &signalName)
    {
        if (QQmlSignalNames::isHandlerName(signalName))
            return signalName;
        return QQmlSignalNames::signalNameToHandlerName(signalName);
    }
};

QT_END_NAMESPACE

#endif // QUICKTESTUTIL_P_H
