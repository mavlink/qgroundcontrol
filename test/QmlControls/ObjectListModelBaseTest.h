#pragma once

#include <QtCore/QObject>

#include "UnitTest.h"

class ObjectListModelBaseTest : public UnitTest
{
    Q_OBJECT

private slots:
    // index() overrides
    void _indexValidRow();
    void _indexInvalidRow();
    void _indexNonZeroColumnInvalid();
    void _indexWithValidParentInvalid();

    // parent() override
    void _parentAlwaysInvalid();

    // columnCount() override
    void _columnCountAlwaysOne();

    // hasChildren() override
    void _hasChildrenRootEmpty();
    void _hasChildrenRootWithItems();
    void _hasChildrenNonRootAlwaysFalse();
};
