// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QUICKTEST_P_H
#define QUICKTEST_P_H

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
#include <QtQuickTest/quicktest.h>

#include <QtQml/qqmlpropertymap.h>
#include <QtQml/qqml.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class Q_QMLTEST_EXPORT QTestRootObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool windowShown READ windowShown NOTIFY windowShownChanged)
    Q_PROPERTY(bool hasTestCase READ hasTestCase WRITE setHasTestCase NOTIFY hasTestCaseChanged)
    Q_PROPERTY(QObject *defined READ defined)

    QML_SINGLETON
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 0)

public:
    static QTestRootObject *create(QQmlEngine *q, QJSEngine *)
    {
        QTestRootObject *result = instance();
        QQmlEngine *engine = qmlEngine(result);
        // You can only test on one engine at a time
        return (engine == nullptr || engine == q) ? result : nullptr;
    }

    static QTestRootObject *instance() {
        static QPointer<QTestRootObject> object = new QTestRootObject;
        if (!object) {
            // QTestRootObject was deleted when previous test ended, create a new one
            object = new QTestRootObject;
        }
        return object;
    }

    bool hasQuit:1;
    bool hasTestCase() const { return m_hasTestCase; }
    void setHasTestCase(bool value) { m_hasTestCase = value; Q_EMIT hasTestCaseChanged(); }

    bool windowShown() const { return m_windowShown; }
    void setWindowShown(bool value) { m_windowShown = value; Q_EMIT windowShownChanged(); }
    QQmlPropertyMap *defined() const { return m_defined; }

    void init() { setWindowShown(false); setHasTestCase(false); hasQuit = false; }

Q_SIGNALS:
    void windowShownChanged();
    void hasTestCaseChanged();

private Q_SLOTS:
    void quit() { hasQuit = true; }

private:
    QTestRootObject(QObject *parent = nullptr)
        : QObject(parent), hasQuit(false), m_windowShown(false), m_hasTestCase(false)  {
        m_defined = new QQmlPropertyMap(this);
#if defined(QT_OPENGL_ES_2_ANGLE)
        m_defined->insert(QLatin1String("QT_OPENGL_ES_2_ANGLE"), QVariant(true));
#endif
    }

    bool m_windowShown : 1;
    bool m_hasTestCase :1;
    QQmlPropertyMap *m_defined;
};

bool qWaitForSignal(QObject *obj, const char* signal, int timeout = 5000);

QT_END_NAMESPACE

#endif // QUICKTEST_P_H
