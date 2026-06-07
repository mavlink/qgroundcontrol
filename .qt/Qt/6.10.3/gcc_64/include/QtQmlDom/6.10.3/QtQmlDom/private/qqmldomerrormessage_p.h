// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef ERRORMESSAGE_H
#define ERRORMESSAGE_H

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

#include "qqmldom_global.h"
#include "qqmldomstringdumper_p.h"
#include "qqmldompath_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QCborArray>
#include <QtCore/QCborMap>
#include <QtCore/QLoggingCategory>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(domLog, QMLDOM_EXPORT);

namespace QQmlJS {
namespace Dom {

QMLDOM_EXPORT ErrorLevel errorLevelFromQtMsgType(QtMsgType msgType);

class ErrorGroups;
class DomItem;
using std::function;

#define NewErrorGroup(name) QQmlJS::Dom::ErrorGroup(QT_TRANSLATE_NOOP("ErrorGroup", name))

class QMLDOM_EXPORT ErrorGroup {
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(ErrorGroup)
public:
    constexpr ErrorGroup(const char *groupId):
        m_groupId(groupId)
    {}


    void dump(const Sink &sink) const;
    void dumpId(const Sink &sink) const;

    QLatin1String groupId() const;
    QString groupName() const;
 private:
    const char *m_groupId;
};

class QMLDOM_EXPORT ErrorGroups{
    Q_GADGET
public:
    void dump(const Sink &sink) const;
    void dumpId(const Sink &sink) const;
    QCborArray toCbor() const;

    [[nodiscard]] ErrorMessage errorMessage(
            const Dumper &msg, ErrorLevel level, const Path &element = Path(),
            const QString &canonicalFilePath = QString(), SourceLocation location = SourceLocation()) const;
    [[nodiscard]] ErrorMessage errorMessage(
            const DiagnosticMessage &msg, const Path &element = Path(),
            const QString &canonicalFilePath = QString()) const;

    void fatal(const Dumper &msg, const Path &element = Path(), QStringView canonicalFilePath = u"",
               SourceLocation location = SourceLocation()) const;

    [[nodiscard]] ErrorMessage debug(const QString &message) const;
    [[nodiscard]] ErrorMessage debug(const Dumper &message) const;
    [[nodiscard]] ErrorMessage info(const QString &message) const;
    [[nodiscard]] ErrorMessage info(const Dumper &message) const;
    [[nodiscard]] ErrorMessage warning(const QString &message) const;
    [[nodiscard]] ErrorMessage warning(const Dumper &message) const;
    [[nodiscard]] ErrorMessage error(const QString &message) const;
    [[nodiscard]] ErrorMessage error(const Dumper &message) const;

    static int cmp(const ErrorGroups &g1, const ErrorGroups &g2);

    QVector<ErrorGroup> groups;
};

inline bool operator==(const ErrorGroups& lhs, const ErrorGroups& rhs){ return ErrorGroups::cmp(lhs,rhs) == 0; }
inline bool operator!=(const ErrorGroups& lhs, const ErrorGroups& rhs){ return ErrorGroups::cmp(lhs,rhs) != 0; }
inline bool operator< (const ErrorGroups& lhs, const ErrorGroups& rhs){ return ErrorGroups::cmp(lhs,rhs) <  0; }
inline bool operator> (const ErrorGroups& lhs, const ErrorGroups& rhs){ return ErrorGroups::cmp(lhs,rhs) >  0; }
inline bool operator<=(const ErrorGroups& lhs, const ErrorGroups& rhs){ return ErrorGroups::cmp(lhs,rhs) <= 0; }
inline bool operator>=(const ErrorGroups& lhs, const ErrorGroups& rhs){ return ErrorGroups::cmp(lhs,rhs) >= 0; }

class QMLDOM_EXPORT ErrorMessage { // reuse Some of the other DiagnosticMessages?
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(ErrorMessage)
public:
    using Level = ErrorLevel;
    // error registry (usage is optional)
    static QLatin1String msg(const char *errorId, ErrorMessage &&err);
    static QLatin1String msg(QLatin1String errorId, ErrorMessage &&err);
    static void visitRegisteredMessages(function_ref<bool (const ErrorMessage &)> visitor);
    [[nodiscard]] static ErrorMessage load(QLatin1String errorId);
    [[nodiscard]] static ErrorMessage load(const char *errorId);
    template<typename... T>
    [[nodiscard]] static ErrorMessage load(QLatin1String errorId, T... args){
        ErrorMessage res = load(errorId);
        res.message = res.message.arg(args...);
        return res;
    }

