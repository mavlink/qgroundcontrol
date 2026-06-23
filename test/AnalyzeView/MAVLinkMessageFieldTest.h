#pragma once

#include "UnitTest.h"

class MAVLinkMessageFieldTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _constructionTest();
    void _initialValueEmptyTest();
    void _selectableDefaultsTrueTest();
    void _setSelectableTest();
    void _updateValueChangesValueTest();
    void _updateValueNoopOnSameValueTest();
    void _labelFormatTest();
};
