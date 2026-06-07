// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TASKING_CONDITIONAL_H
#define TASKING_CONDITIONAL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "tasking_global.h"

#include "tasktree.h"

QT_BEGIN_NAMESPACE

namespace Tasking {

class Then;
class ThenItem;
class ElseItem;
class ElseIfItem;

class TASKING_EXPORT If
{
public:
    explicit If(const ExecutableItem &condition) : m_condition(condition) {}

    template <typename Handler,
              std::enable_if_t<!std::is_base_of_v<ExecutableItem, std::decay_t<Handler>>, bool> = true>
    explicit If(Handler &&handler) : m_condition(Sync(std::forward<Handler>(handler))) {}

private:
    TASKING_EXPORT friend ThenItem operator>>(const If &ifItem, const Then &thenItem);

    friend class ThenItem;
    ExecutableItem m_condition;
};

class TASKING_EXPORT ElseIf
{
public:
    explicit ElseIf(const ExecutableItem &condition) : m_condition(condition) {}

    template <typename Handler,
             std::enable_if_t<!std::is_base_of_v<ExecutableItem, std::decay_t<Handler>>, bool> = true>
    explicit ElseIf(Handler &&handler) : m_condition(Sync(std::forward<Handler>(handler))) {}

private:
    friend class ElseIfItem;
    ExecutableItem m_condition;
};

class TASKING_EXPORT Else
{
public:
    explicit Else(const GroupItems &children) : m_body({children}) {}
    explicit Else(std::initializer_list<GroupItem> children) : m_body({children}) {}

private:
    friend class ElseItem;
    Group m_body;
};

class TASKING_EXPORT Then
{
public:
    explicit Then(const GroupItems &children) : m_body({children}) {}
    explicit Then(std::initializer_list<GroupItem> children) : m_body({children}) {}

private:
    friend class ThenItem;
    Group m_body;
};

class ConditionData
{
public:
    std::optional<ExecutableItem> m_condition;
    Group m_body;
};

class ElseIfItem;

class TASKING_EXPORT ThenItem
{
public:
    operator ExecutableItem() const;

private:
    ThenItem(const If &ifItem, const Then &thenItem);
    ThenItem(const ElseIfItem &elseIfItem, const Then &thenItem);

    TASKING_EXPORT friend ElseItem operator>>(const ThenItem &thenItem, const Else &elseItem);
    TASKING_EXPORT friend ElseIfItem operator>>(const ThenItem &thenItem, const ElseIf &elseIfItem);
    TASKING_EXPORT friend ThenItem operator>>(const If &ifItem, const Then &thenItem);
    TASKING_EXPORT friend ThenItem operator>>(const ElseIfItem &elseIfItem, const Then &thenItem);

    friend class ElseItem;
    friend class ElseIfItem;
    QList<ConditionData> m_conditions;
};

class TASKING_EXPORT ElseItem
{
public:
    operator ExecutableItem() const;

private:
    ElseItem(const ThenItem &thenItem, const Else &elseItem);

    TASKING_EXPORT friend ElseItem operator>>(const ThenItem &thenItem, const Else &elseItem);

    QList<ConditionData> m_conditions;
};

class TASKING_EXPORT ElseIfItem
{
private:
    ElseIfItem(const ThenItem &thenItem, const ElseIf &elseIfItem);

    TASKING_EXPORT friend ThenItem operator>>(const ElseIfItem &elseIfItem, const Then &thenItem);
    TASKING_EXPORT friend ElseIfItem operator>>(const ThenItem &thenItem, const ElseIf &elseIfItem);

    friend class ThenItem;
    QList<ConditionData> m_conditions;
    ExecutableItem m_nextCondition;
};

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_CONDITIONAL_H
