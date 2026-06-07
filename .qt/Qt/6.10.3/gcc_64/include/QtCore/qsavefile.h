// Copyright (C) 2012 David Faure <faure@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QSAVEFILE_H
#define QSAVEFILE_H

#include <QtCore/qglobal.h>

#if QT_CONFIG(temporaryfile)

#include <QtCore/qfiledevice.h>
#include <QtCore/qstring.h>

#ifdef open
#error qsavefile.h must be included before any header file that defines open
#endif

QT_BEGIN_NAMESPACE

class QAbstractFileEngine;
class QSaveFilePrivate;

class Q_CORE_EXPORT QSaveFile : public QFileDevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSaveFile)

public:

    explicit QSaveFile(const QString &name);
    explicit QSaveFile(QObject *parent = nullptr);
    explicit QSaveFile(const QString &name, QObject *parent);
    ~QSaveFile();

    QString fileName() const override;
    void setFileName(const QString &name);

    QFILE_MAYBE_NODISCARD bool open(OpenMode flags) override;
    bool commit();

    void cancelWriting();

    void setDirectWriteFallback(bool enabled);
    bool directWriteFallback() const;

protected:
    qint64 writeData(const char *data, qint64 len) override;

private:
    void close() override;
#if !QT_CONFIG(translation)
    static QString tr(const char *string) { return QString::fromLatin1(string); }
#endif

private:
    Q_DISABLE_COPY(QSaveFile)
    friend class QFilePrivate;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(temporaryfile)

#endif // QSAVEFILE_H
