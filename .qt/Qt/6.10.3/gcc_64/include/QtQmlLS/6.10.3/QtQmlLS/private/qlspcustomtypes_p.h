// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLSPCUSTOMTYPES_P_H
#define QLSPCUSTOMTYPES_P_H

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

#include <QtLanguageServer/private/qlanguageserverspec_p.h>

QT_BEGIN_NAMESPACE

namespace QLspSpecification {

class UriToBuildDirs
{
public:
    QByteArray baseUri = {};
    QList<QByteArray> buildDirs = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "baseUri", baseUri);
        field(w, "buildDirs", buildDirs);
    }
};

namespace Notifications {
constexpr auto AddBuildDirsMethod = "$/addBuildDirs";

class AddBuildDirsParams
{
public:
    QList<UriToBuildDirs> buildDirsToSet = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "buildDirsToSet", buildDirsToSet);
    }
};
} // namespace Notifications
} // namespace QLspSpecification

QT_END_NAMESPACE

#endif // QLSPCUSTOMTYPES_P_H
