// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEXTUREFILEREADER_H
#define QTEXTUREFILEREADER_H

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
#include <QString>
#include <QFileInfo>

QT_BEGIN_NAMESPACE

class QIODevice;
class QTextureFileHandler;

class Q_GUI_EXPORT QTextureFileReader
{
public:
    QTextureFileReader(QIODevice *device, const QString &fileName = QString());  //### drop this logname thing?
    ~QTextureFileReader();

    bool canRead();
    QTextureFileData read();

    // TBD access function to params
    // TBD ask for identified fmt

    static QList<QByteArray> supportedFileFormats();

private:
    bool init();
    QIODevice *m_device = nullptr;
    QString m_fileName;
    QTextureFileHandler *m_handler = nullptr;
    bool checked = false;
};

QT_END_NAMESPACE


#endif // QTEXTUREFILEREADER_H
