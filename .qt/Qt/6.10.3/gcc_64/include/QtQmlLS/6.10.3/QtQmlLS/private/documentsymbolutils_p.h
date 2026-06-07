// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef DOCUMENT_SYMBOL_UTILS_P_H
#define DOCUMENT_SYMBOL_UTILS_P_H

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

#include <QtQmlDom/private/qqmldom_fwd_p.h>
#include <qxpfunctional.h>

QT_BEGIN_NAMESPACE

namespace QLspSpecification {
class DocumentSymbol;
class Range;
enum class SymbolKind;
} // namespace QLspSpecification

namespace DocumentSymbolUtils {

using QQmlJS::Dom::DomItem;
using SymbolsList = QList<QLspSpecification::DocumentSymbol>;
using AssemblingFunction =
        qxp::function_ref<SymbolsList(const QQmlJS::Dom::DomItem &item, SymbolsList &&) const>;

[[nodiscard]] SymbolsList buildSymbolOrReturnChildren(const DomItem &item, SymbolsList &&children);

[[nodiscard]] std::pair<QLspSpecification::Range, QLspSpecification::Range>
symbolRangesOf(const DomItem &item);

[[nodiscard]] QByteArray symbolNameOf(const DomItem &item);

[[nodiscard]] QLspSpecification::SymbolKind symbolKindOf(const DomItem &item);

[[nodiscard]] std::optional<QByteArray> tryGetDetailOf(const DomItem &item);

[[nodiscard]] SymbolsList
assembleSymbolsForQmlFile(const DomItem &item,
                          const AssemblingFunction af = buildSymbolOrReturnChildren);

void reorganizeForOutlineView(SymbolsList &symbols);
} // namespace DocumentSymbolUtils

QT_END_NAMESPACE

#endif // DOCUMENT_SYMBOL_UTILS_P_H
