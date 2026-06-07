// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMMODULEINDEX_P_H
#define QQMLDOMMODULEINDEX_P_H

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

#include "qqmldomelements_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT ModuleScope final : public DomBase
{
public:
    constexpr static DomType kindValue = DomType::ModuleScope;
    DomType kind() const override { return kindValue; }

    ModuleScope(const QString &uri = QString(), const Version &version = Version())
        : uri(uri), version(version)
    {
    }

    Path pathFromOwner() const
    {
        return Path::fromField(Fields::moduleScope)
                .withKey(version.isValid() ? QString::number(version.minorVersion) : QString());
    }
    Path pathFromOwner(const DomItem &) const override { return pathFromOwner(); }
    Path canonicalPath(const DomItem &self) const override
    {
        return self.owner().canonicalPath().withPath(pathFromOwner());
    }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;

    QString uri;
    Version version;
};

class QMLDOM_EXPORT ModuleIndex final : public OwningItem
{
    Q_DECLARE_TR_FUNCTIONS(ModuleIndex);

protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;

public:
    enum class Status { NotLoaded, Loading, Loaded };
    constexpr static DomType kindValue = DomType::ModuleIndex;
    DomType kind() const override { return kindValue; }

    ModuleIndex(
            const QString &uri, int majorVersion, int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : OwningItem(derivedFrom, lastDataUpdateAt), m_uri(uri), m_majorVersion(majorVersion)
    {
    }

    ModuleIndex(const ModuleIndex &o);

    ~ModuleIndex();

    std::shared_ptr<ModuleIndex> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ModuleIndex>(doCopy(self));
    }

    Path canonicalPath(const DomItem &) const override
    {
        return Paths::moduleIndexPath(uri(), majorVersion());
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;

    QSet<QString> exportNames(const DomItem &self) const;

    QList<DomItem> exportsWithNameAndMinorVersion(const DomItem &self, const QString &name,
                                                  int minorVersion) const;

    QString uri() const { return m_uri; }
    int majorVersion() const { return m_majorVersion; }
    QList<Path> sources() const;

    QList<int> minorVersions() const
    {
        QMutexLocker l(mutex());
        return m_moduleScope.keys();
    }
    ModuleScope *ensureMinorVersion(int minorVersion);
    void mergeWith(const std::shared_ptr<ModuleIndex> &o);
    void addQmltypeFilePath(const Path &p)
    {
        QMutexLocker l(mutex());
        if (!m_qmltypesFilesPaths.contains(p))
            m_qmltypesFilesPaths.append(p);
    }

    QList<Path> qmldirsToLoad(const DomItem &self);
    QList<Path> qmltypesFilesPaths() const
    {
        QMutexLocker l(mutex());
        return m_qmltypesFilesPaths;
    }
    QList<Path> qmldirPaths() const
    {
        QMutexLocker l(mutex());
        return m_qmldirPaths;
    }
    QList<Path> directoryPaths() const
    {
        QMutexLocker l(mutex());
        return m_directoryPaths;
    }
    QList<DomItem> autoExports(const DomItem &self) const;

private:
    QString m_uri;
    int m_majorVersion;

    QList<Path> m_qmltypesFilesPaths;
    QList<Path> m_qmldirPaths;
    QList<Path> m_directoryPaths;
    QMap<int, ModuleScope *> m_moduleScope;
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMMODULEINDEX_P_H
