// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLSASOURCELOCATION_H
#define QQMLSASOURCELOCATION_H

#include <QtQmlCompiler/qtqmlcompilerexports.h>

#include <QtCore/qstringview.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
class SourceLocation;
} // namespace QQmlJS

namespace QQmlSA {

class SourceLocationPrivate;

class Q_QMLCOMPILER_EXPORT SourceLocation
{
    friend class QT_PREPEND_NAMESPACE(QQmlSA::SourceLocationPrivate);

public:
    explicit SourceLocation(quint32 offset = 0, quint32 length = 0, quint32 line = 0,
                            quint32 column = 0);
    SourceLocation(const SourceLocation &);
    SourceLocation(SourceLocation &&other) noexcept
    {
        memcpy(m_data, other.m_data, sizeofSourceLocation);
        memset(other.m_data, 0, sizeofSourceLocation);
    }
    SourceLocation &operator=(const SourceLocation &);
    SourceLocation &operator=(SourceLocation &&other) noexcept
    {
        memcpy(m_data, other.m_data, sizeofSourceLocation);
        memset(other.m_data, 0, sizeofSourceLocation);
        return *this;
    }
    ~SourceLocation();

    bool isValid() const;

    quint32 begin() const;
    quint32 end() const;

    quint32 offset() const;
    quint32 length() const;
    quint32 startLine() const;
    quint32 startColumn() const;

    SourceLocation startZeroLengthLocation() const;
    SourceLocation endZeroLengthLocation(QStringView text) const;

    friend qsizetype qHash(const SourceLocation &location, qsizetype seed = 0)
    {
        return qHashImpl(location, seed);
    }

    friend bool operator==(const SourceLocation &lhs, const SourceLocation &rhs)
    {
        return operatorEqualsImpl(lhs, rhs);
    }

    friend bool operator!=(const SourceLocation &lhs, const SourceLocation &rhs)
    {
        return !(lhs == rhs);
    }

private:
    static qsizetype qHashImpl(const SourceLocation &location, qsizetype seed);
    static bool operatorEqualsImpl(const SourceLocation &, const SourceLocation &);

    static constexpr qsizetype sizeofSourceLocation = 4 * sizeof(quint32);
    alignas(int) char m_data[sizeofSourceLocation] = {};
};

} // namespace QQmlSA

QT_END_NAMESPACE

#endif // QQMLSASOURCELOCATION_H
