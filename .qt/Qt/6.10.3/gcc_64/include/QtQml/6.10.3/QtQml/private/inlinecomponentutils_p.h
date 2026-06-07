// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef INLINECOMPONENTUTILS_P_H
#define INLINECOMPONENTUTILS_P_H

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

#include <private/qqmlmetatype_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4resolvedtypereference_p.h>

QT_BEGIN_NAMESPACE

namespace icutils {
struct Node {
private:
    using IndexType = std::vector<QV4::CompiledData::InlineComponent>::size_type;
    using IndexField = quint32_le_bitfield_member<0, 30, IndexType>;
    using TemporaryMarkField = quint32_le_bitfield_member<30, 1>;
    using PermanentMarkField = quint32_le_bitfield_member<31, 1>;
    quint32_le_bitfield_union<IndexField, TemporaryMarkField, PermanentMarkField> m_data;

public:
    Node() = default;
    Node(const Node &) = default;
    Node(Node &&) = default;
    Node& operator=(Node const &) = default;
    Node& operator=(Node &&) = default;
    bool operator==(Node const &other) const {return m_data.data() == other.m_data.data(); }

    Node(IndexType s) : m_data(QSpecialIntegerBitfieldZero) { m_data.set<IndexField>(s); }

    bool hasPermanentMark() const { return m_data.get<PermanentMarkField>(); }
    bool hasTemporaryMark() const { return m_data.get<TemporaryMarkField>(); }

    void setPermanentMark()
    {
        m_data.set<TemporaryMarkField>(0);
        m_data.set<PermanentMarkField>(1);
    }

    void setTemporaryMark()
    {
        m_data.set<TemporaryMarkField>(1);
    }

    IndexType index() const { return m_data.get<IndexField>(); }
};

using NodeList = std::vector<Node>;
using AdjacencyList = std::vector<std::vector<Node*>>;

inline bool containedInSameType(const QQmlType &a, const QQmlType &b)
{
    return QQmlMetaType::equalBaseUrls(a.sourceUrl(), b.sourceUrl());
}

template<typename ObjectContainer, typename InlineComponent>
void fillAdjacencyListForInlineComponents(ObjectContainer *objectContainer,
                                          AdjacencyList &adjacencyList, NodeList &nodes,
                                          const std::vector<InlineComponent> &allICs)
{
    using CompiledObject = typename ObjectContainer::CompiledObject;
    // add an edge from A to B if A and B are inline components with the same containing type
    // and A inherits from B (ignore indirect chains through external types for now)
    // or if A instantiates B
    for (typename std::vector<InlineComponent>::size_type i = 0; i < allICs.size(); ++i) {
        const auto& ic = allICs[i];
        const CompiledObject *obj = objectContainer->objectAt(ic.objectIndex);
        QV4::ResolvedTypeReference *currentICTypeRef = objectContainer->resolvedType(ic.nameIndex);
        auto createEdgeFromTypeRef = [&](QV4::ResolvedTypeReference *targetTypeRef) {
            if (targetTypeRef) {
                const auto targetType = targetTypeRef->type();
                if (targetType.isInlineComponentType()
                        && containedInSameType(targetType, currentICTypeRef->type())) {
                    auto icIt = std::find_if(allICs.cbegin(), allICs.cend(), [&](const QV4::CompiledData::InlineComponent &icSearched){
                        return objectContainer->stringAt(icSearched.nameIndex)
                               == targetType.elementName();
                    });
                    Q_ASSERT(icIt != allICs.cend());
                    Node& target = nodes[i];
                    adjacencyList[std::distance(allICs.cbegin(), icIt)].push_back(&target);
                }
            }
        };
        if (obj->inheritedTypeNameIndex != 0) {
            QV4::ResolvedTypeReference *parentTypeRef = objectContainer->resolvedType(obj->inheritedTypeNameIndex);
            createEdgeFromTypeRef(parentTypeRef);

        }
        auto referencedInICObjectIndex = ic.objectIndex + 1;
        while (int(referencedInICObjectIndex) < objectContainer->objectCount()) {
            auto potentiallyReferencedInICObject = objectContainer->objectAt(referencedInICObjectIndex);
            bool stillInIC
                    = !potentiallyReferencedInICObject->hasFlag(
                              QV4::CompiledData::Object::IsInlineComponentRoot)
                    && potentiallyReferencedInICObject->hasFlag(
                            QV4::CompiledData::Object::IsPartOfInlineComponent);
            if (!stillInIC)
                break;
            createEdgeFromTypeRef(objectContainer->resolvedType(potentiallyReferencedInICObject->inheritedTypeNameIndex));
            ++referencedInICObjectIndex;
        }
    }
};

inline void topoVisit(Node *node, AdjacencyList &adjacencyList, bool &hasCycle,
                      NodeList &nodesSorted)
{
    if (node->hasPermanentMark())
        return;
    if (node->hasTemporaryMark()) {
        hasCycle = true;
        return;
    }
    node->setTemporaryMark();

    auto const &edges = adjacencyList[node->index()];
    for (auto edgeTarget =edges.begin(); edgeTarget != edges.end(); ++edgeTarget) {
        topoVisit(*edgeTarget, adjacencyList, hasCycle, nodesSorted);
    }

    node->setPermanentMark();
    nodesSorted.push_back(*node);
};

// Use DFS based topological sorting (https://en.wikipedia.org/wiki/Topological_sorting)
inline NodeList topoSort(NodeList &nodes, AdjacencyList &adjacencyList, bool &hasCycle)
{
    NodeList nodesSorted;
    nodesSorted.reserve(nodes.size());

    hasCycle = false;
    auto currentNodeIt = std::find_if(nodes.begin(), nodes.end(), [](const Node& node) {
        return !node.hasPermanentMark();
    });
    // Do a topological sort of all inline components
    // afterwards, nodesSorted contains the nodes for the inline components in reverse topological order
    while (currentNodeIt != nodes.end() && !hasCycle) {
        Node& currentNode = *currentNodeIt;
        topoVisit(&currentNode, adjacencyList, hasCycle, nodesSorted);
        currentNodeIt = std::find_if(nodes.begin(), nodes.end(), [](const Node& node) {
            return !node.hasPermanentMark();
        });
    }
    return nodesSorted;
}
}

QT_END_NAMESPACE

#endif // INLINECOMPONENTUTILS_P_H
