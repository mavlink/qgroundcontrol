// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMLINEWRITERFACTORY_P_H
#define QQMLDOMLINEWRITERFACTORY_P_H

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

#include "qqmldomlinewriter_p.h"
#include "qqmldomindentinglinewriter_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

inline std::unique_ptr<LineWriter> createLineWriter(const SinkF &innerSink, const QString &fileName,
                                                    const LineWriterOptions &options)
{
    return options.maxLineLength > 0
            ? std::make_unique<IndentingLineWriter>(innerSink, fileName, options)
            : std::make_unique<LineWriter>(innerSink, fileName, options);
}

} // namespace Dom
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLDOMLINEWRITERFACTORY_P_H
