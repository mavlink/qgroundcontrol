// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONT_P_H
#define QFONT_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qfont.h"
#include "QtCore/qbasictimer.h"
#include "QtCore/qmap.h"
#include "QtCore/qhash.h"
#include "QtCore/qobject.h"
#include "QtCore/qstringlist.h"
#include <QtGui/qfontdatabase.h>
#include "private/qfixed_p.h"
#include "private/qfontdatabase_p.h"

QT_BEGIN_NAMESPACE

// forwards
class QFontCache;
class QFontEngine;

#define QFONT_WEIGHT_MIN 1
#define QFONT_WEIGHT_MAX 1000

struct QFontDef
{
    inline QFontDef()
        : pointSize(-1.0),
          pixelSize(-1),
          styleStrategy(QFont::PreferDefault),
          stretch(QFont::AnyStretch),
          style(QFont::StyleNormal),
          hintingPreference(QFont::PreferDefaultHinting),
          styleHint(QFont::AnyStyle),
          weight(QFont::Normal),
          fixedPitch(false),
          ignorePitch(true),
          fixedPitchComputed(0),
          reserved(0)
    {
    }

    QStringList families;
    QString styleName;

    QStringList fallBackFamilies;
    QMap<QFont::Tag, float> variableAxisValues;

    qreal pointSize;
    qreal pixelSize;

    // Note: Variable ordering matters to make sure no variable overlaps two 32-bit registers.
    uint styleStrategy : 16;
    uint stretch : 12; // 0-4000
    uint style : 2;
    uint hintingPreference : 2;

    uint styleHint : 8;
    uint weight : 10; // 1-1000
    uint fixedPitch :  1;
    uint ignorePitch : 1;
    uint fixedPitchComputed : 1; // for Mac OS X only
    uint reserved : 11; // for future extensions

    bool exactMatch(const QFontDef &other) const;
    bool operator==(const QFontDef &other) const
    {
        return pixelSize == other.pixelSize
                    && weight == other.weight
                    && style == other.style
                    && stretch == other.stretch
                    && styleHint == other.styleHint
                    && styleStrategy == other.styleStrategy
                    && ignorePitch == other.ignorePitch && fixedPitch == other.fixedPitch
                    && families == other.families
                    && styleName == other.styleName
                    && hintingPreference == other.hintingPreference
                    && variableAxisValues == other.variableAxisValues
                          ;
    }
    inline bool operator<(const QFontDef &other) const
    {
        if (pixelSize != other.pixelSize) return pixelSize < other.pixelSize;
        if (weight != other.weight) return weight < other.weight;
        if (style != other.style) return style < other.style;
        if (stretch != other.stretch) return stretch < other.stretch;
        if (styleHint != other.styleHint) return styleHint < other.styleHint;
        if (styleStrategy != other.styleStrategy) return styleStrategy < other.styleStrategy;
        if (families != other.families) return families < other.families;
        if (styleName != other.styleName)
            return styleName < other.styleName;
        if (hintingPreference != other.hintingPreference) return hintingPreference < other.hintingPreference;


        if (ignorePitch != other.ignorePitch) return ignorePitch < other.ignorePitch;
        if (fixedPitch != other.fixedPitch) return fixedPitch < other.fixedPitch;
        if (variableAxisValues != other.variableAxisValues) {
            if (variableAxisValues.size() != other.variableAxisValues.size())
                return variableAxisValues.size() < other.variableAxisValues.size();

            {
                auto it = variableAxisValues.constBegin();
                auto jt = other.variableAxisValues.constBegin();
                for (; it != variableAxisValues.constEnd(); ++it, ++jt) {
                    if (it.key() != jt.key())
                        return jt.key() < it.key();
                    if (it.value() != jt.value())
                        return jt.value() < it.value();
                }
            }
        }

        return false;
    }
};

inline size_t qHash(const QFontDef &fd, size_t seed = 0) noexcept
{
    return qHashMulti(seed,
                      qRound64(fd.pixelSize*10000), // use only 4 fractional digits
                      fd.weight,
                      fd.style,
                      fd.stretch,
                      fd.styleHint,
                      fd.styleStrategy,
                      fd.ignorePitch,
                      fd.fixedPitch,
                      fd.families,
                      fd.styleName,
                      fd.hintingPreference,
                      fd.variableAxisValues.keys(),
                      fd.variableAxisValues.values());
}

class QFontEngineData
{
public:
    QFontEngineData();
    ~QFontEngineData();

    QAtomicInt ref;
    const int fontCacheId;

    QFontEngine *engines[QFontDatabasePrivate::ScriptCount];
private:
    Q_DISABLE_COPY_MOVE(QFontEngineData)
};


