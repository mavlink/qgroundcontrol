// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQUICKLISTMODELWORKERAGENT_P_H
#define QQUICKLISTMODELWORKERAGENT_P_H

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

#include <qtqmlmodelsglobal_p.h>

#include <QEvent>
#include <QMutex>
#include <QWaitCondition>
#include <QtQml/qqml.h>

#include <private/qv4engine_p.h>

QT_REQUIRE_CONFIG(qml_list_model);

QT_BEGIN_NAMESPACE


class QQmlListModel;

class QQmlListModelWorkerAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count FINAL)
    Q_PROPERTY(QQmlV4ExecutionEnginePtr engine READ engine WRITE setEngine NOTIFY engineChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQmlListModelWorkerAgent(QQmlListModel *);
    ~QQmlListModelWorkerAgent();

    QV4::ExecutionEngine *engine() const;
    void setEngine(QV4::ExecutionEngine *eng);

    Q_INVOKABLE void addref();
    Q_INVOKABLE void release();

    int count() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void remove(QQmlV4FunctionPtr args);
    Q_INVOKABLE void append(QQmlV4FunctionPtr args);
    Q_INVOKABLE void insert(QQmlV4FunctionPtr args);
    Q_INVOKABLE QJSValue get(int index) const;
    Q_INVOKABLE void set(int index, const QJSValue &value);
    Q_INVOKABLE void setProperty(int index, const QString& property, const QVariant& value);
    Q_INVOKABLE void move(int from, int to, int count);
    Q_INVOKABLE void sync();

    void modelDestroyed();

Q_SIGNALS:
    void engineChanged(QQmlV4ExecutionEnginePtr engine);

protected:
    bool event(QEvent *) override;

private:
    friend class QQuickWorkerScriptEnginePrivate;
    friend class QQmlListModel;

    struct Sync : public QEvent {
        Sync(QQmlListModel *l)
            : QEvent(QEvent::User)
            , list(l)
        {}
        ~Sync();
        QQmlListModel *list;
    };

    QAtomicInt m_ref;
    QQmlListModel *m_orig;
    QQmlListModel *m_copy;
    QMutex mutex;
    QWaitCondition syncDone;
};

QT_END_NAMESPACE

#endif // QQUICKLISTMODELWORKERAGENT_P_H

