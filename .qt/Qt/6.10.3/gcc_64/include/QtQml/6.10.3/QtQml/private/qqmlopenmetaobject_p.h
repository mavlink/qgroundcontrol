// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLOPENMETAOBJECT_H
#define QQMLOPENMETAOBJECT_H

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

#include <QtCore/QMetaObject>
#include <QtCore/QObject>

#include <private/qobject_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qtqmlglobal_p.h>
#include <private/qqmlpropertycache_p.h>

QT_BEGIN_NAMESPACE


class QQmlEngine;
class QMetaPropertyBuilder;
class QQmlOpenMetaObjectTypePrivate;
class Q_QML_EXPORT QQmlOpenMetaObjectType final
    : public QQmlRefCounted<QQmlOpenMetaObjectType>
{
public:
    QQmlOpenMetaObjectType(const QMetaObject *base);
    ~QQmlOpenMetaObjectType();

    void createProperties(const QVector<QByteArray> &names);
    int createProperty(const QByteArray &name);

    int propertyOffset() const;
    int signalOffset() const;

    int propertyCount() const;
    QByteArray propertyName(int) const;

    QQmlPropertyCache::Ptr cache() const;

private:
    void propertyCreated(int, QMetaPropertyBuilder &);

    QQmlOpenMetaObjectTypePrivate *d;
    friend class QQmlOpenMetaObject;
    friend class QQmlOpenMetaObjectPrivate;
};

class QQmlOpenMetaObjectPrivate;
class Q_QML_EXPORT QQmlOpenMetaObject : public QAbstractDynamicMetaObject
{
public:
    QQmlOpenMetaObject(QObject *, const QMetaObject * = nullptr);
    QQmlOpenMetaObject(QObject *, const QQmlRefPointer<QQmlOpenMetaObjectType> &);
    ~QQmlOpenMetaObject() override;

    QVariant value(const QByteArray &) const;
    bool setValue(const QByteArray &, const QVariant &, bool force = false);
    void setValues(const QHash<QByteArray, QVariant> &, bool force = false);
    QVariant value(int) const;
    void setValue(int, const QVariant &);
    QVariant &valueRef(const QByteArray &);
    bool hasValue(int) const;

    int count() const;
    QByteArray name(int) const;

    QObject *object() const;
    virtual QVariant initialValue(int);

    // Be careful - once setCached(true) is called createProperty() is no
    // longer automatically called for new properties.
    void setCached(bool);

    bool autoCreatesProperties() const;
    void setAutoCreatesProperties(bool autoCreate);

    QQmlOpenMetaObjectType *type() const;
    QDynamicMetaObjectData *parent() const;

    void emitPropertyNotification(const QByteArray &propertyName);
    void unparent();

protected:
    int metaCall(QObject *o, QMetaObject::Call _c, int _id, void **_a) override;
    int createProperty(const char *, const char *) override;

    virtual void propertyRead(int);
    virtual void propertyWrite(int);
    virtual QVariant propertyWriteValue(int, const QVariant &);
    virtual void propertyWritten(int);
    virtual void propertyCreated(int, QMetaPropertyBuilder &);


    bool checkedSetValue(int index, const QVariant &value, bool force);

private:
    QQmlOpenMetaObjectPrivate *d;
    friend class QQmlOpenMetaObjectType;
};

QT_END_NAMESPACE

#endif // QQMLOPENMETAOBJECT_H
