// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTIMER_H
#define QQMLTIMER_H

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

#include <private/qtqmlglobal_p.h>

#include <QtQmlMeta/qtqmlmetaexports.h>
#include <QtQml/qqml.h>
#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class QQmlTimerPrivate;
class Q_QMLMETA_EXPORT QQmlTimer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlTimer)
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool repeat READ isRepeating WRITE setRepeating NOTIFY repeatChanged)
    Q_PROPERTY(bool triggeredOnStart READ triggeredOnStart WRITE setTriggeredOnStart NOTIFY triggeredOnStartChanged)
    Q_PROPERTY(QObject *parent READ parent CONSTANT)
    Q_CLASSINFO("ParentProperty", "parent")
    QML_NAMED_ELEMENT(Timer)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQmlTimer(QObject *parent=nullptr);

    void setInterval(int interval);
    int interval() const;

    bool isRunning() const;
    void setRunning(bool running);

    bool isRepeating() const;
    void setRepeating(bool repeating);

    bool triggeredOnStart() const;
    void setTriggeredOnStart(bool triggeredOnStart);

protected:
    void classBegin() override;
    void componentComplete() override;

    bool event(QEvent *) override;

public Q_SLOTS:
    void start();
    void stop();
    void restart();

Q_SIGNALS:
    void triggered();
    void runningChanged();
    void intervalChanged();
    void repeatChanged();
    void triggeredOnStartChanged();

private:
    void update();

private Q_SLOTS:
    void ticked();
};

QT_END_NAMESPACE

#endif
