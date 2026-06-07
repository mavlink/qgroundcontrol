// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMIMEGLOBPATTERN_P_H
#define QMIMEGLOBPATTERN_P_H

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

#include <QtCore/private/qglobal_p.h>

QT_REQUIRE_CONFIG(mimetype);

#include <QtCore/qstringlist.h>
#include <QtCore/qhash.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

struct QMimeGlobMatchResult
{
    void addMatch(const QString &mimeType, int weight, const QString &pattern,
                  qsizetype knownSuffixLength = 0);

    QStringList m_matchingMimeTypes; // only those with highest weight
    QStringList m_allMatchingMimeTypes;
    int m_weight = 0;
    qsizetype m_matchingPatternLength = 0;
    qsizetype m_knownSuffixLength = 0;
};

class QMimeGlobPattern
{
public:
    static const unsigned MaxWeight = 100;
    static const unsigned DefaultWeight = 50;
    static const unsigned MinWeight = 1;

    explicit QMimeGlobPattern(const QString &thePattern, const QString &theMimeType,
                              unsigned theWeight = DefaultWeight,
                              Qt::CaseSensitivity s = Qt::CaseInsensitive) :
        m_pattern(s == Qt::CaseInsensitive ? thePattern.toLower() : thePattern),
        m_mimeType(theMimeType),
        m_weight(theWeight),
        m_caseSensitivity(s),
        m_patternType(detectPatternType(m_pattern))
    {
    }

    void swap(QMimeGlobPattern &other) noexcept
    {
        qSwap(m_pattern,         other.m_pattern);
        qSwap(m_mimeType,        other.m_mimeType);
        qSwap(m_weight,          other.m_weight);
        qSwap(m_caseSensitivity, other.m_caseSensitivity);
        qSwap(m_patternType,     other.m_patternType);
    }

    bool matchFileName(const QString &inputFileName) const;

    inline const QString &pattern() const { return m_pattern; }
    inline unsigned weight() const { return m_weight; }
    inline const QString &mimeType() const { return m_mimeType; }
    inline bool isCaseSensitive() const { return m_caseSensitivity == Qt::CaseSensitive; }

private:
    enum PatternType {
        SuffixPattern,
        PrefixPattern,
        LiteralPattern,
        VdrPattern,        // special handling for "[0-9][0-9][0-9].vdr" pattern
        AnimPattern,       // special handling for "*.anim[1-9j]" pattern
        OtherPattern
    };
    PatternType detectPatternType(QStringView pattern) const;

    QString m_pattern;
    QString m_mimeType;
    int m_weight;
    Qt::CaseSensitivity m_caseSensitivity;
    PatternType m_patternType;
};
Q_DECLARE_SHARED(QMimeGlobPattern)

using AddMatchFilterFunc = std::function<bool(const QString &)>;

class QMimeGlobPatternList : public QList<QMimeGlobPattern>
{
public:
    bool hasPattern(QStringView mimeType, QStringView pattern) const
    {
        auto matchesMimeAndPattern = [mimeType, pattern](const QMimeGlobPattern &e) {
            return e.pattern() == pattern && e.mimeType() == mimeType;
        };
        return std::any_of(begin(), end(), matchesMimeAndPattern);
    }

    /*!
        "noglobs" is very rare occurrence, so it's ok if it's slow
     */
    void removeMimeType(QStringView mimeType)
    {
        auto isMimeTypeEqual = [mimeType](const QMimeGlobPattern &pattern) {
            return pattern.mimeType() == mimeType;
        };
        removeIf(isMimeTypeEqual);
    }

    void match(QMimeGlobMatchResult &result, const QString &fileName,
               const AddMatchFilterFunc &filterFunc) const;
};

/*!
    Result of the globs parsing, as data structures ready for efficient MIME type matching.
    This contains:
    1) a map of fast regular patterns (e.g. *.txt is stored as "txt" in a qhash's key)
    2) a linear list of high-weight globs
    3) a linear list of low-weight globs
 */
class QMimeAllGlobPatterns
{
public:
    typedef QHash<QString, QStringList> PatternsMap; // MIME type -> patterns

    void addGlob(const QMimeGlobPattern &glob);
    void removeMimeType(const QString &mimeType);
    void matchingGlobs(const QString &fileName, QMimeGlobMatchResult &result,
                       const AddMatchFilterFunc &filterFunc) const;
    void clear();

    PatternsMap m_fastPatterns; // example: "doc" -> "application/msword", "text/plain"
    QMimeGlobPatternList m_highWeightGlobs;
    QMimeGlobPatternList m_lowWeightGlobs; // <= 50, including the non-fast 50 patterns
};

QT_END_NAMESPACE

#endif // QMIMEGLOBPATTERN_P_H
