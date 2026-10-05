#pragma once

#include "QmlUITestBase.h"

/// Changing vertical units while the guided altitude slider is open must re-capture the
/// slider range in the new units, so the value sent to the vehicle uses the displayed units.
class FlyViewGuidedUnitsUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testSliderFollowsUnitsChange();
};
