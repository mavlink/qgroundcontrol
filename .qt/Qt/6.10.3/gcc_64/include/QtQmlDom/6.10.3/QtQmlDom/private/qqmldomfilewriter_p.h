// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMFILEWRITER_P
#define QQMLDOMFILEWRITER_P

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
#include "qqmldomfunctionref_p.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT FileWriter
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(FileWriter)
public:
    enum class Status { ShouldWrite, DidWrite, SkippedEqual, SkippedDueToFailure };

    FileWriter() = default;

    ~FileWriter()
    {
        if (!silentWarnings) {
            for (const QString &w : std::as_const(warnings))
                qWarning("%ls", qUtf16Printable(w));
        }
        if (shouldRemoveTempFile)
            tempFile.remove();
    }

    Status write(const QString &targetFile, function_ref<bool(QTextStream &)> write, int nBk = 2);

    bool shouldRemoveTempFile = false;
    bool silentWarnings = false;
    Status status = Status::SkippedDueToFailure;
    QString targetFile;
    QFile tempFile;
    QStringList newBkFiles;
    QStringList warnings;

private:
    Q_DISABLE_COPY_MOVE(FileWriter)
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
#endif
