// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYMAP_H
#define QQMLPROPERTYMAP_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlregistration.h>

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE


class QQmlPropertyMapPrivate;
class Q_QML_EXPORT QQmlPropertyMap : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit QQmlPropertyMap(QObject *parent = nullptr);
    ~QQmlPropertyMap() override;

    QVariant value(const QString &key) const;
    void insert(const QString &key, const QVariant &value);
    void insert(const QVariantHash &values);
    void clear(const QString &key);
    void freeze();

    Q_INVOKABLE QStringList keys() const;

    int count() const;
    int size() const;
    bool isEmpty() const;
    bool contains(const QString &key) const;

    QVariant &operator[](const QString &key);
    QVariant operator[](const QString &key) const;

Q_SIGNALS:
    void valueChanged(const QString &key, const QVariant &value);

protected:
    virtual QVariant updateValue(const QString &key, const QVariant &input);

    template<class DerivedType>
    QQmlPropertyMap(DerivedType *derived, QObject *parentObj)
        : QQmlPropertyMap(&DerivedType::staticMetaObject, parentObj)
    {
        Q_UNUSED(derived);
    }

private:
    QQmlPropertyMap(const QMetaObject *staticMetaObject, QObject *parent);

    Q_DECLARE_PRIVATE(QQmlPropertyMap)
    Q_DISABLE_COPY(QQmlPropertyMap)
};

QT_END_NAMESPACE

#endif
