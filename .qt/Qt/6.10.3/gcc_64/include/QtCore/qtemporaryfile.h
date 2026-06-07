// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEMPORARYFILE_H
#define QTEMPORARYFILE_H

#include <QtCore/qiodevice.h>
#include <QtCore/qfile.h>

#ifdef open
#error qtemporaryfile.h must be included before any header file that defines open
#endif

QT_BEGIN_NAMESPACE


#if QT_CONFIG(temporaryfile)

class QTemporaryFilePrivate;
class QLockFilePrivate;

class Q_CORE_EXPORT QTemporaryFile : public QFile
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif
    Q_DECLARE_PRIVATE(QTemporaryFile)

public:
    QTemporaryFile();
    explicit QTemporaryFile(const QString &templateName);
#ifndef QT_NO_QOBJECT
    explicit QTemporaryFile(QObject *parent);
    QTemporaryFile(const QString &templateName, QObject *parent);

#  if QT_CONFIG(cxx17_filesystem) || defined(Q_QDOC)
    Q_WEAK_OVERLOAD
    explicit QTemporaryFile(const std::filesystem::path &templateName, QObject *parent = nullptr)
        : QTemporaryFile(QtPrivate::fromFilesystemPath(templateName), parent)
    {
    }
#  endif // QT_CONFIG(cxx17_filesystem)
#endif // !QT_NO_QOBJECT

    ~QTemporaryFile();

    bool autoRemove() const;
    void setAutoRemove(bool b);

    // ### Hides open(flags)
    QFILE_MAYBE_NODISCARD bool open() { return open(QIODevice::ReadWrite); }

    QString fileName() const override;
    QString fileTemplate() const;
    void setFileTemplate(const QString &name);
#if QT_CONFIG(cxx17_filesystem) || defined(Q_QDOC)
    Q_WEAK_OVERLOAD
    void setFileTemplate(const std::filesystem::path &name)
    {
        return setFileTemplate(QtPrivate::fromFilesystemPath(name));
    }
#endif // QT_CONFIG(cxx17_filesystem)

    // Hides QFile::rename
    bool rename(const QString &newName);

#if QT_CONFIG(cxx17_filesystem) || defined(Q_QDOC)
    Q_WEAK_OVERLOAD
    bool rename(const std::filesystem::path &newName)
    {
        return rename(QtPrivate::fromFilesystemPath(newName));
    }
#endif // QT_CONFIG(cxx17_filesystem)

    inline static QTemporaryFile *createNativeFile(const QString &fileName)
        { QFile file(fileName); return createNativeFile(file); }
    static QTemporaryFile *createNativeFile(QFile &file);

#if QT_CONFIG(cxx17_filesystem) || defined(Q_QDOC)
    Q_WEAK_OVERLOAD
    static QTemporaryFile *createNativeFile(const std::filesystem::path &fileName)
    {
        QFile file(fileName);
        return createNativeFile(file);
    }
#endif // QT_CONFIG(cxx17_filesystem)

protected:
    bool open(OpenMode flags) override;

private:
    friend class QFile;
    friend class QLockFilePrivate;
    Q_DISABLE_COPY(QTemporaryFile)
};

#endif // QT_CONFIG(temporaryfile)

QT_END_NAMESPACE

#endif // QTEMPORARYFILE_H
