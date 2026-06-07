// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLBUILTINS_H
#define QQMLBUILTINS_H

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

#include <private/qqmlcomponentattached_p.h>

#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlscriptstring.h>

#include <QtQmlIntegration/qqmlintegration.h>

#include <QtCore/qobject.h>
#include <QtCore/qglobal.h>
#include <QtCore/qtmetamacros.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariantmap.h>
#include <QtCore/qtypes.h>
#include <QtCore/qchar.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonarray.h>

#include <climits>

#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif

QT_BEGIN_NAMESPACE

// moc doesn't do 64bit constants, so we have to determine the size of qsizetype indirectly.
// We assume that qsizetype is always the same size as a pointer. I haven't seen a platform
// where this is not the case.
// Furthermore moc is wrong about pretty much everything on 64bit windows. We need to hardcode
// the size there.
// Likewise, we also have to determine the size of long and ulong indirectly.

#if defined(Q_OS_WIN64)

static_assert(sizeof(long) == 4);
#define QML_LONG_IS_32BIT
static_assert(sizeof(qsizetype) == 8);
#define QML_SIZE_IS_64BIT

#elif QT_POINTER_SIZE == 4

static_assert(sizeof(long) == 4);
#define QML_LONG_IS_32BIT
static_assert(sizeof(qsizetype) == 4);
#define QML_SIZE_IS_32BIT

#else

static_assert(sizeof(long) == 8);
#define QML_LONG_IS_64BIT
static_assert(sizeof(qsizetype) == 8);
#define QML_SIZE_IS_64BIT

#endif

#define QML_EXTENDED_JAVASCRIPT(EXTENDED_TYPE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_TYPE) \
    Q_CLASSINFO("QML.ExtensionIsJavaScript", "true")

template<typename A> struct QQmlPrimitiveAliasFriend {};

#define QML_PRIMITIVE_ALIAS(PRIMITIVE_ALIAS) \
    Q_CLASSINFO("QML.PrimitiveAlias", #PRIMITIVE_ALIAS) \
    friend QQmlPrimitiveAliasFriend<PRIMITIVE_ALIAS>;

struct QQmlVoidForeign
{
    Q_GADGET
    QML_VALUE_TYPE(void)
    QML_EXTENDED_JAVASCRIPT(undefined)
#if !QT_CONFIG(regularexpression)
    QML_VALUE_TYPE(regexp)
#endif
    QML_FOREIGN(void)
};

struct QQmlVarForeign
{
    Q_GADGET
    QML_VALUE_TYPE(var)
    QML_VALUE_TYPE(variant)
    QML_FOREIGN(QVariant)
    QML_EXTENDED(QQmlVarForeign)
};

struct QQmlQtObjectForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(QtObject)
    QML_EXTENDED_JAVASCRIPT(Object)
    QML_FOREIGN(QObject)
    Q_CLASSINFO("QML.Root", "true")
};

struct QQmlIntForeign
{
    Q_GADGET
    QML_VALUE_TYPE(int)
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(int)
#ifdef QML_SIZE_IS_32BIT
    // Keep qsizetype as primitive alias. We want it as separate type.
    QML_PRIMITIVE_ALIAS(qsizetype)
#endif
};

struct QQmlQint32Foreign
{
    Q_GADGET
    QML_FOREIGN(qint32)
    QML_USING(int)
};

struct QQmlInt32TForeign
{
    Q_GADGET
    QML_FOREIGN(int32_t)
    QML_USING(int)
};

struct QQmlDoubleForeign
{
    Q_GADGET
    QML_VALUE_TYPE(real)
    QML_VALUE_TYPE(double)
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(double)
};

struct QQmlStringForeign
{
    Q_GADGET
    QML_VALUE_TYPE(string)
    QML_EXTENDED_JAVASCRIPT(String)
    QML_FOREIGN(QString)
};

struct QQmlAnyStringViewForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(String)
    QML_FOREIGN(QAnyStringView)
};

struct QQmlBoolForeign
{
    Q_GADGET
    QML_VALUE_TYPE(bool)
    QML_EXTENDED_JAVASCRIPT(Boolean)
    QML_FOREIGN(bool)
};

struct QQmlDateForeign
{
    Q_GADGET
    QML_VALUE_TYPE(date)
    QML_EXTENDED_JAVASCRIPT(Date)
    QML_FOREIGN(QDateTime)
};

struct QQmlUrlForeign
{
    Q_GADGET
    QML_VALUE_TYPE(url)
    QML_EXTENDED_JAVASCRIPT(URL)
    QML_FOREIGN(QUrl)
};

#if QT_CONFIG(regularexpression)
struct QQmlRegexpForeign
{
    Q_GADGET
    QML_VALUE_TYPE(regexp)
    QML_EXTENDED_JAVASCRIPT(RegExp)
    QML_FOREIGN(QRegularExpression)
};
#endif

struct QQmlNullForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(std::nullptr_t)
    QML_EXTENDED(QQmlNullForeign)
};

struct QQmlQVariantMapForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QVariantMap)
    QML_EXTENDED_JAVASCRIPT(Object)
};

struct QQmlQVariantHashForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QVariantHash)
    QML_EXTENDED_JAVASCRIPT(Object)
};

struct QQmlQint8Foreign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(qint8)
};

struct QQmlInt8TForeign
{
    Q_GADGET
    QML_FOREIGN(int8_t)
    QML_USING(qint8)
};

struct QQmlQuint8Foreign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(quint8)
};

struct QQmlUint8TForeign
{
    Q_GADGET
    QML_FOREIGN(uint8_t)
    QML_USING(quint8)
};

