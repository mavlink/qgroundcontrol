// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLOCALE_H
#define QQMLLOCALE_H

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

#include <qqml.h>

#include <QtCore/qlocale.h>
#include <QtCore/qobject.h>
#include <private/qtqmlglobal_p.h>
#include <private/qv4object_p.h>

QT_REQUIRE_CONFIG(qml_locale);

QT_BEGIN_NAMESPACE


class QQmlDateExtension
{
public:
    static void registerExtension(QV4::ExecutionEngine *engine);

private:
    static QV4::ReturnedValue method_toLocaleString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_toLocaleTimeString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_toLocaleDateString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_fromLocaleString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_fromLocaleTimeString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_fromLocaleDateString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_timeZoneUpdated(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
};


class QQmlNumberExtension
{
public:
    static void registerExtension(QV4::ExecutionEngine *engine);

private:
    static QV4::ReturnedValue method_toLocaleString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_fromLocaleString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_toLocaleCurrencyString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
};

// This needs to be a struct so that we can derive from QLocale and inherit its enums. Then we can
// use it as extension in QQmlLocaleEnums and expose all the enums in one go, without duplicating
// any in different qmltypes files.
struct Q_QML_EXPORT QQmlLocale : public QLocale
{
    Q_GADGET
    QML_ANONYMOUS
public:

    // Qt defines Sunday as 7, but JS Date assigns Sunday 0
    enum DayOfWeek {
        Sunday = 0,
        Monday = Qt::Monday,
        Tuesday = Qt::Tuesday,
        Wednesday = Qt::Wednesday,
        Thursday = Qt::Thursday,
        Friday = Qt::Friday,
        Saturday = Qt::Saturday
    };
    Q_ENUM(DayOfWeek)

    static QV4::ReturnedValue locale(QV4::ExecutionEngine *engine, const QString &localeName);
    static void registerStringLocaleCompare(QV4::ExecutionEngine *engine);
    static QV4::ReturnedValue method_localeCompare(
            const QV4::FunctionObject *, const QV4::Value *thisObject,
            const QV4::Value *argv, int argc);
};

struct DayOfWeekList
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QList<QQmlLocale::DayOfWeek>)
    QML_SEQUENTIAL_CONTAINER(QQmlLocale::DayOfWeek)
};

class QQmlLocaleValueType
{
    QLocale locale;

    Q_PROPERTY(QQmlLocale::DayOfWeek firstDayOfWeek READ firstDayOfWeek CONSTANT)
    Q_PROPERTY(QLocale::MeasurementSystem measurementSystem READ measurementSystem CONSTANT)
    Q_PROPERTY(Qt::LayoutDirection textDirection READ textDirection CONSTANT)
    Q_PROPERTY(QList<QQmlLocale::DayOfWeek> weekDays READ weekDays CONSTANT)
    Q_PROPERTY(QStringList uiLanguages READ uiLanguages CONSTANT)

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString nativeLanguageName READ nativeLanguageName CONSTANT)
#if QT_DEPRECATED_SINCE(6, 6)
    Q_PROPERTY(QString nativeCountryName READ nativeCountryName CONSTANT)
#endif
    Q_PROPERTY(QString nativeTerritoryName READ nativeTerritoryName CONSTANT)
    Q_PROPERTY(QString decimalPoint READ decimalPoint CONSTANT)
    Q_PROPERTY(QString groupSeparator READ groupSeparator CONSTANT)
    Q_PROPERTY(QString percent READ percent CONSTANT)
    Q_PROPERTY(QString zeroDigit READ zeroDigit CONSTANT)
    Q_PROPERTY(QString negativeSign READ negativeSign CONSTANT)
    Q_PROPERTY(QString positiveSign READ positiveSign CONSTANT)
    Q_PROPERTY(QString exponential READ exponential CONSTANT)
    Q_PROPERTY(QString amText READ amText CONSTANT)
    Q_PROPERTY(QString pmText READ pmText CONSTANT)

    Q_PROPERTY(QLocale::NumberOptions numberOptions READ numberOptions WRITE setNumberOptions)

    Q_GADGET_EXPORT(Q_QML_EXPORT)
    QML_ANONYMOUS
    QML_FOREIGN(QLocale)
    QML_EXTENDED(QQmlLocaleValueType)
    QML_CONSTRUCTIBLE_VALUE

