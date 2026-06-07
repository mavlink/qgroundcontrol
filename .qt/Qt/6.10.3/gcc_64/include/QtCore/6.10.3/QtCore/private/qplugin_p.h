// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLUGIN_P_H
#define QPLUGIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/qglobal_p.h>

#include "QtCore/qplugin.h"
#include "QtCore/qcborvalue.h"
#include "QtCore/qcbormap.h"

QT_BEGIN_NAMESPACE

enum class QtPluginMetaDataKeys {
    QtVersion,
    Requirements,
    IID,
    ClassName,
    MetaData,
    URI,
    IsDebug,
};

// F(IntKey, StringKey, Description)
// Keep this list sorted in the order moc should output.
#define QT_PLUGIN_FOREACH_METADATA(F) \
    F(QtPluginMetaDataKeys::IID, "IID", "Plugin's Interface ID")                \
    F(QtPluginMetaDataKeys::ClassName, "className", "Plugin class name")        \
    F(QtPluginMetaDataKeys::MetaData, "MetaData", "Other meta data")            \
    F(QtPluginMetaDataKeys::URI, "URI", "Plugin URI")                           \
    /* not output by moc in CBOR */                                             \
    F(QtPluginMetaDataKeys::QtVersion, "version", "Qt version")                 \
    F(QtPluginMetaDataKeys::Requirements, "archlevel", "Architectural level")   \
    F(QtPluginMetaDataKeys::IsDebug, "debug", "Debug-mode plugin")              \
    /**/

namespace {
struct DecodedArchRequirements
{
    quint8 level;
    bool isDebug;
    friend constexpr bool operator==(DecodedArchRequirements r1, DecodedArchRequirements r2)
    {
        return r1.level == r2.level && r1.isDebug == r2.isDebug;
    }
};

static constexpr DecodedArchRequirements decodeVersion0ArchRequirements(quint8 value)
{
    // see qPluginArchRequirements() and QPluginMetaDataV2::archRequirements()
    DecodedArchRequirements r = {};
#ifdef Q_PROCESSOR_X86
    if (value & 4)
        r.level = 4;            // AVX512F -> x86-64-v4
    else if (value & 2)
        r.level = 3;            // AVX2 -> x86-64-v3
#endif
    if (value & 1)
        r.isDebug = true;
    return r;
}
// self checks
static_assert(decodeVersion0ArchRequirements(0) == DecodedArchRequirements{ 0, false });
static_assert(decodeVersion0ArchRequirements(1) == DecodedArchRequirements{ 0, true });
#ifdef Q_PROCESSOR_X86
static_assert(decodeVersion0ArchRequirements(2) == DecodedArchRequirements{ 3, false });
static_assert(decodeVersion0ArchRequirements(3) == DecodedArchRequirements{ 3, true });
static_assert(decodeVersion0ArchRequirements(4) == DecodedArchRequirements{ 4, false });
static_assert(decodeVersion0ArchRequirements(5) == DecodedArchRequirements{ 4, true });
#endif

static constexpr DecodedArchRequirements decodeVersion1ArchRequirements(quint8 value)
{
    return { quint8(value & 0x7f), bool(value & 0x80) };
}
// self checks
static_assert(decodeVersion1ArchRequirements(0) == DecodedArchRequirements{ 0, false });
static_assert(decodeVersion1ArchRequirements(0x80) == DecodedArchRequirements{ 0, true });
#ifdef Q_PROCESSOR_X86
static_assert(decodeVersion1ArchRequirements(1) == DecodedArchRequirements{ 1, false });
static_assert(decodeVersion1ArchRequirements(3) == DecodedArchRequirements{ 3, false});
static_assert(decodeVersion1ArchRequirements(4) == DecodedArchRequirements{ 4, false });
static_assert(decodeVersion1ArchRequirements(0x82) == DecodedArchRequirements{ 2, true });
static_assert(decodeVersion1ArchRequirements(0x84) == DecodedArchRequirements{ 4, true });
#endif
} // unnamed namespace

class QPluginParsedMetaData
{
    QCborValue data;
    bool setError(const QString &errorString) Q_DECL_COLD_FUNCTION
    {
        data = errorString;
        return false;
    }
public:
    QPluginParsedMetaData() = default;
    QPluginParsedMetaData(QByteArrayView input)     { parse(input); }

    bool isError() const                            { return !data.isMap(); }
    QString errorString() const                     { return data.toString(); }

    bool parse(QByteArrayView input);
    bool parse(QPluginMetaData metaData)
    { return parse(QByteArrayView(reinterpret_cast<const char *>(metaData.data), metaData.size)); }

    QJsonObject toJson() const;     // only for QLibrary & QPluginLoader

    // if data is not a map, toMap() returns empty, so shall these functions
    QCborMap toCbor() const                         { return data.toMap(); }
    QCborValue value(QtPluginMetaDataKeys k) const  { return data[int(k)]; }
};


QT_END_NAMESPACE

#endif // QPLUGIN_P_H
