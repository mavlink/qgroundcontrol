// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2024 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDIRLISTING_H
#define QDIRLISTING_H

#include <QtCore/qtdeprecationmarkers.h>
#include <QtCore/qfiledevice.h>
#include <QtCore/qflags.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qdatetime.h>

#include <iterator>
#include <utility>

QT_BEGIN_NAMESPACE

class QDirListingPrivate;
class QFileInfo;
class QDir;
class QTimeZone;

class QDirListing
{
public:
    enum class IteratorFlag {
        Default =               0x000000,
        ExcludeFiles =          0x000004,
        ExcludeDirs =           0x000008,
#if QT_DEPRECATED_SINCE(6, 14)
        ExcludeSpecial QT_DEPRECATED_VERSION_X_6_14("Use ExcludeOther instead.") = 0x000010,
#endif
        ExcludeOther =          0x000010,
        ResolveSymlinks =       0x000020,
        FilesOnly =             ExcludeDirs  | ExcludeOther,
        DirsOnly =              ExcludeFiles | ExcludeOther,
        IncludeHidden =         0x000040,
        IncludeDotAndDotDot =   0x000080,
        CaseSensitive =         0x000100,
        Recursive =             0x000400,
        FollowDirSymlinks =     0x000800,
    };
    Q_DECLARE_FLAGS(IteratorFlags, IteratorFlag)

    Q_CORE_EXPORT explicit QDirListing(const QString &path,
                                       IteratorFlags flags = IteratorFlag::Default);
    Q_CORE_EXPORT explicit QDirListing(const QString &path, const QStringList &nameFilters,
                                       IteratorFlags flags = IteratorFlag::Default);

    QDirListing(QDirListing &&other) noexcept
        : d{std::exchange(other.d, nullptr)} {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QDirListing)

    void swap(QDirListing &other) noexcept { qt_ptr_swap(d, other.d); }

    Q_CORE_EXPORT ~QDirListing();

    Q_CORE_EXPORT QString iteratorPath() const;
    Q_CORE_EXPORT IteratorFlags iteratorFlags() const;
    Q_CORE_EXPORT QStringList nameFilters() const;

    class DirEntry
    {
        friend class QDirListing;
        QDirListingPrivate *dirListPtr = nullptr;
    public:
        Q_CORE_EXPORT QString fileName() const;
        Q_CORE_EXPORT QString baseName() const;
        Q_CORE_EXPORT QString completeBaseName() const;
        Q_CORE_EXPORT QString suffix() const;
        Q_CORE_EXPORT QString bundleName() const;
        Q_CORE_EXPORT QString completeSuffix() const;
        Q_CORE_EXPORT QString filePath() const;
        Q_CORE_EXPORT bool isDir() const;
        Q_CORE_EXPORT bool isFile() const;
        Q_CORE_EXPORT bool isSymLink() const;
        Q_CORE_EXPORT bool exists() const;
        Q_CORE_EXPORT bool isHidden() const;
        Q_CORE_EXPORT bool isReadable() const;
        Q_CORE_EXPORT bool isWritable() const;
        Q_CORE_EXPORT bool isExecutable() const;
        Q_CORE_EXPORT QFileInfo fileInfo() const;
        Q_CORE_EXPORT QString canonicalFilePath() const;
        Q_CORE_EXPORT QString absoluteFilePath() const;
        Q_CORE_EXPORT QString absolutePath() const;
        Q_CORE_EXPORT qint64 size() const;

        QDateTime birthTime(const QTimeZone &tz) const
        { return fileTime(QFileDevice::FileBirthTime, tz); }
        QDateTime metadataChangeTime(const QTimeZone &tz) const
        { return fileTime(QFileDevice::FileMetadataChangeTime, tz); }
        QDateTime lastModified(const QTimeZone &tz) const
        { return fileTime(QFileDevice::FileModificationTime, tz); }
        QDateTime lastRead(const QTimeZone &tz) const
        { return fileTime(QFileDevice::FileAccessTime, tz); }
        Q_CORE_EXPORT QDateTime fileTime(QFileDevice::FileTime type, const QTimeZone &tz) const;
    };

    class sentinel
    {
        friend constexpr bool operator==(sentinel, sentinel) noexcept { return true; }
        friend constexpr bool operator!=(sentinel, sentinel) noexcept { return false; }
    };

    class const_iterator
    {
        Q_DISABLE_COPY(const_iterator)
        friend class QDirListing;
        explicit const_iterator(QDirListingPrivate *dp) { dirEntry.dirListPtr = dp; }
        DirEntry dirEntry;
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = DirEntry;
        using difference_type = qint64;
        using pointer = const value_type *;
        using reference = const value_type &;

        const_iterator() = default;
        const_iterator(const_iterator &&) noexcept = default;
        const_iterator &operator=(const_iterator &&) noexcept = default;

        reference operator*() const { return dirEntry; }
        pointer operator->() const { return &dirEntry; }
        const_iterator &operator++() { dirEntry = next(dirEntry); return *this; }
        void operator++(int) { ++*this; } // [iterator.concept.winc]/14 not required to return sth
    private:
        bool atEnd() const noexcept { return dirEntry.dirListPtr == nullptr; }
        friend bool operator==(const const_iterator &lhs, sentinel) noexcept { return lhs.atEnd(); }
#ifndef __cpp_impl_three_way_comparison
        friend bool operator!=(const const_iterator &lhs, sentinel) noexcept
        { return !operator==(lhs, sentinel{}); }
        friend bool operator==(sentinel, const const_iterator &rhs) noexcept
        { return operator==(rhs, sentinel{}); }
        friend bool operator!=(sentinel, const const_iterator &rhs) noexcept
        { return !operator==(sentinel{}, rhs); }
#endif // __cpp_impl_three_way_comparison
    };

    Q_CORE_EXPORT const_iterator begin() const;
    const_iterator cbegin() const { return begin(); }
    sentinel end() const { return {}; }
    sentinel cend() const { return end(); }

    // Qt compatibility
    const_iterator constBegin() const { return begin(); }
    sentinel constEnd() const { return end(); }

private:
    Q_DISABLE_COPY(QDirListing)

    Q_CORE_EXPORT static DirEntry next(DirEntry);

    // Private constructor that is used in deprecated code paths.
    // `uint` instead of QDir::Filters and QDirIterator::IteratorFlags
    // because qdir.h can't be included here; qdiriterator.h can't included
    // either, because it includes qdir.h
    Q_CORE_EXPORT QDirListing(const QString &path, const QStringList &nameFilters, uint dirFilters,
                              uint qdirIteratorFlags = 0); // QDirIterator::NoIteratorFlags == 0x0

    QDirListingPrivate *d;
    friend class QDir;
    friend class QDirPrivate;
    friend class QDirIteratorPrivate;
    friend class QAbstractFileEngine;
    friend class QFileInfoGatherer;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDirListing::IteratorFlags)

QT_END_NAMESPACE

#endif // QDIRLISTING_H
