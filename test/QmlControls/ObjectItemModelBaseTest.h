#pragma once

#include <QtCore/QObject>

#include "UnitTest.h"

class ObjectItemModelBaseTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Nested beginResetModel / endResetModel
    void _singleResetCallsBaseOnce();
    void _nestedResetTwoDeep();
    void _nestedResetThreeDeep();
    void _endResetWithoutBeginLogsWarning();

    // Count signal suppression during reset
    void _countChangedSuppressedInsideReset();
    void _countChangedEmittedOnEndReset();

    // Dirty tracking via _childDirtyChanged
    void _childDirtySetsDirtyTrue();
    void _childDirtyDoesNotClearDirty();
    void _childDirtyEmitsDirtyChanged();

    // roleNames
    void _roleNamesContainObjectAndText();
};
