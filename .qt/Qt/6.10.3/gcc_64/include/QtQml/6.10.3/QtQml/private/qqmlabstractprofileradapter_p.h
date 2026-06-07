// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLABSTRACTPROFILERADAPTER_P_H
#define QQMLABSTRACTPROFILERADAPTER_P_H

#include <private/qtqmlglobal_p.h>
#include <private/qqmlprofilerdefinitions_p.h>

#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>

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

QT_BEGIN_NAMESPACE

QT_REQUIRE_CONFIG(qml_debug);

class QQmlProfilerService;
class Q_QML_EXPORT QQmlAbstractProfilerAdapter : public QObject, public QQmlProfilerDefinitions {
    Q_OBJECT

public:
    static const int s_numMessagesPerBatch = 1000;

    QQmlAbstractProfilerAdapter(QObject *parent = nullptr) :
        QObject(parent), service(nullptr), waiting(true), featuresEnabled(0) {}
    ~QQmlAbstractProfilerAdapter() override {}
    void setService(QQmlProfilerService *new_service) { service = new_service; }

    virtual qint64 sendMessages(qint64 until, QList<QByteArray> &messages) = 0;

    void startProfiling(quint64 features);

    void stopProfiling();

    void reportData() { Q_EMIT dataRequested(); }

    void stopWaiting() { waiting = false; }
    void startWaiting() { waiting = true; }

    bool isRunning() const { return featuresEnabled != 0; }
    quint64 features() const { return featuresEnabled; }

    void synchronize(const QElapsedTimer &t) { Q_EMIT referenceTimeKnown(t); }

Q_SIGNALS:
    void profilingEnabled(quint64 features);
    void profilingEnabledWhileWaiting(quint64 features);

    void profilingDisabled();
    void profilingDisabledWhileWaiting();

    void dataRequested();
    void referenceTimeKnown(const QElapsedTimer &timer);

protected:
    QQmlProfilerService *service;

private:
    bool waiting;
    quint64 featuresEnabled;
};

class Q_QML_EXPORT QQmlAbstractProfilerAdapterFactory : public QObject
{
    Q_OBJECT
public:
    virtual QQmlAbstractProfilerAdapter *create(const QString &key) = 0;
};

#define QQmlAbstractProfilerAdapterFactory_iid "org.qt-project.Qt.QQmlAbstractProfilerAdapterFactory"

QT_END_NAMESPACE

#endif // QQMLABSTRACTPROFILERADAPTER_P_H
