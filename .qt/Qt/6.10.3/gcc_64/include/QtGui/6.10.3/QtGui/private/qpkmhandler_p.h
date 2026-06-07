// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPKMHANDLER_H
#define QPKMHANDLER_H

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

#include "qtexturefilehandler_p.h"

QT_BEGIN_NAMESPACE

class QPkmHandler : public QTextureFileHandler
{
public:
    using QTextureFileHandler::QTextureFileHandler;
    ~QPkmHandler() override;

    static bool canRead(const QByteArray &suffix, const QByteArray &block);

    QTextureFileData read() override;
};

QT_END_NAMESPACE

#endif // QPKMHANDLER_H