struct QQmlUcharForeign
{
    Q_GADGET
    QML_FOREIGN(uchar)
    QML_USING(quint8)
};

struct QQmlCharForeign
{
    Q_GADGET
    QML_FOREIGN(char)
#if CHAR_MAX == UCHAR_MAX
    QML_USING(quint8)
#elif CHAR_MAX == SCHAR_MAX
    QML_USING(qint8)
#else
#   error char is neither quint8 nor qint8
#endif
};

struct QQmlShortForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(short)
};

struct QQmlQint16Foreign
{
    Q_GADGET
    QML_FOREIGN(qint16)
    QML_USING(short)
};

struct QQmlInt16TForeign
{
    Q_GADGET
    QML_FOREIGN(int16_t)
    QML_USING(short)
};

struct QQmlUshortForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(ushort)
};

struct QQmlQuint16Foreign
{
    Q_GADGET
    QML_FOREIGN(quint16)
    QML_USING(ushort)
};

struct QQmlUint16TForeign
{
    Q_GADGET
    QML_FOREIGN(uint16_t)
    QML_USING(ushort)
};

struct QQmlUintForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(uint)
};

struct QQmlQuint32Foreign
{
    Q_GADGET
    QML_FOREIGN(quint32)
    QML_USING(uint)
};

struct QQmlUint32TForeign
{
    Q_GADGET
    QML_FOREIGN(uint32_t)
    QML_USING(uint)
};

struct QQmlQlonglongForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(qlonglong)
#ifdef QML_SIZE_IS_64BIT
    // Keep qsizetype as primitive alias. We want it as separate type.
    QML_PRIMITIVE_ALIAS(qsizetype)
#endif
};

struct QQmlQint64Foreign
{
    Q_GADGET
    QML_FOREIGN(qint64)
    QML_USING(qlonglong)
};

struct QQmlInt64TForeign
{
    Q_GADGET
    QML_FOREIGN(int64_t)
    QML_USING(qlonglong)
};

struct QQmlLongForeign
{
    Q_GADGET
    QML_FOREIGN(long)
#if defined QML_LONG_IS_32BIT
    QML_USING(int)
#elif defined QML_LONG_IS_64BIT
    QML_USING(qlonglong)
#else
#   error long is neither 32bit nor 64bit
#endif
};

struct QQmlQulonglongForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(qulonglong)
};

struct QQmlQuint64Foreign
{
    Q_GADGET
    QML_FOREIGN(quint64)
    QML_USING(qulonglong)
};

struct QQmlUint64TForeign
{
    Q_GADGET
    QML_FOREIGN(uint64_t)
    QML_USING(qulonglong)
};

struct QQmlUlongForeign
{
    Q_GADGET
    QML_FOREIGN(ulong)
#if defined QML_LONG_IS_32BIT
    QML_USING(uint)
#elif defined QML_LONG_IS_64BIT
    QML_USING(qulonglong)
#else
#   error ulong is neither 32bit nor 64bit
#endif
};

struct QQmlFloatForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(float)
};

struct QQmlQRealForeign
{
    Q_GADGET
    QML_FOREIGN(qreal)
#if !defined(QT_COORD_TYPE) || defined(QT_COORD_TYPE_IS_DOUBLE)
    QML_USING(double)
#elif defined(QT_COORD_TYPE_IS_FLOAT)
    QML_USING(float)
#else
#   error qreal is neither float nor double
#endif
};

struct QQmlQCharForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QChar)
    QML_EXTENDED_JAVASCRIPT(String)
};

struct QQmlQDateForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QDate)
    QML_EXTENDED_JAVASCRIPT(Date)
};

struct QQmlQTimeForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QTime)
    QML_EXTENDED_JAVASCRIPT(Date)
};

struct QQmlQByteArrayForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(ArrayBuffer)
    QML_FOREIGN(QByteArray)
};

struct QQmlQByteArrayListForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QByteArrayList)
    QML_SEQUENTIAL_CONTAINER(QByteArray)
};

struct QQmlQStringListForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QStringList)
    QML_SEQUENTIAL_CONTAINER(QString)
};

struct QQmlQVariantListForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QVariantList)
    QML_SEQUENTIAL_CONTAINER(QVariant)
};

struct QQmlQObjectListForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QObjectList)
    QML_SEQUENTIAL_CONTAINER(QObject*)
};

struct QQmlQListQObjectForeign
{
    Q_GADGET
    QML_FOREIGN(QList<QObject*>)
    QML_USING(QObjectList)
};

struct QQmlQJSValueForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QJSValue)
    QML_EXTENDED(QQmlQJSValueForeign)
};

struct QQmlComponentForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(Component)
    QML_FOREIGN(QQmlComponent)
    QML_ATTACHED(QQmlComponentAttached)
};

struct QQmlScriptStringForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQmlScriptString)
};

struct QQmlV4FunctionPtrForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQmlV4FunctionPtr)
    QML_EXTENDED(QQmlV4FunctionPtrForeign)
};

struct QQmlQJsonObjectForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QJsonObject)
    QML_EXTENDED_JAVASCRIPT(Object)
};

struct QQmlQJsonValueForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QJsonValue)
    QML_EXTENDED(QQmlQJsonValueForeign)
};

struct QQmlQJsonArrayForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QJsonArray)
    QML_SEQUENTIAL_CONTAINER(QJsonValue)
};

QT_END_NAMESPACE

#endif // QQMLBUILTINS_H
