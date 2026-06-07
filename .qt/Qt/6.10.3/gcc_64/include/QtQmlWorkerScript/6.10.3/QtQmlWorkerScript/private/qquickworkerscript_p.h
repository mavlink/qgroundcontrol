// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQUICKWORKERSCRIPT_P_H
#define QQUICKWORKERSCRIPT_P_H

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

#include <qqml.h>

#include <QtQmlWorkerScript/private/qtqmlworkerscriptglobal_p.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtCore/qthread.h>
#include <QtQml/qjsvalue.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE


class QQuickWorkerScript;
class QQuickWorkerScriptEnginePrivate;
class QQuickWorkerScriptEngine : public QThread
{
Q_OBJECT
public:
    QQuickWorkerScriptEngine(QQmlEngine *parent = nullptr);
    ~QQuickWorkerScriptEngine();

    int registerWorkerScript(QQuickWorkerScript *);
    void removeWorkerScript(int);
    void executeUrl(int, const QUrl &);
    void sendMessage(int, const QByteArray &);

protected:
    void run() override;

private:
    QQuickWorkerScriptEnginePrivate *d;
};

class Q_QMLWORKERSCRIPT_EXPORT QQuickWorkerScript : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QQuickWorkerScript)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged REVISION(2, 15))

    QML_NAMED_ELEMENT(WorkerScript);
    QML_ADDED_IN_VERSION(2, 0)

    Q_INTERFACES(QQmlParserStatus)
public:
    QQuickWorkerScript(QObject *parent = nullptr);
    ~QQuickWorkerScript();

    QUrl source() const;
    void setSource(const QUrl &);

    bool ready() const;

public Q_SLOTS:
    void sendMessage(QQmlV4FunctionPtr);

Q_SIGNALS:
    void sourceChanged();
    Q_REVISION(2, 15) void readyChanged();
    void message(const QJSValue &messageObject);

protected:
    void classBegin() override;
    void componentComplete() override;
    bool event(QEvent *) override;

private:
    QQuickWorkerScriptEngine *engine();
    QQuickWorkerScriptEngine *m_engine;
    int m_scriptId;
    QUrl m_source;
    bool m_componentComplete;
};

QT_END_NAMESPACE

#endif // QQUICKWORKERSCRIPT_P_H
