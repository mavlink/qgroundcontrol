// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMMOCK_P_H
#define QQMLDOMMOCK_P_H

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

#include "qqmldomitem_p.h"
#include "qqmldomconstants_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomcomments_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#include <QtCore/QCborValue>
#include <QtCore/QCborMap>
#include <QtCore/QMutexLocker>

#include <functional>
#include <limits>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

// mainly for debugging purposes
class MockObject final : public CommentableDomElement
{
public:
    constexpr static DomType kindValue = DomType::MockObject;
    DomType kind() const override { return kindValue; }

    MockObject(const Path &pathFromOwner = Path(), QMap<QString, MockObject> subObjects = {},
               QMap<QString, QCborValue> subValues = {})
        : CommentableDomElement(pathFromOwner), subObjects(subObjects), subValues(subValues)
    {
    }

    MockObject copy() const;
    std::pair<QString, MockObject> asStringPair() const;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;

    QMap<QString, MockObject> subObjects;
    QMap<QString, QCborValue> subValues;
};

// mainly for debugging purposes
class MockOwner final : public OwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;

public:
    constexpr static DomType kindValue = DomType::MockOwner;
    DomType kind() const override { return kindValue; }

    MockOwner(const Path &pathFromTop = Path(), int derivedFrom = 0,
              QMap<QString, MockObject> subObjects = {}, QMap<QString, QCborValue> subValues = {},
              QMap<QString, QMap<QString, MockObject>> subMaps = {},
              QMap<QString, QMultiMap<QString, MockObject>> subMultiMaps = {},
              QMap<QString, QList<MockObject>> subLists = {})
        : OwningItem(derivedFrom),
          pathFromTop(pathFromTop),
          subObjects(subObjects),
          subValues(subValues),
          subMaps(subMaps),
          subMultiMaps(subMultiMaps),
          subLists(subLists)
    {
    }

    MockOwner(const Path &pathFromTop, int derivedFrom, QDateTime dataRefreshedAt,
              QMap<QString, MockObject> subObjects = {}, QMap<QString, QCborValue> subValues = {})
        : OwningItem(derivedFrom, dataRefreshedAt),
          pathFromTop(pathFromTop),
          subObjects(subObjects),
          subValues(subValues)
    {
    }

    MockOwner(const MockOwner &o);

    std::shared_ptr<MockOwner> makeCopy(const DomItem &self) const;
    Path canonicalPath(const DomItem &self) const override;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;

    Path pathFromTop;
    QMap<QString, MockObject> subObjects;
    QMap<QString, QCborValue> subValues;
    QMap<QString, QMap<QString, MockObject>> subMaps;
    QMap<QString, QMultiMap<QString, MockObject>> subMultiMaps;
    QMap<QString, QList<MockObject>> subLists;
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMELEMENTS_P_H
