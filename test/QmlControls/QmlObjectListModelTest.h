#pragma once

#include <QtCore/QObject>

#include "QmlObjectListModel.h"
#include "UnitTest.h"

class QmlObjectListModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _childDirtyPropagates();
    void _removeDisconnectsDirtyPropagation();
    void _skipDirtyFirstItemSkipsFirstConnection();
    void _duplicateObjectDirtyPropagation();
    void _objectNameChangesNotifyTextRole();
    void _insertLifetimeDuringNotification();
    void _insertNotificationReentrancy();
    void _reentrantSwapRejected();
    void _firstItemLifetimeDuringNotification();
    void _clearDisconnectsDirtyPropagation();
    void _destroyedObjectsAreRemoved();
    void _destroyedObjectNotificationReentrancy();
    void _destroyedObjectCleanupIsCoalesced();
    void _structuralNotificationReentrancy();
    void _directConnectionModelDeletion();
    void _moveContract();
    void _appendObjectWithoutDirtySignal();
    void _modelContract();
    void _lowerBoundIndex();
};
