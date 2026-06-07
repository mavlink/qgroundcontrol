// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLSETTINGS_P_H
#define QQMLSETTINGS_P_H

#include "qqmlsettingsglobal_p.h"

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

#include <QtQml/qqml.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtQml/qqmlparserstatus.h>

QT_BEGIN_NAMESPACE

class QQmlSettingsLabsPrivate;

class Q_LABSSETTINGS_EXPORT QQmlSettingsLabs : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString category READ category WRITE setCategory FINAL)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName FINAL)
    QML_NAMED_ELEMENT(Settings)
    QML_ADDED_IN_VERSION(1, 0)

public:
    explicit QQmlSettingsLabs(QObject *parent = nullptr);
    ~QQmlSettingsLabs();

    QString category() const;
    void setCategory(const QString &category);

    QString fileName() const;
    void setFileName(const QString &fileName);

    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void sync();

protected:
    void timerEvent(QTimerEvent *event) override;

    void classBegin() override;
    void componentComplete() override;

private:
    Q_DISABLE_COPY(QQmlSettingsLabs)
    Q_DECLARE_PRIVATE(QQmlSettingsLabs)
    QScopedPointer<QQmlSettingsLabsPrivate> d_ptr;
    Q_PRIVATE_SLOT(d_func(), void _q_propertyChanged())
};

QT_END_NAMESPACE

#endif // QQMLSETTINGS_P_H
