// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTY_H
#define QQMLPROPERTY_H

#include <QtCore/qstring.h>
#include <QtCore/qhashfunctions.h>
#include <QtQml/qtqmlglobal.h>
#include <QtCore/qmetaobject.h>
#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE


class QObject;
class QVariant;
class QQmlContext;
class QQmlEngine;

class QQmlPropertyPrivate;
class Q_QML_EXPORT QQmlProperty
{
    Q_GADGET
    QML_ANONYMOUS

    Q_PROPERTY(QObject *object READ object CONSTANT FINAL)
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
public:
    enum PropertyTypeCategory {
        InvalidCategory,
        List,
        Object,
        Normal
    };

    enum Type {
        Invalid,
        Property,
        SignalProperty
    };

    QQmlProperty();
    ~QQmlProperty();

    QQmlProperty(QObject *);
    QQmlProperty(QObject *, QQmlContext *);
    QQmlProperty(QObject *, QQmlEngine *);

    QQmlProperty(QObject *, const QString &);
    QQmlProperty(QObject *, const QString &, QQmlContext *);
    QQmlProperty(QObject *, const QString &, QQmlEngine *);

    QQmlProperty(const QQmlProperty &);
    QQmlProperty &operator=(const QQmlProperty &);

    QQmlProperty(QQmlProperty &&other) noexcept : d(std::exchange(other.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QQmlProperty)

    void swap(QQmlProperty &other) noexcept { qt_ptr_swap(d, other.d); }
    bool operator==(const QQmlProperty &) const;

    Type type() const;
    bool isValid() const;
    bool isProperty() const;
    bool isSignalProperty() const;

    int propertyType() const;
    QMetaType propertyMetaType() const;
    PropertyTypeCategory propertyTypeCategory() const;
    const char *propertyTypeName() const;

    QString name() const;

    QVariant read() const;
    static QVariant read(const QObject *, const QString &);
    static QVariant read(const QObject *, const QString &, QQmlContext *);
    static QVariant read(const QObject *, const QString &, QQmlEngine *);

    bool write(const QVariant &) const;
    static bool write(QObject *, const QString &, const QVariant &);
    static bool write(QObject *, const QString &, const QVariant &, QQmlContext *);
    static bool write(QObject *, const QString &, const QVariant &, QQmlEngine *);

    bool reset() const;

    bool hasNotifySignal() const;
    bool needsNotifySignal() const;
    bool connectNotifySignal(QObject *dest, const char *slot) const;
    bool connectNotifySignal(QObject *dest, int method) const;

    bool isWritable() const;
    bool isBindable() const;
    bool isDesignable() const;
    bool isResettable() const;
    QObject *object() const;

    int index() const;
    QMetaProperty property() const;
    QMetaMethod method() const;

private:
    friend class QQmlPropertyPrivate;
    QQmlPropertyPrivate *d = nullptr;
};
typedef QList<QQmlProperty> QQmlProperties;

inline size_t qHash (const QQmlProperty &key, size_t seed = 0)
{
    return qHashMulti(seed, key.object(), key.name());
}

Q_DECLARE_TYPEINFO(QQmlProperty, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QQMLPROPERTY_H
