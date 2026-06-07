// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRESOURCE_P_H
#define QRESOURCE_P_H

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

#include "qabstractfileengine_p.h"

QT_BEGIN_NAMESPACE

class QResourceFileEnginePrivate;
class QResourceFileEngine : public QAbstractFileEngine
{
private:
    Q_DECLARE_PRIVATE(QResourceFileEngine)
public:
    explicit QResourceFileEngine(const QString &path);
    ~QResourceFileEngine();

    void setFileName(const QString &file) override;

    bool open(QIODevice::OpenMode flags, std::optional<QFile::Permissions> permissions) override;
    bool close() override;
    bool flush() override;
    qint64 size() const override;
    qint64 pos() const override;
    virtual bool atEnd() const;
    bool seek(qint64) override;
    qint64 read(char *data, qint64 maxlen) override;

    bool caseSensitive() const override;

    FileFlags fileFlags(FileFlags type) const override;

    QString fileName(QAbstractFileEngine::FileName file) const override;

    uint ownerId(FileOwner) const override;

    QDateTime fileTime(QFile::FileTime time) const override;

    IteratorUniquePtr beginEntryList(const QString &path, QDirListing::IteratorFlags filters,
                                     const QStringList &filterNames) override;

    bool extension(Extension extension, const ExtensionOption *option = nullptr, ExtensionReturn *output = nullptr) override;
    bool supportsExtension(Extension extension) const override;
};

QT_END_NAMESPACE

#endif // QRESOURCE_P_H
