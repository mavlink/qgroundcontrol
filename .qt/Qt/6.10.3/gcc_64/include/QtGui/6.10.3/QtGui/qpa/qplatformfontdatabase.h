// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMFONTDATABASE_H
#define QPLATFORMFONTDATABASE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtGui/QFontDatabase>
#include <QtGui/private/qfontengine_p.h>
#include <QtGui/private/qfontdatabase_p.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcQpaFonts, Q_GUI_EXPORT)

class QWritingSystemsPrivate;

class Q_GUI_EXPORT QSupportedWritingSystems
{
public:

    QSupportedWritingSystems();
    QSupportedWritingSystems(const QSupportedWritingSystems &other);
    QSupportedWritingSystems &operator=(const QSupportedWritingSystems &other);
    ~QSupportedWritingSystems();

    void setSupported(QFontDatabase::WritingSystem, bool supported = true);
    bool supported(QFontDatabase::WritingSystem) const;

private:
    void detach();

    QWritingSystemsPrivate *d;

    friend Q_GUI_EXPORT bool operator==(const QSupportedWritingSystems &, const QSupportedWritingSystems &);
    friend Q_GUI_EXPORT bool operator!=(const QSupportedWritingSystems &, const QSupportedWritingSystems &);
#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug, const QSupportedWritingSystems &);
#endif
};

Q_GUI_EXPORT bool operator==(const QSupportedWritingSystems &, const QSupportedWritingSystems &);
Q_GUI_EXPORT bool operator!=(const QSupportedWritingSystems &, const QSupportedWritingSystems &);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QSupportedWritingSystems &);
#endif

class QFontRequestPrivate;
class QFontEngineMulti;

class Q_GUI_EXPORT QPlatformFontDatabase
{
public:
    virtual ~QPlatformFontDatabase();
    virtual void populateFontDatabase();
    virtual bool populateFamilyAliases(const QString &missingFamily) { Q_UNUSED(missingFamily); return false; }
    virtual void populateFamily(const QString &familyName);
    virtual void invalidate();

    virtual QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QFontDatabasePrivate::ExtendedScript script) const;
    virtual QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *font = nullptr);

    virtual QFontEngine *fontEngine(const QFontDef &fontDef, void *handle);
    virtual QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference);
    virtual QFontEngineMulti *fontEngineMulti(QFontEngine *fontEngine, QFontDatabasePrivate::ExtendedScript script);
    virtual void releaseHandle(void *handle);

    virtual QString fontDir() const;

    virtual QFont defaultFont() const;
    virtual bool isPrivateFontFamily(const QString &family) const;

    virtual QString resolveFontFamilyAlias(const QString &family) const;
    virtual bool fontsAlwaysScalable() const;
    virtual QList<int> standardSizes() const;

    virtual bool supportsVariableApplicationFonts() const;
    virtual bool supportsColrv0Fonts() const;

    // helper
    static QSupportedWritingSystems writingSystemsFromTrueTypeBits(quint32 unicodeRange[4], quint32 codePageRange[2]);
    static QSupportedWritingSystems writingSystemsFromOS2Table(const char *os2Table, size_t length);

    //callback
    static void registerFont(const QString &familyname, const QString &stylename,
                             const QString &foundryname, QFont::Weight weight,
                             QFont::Style style, QFont::Stretch stretch, bool antialiased,
                             bool scalable, int pixelSize, bool fixedPitch, bool colorFont,
                             const QSupportedWritingSystems &writingSystems, void *handle);

    static void registerFontFamily(const QString &familyName);
    static void registerAliasToFontFamily(const QString &familyName, const QString &alias);

    static void repopulateFontDatabase();

    static bool isFamilyPopulated(const QString &familyName);
};

QT_END_NAMESPACE

#endif // QPLATFORMFONTDATABASE_H