    ErrorMessage(
            const QString &message, const ErrorGroups &errorGroups, Level level = Level::Warning,
            const Path &path = Path(), const QString &file = QString(),
            SourceLocation location = SourceLocation(), QLatin1String errorId = QLatin1String(""));
    ErrorMessage(
            const ErrorGroups &errorGroups, const DiagnosticMessage &msg, const Path &path = Path(),
            const QString &file = QString(), QLatin1String errorId = QLatin1String(""));

    [[nodiscard]] ErrorMessage &withErrorId(QLatin1String errorId);
    [[nodiscard]] ErrorMessage &withPath(const Path &);
    [[nodiscard]] ErrorMessage &withFile(const QString &);
    [[nodiscard]] ErrorMessage &withFile(QStringView);
    [[nodiscard]] ErrorMessage &withLocation(SourceLocation);
    [[nodiscard]] ErrorMessage &withItem(const DomItem &);

    ErrorMessage handle(const ErrorHandler &errorHandler=nullptr);

    void dump(const Sink &s) const;
    QString toString() const;
    QCborMap toCbor() const;
    friend int compare(const ErrorMessage &msg1, const ErrorMessage &msg2)
    {
        int c;
        c = msg1.location.offset - msg2.location.offset;
        if (c != 0)
            return c;
        c = msg1.location.startLine - msg2.location.startLine;
        if (c != 0)
            return c;
        c = msg1.errorId.compare(msg2.errorId);
        if (c != 0)
            return c;
        if (!msg1.errorId.isEmpty())
            return 0;
        c = msg1.message.compare(msg2.message);
        if (c != 0)
            return c;
        c = msg1.file.compare(msg2.file);
        if (c != 0)
            return c;
        c = Path::cmp(msg1.path, msg2.path);
        if (c != 0)
            return c;
        c = int(msg1.level) - int(msg2.level);
        if (c != 0)
            return c;
        c = int(msg1.errorGroups.groups.size() - msg2.errorGroups.groups.size());
        if (c != 0)
            return c;
        for (qsizetype i = 0; i < msg1.errorGroups.groups.size(); ++i) {
            c = msg1.errorGroups.groups[i].groupId().compare(msg2.errorGroups.groups[i].groupId());
            if (c != 0)
                return c;
        }
        c = msg1.location.length - msg2.location.length;
        if (c != 0)
            return c;
        c = msg1.location.startColumn - msg2.location.startColumn;
        return c;
    }

    QLatin1String errorId;
    QString message;
    ErrorGroups errorGroups;
    Level level;
    Path path;
    QString file;
    SourceLocation location;
};

inline bool operator !=(const ErrorMessage &e1, const ErrorMessage &e2) {
    return compare(e1, e2) != 0;
}
inline bool operator ==(const ErrorMessage &e1, const ErrorMessage &e2) {
    return compare(e1, e2) == 0;
}
inline bool operator<(const ErrorMessage &e1, const ErrorMessage &e2)
{
    return compare(e1, e2) < 0;
}
inline bool operator<=(const ErrorMessage &e1, const ErrorMessage &e2)
{
    return compare(e1, e2) <= 0;
}
inline bool operator>(const ErrorMessage &e1, const ErrorMessage &e2)
{
    return compare(e1, e2) > 0;
}
inline bool operator>=(const ErrorMessage &e1, const ErrorMessage &e2)
{
    return compare(e1, e2) >= 0;
}

QMLDOM_EXPORT void silentError(const ErrorMessage &);
QMLDOM_EXPORT void errorToQDebug(const ErrorMessage &);

QMLDOM_EXPORT void defaultErrorHandler(const ErrorMessage &);
QMLDOM_EXPORT void setDefaultErrorHandler(const ErrorHandler &h);

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // ERRORMESSAGE_H
