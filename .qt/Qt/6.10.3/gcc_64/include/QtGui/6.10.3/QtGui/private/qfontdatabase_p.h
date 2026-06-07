// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONTDATABASE_P_H
#define QFONTDATABASE_P_H

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

#include <QtCore/qcache.h>
#include <QtCore/qloggingcategory.h>

#include <QtGui/qfontdatabase.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcFontDb)
Q_DECLARE_LOGGING_CATEGORY(lcFontMatch)

struct QtFontDesc;

struct QtFontFallbacksCacheKey
{
    QString family;
    QFont::Style style;
    QFont::StyleHint styleHint;
    int script;
};

inline bool operator==(const QtFontFallbacksCacheKey &lhs, const QtFontFallbacksCacheKey &rhs) noexcept
{
    return lhs.script == rhs.script &&
            lhs.styleHint == rhs.styleHint &&
            lhs.style == rhs.style &&
            lhs.family == rhs.family;
}

inline bool operator!=(const QtFontFallbacksCacheKey &lhs, const QtFontFallbacksCacheKey &rhs) noexcept
{
    return !operator==(lhs, rhs);
}

inline size_t qHash(const QtFontFallbacksCacheKey &key, size_t seed = 0) noexcept
{
    QtPrivate::QHashCombineWithSeed hash(seed);
    seed = hash(seed, key.family);
    seed = hash(seed, int(key.style));
    seed = hash(seed, int(key.styleHint));
    seed = hash(seed, int(key.script));
    return seed;
}

struct Q_GUI_EXPORT QtFontSize
{
    void *handle;
    unsigned short pixelSize : 16;
};

struct Q_GUI_EXPORT QtFontStyle
{
    struct Key
    {
        Key(const QString &styleString);

        Key()
            : style(QFont::StyleNormal)
            , weight(QFont::Normal)
            , stretch(0)
        {}

        Key(const Key &o)
            : style(o.style)
            , weight(o.weight)
            , stretch(o.stretch)
        {}

        uint style          : 2;
        uint weight         : 10;
        signed int stretch  : 12;

        bool operator==(const Key &other) const noexcept
        {
            return (style == other.style && weight == other.weight &&
                    (stretch == 0 || other.stretch == 0 || stretch == other.stretch));
        }

        bool operator!=(const Key &other) const noexcept
        {
            return !operator==(other);
        }
    };

    QtFontStyle(const Key &k)
        : key(k)
        , bitmapScalable(false)
        , smoothScalable(false)
        , count(0)
        , pixelSizes(nullptr)
    {
    }

    ~QtFontStyle();

    QtFontSize *pixelSize(unsigned short size, bool = false);

    Key key;
    bool bitmapScalable : 1;
    bool smoothScalable : 1;
    signed int count    : 30;
    QtFontSize *pixelSizes;
    QString styleName;
    bool antialiased;
};

struct Q_GUI_EXPORT QtFontFoundry
{
    QtFontFoundry(const QString &n)
        : name(n)
        , count(0)
        , styles(nullptr)
    {}

    ~QtFontFoundry()
    {
        while (count--)
            delete styles[count];
        free(styles);
    }

    QString name;
    int count;
    QtFontStyle **styles;

    enum StyleRetrievalFlags : quint8 {
        NoRetrievalFlags = 0,
        AddWhenMissing = 1,
        MatchAllProperties = 2,
        AllRetrievalFlags = 3,
    };

    QtFontStyle *style(const QtFontStyle::Key &,
                       const QString & = QString(),
                       StyleRetrievalFlags flags = NoRetrievalFlags);
};

struct Q_GUI_EXPORT QtFontFamily
{
    enum WritingSystemStatus {
        Unknown         = 0,
        Supported       = 1,
        UnsupportedFT  = 2,
        Unsupported     = UnsupportedFT
    };

    QtFontFamily(const QString &n)
        :
        populated(false),
        fixedPitch(false),
        colorFont(false),
        name(n), count(0), foundries(nullptr)
    {
        memset(writingSystems, 0, sizeof(writingSystems));
    }
    ~QtFontFamily() {
        while (count--)
            delete foundries[count];
        free(foundries);
    }

