// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSQLFIELD_H
#define QSQLFIELD_H

#include <QtSql/qtsqlglobal.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE


class QSqlFieldPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QSqlFieldPrivate, Q_SQL_EXPORT)

class Q_SQL_EXPORT QSqlField
{
    Q_GADGET
    Q_PROPERTY(QVariant value READ value WRITE setValue)
    Q_PROPERTY(QVariant defaultValue READ defaultValue WRITE setDefaultValue)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString tableName READ tableName WRITE setTableName)
    Q_PROPERTY(QMetaType metaType READ metaType WRITE setMetaType)
    Q_PROPERTY(RequiredStatus requiredStatus READ requiredStatus WRITE setRequiredStatus)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool generated READ isGenerated WRITE setGenerated)
    Q_PROPERTY(bool autoValue READ isAutoValue WRITE setAutoValue)
    Q_PROPERTY(int length READ length WRITE setLength)
    Q_PROPERTY(int precision READ precision WRITE setPrecision)

public:
    enum RequiredStatus { Unknown = -1, Optional = 0, Required = 1 };

    explicit QSqlField(const QString& fieldName = QString(), QMetaType type = QMetaType(), const QString &tableName = QString());

    QSqlField(const QSqlField& other);
    QSqlField& operator=(const QSqlField& other);
    QSqlField(QSqlField &&other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QSqlField)
    ~QSqlField();

    void swap(QSqlField &other) noexcept { val.swap(other.val); d.swap(other.d); }

    bool operator==(const QSqlField& other) const;
    inline bool operator!=(const QSqlField &other) const { return !operator==(other); }

    void setValue(const QVariant& value);
    inline QVariant value() const
    { return val; }
    void setName(const QString& name);
    QString name() const;
    void setTableName(const QString &tableName);
    QString tableName() const;
    bool isNull() const;
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    void clear();
    bool isAutoValue() const;

    QMetaType metaType() const;
    void setMetaType(QMetaType type);

#if QT_DEPRECATED_SINCE(6, 0)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QT_DEPRECATED_VERSION_X_6_0("Use the constructor using a QMetaType instead")
    QSqlField(const QString& fieldName, QVariant::Type type, const QString &tableName = QString())
        : QSqlField(fieldName, QMetaType(type), tableName)
    {}
    QT_DEPRECATED_VERSION_X_6_0("Use metaType() instead")
    QVariant::Type type() const { return QVariant::Type(metaType().id()); }
    QT_DEPRECATED_VERSION_X_6_0("Use setMetaType() instead")
    void setType(QVariant::Type type) { setMetaType(QMetaType(int(type))); }
    QT_WARNING_POP
#endif

    void setRequiredStatus(RequiredStatus status);
    inline void setRequired(bool required)
    { setRequiredStatus(required ? Required : Optional); }
    void setLength(int fieldLength);
    void setPrecision(int precision);
    void setDefaultValue(const QVariant &value);
#if QT_DEPRECATED_SINCE(6, 8)
    QT_DEPRECATED_VERSION_X_6_8("This internal value is no longer used.")
    void setSqlType(int type);
#endif
    void setGenerated(bool gen);
    void setAutoValue(bool autoVal);

    RequiredStatus requiredStatus() const;
    int length() const;
    int precision() const;
    QVariant defaultValue() const;
#if QT_DEPRECATED_SINCE(6, 8)
    QT_DEPRECATED_VERSION_X_6_8("This internal value is no longer used.")
    int typeID() const;
#endif
    bool isGenerated() const;
    bool isValid() const;

private:
    void detach();
    // ### Qt7: move to private class
    QVariant val;
    QExplicitlySharedDataPointer<QSqlFieldPrivate> d;
};

Q_DECLARE_SHARED(QSqlField)

#ifndef QT_NO_DEBUG_STREAM
Q_SQL_EXPORT QDebug operator<<(QDebug, const QSqlField &);
#endif

QT_END_NAMESPACE

#endif // QSQLFIELD_H
