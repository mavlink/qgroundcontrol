// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLSETTINGS_P_H
#define QQMLSETTINGS_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qurl.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQmlCore/private/qqmlcoreglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlSettingsPrivate;

class Q_QMLCORE_EXPORT QQmlSettings : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_DECLARE_PRIVATE(QQmlSettings)
    QML_NAMED_ELEMENT(Settings)
    QML_ADDED_IN_VERSION(6, 5)

    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged FINAL)
    Q_PROPERTY(QUrl location READ location WRITE setLocation NOTIFY locationChanged FINAL)

public:
    explicit QQmlSettings(QObject *parent = nullptr);
    ~QQmlSettings() override;

    QString category() const;
    void setCategory(const QString &category);

    QUrl location() const;
    void setLocation(const QUrl &location);

    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void sync();

Q_SIGNALS:
    void categoryChanged(const QString &arg);
    void locationChanged(const QUrl &arg);

protected:
    void timerEvent(QTimerEvent *event) override;

    void classBegin() override;
    void componentComplete() override;

private:
    QScopedPointer<QQmlSettingsPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_propertyChanged())
};

QT_END_NAMESPACE

#endif // QQMLSETTINGS_P_H
