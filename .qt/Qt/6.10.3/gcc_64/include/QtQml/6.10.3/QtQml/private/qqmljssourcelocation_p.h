// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLJSSOURCELOCATION_P_H
#define QQMLJSSOURCELOCATION_P_H

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qhashfunctions.h>

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

QT_BEGIN_NAMESPACE

namespace QQmlJS {

class SourceLocation
{
    // see also Lexer::illegalFileLengthError() and its enforcement in qqmljs.g
    static constexpr bool qSizeTypeCanHoldQuint32()
    {
        return sizeof(qsizetype) > sizeof(quint32);
    }
public:
    explicit SourceLocation(quint32 offset = 0, quint32 length = 0, quint32 line = 0, quint32 column = 0)
        : offset(offset), length(length),
          startLine(line), startColumn(column)
    {
        if constexpr (!qSizeTypeCanHoldQuint32()) {
            constexpr quint32 maxLength = quint32(std::numeric_limits<qsizetype>::max());

            // note: no overflow when adding offset and length because:
            // offset + length <= (maxLength - 1) + (maxLength - 1) = std::numeric_limits<quint32>::max()
            Q_ASSERT_X(length < maxLength && offset < maxLength && offset + length < maxLength,
                       "QQmlJS::SourceLocation", "File size is limited to 2GB!");
            Q_ASSERT_X(line < maxLength, "QQmlJS::SourceLocation",
                       "Line exceeded maxLength of 2^31 - 1");
            Q_ASSERT_X(column < maxLength, "QQmlJS::SourceLocation",
                       "Column exceeded maxLength of 2^31 - 1");
        }
    }

    static SourceLocation fromQSizeType(qsizetype offset, qsizetype length = 0, qsizetype line = 0,
                                        qsizetype column = 0)
    {
        if constexpr (qSizeTypeCanHoldQuint32()) {
            constexpr qsizetype maxLength = qsizetype(std::numeric_limits<quint32>::max());

            Q_ASSERT_X(offset + length < maxLength, "QQmlJS::SourceLocation",
                       "File size is limited to 4GB!");
            Q_ASSERT_X(line < maxLength, "QQmlJS::SourceLocation",
                       "Line exceeded maxLength of 2^32 - 1");
            Q_ASSERT_X(column < maxLength, "QQmlJS::SourceLocation",
                       "Column exceeded maxLength of 2^32 - 1");
        }
        return SourceLocation(quint32(offset), quint32(length), quint32(line), quint32(column));
    }

private:
    struct LocationInfo
    {
        quint32 offset;
        quint32 startLine;
        quint32 startColumn;
    };

    template <typename Predicate>
    static LocationInfo findLocationIf(QStringView text, Predicate &&predicate,
                                       const SourceLocation &startHint = SourceLocation{})
    {
        quint32 i = startHint.isValid() ? startHint.offset : 0;
        quint32 endLine = startHint.isValid() ? startHint.startLine : 1;
        quint32 endColumn = startHint.isValid() ? startHint.startColumn : 1;
        const quint32 end = quint32(text.size());

        for (; i < end; ++i) {
            if (predicate(i, endLine, endColumn))
                return LocationInfo{ i, endLine, endColumn };

            const QChar currentChar = text.at(i);
            const bool isLineFeed = currentChar == u'\n';
            const bool isCarriageReturn = currentChar == u'\r';
            // note: process the newline on the "\n" part of "\r\n", and treat "\r" as normal
            // character
            const bool isHalfANewline =
                    isCarriageReturn && (i + 1 < end && text.at(i + 1) == u'\n');

            if (isHalfANewline || (!isCarriageReturn && !isLineFeed)) {
                ++endColumn;
                continue;
            }

            // catch positions after the end of the line
            if (predicate(i, endLine, std::numeric_limits<quint32>::max()))
                return LocationInfo{ i, endLine, endColumn };

            // catch positions after the end of the file and return the last character of the last
            // line
            if (i == end - 1) {
                if (predicate(i, std::numeric_limits<quint32>::max(),
                              std::numeric_limits<quint32>::max())) {
                    return LocationInfo{ i, endLine, endColumn };
                }
            }

            ++endLine;
            endColumn = 1;
        }

        // not found, return last position
        return LocationInfo{ i, endLine, endColumn };
    }

public:
    static quint32 offsetFrom(QStringView text, quint32 line, quint32 column,
                              const SourceLocation &startHint = SourceLocation{})
    {
        // sanity check that hint can actually be used
        const SourceLocation hint =
                (startHint.startLine < line
                 || (startHint.startLine == line && startHint.startColumn <= column))
                ? startHint
                : SourceLocation{};

        const auto result = findLocationIf(
                text,
                [line, column](quint32, quint32 currentLine, quint32 currentColumn) {
                    return line <= currentLine && column <= currentColumn;
                },
                hint);
        return result.offset;
    }
    static std::pair<quint32, quint32>
    rowAndColumnFrom(QStringView text, quint32 offset,
                     const SourceLocation &startHint = SourceLocation{})
    {
        // sanity check that hint can actually be used
        const SourceLocation hint = startHint.offset <= offset ? startHint : SourceLocation{};

        const auto result = findLocationIf(
                text,
                [offset](quint32 currentOffset, quint32, quint32) {
                    return offset == currentOffset;
                },
                hint);
        return std::make_pair(result.startLine, result.startColumn);
    }

    bool isValid() const { return *this != SourceLocation(); }

    qsizetype begin() const { return qsizetype(offset); }
    qsizetype end() const { return qsizetype(offset) + length; }

    // Returns a zero length location at the start of the current one.
    SourceLocation startZeroLengthLocation() const
    {
        return SourceLocation(offset, 0, startLine, startColumn);
    }
    // Returns a zero length location at the end of the current one.
    SourceLocation endZeroLengthLocation(QStringView text) const
    {
        auto [row, column] = rowAndColumnFrom(text, offset + length, *this);
        return SourceLocation{ offset + length, 0, row, column };
    }

// attributes
    // ### encode

    // Those quint32 can be casted to qsizetype because the Parser aborts on files with size >=
    // std::min(std::numeric_limits<quint32>::max(), std::numeric_limits<qsizetype>::max()).
    quint32 offset;
    quint32 length;
    quint32 startLine;
    quint32 startColumn;

    friend size_t qHash(const SourceLocation &location, size_t seed = 0)
    {
        return qHashMulti(seed, location.offset, location.length,
                          location.startLine, location.startColumn);
    }

    friend bool operator==(const SourceLocation &a, const SourceLocation &b)
    {
        return a.offset == b.offset && a.length == b.length
                && a.startLine == b.startLine && a.startColumn == b.startColumn;
    }

    friend bool operator!=(const SourceLocation &a, const SourceLocation &b) { return !(a == b); }

    // Returns a source location starting at the beginning of l1, l2 and ending at the end of them.
    // Ignores invalid source locations.
    friend SourceLocation combine(const SourceLocation &l1, const SourceLocation &l2) {
        quint32 e = qMax(l1.end(), l2.end());
        SourceLocation res;
        if (l1.offset <= l2.offset)
            res = (l1.isValid() ? l1 : l2);
        else
            res = (l2.isValid() ? l2 : l1);
        res.length = e - res.offset;
        return res;
    }
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif
