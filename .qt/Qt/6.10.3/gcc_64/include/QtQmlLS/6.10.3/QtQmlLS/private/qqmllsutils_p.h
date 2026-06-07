// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLANGUAGESERVERUTILS_P_H
#define QLANGUAGESERVERUTILS_P_H

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

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <algorithm>
#include <optional>
#include <tuple>
#include <variant>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QQmlLSUtilsLog);

namespace QQmlLSUtils {

struct ItemLocation
{
    QQmlJS::Dom::DomItem domItem;
    QQmlJS::Dom::FileLocations::Tree fileLocation;
};

struct TextPosition
{
    int line = 0;
    int character = 0;
};

enum IdentifierType : quint8 {
    NotAnIdentifier, // when resolving expressions like `Qt.point().x` for example, where
                     // `Qt.point()` is not an identifier

    JavaScriptIdentifier,
    PropertyIdentifier,
    PropertyChangedSignalIdentifier,
    PropertyChangedHandlerIdentifier,
    SignalIdentifier,
    SignalHandlerIdentifier,
    MethodIdentifier,
    LambdaMethodIdentifier,
    QmlObjectIdIdentifier,
    SingletonIdentifier,
    EnumeratorIdentifier,
    EnumeratorValueIdentifier,
    AttachedTypeIdentifier,
    GroupedPropertyIdentifier,
    QmlComponentIdentifier,
    QualifiedModuleIdentifier,
};

struct ErrorMessage
{
    int code = 0;
    QString message;
};

struct ExpressionType
{
    std::optional<QString> name;
    QQmlJSScope::ConstPtr semanticScope;
    IdentifierType type = NotAnIdentifier;
};

class Location
{
public:
    Location() = default;
    Location(const QString &filename, const QQmlJS::SourceLocation &sourceLocation,
             const TextPosition &end)
        : m_filename(filename), m_sourceLocation(sourceLocation), m_end(end)
    {
    }

    QString filename() const { return m_filename; }
    QQmlJS::SourceLocation sourceLocation() const { return m_sourceLocation; }
    TextPosition end() const { return m_end; }

    static Location from(const QString &fileName, const QString &code, qsizetype startLine,
                         qsizetype startCharacter, qsizetype length);
    static Location from(const QString &fileName, const QQmlJS::SourceLocation &sourceLocation,
                         const QString &code);
    static std::optional<Location> tryFrom(const QString &fileName,
                                           const QQmlJS::SourceLocation &sourceLocation,
                                           const QQmlJS::Dom::DomItem &someItem);

    friend bool operator<(const Location &a, const Location &b)
    {
        return std::make_tuple(a.m_filename, a.m_sourceLocation.begin(), a.m_sourceLocation.end())
                < std::make_tuple(b.m_filename, b.m_sourceLocation.begin(),
                                  b.m_sourceLocation.end());
    }
    friend bool operator==(const Location &a, const Location &b)
    {
        return std::make_tuple(a.m_filename, a.m_sourceLocation.begin(), a.m_sourceLocation.end())
                == std::make_tuple(b.m_filename, b.m_sourceLocation.begin(),
                                   b.m_sourceLocation.end());
    }

private:
    QString m_filename;
    QQmlJS::SourceLocation m_sourceLocation;
    TextPosition m_end;
};

/*!
Represents a rename operation where the file itself needs to be renamed.
\internal
*/
struct FileRename
{
    QString oldFilename;
    QString newFilename;

    friend bool comparesEqual(const FileRename &a, const FileRename &b) noexcept
    {
        return std::tie(a.oldFilename, a.newFilename) == std::tie(b.oldFilename, b.newFilename);
    }
    friend Qt::strong_ordering compareThreeWay(const FileRename &a, const FileRename &b) noexcept
    {
        if (a.oldFilename != b.oldFilename)
            return compareThreeWay(a.oldFilename, b.oldFilename);
        return compareThreeWay(a.newFilename, b.newFilename);
    }
    Q_DECLARE_STRONGLY_ORDERED(FileRename);
};

struct Edit
{
    Location location;
    QString replacement;

    static Edit from(const QString &fileName, const QString &code, qsizetype startLine,
                     qsizetype startCharacter, qsizetype length, const QString &newName);

    friend bool operator<(const Edit &a, const Edit &b)
    {
        return std::make_tuple(a.location, a.replacement)
                < std::make_tuple(b.location, b.replacement);
    }
    friend bool operator==(const Edit &a, const Edit &b)
    {
        return std::make_tuple(a.location, a.replacement)
                == std::make_tuple(b.location, b.replacement);
    }
};

/*!
Represents the locations where some highlighting should take place, like in the "find all
references" feature of the LSP. Those locations are pointing to parts of a Qml file or to a Qml
file name.

The file names are not reported as usage to the LSP and are currently only needed for the renaming
operation to be able to rename files.

\internal
*/
class Usages
{
public:
    void sort();
    bool isEmpty() const;

