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
    void _appendObjectWithoutDirtySignal();
};
