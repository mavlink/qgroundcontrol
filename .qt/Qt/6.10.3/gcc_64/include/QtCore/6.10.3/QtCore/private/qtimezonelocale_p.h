// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTIMEZONELOCALE_P_H
#define QTIMEZONELOCALE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//
#include <private/qglobal_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qtimezone.h>

#if QT_CONFIG(icu)
#include <unicode/ucal.h>
#else
#include <QtCore/private/qlocale_p.h> // for DataRange
#endif

QT_REQUIRE_CONFIG(timezone);
QT_REQUIRE_CONFIG(timezone_locale);
// #define QT_CLDR_ZONE_DEBUG

QT_BEGIN_NAMESPACE

namespace QtTimeZoneLocale {
#if QT_CONFIG(icu)
QString ucalTimeZoneDisplayName(UCalendar *ucal, QTimeZone::TimeType timeType,
                                QTimeZone::NameType nameType,
                                const QByteArray &localeCode);

bool ucalKnownTimeZoneId(const QString &id);
#else

QList<QByteArrayView> ianaIdsForTerritory(QLocale::Territory territory);

QList<qsizetype> fallbackLocalesFor(qsizetype index); // qlocale.cpp

// Note: "repurposes" QLocale::FormatType with its own twisted meaning.
// See comments in QTZL.cpp; qlocale.cpp has a fallback implementation.
QString zoneOffsetFormat(const QLocale &locale, qsizetype locInd, QLocale::FormatType width,
                         const QDateTime &when, int offsetSeconds);

// Define data types for QTZL_data_p.h

// Accessor methods returning DataRange:
#define rangeGetter(name) \
    constexpr QLocaleData::DataRange name() const \
    { return { m_ ## name ## _idx, m_ ## name ## _size }; }
// Indices of starts of formats within their tables:
#define declFieldIndex(name) quint16 m_ ## name ## _idx;
#define declBigFieldIndex(name) quint32 m_ ## name ## _idx;
// Lengths of formats:
#define declFieldSize(name) quint8 m_ ## name ## _size;
// Generic, standard, daylight-saving triples:
#define forEachNameType(X, form) X(form ## Generic) X(form ## Standard) X(form ## DaylightSaving)
// Mapping TimeType to the appropriate one of those:
#define timeTypeRange(form) \
    constexpr QLocaleData::DataRange form ## Name(QTimeZone::TimeType timeType) const \
    { \
        switch (timeType) { \
        case QTimeZone::StandardTime: return form ## Standard(); \
        case QTimeZone::DaylightTime: return form ## DaylightSaving(); \
        case QTimeZone::GenericTime: return form ## Generic(); \
        } \
        Q_UNREACHABLE_RETURN({}); \
    }
// Mostly form is short or long and we want to append Name.
// See kludge below for regionFormat, for which that's not the name the method wants.

struct LocaleZoneData
{
#ifdef QT_CLDR_ZONE_DEBUG
    // Only included when this define is set, for the sake of asserting
    // consistency with QLocaleData at matching index in tables.
    quint16 m_language_id, m_script_id, m_territory_id;
#endif

    // Indices for this locale:
    quint32 m_exemplarTableStart; // first LocaleZoneExemplar
    quint32 m_metaLongTableStart; // first LocaleMetaZoneLongNames
    quint16 m_metaShortTableStart; // first LocaleMetaZoneShortNames
    quint16 m_zoneTableStart; // first LocaleZoneNames

    // Zone-independent formats:
#define forEachField(X) \
    X(posHourFormat) X(negHourFormat) X(offsetGmtFormat) X(fallbackFormat) \
    forEachNameType(X, regionFormat)
    // Hour formats: HH is hour, mm is minutes (always use two digits for each).
    // GMT format: %0 is an hour format's result.
    // Region formats: %0 is exemplar city or territory.
    // Fallback format: %0 is exemplar city or territory, %1 is meta-zone name.

    forEachField(rangeGetter)
    forEachField(declFieldIndex)
    forEachField(declFieldSize)

#undef forEachField
#define regionFormatName regionFormatRange // kludge naming
    timeTypeRange(regionFormat)
#undef regionFormatName
};

// Sorted by localeIndex, then ianaIdIndex
struct LocaleZoneExemplar
{
    quint16 localeIndex; // Index in locale data tables
    quint16 ianaIdIndex; // Location in IANA ID table
    constexpr QByteArrayView ianaId() const; // Defined in QTZL.cpp
    rangeGetter(exemplarCity);
    quint32 m_exemplarCity_idx;
    quint8 m_exemplarCity_size;
};

// Sorted by localeIndex, then ianaIdIndex
struct LocaleZoneNames
{
    quint16 localeIndex; // Index in locale data tables
    quint16 ianaIdIndex; // Location in IANA ID table
    constexpr QByteArrayView ianaId() const; // Defined in QTZL.cpp
    constexpr QLocaleData::DataRange name(QTimeZone::NameType nameType,
                                          QTimeZone::TimeType timeType) const
    {
        return nameType == QTimeZone::ShortName ? shortName(timeType) : longName(timeType);
    }
    timeTypeRange(long)
    timeTypeRange(short)
#define forEach32BitField(X) forEachNameType(X, long)
#define forEach16BitField(X) forEachNameType(X, short)
#define forEachField(X) forEach32BitField(X) forEach16BitField(X)
    // Localized name of exemplar city for zone.
    // Long and short localized names (length zero for unspecified) for the zone
    // in its generic, standard and daylight-saving forms.

    forEachField(rangeGetter)
    forEach32BitField(declBigFieldIndex)
    forEach16BitField(declFieldIndex)
    forEachField(declFieldSize)

#undef forEachField
#undef forEach16BitField
#undef forEach32BitField
};

// Sorted by localeIndex, then metaIdIndex
struct LocaleMetaZoneLongNames
{
    quint16 localeIndex; // Index in locale data tables
    quint16 metaIdIndex; // As for MetaZoneData metaZoneTable[].
    timeTypeRange(long)
#define forEachField(X) forEachNameType(X, long)
    // Long localized names (length zero for unspecified) for the
    // metazone in its generic, standard and daylight-saving forms.

    forEachField(rangeGetter)
    forEachField(declBigFieldIndex)
    forEachField(declFieldSize)

#undef forEachField
};

// Sorted by localeIndex, then metaIdIndex
struct LocaleMetaZoneShortNames
{
    quint16 localeIndex; // Index in locale data tables
    quint16 metaIdIndex; // metaZoneTable[metaZoneKey - 1].metaIdIndex
    timeTypeRange(short)
#define forEachField(X) forEachNameType(X, short)
    // Short localized names (length zero for unspecified) for the
    // metazone in its generic, standard and daylight-saving forms.

    forEachField(rangeGetter)
    forEachField(declFieldIndex)
    forEachField(declFieldSize)

#undef forEachField
};

#undef timeTypeRange
#undef forEachNameType
#undef declFieldSize
#undef declFieldIndex
#undef rangeGetter
#endif
} // QtTimeZoneLocale

QT_END_NAMESPACE

#endif // QTIMEZONELOCALE_P_H