    friend bool comparesEqual(const Usages &a, const Usages &b) noexcept
    {
        return a.m_usagesInFile == b.m_usagesInFile && a.m_usagesInFilename == b.m_usagesInFilename;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(Usages)

    Usages() = default;
    Usages(const QList<Location> &usageInFile, const QList<QString> &usageInFilename);

    QList<Location> usagesInFile() const { return m_usagesInFile; };
    QList<QString> usagesInFilename() const { return m_usagesInFilename; };

    void appendUsage(const Location &edit)
    {
        if (!m_usagesInFile.contains(edit))
            m_usagesInFile.append(edit);
    };
    void appendFilenameUsage(const QString &edit)
    {

        if (!m_usagesInFilename.contains(edit))
            m_usagesInFilename.append(edit);
    };

private:
    QList<Location> m_usagesInFile;
    QList<QString> m_usagesInFilename;
};

/*!
Represents the locations where a renaming should take place. Parts of text inside a file can be
renamed and also filename themselves can be renamed.

\internal
*/
class RenameUsages
{
public:
    friend bool comparesEqual(const RenameUsages &a, const RenameUsages &b) noexcept
    {
        return std::tie(a.m_renamesInFile, a.m_renamesInFilename)
                == std::tie(b.m_renamesInFile, b.m_renamesInFilename);
    }
    Q_DECLARE_EQUALITY_COMPARABLE(RenameUsages)

    RenameUsages() = default;
    RenameUsages(const QList<Edit> &renamesInFile, const QList<FileRename> &renamesInFilename);

    QList<Edit> renameInFile() const { return m_renamesInFile; };
    QList<FileRename> renameInFilename() const { return m_renamesInFilename; };

    void appendRename(const Edit &edit) { m_renamesInFile.append(edit); };
    void appendRename(const FileRename &edit) { m_renamesInFilename.append(edit); };

private:
    QList<Edit> m_renamesInFile;
    QList<FileRename> m_renamesInFilename;
};

/*!
   \internal
    Choose whether to resolve the owner type or the entire type (the latter is only required to
    resolve the types of qualified names and property accesses).

    For properties, methods, enums and co:
    * ResolveOwnerType returns the base type of the owner that owns the property, method, enum
      and co. For example, resolving "x" in "myRectangle.x" will return the Item as the owner, as
      Item is the base type of Rectangle that defines the "x" property.
    * ResolveActualTypeForFieldMemberExpression is used to resolve field member expressions, and
      might lose some information about the owner. For example, resolving "x" in "myRectangle.x"
      will return the JS type for float that was used to define the "x" property.
 */
enum ResolveOptions {
    ResolveOwnerType,
    ResolveActualTypeForFieldMemberExpression,
};

using DomItem = QQmlJS::Dom::DomItem;

qsizetype textOffsetFrom(const QString &code, int row, int character);
TextPosition textRowAndColumnFrom(const QString &code, qsizetype offset);
QList<ItemLocation> itemsFromTextLocation(const DomItem &file, int line, int character);
DomItem sourceLocationToDomItem(const DomItem &file, const QQmlJS::SourceLocation &location);
QByteArray lspUriToQmlUrl(const QByteArray &uri);
QByteArray qmlUrlToLspUri(const QByteArray &url);
QLspSpecification::Range qmlLocationToLspLocation(Location qmlLocation);
DomItem baseObject(const DomItem &qmlObject);
std::optional<Location> findTypeDefinitionOf(const DomItem &item);
std::optional<Location> findDefinitionOf(const DomItem &item);
Usages findUsagesOf(const DomItem &item);

std::optional<ErrorMessage>
checkNameForRename(const DomItem &item, const QString &newName,
                   const std::optional<ExpressionType> &targetType = std::nullopt);
RenameUsages renameUsagesOf(const DomItem &item, const QString &newName,
                            const std::optional<ExpressionType> &targetType = std::nullopt);
std::optional<ExpressionType> resolveExpressionType(const DomItem &item, ResolveOptions);
bool isValidEcmaScriptIdentifier(QStringView view);

std::pair<QString, QStringList> cmakeBuildCommand(const QString &path);

bool isFieldMemberExpression(const DomItem &item);
bool isFieldMemberAccess(const DomItem &item);
bool isFieldMemberBase(const DomItem &item);
QStringList fieldMemberExpressionBits(const DomItem &item, const DomItem &stopAtChild = {});

QString qualifiersFrom(const DomItem &el);

QQmlJSScope::ConstPtr findDefiningScopeForProperty(const QQmlJSScope::ConstPtr &referrerScope,
                                                        const QString &nameToCheck);
QQmlJSScope::ConstPtr findDefiningScopeForBinding(const QQmlJSScope::ConstPtr &referrerScope,
                                                        const QString &nameToCheck);
QQmlJSScope::ConstPtr findDefiningScopeForMethod(const QQmlJSScope::ConstPtr &referrerScope,
                                                        const QString &nameToCheck);
QQmlJSScope::ConstPtr findDefiningScopeForEnumeration(const QQmlJSScope::ConstPtr &referrerScope,
                                                            const QString &nameToCheck);
QQmlJSScope::ConstPtr findDefiningScopeForEnumerationKey(const QQmlJSScope::ConstPtr &referrerScope,
                                                        const QString &nameToCheck);
} // namespace QQmlLSUtils

QT_END_NAMESPACE

#endif // QLANGUAGESERVERUTILS_P_H
