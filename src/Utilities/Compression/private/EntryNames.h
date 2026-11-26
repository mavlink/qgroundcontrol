#pragma once

/// @file EntryNames.h
/// @brief The names that an extraction writes the same way on every system
/// @note Private to QGCCompression

#include <utility>

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringView>

namespace LibArchive {

/// Longest entry name, in bytes of UTF-8, that an extraction writes
constexpr qsizetype kMaxEntryNameBytes = 4096;

/// What in an entry name would be written differently, or not at all, on some system, such as "a device name of
/// Windows", or an empty string if the name is portable. The name, whose separator is the slash, must be at most
/// kMaxEntryNameBytes long in UTF-8. Each component of it must be at most 255 bytes in UTF-8, must not end with a dot
/// or a space, must not hold a control character or one of < > : " | ? * \, and must not be a device name of Windows
/// (CON, PRN, AUX, NUL, COM0 to COM9, LPT0 to LPT9, CONIN$, CONOUT$), also with an extension. Empty components and
/// "." and ".." are left to the check that the entry stays inside the output directory.
QString nonPortableNameReason(QStringView name);

/// The key under which two names are the same on a file system that ignores case or Unicode normalisation, as
/// those of Windows and macOS do
QString nameCollisionKey(QStringView name);

/// \brief The spellings of the paths that an extraction writes, so that a path in another case or normalisation
/// is not written over one written before
///
/// The paths are held one component at a time, as a tree: a path costs memory in proportion to the components of it
/// that no earlier path has.
class EntrySpellings
{
public:
    /// Claim the spelling of a path relative to the output directory, and of each directory above it
    /// @return An empty string if the path is claimed, or the spelling of the earlier path that the path, or a
    ///         directory above it, takes under another spelling. Nothing is claimed then
    QString claim(QStringView relativePath);

    /// Characters held for the components claimed so far, their keys included
    qsizetype storedCharacters() const { return _storedCharacters; }

private:
    struct Component
    {
        quint32 id;
        QString spelling;
    };

    /// Component by the id of its parent (0 = the output directory) and its nameCollisionKey()
    QHash<std::pair<quint32, QString>, Component> _components;
    quint32 _lastId = 0;
    qsizetype _storedCharacters = 0;
};

}  // namespace LibArchive