    bool populated : 1;
    bool fixedPitch : 1;
    bool colorFont : 1;

    QString name;
    QStringList aliases;
    int count;
    QtFontFoundry **foundries;

    unsigned char writingSystems[QFontDatabase::WritingSystemsCount];

    bool matchesFamilyName(const QString &familyName) const;
    QtFontFoundry *foundry(const QString &f, bool = false);

    bool ensurePopulated();
};

class Q_GUI_EXPORT QFontDatabasePrivate
{
public:
    QFontDatabasePrivate()
        : count(0)
        , families(nullptr)
        , fallbacksCache(64)
    { }

    ~QFontDatabasePrivate() {
        clearFamilies();
    }

    void clearFamilies();

    enum FamilyRequestFlags {
        RequestFamily = 0,
        EnsureCreated,
        EnsurePopulated
    };

    // Expands QChar::Script by adding a special "script" for emoji sequences
    enum ExtendedScript {
        Script_Common = QChar::Script_Common,
        Script_Latin = QChar::Script_Latin,
        Script_Emoji = QChar::ScriptCount,
        ScriptCount
    };

    QtFontFamily *family(const QString &f, FamilyRequestFlags flags = EnsurePopulated);

    int count;
    QtFontFamily **families;
    bool populated = false;

    QHash<ExtendedScript, QStringList> applicationFallbackFontFamiliesHash;

    QCache<QtFontFallbacksCacheKey, QStringList> fallbacksCache;
    struct ApplicationFont {
        QString fileName;

        // Note: The data may be implicitly shared throughout the
        // font database and platform font database, so be careful
        // to never detach when accessing this member!
        QByteArray data;

        bool isNull() const { return fileName.isEmpty(); }
        bool isPopulated() const { return !properties.isEmpty(); }

        struct Properties {
            QString familyName;
            QString styleName;
            int weight = 0;
            QFont::Style style = QFont::StyleNormal;
            int stretch = QFont::Unstretched;
        };

        QList<Properties> properties;
    };
    QList<ApplicationFont> applicationFonts;
    int addAppFont(const QByteArray &fontData, const QString &fileName);
    bool isApplicationFont(const QString &fileName);

    void setApplicationFallbackFontFamilies(ExtendedScript script, const QStringList &familyNames);
    QStringList applicationFallbackFontFamilies(ExtendedScript script);
    bool removeApplicationFallbackFontFamily(ExtendedScript script, const QString &familyName);
    void addApplicationFallbackFontFamily(ExtendedScript script, const QString &familyName);

    static QFontDatabasePrivate *instance();

    static void parseFontName(const QString &name, QString &foundry, QString &family);
    static QString resolveFontFamilyAlias(const QString &family);
    static QFontEngine *findFont(const QFontDef &request,
                                 int script /* QFontDatabasePrivate::ExtendedScript */,
                                 bool preferScriptOverFamily = false);
    static void load(const QFontPrivate *d, int script /* QFontDatabasePrivate::ExtendedScript */);
    static QFontDatabasePrivate *ensureFontDatabase();

    void invalidate();

private:
    static int match(int script,
                     const QFontDef &request,
                     const QString &family_name,
                     const QString &foundry_name,
                     QtFontDesc *desc,
                     const QList<int> &blacklistedFamilies,
                     unsigned int *resultingScore = nullptr);

    static unsigned int bestFoundry(int script, unsigned int score, int styleStrategy,
                            const QtFontFamily *family, const QString &foundry_name,
                            QtFontStyle::Key styleKey, int pixelSize, char pitch,
                            QtFontDesc *desc, const QString &styleName = QString());

    static QFontEngine *loadSingleEngine(int script, const QFontDef &request,
                            QtFontFamily *family, QtFontFoundry *foundry,
                            QtFontStyle *style, QtFontSize *size);

    static QFontEngine *loadEngine(int script, const QFontDef &request,
                            QtFontFamily *family, QtFontFoundry *foundry,
                            QtFontStyle *style, QtFontSize *size);

};
Q_DECLARE_TYPEINFO(QFontDatabasePrivate::ApplicationFont, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QFONTDATABASE_P_H