class Q_GUI_EXPORT QFontPrivate
{
public:
    enum class EngineQueryOption {
        Default = 0,
        IgnoreSmallCapsEngine = 0x1,
    };
    Q_DECLARE_FLAGS(EngineQueryOptions, EngineQueryOption)

    QFontPrivate();
    QFontPrivate(const QFontPrivate &other);
    QFontPrivate &operator=(const QFontPrivate &) = delete;
    ~QFontPrivate();

    QFontEngine *engineForScript(int script) const;
    QFontEngine *engineForCharacter(char32_t c, EngineQueryOptions opt = {}) const;
    void alterCharForCapitalization(QChar &c) const;

    QAtomicInt ref;
    QFontDef request;
    mutable QFontEngineData *engineData;
    int dpi;

    uint underline  :  1;
    uint overline   :  1;
    uint strikeOut  :  1;
    uint kerning    :  1;
    uint capital    :  3;
    bool letterSpacingIsAbsolute : 1;

    QFixed letterSpacing;
    QFixed wordSpacing;
    QMap<QFont::Tag, quint32> features;

    mutable QFontPrivate *scFont;
    QFont smallCapsFont() const { return QFont(smallCapsFontPrivate()); }
    QFontPrivate *smallCapsFontPrivate() const;

    static QFontPrivate *get(const QFont &font)
    {
        return font.d.data();
    }

    void resolve(uint mask, const QFontPrivate *other);

    static void detachButKeepEngineData(QFont *font);

    void setFeature(QFont::Tag tag, quint32 value);
    void unsetFeature(QFont::Tag tag);

    void setVariableAxis(QFont::Tag tag, float value);
    void unsetVariableAxis(QFont::Tag tag);
    bool hasVariableAxis(QFont::Tag tag, float value) const;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QFontPrivate::EngineQueryOptions)


class Q_GUI_EXPORT QFontCache : public QObject
{
public:
    // note: these static functions work on a per-thread basis
    static QFontCache *instance();
    static void cleanup();

    QFontCache();
    ~QFontCache();

    int id() const { return m_id; }

    void clear();

    struct Key {
        Key() : script(0), multi(0) { }
        Key(const QFontDef &d, uchar c, bool m = 0)
            : def(d), script(c), multi(m) { }

        QFontDef def;
        uchar script;
        uchar multi: 1;

        inline bool operator<(const Key &other) const
        {
            if (script != other.script) return script < other.script;
            if (multi != other.multi) return multi < other.multi;
            if (multi && def.fallBackFamilies.size() != other.def.fallBackFamilies.size())
                return def.fallBackFamilies.size() < other.def.fallBackFamilies.size();
            return def < other.def;
        }
        inline bool operator==(const Key &other) const
        {
            return script == other.script
                    && multi == other.multi
                    && (!multi || def.fallBackFamilies == other.def.fallBackFamilies)
                    && def == other.def;
        }
    };

    // QFontEngineData cache
    typedef QMap<QFontDef, QFontEngineData*> EngineDataCache;
    EngineDataCache engineDataCache;

    QFontEngineData *findEngineData(const QFontDef &def) const;
    void insertEngineData(const QFontDef &def, QFontEngineData *engineData);

    // QFontEngine cache
    struct Engine {
        Engine() : data(nullptr), timestamp(0), hits(0) { }
        Engine(QFontEngine *d) : data(d), timestamp(0), hits(0) { }

        QFontEngine *data;
        uint timestamp;
        uint hits;
    };

    typedef QMultiMap<Key,Engine> EngineCache;
    EngineCache engineCache;
    QHash<QFontEngine *, int> engineCacheCount;

    QFontEngine *findEngine(const Key &key);

    void updateHitCountAndTimeStamp(Engine &value);
    void insertEngine(const Key &key, QFontEngine *engine, bool insertMulti = false);

private:
    void increaseCost(uint cost);
    void decreaseCost(uint cost);
    void timerEvent(QTimerEvent *event) override;
    void decreaseCache();

    static const uint min_cost;
    uint total_cost, max_cost;
    uint current_timestamp;
    bool fast;
    const bool autoClean;
    QBasicTimer timer;
    const int m_id;
};

Q_GUI_EXPORT QPoint qt_defaultDpis();
Q_GUI_EXPORT int qt_defaultDpiX();
Q_GUI_EXPORT int qt_defaultDpiY();
Q_GUI_EXPORT int qt_defaultDpi();

Q_GUI_EXPORT int qt_legacyToOpenTypeWeight(int weight);
Q_GUI_EXPORT int qt_openTypeToLegacyWeight(int weight);

QT_END_NAMESPACE

#endif // QFONT_P_H
