// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTRANSLATION_P_H
#define QQMLTRANSLATION_P_H

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

#include <private/qtqmlglobal_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

class QQmlTranslation
{
public:
    class QsTrData
    {
        QByteArray m_context;
        QByteArray m_text;
        QByteArray m_comment;
        int m_number = 0;

    public:
        QsTrData(const QString &fileNameForContext, const QString &text, const QString &comment,
                 int number)
            : m_context(fileNameForContext.toUtf8())
            , m_text(text.toUtf8())
            , m_comment(comment.toUtf8())
            , m_number(number)
        {
        }

        QString translate() const
        {
#if QT_CONFIG(translation)
            return QCoreApplication::translate(m_context.constData(), m_text.constData(), m_comment.constData(), m_number);
#else
            return QString();
#endif
        }

        QByteArray context() const { return m_context; }
        QByteArray text() const { return m_text; }
        QByteArray comment() const { return m_comment; }
        int number() const { return m_number; }
    };

    class QsTrIdData
    {
        QByteArray m_id;
        int m_number = 0;

    public:
        QsTrIdData(const QString &id, int number)
            : m_id(id.toUtf8()), m_number(number)
        {
        }

        QString translate() const
        {
#if QT_CONFIG(translation)
            return qtTrId(m_id.constData(), m_number);
#else
            return QString();
#endif
        }

        QByteArray id() const { return m_id; }
        int number() const { return m_number; }
    };

    // The static analyzer hates std::monostate in std::variant because
    // that results in various uninitialized memory "problems". Just use
    // std::nullptr_t to indicate "empty".
    using Data = std::variant<std::nullptr_t, QsTrData, QsTrIdData>;

    QQmlTranslation(const Data &d) : m_data(d) {}
    QQmlTranslation() : m_data(nullptr) {}

    template<typename F>
    auto visit(F &&f) const { return std::visit(std::forward<F>(f), m_data); }

    QString translate() const
    {
        return visit(
                [](auto &&arg) -> QString {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (!std::is_same_v<T, std::nullptr_t>)
                        return arg.translate();
                    else {
                        Q_ASSERT_X(false, "QQmlTranslation", "Uninitialized Translation");
                        return {};
                    }
                });
    }

    static QString contextFromQmlFilename(const QString &qmlFilename)
    {
        int lastSlash = qmlFilename.lastIndexOf(QLatin1Char('/'));
        QStringView contextView = (lastSlash > -1)
                ? QStringView{ qmlFilename }.mid(lastSlash + 1, qmlFilename.size() - lastSlash - 5)
                : QStringView();
        return contextView.toString();
    }

private:
    Data m_data;
};

QT_END_NAMESPACE

#endif // QQMLTRANSLATION_P_H
