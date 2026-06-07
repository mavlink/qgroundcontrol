// Copyright (C) 2013 David Faure <faure@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QSAVEFILE_P_H
#define QSAVEFILE_P_H

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

#include <QtCore/qsavefile.h>

#if QT_CONFIG(temporaryfile)

#include "private/qfiledevice_p.h"

QT_BEGIN_NAMESPACE

class QSaveFilePrivate : public QFileDevicePrivate
{
    Q_DECLARE_PUBLIC(QSaveFile)

protected:
    QSaveFilePrivate();
    ~QSaveFilePrivate();

    QString fileName;
    QString finalFileName; // fileName with symbolic links resolved

    QFileDevice::FileError writeError;

    bool useTemporaryFile;
    bool directWriteFallback;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(temporaryfile)

#endif // QSAVEFILE_P_H
