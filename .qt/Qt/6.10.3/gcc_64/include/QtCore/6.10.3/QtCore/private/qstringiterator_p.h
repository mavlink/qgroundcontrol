// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSTRINGITERATOR_H
#define QSTRINGITERATOR_H

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
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QStringIterator
{
    QString::const_iterator i, pos, e;
    static_assert((std::is_same<QString::const_iterator, const QChar *>::value));
    static bool less(const QChar *lhs, const QChar *rhs) noexcept
    { return std::less{}(lhs, rhs); }
public:
    explicit QStringIterator(QStringView string, qsizetype idx = 0)
        : i(string.begin()),
          pos(i + idx),
          e(string.end())
    {
    }

    inline explicit QStringIterator(const QChar *begin, const QChar *end)
        : i(begin),
          pos(begin),
          e(end)
    {
    }

    explicit QStringIterator(const QChar *begin, qsizetype idx, const QChar *end)
        : i(begin),
          pos(begin + idx),
          e(end)
    {
    }

    inline QString::const_iterator position() const
    {
        return pos;
    }

    qsizetype index() const
    {
        return pos - i;
    }

    inline void setPosition(QString::const_iterator position)
    {
        Q_ASSERT_X(!less(position, i) && !less(e, position),
                   Q_FUNC_INFO, "position out of bounds");
        pos = position;
    }

    // forward iteration

    inline bool hasNext() const
    {
        return less(pos, e);
    }

    inline void advance()
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        if (Q_UNLIKELY((pos++)->isHighSurrogate())) {
            if (Q_LIKELY(pos != e && pos->isLowSurrogate()))
                ++pos;
        }
    }

    inline void advanceUnchecked()
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        if (Q_UNLIKELY((pos++)->isHighSurrogate())) {
            Q_ASSERT(hasNext() && pos->isLowSurrogate());
            ++pos;
        }
    }

    inline char32_t peekNextUnchecked() const
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        if (Q_UNLIKELY(pos->isHighSurrogate())) {
            Q_ASSERT(less(pos + 1, e) && pos[1].isLowSurrogate());
            return QChar::surrogateToUcs4(pos[0], pos[1]);
        }

        return pos->unicode();
    }

    inline char32_t peekNext(char32_t invalidAs = QChar::ReplacementCharacter) const
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        if (Q_UNLIKELY(pos->isSurrogate())) {
            if (Q_LIKELY(pos->isHighSurrogate())) {
                const QChar *low = pos + 1;
                if (Q_LIKELY(low != e && low->isLowSurrogate()))
                    return QChar::surrogateToUcs4(*pos, *low);
            }
            return invalidAs;
        }

        return pos->unicode();
    }

    inline char32_t nextUnchecked()
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        const QChar cur = *pos++;
        if (Q_UNLIKELY(cur.isHighSurrogate())) {
            Q_ASSERT(hasNext() && pos->isLowSurrogate());
            return QChar::surrogateToUcs4(cur, *pos++);
        }
        return cur.unicode();
    }

    inline char32_t next(char32_t invalidAs = QChar::ReplacementCharacter)
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        const QChar uc = *pos++;
        if (Q_UNLIKELY(uc.isSurrogate())) {
            if (Q_LIKELY(uc.isHighSurrogate() && hasNext() && pos->isLowSurrogate()))
                return QChar::surrogateToUcs4(uc, *pos++);
            return invalidAs;
        }

        return uc.unicode();
    }

    char32_t nextOrRawCodeUnit()
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        const QChar uc = *pos++;
        if (uc.isHighSurrogate() && hasNext() && pos->isLowSurrogate())
            return QChar::surrogateToUcs4(uc, *pos++);

        return uc.unicode();
    }

    // backwards iteration

    inline bool hasPrevious() const
    {
        return less(i, pos);
    }

    inline void recede()
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        if (Q_UNLIKELY((--pos)->isLowSurrogate())) {
            const QChar *high = pos - 1;
            if (Q_LIKELY(high != i - 1 && high->isHighSurrogate()))
                --pos;
        }
    }

    inline void recedeUnchecked()
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        if (Q_UNLIKELY((--pos)->isLowSurrogate())) {
            Q_ASSERT(hasPrevious() && pos[-1].isHighSurrogate());
            --pos;
        }
    }

    inline char32_t peekPreviousUnchecked() const
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        if (Q_UNLIKELY(pos[-1].isLowSurrogate())) {
            Q_ASSERT(less(i + 1, pos) && pos[-2].isHighSurrogate());
            return QChar::surrogateToUcs4(pos[-2], pos[-1]);
        }
        return pos[-1].unicode();
    }

    inline char32_t peekPrevious(char32_t invalidAs = QChar::ReplacementCharacter) const
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        if (Q_UNLIKELY(pos[-1].isSurrogate())) {
            if (Q_LIKELY(pos[-1].isLowSurrogate())) {
                const QChar *high = pos - 2;
                if (Q_LIKELY(high != i - 1 && high->isHighSurrogate()))
                    return QChar::surrogateToUcs4(*high, pos[-1]);
            }
            return invalidAs;
        }

        return pos[-1].unicode();
    }

    inline char32_t previousUnchecked()
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        const QChar cur = *--pos;
        if (Q_UNLIKELY(cur.isLowSurrogate())) {
            Q_ASSERT(hasPrevious() && pos[-1].isHighSurrogate());
            return QChar::surrogateToUcs4(*--pos, cur);
        }
        return cur.unicode();
    }

    inline char32_t previous(char32_t invalidAs = QChar::ReplacementCharacter)
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        const QChar uc = *--pos;
        if (Q_UNLIKELY(uc.isSurrogate())) {
            if (Q_LIKELY(uc.isLowSurrogate() && hasPrevious() && pos[-1].isHighSurrogate()))
                return QChar::surrogateToUcs4(*--pos, uc);
            return invalidAs;
        }

        return uc.unicode();
    }

    char32_t previousOrRawCodeUnit()
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        const QChar uc = *--pos;
        if (uc.isLowSurrogate() && hasPrevious() && pos[-1].isHighSurrogate())
            return QChar::surrogateToUcs4(*--pos, uc);

        return uc.unicode();
    }
};

QT_END_NAMESPACE

#endif // QSTRINGITERATOR_H
