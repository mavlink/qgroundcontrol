// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEXTUREFILEHANDLER_P_H
#define QTEXTUREFILEHANDLER_P_H

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

#include "qtexturefiledata_p.h"

QT_BEGIN_NAMESPACE

class QTextureFileHandler
{
public:
    QTextureFileHandler(QIODevice *device, const QByteArray &logName = QByteArray())
        : m_device(device)
    {
        m_logName = !logName.isEmpty() ? logName : QByteArrayLiteral("(unknown)");
    }
    virtual ~QTextureFileHandler();

    virtual QTextureFileData read() = 0;
    QIODevice *device() const { return m_device; }
    QByteArray logName() const { return m_logName; }

private:
    QIODevice *m_device = nullptr;
    QByteArray m_logName;
};

QT_END_NAMESPACE

#endif // QTEXTUREFILEHANDLER_P_H
