// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QJSONDOCUMENT_H
#define QJSONDOCUMENT_H

#include <QtCore/qcompare.h>
#include <QtCore/qjsonparseerror.h>
#if (QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)) || defined(QT_BOOTSTRAPPED)
#include <QtCore/qjsonvalue.h>
#endif
#include <QtCore/qlatin1stringview.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qstringview.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QDebug;
class QCborValue;
class QJsonArray;
class QJsonObject;
class QJsonValue;

namespace QJsonPrivate { class Parser; }

class QJsonDocumentPrivate;
class Q_CORE_EXPORT QJsonDocument
{
public:
#ifdef Q_LITTLE_ENDIAN
    static const uint BinaryFormatTag = ('q') | ('b' << 8) | ('j' << 16) | ('s' << 24);
#else
    static const uint BinaryFormatTag = ('q' << 24) | ('b' << 16) | ('j' << 8) | ('s');
#endif

    QJsonDocument();
    explicit QJsonDocument(const QJsonObject &object);
    explicit QJsonDocument(const QJsonArray &array);
    ~QJsonDocument();

    QJsonDocument(const QJsonDocument &other);
    QJsonDocument &operator =(const QJsonDocument &other);

    QJsonDocument(QJsonDocument &&other) noexcept;

    QJsonDocument &operator =(QJsonDocument &&other) noexcept
    {
        swap(other);
        return *this;
    }

    void swap(QJsonDocument &other) noexcept;

    static QJsonDocument fromVariant(const QVariant &variant);
    QVariant toVariant() const;

#if (QT_VERSION < QT_VERSION_CHECK(7, 0, 0)) && !defined(QT_BOOTSTRAPPED)
    enum JsonFormat {
        Indented,
        Compact
    };
#else
    using JsonFormat = QJsonValue::JsonFormat;
#  ifdef __cpp_using_enum
    using enum QJsonValue::JsonFormat;
#  else
    // keep in sync with qjsonvalue.h
    static constexpr auto Indented = JsonFormat::Indented;
    static constexpr auto Compact = JsonFormat::Compact;
#  endif
#endif

    static QJsonDocument fromJson(const QByteArray &json, QJsonParseError *error = nullptr);

    QByteArray toJson(JsonFormat format = JsonFormat::Indented) const;

    bool isEmpty() const;
    bool isArray() const;
    bool isObject() const;

    QJsonObject object() const;
    QJsonArray array() const;

    void setObject(const QJsonObject &object);
    void setArray(const QJsonArray &array);

    const QJsonValue operator[](const QString &key) const;
    const QJsonValue operator[](QStringView key) const;
    const QJsonValue operator[](QLatin1StringView key) const;
    const QJsonValue operator[](qsizetype i) const;
#if QT_CORE_REMOVED_SINCE(6, 8)
    bool operator==(const QJsonDocument &other) const;
    bool operator!=(const QJsonDocument &other) const { return !operator==(other); }
#endif
    bool isNull() const;

private:
    friend class QJsonValue;
    friend class QJsonPrivate::Parser;
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonDocument &);
    friend Q_CORE_EXPORT bool comparesEqual(const QJsonDocument &lhs,
                                            const QJsonDocument &rhs) noexcept;
    Q_DECLARE_EQUALITY_COMPARABLE(QJsonDocument)

    QJsonDocument(const QCborValue &data);

    std::unique_ptr<QJsonDocumentPrivate> d;
};

Q_DECLARE_SHARED(QJsonDocument)

#if !defined(QT_NO_DEBUG_STREAM)
Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonDocument &);
#endif

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QJsonDocument &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QJsonDocument &);
#endif

QT_END_NAMESPACE

#endif // QJSONDOCUMENT_H
