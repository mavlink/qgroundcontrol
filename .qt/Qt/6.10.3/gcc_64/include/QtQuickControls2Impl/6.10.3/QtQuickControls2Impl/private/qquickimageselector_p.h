// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKIMAGESELECTOR_P_H
#define QQUICKIMAGESELECTOR_P_H

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

#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/private/qqmlpropertyvalueinterceptor_p.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickImageSelector : public QObject, public QQmlParserStatus, public QQmlPropertyValueInterceptor
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QString name READ name WRITE setName FINAL)
    Q_PROPERTY(QString path READ path WRITE setPath FINAL)
    Q_PROPERTY(QVariantList states READ states WRITE setStates FINAL)
    Q_PROPERTY(QString separator READ separator WRITE setSeparator FINAL)
    Q_PROPERTY(bool cache READ cache WRITE setCache FINAL)
    Q_INTERFACES(QQmlParserStatus QQmlPropertyValueInterceptor)
    QML_NAMED_ELEMENT(ImageSelector)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickImageSelector(QObject *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &source);

    QString name() const;
    void setName(const QString &name);

    QString path() const;
    void setPath(const QString &path);

    QVariantList states() const;
    void setStates(const QVariantList &states);

    QString separator() const;
    void setSeparator(const QString &separator);

    bool cache() const;
    void setCache(bool cache);

    void write(const QVariant &value) override;
    void setTarget(const QQmlProperty &property) override;

Q_SIGNALS:
    void sourceChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    virtual QStringList fileExtensions() const;

    QString cacheKey() const;
    void updateSource();
    void setUrl(const QUrl &url);
    bool updateActiveStates();
    int calculateScore(const QStringList &states) const;

private:
    bool m_cache = false;
    bool m_complete = false;
    QUrl m_source;
    QString m_path;
    QString m_name;
    QString m_separator = QLatin1String("-");
    QVariantList m_allStates;
    QStringList m_activeStates;
    QQmlProperty m_property;
};

class QQuickNinePatchImageSelector : public QQuickImageSelector
{
    Q_OBJECT
    QML_NAMED_ELEMENT(NinePatchImageSelector)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickNinePatchImageSelector(QObject *parent = nullptr);

protected:
    QStringList fileExtensions() const override;
};

class QQuickAnimatedImageSelector : public QQuickImageSelector
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AnimatedImageSelector)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickAnimatedImageSelector(QObject *parent = nullptr);

protected:
    QStringList fileExtensions() const override;
};

QT_END_NAMESPACE

#endif // QQUICKIMAGESELECTOR_P_H