public:
    Q_INVOKABLE QQmlLocaleValueType(const QString &name) : locale(name) {}

    Q_INVOKABLE QString currencySymbol(
            QLocale::CurrencySymbolFormat format = QLocale::CurrencySymbol) const
    {
        return locale.currencySymbol(format);
    }

    Q_INVOKABLE QString dateTimeFormat(QLocale::FormatType format = QLocale::LongFormat) const
    {
        return locale.dateTimeFormat(format);
    }

    Q_INVOKABLE QString timeFormat(QLocale::FormatType format = QLocale::LongFormat) const
    {
        return locale.timeFormat(format);
    }

    Q_INVOKABLE QString dateFormat(QLocale::FormatType format = QLocale::LongFormat) const
    {
        return locale.dateFormat(format);
    }

    Q_INVOKABLE QString monthName(int index, QLocale::FormatType format = QLocale::LongFormat) const
    {
        // +1 added to idx because JS is 0-based, whereas QLocale months begin at 1.
        return locale.monthName(index + 1, format);
    }

    Q_INVOKABLE QString standaloneMonthName(
            int index, QLocale::FormatType format = QLocale::LongFormat) const
    {
        // +1 added to idx because JS is 0-based, whereas QLocale months begin at 1.
        return locale.standaloneMonthName(index + 1, format);
    }

    Q_INVOKABLE QString dayName(int index, QLocale::FormatType format = QLocale::LongFormat) const
    {
        // 0 -> 7 as Qt::Sunday is 7, but Sunday is 0 in JS Date
        return locale.dayName(index == 0 ? 7 : index, format);
    }

    Q_INVOKABLE QString standaloneDayName(
            int index, QLocale::FormatType format = QLocale::LongFormat) const
    {
        // 0 -> 7 as Qt::Sunday is 7, but Sunday is 0 in JS Date
        return locale.standaloneDayName(index == 0 ? 7 : index, format);
    }

    Q_INVOKABLE void formattedDataSize(QQmlV4FunctionPtr args) const;
    Q_INVOKABLE QString formattedDataSize(
            double bytes, int precision = 2,
            QLocale::DataSizeFormats format = QLocale::DataSizeIecFormat) const
    {
        return locale.formattedDataSize(
                qint64(QV4::Value::toInteger(bytes)), precision, format);
    }

    Q_INVOKABLE void toString(QQmlV4FunctionPtr args) const;

    // As a special (undocumented) case, when called with no arguments,
    // just forward to QDebug. This makes it consistent with other types
    // in JS that can be converted to a string via toString().
    Q_INVOKABLE QString toString() const { return QDebug::toString(locale); }

    Q_INVOKABLE QString toString(int i) const { return locale.toString(i); }
    Q_INVOKABLE QString toString(double f) const
    {
        return QJSNumberCoercion::isInteger(f) ? toString(int(f)) : locale.toString(f);
    }
    Q_INVOKABLE QString toString(double f, const QString &format, int precision = 6) const
    {
        // Lacking a char type, we have to use QString here
        return format.length() < 1
                ? QString()
                : locale.toString(f, format.at(0).toLatin1(), precision);
    }
    Q_INVOKABLE QString toString(const QDateTime &dateTime, const QString &format) const
    {
        return locale.toString(dateTime, format);
    }
    Q_INVOKABLE QString toString(
            const QDateTime &dateTime, QLocale::FormatType format = QLocale::LongFormat) const
    {
        return locale.toString(dateTime, format);
    }

    Q_INVOKABLE QString createSeparatedList(const QStringList &list) const
    {
        return locale.createSeparatedList(list);
    }

    QQmlLocale::DayOfWeek firstDayOfWeek() const;
    QLocale::MeasurementSystem measurementSystem() const { return locale.measurementSystem(); }
    Qt::LayoutDirection textDirection() const { return locale.textDirection(); }
    QList<QQmlLocale::DayOfWeek> weekDays() const;
    QStringList uiLanguages() const { return locale.uiLanguages(); }

    QString name() const { return locale.name(); }
    QString nativeLanguageName() const { return locale.nativeLanguageName(); }
#if QT_DEPRECATED_SINCE(6, 6)
    QString nativeCountryName() const
    {
        QT_IGNORE_DEPRECATIONS(return locale.nativeCountryName();)
    }
#endif
    QString nativeTerritoryName() const { return locale.nativeTerritoryName(); }
    QString decimalPoint() const { return locale.decimalPoint(); }
    QString groupSeparator() const { return locale.groupSeparator(); }
    QString percent() const { return locale.percent(); }
    QString zeroDigit() const { return locale.zeroDigit(); }
    QString negativeSign() const { return locale.negativeSign(); }
    QString positiveSign() const { return locale.positiveSign(); }
    QString exponential() const { return locale.exponential(); }
    QString amText() const { return locale.amText(); }
    QString pmText() const { return locale.pmText(); }

    QLocale::NumberOptions numberOptions() const { return locale.numberOptions(); }
    void setNumberOptions(const QLocale::NumberOptions &numberOptions)
    {
        locale.setNumberOptions(numberOptions);
    }
};

QT_END_NAMESPACE

#endif
