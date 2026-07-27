#pragma once

#include "VehicleConfigUITestBase.h"

class QQuickItem;

/// UI test for ParameterDiffDialog: verifies the load-from-file summary and
/// sections for each diff outcome (changes, all-match, missing-on-vehicle,
/// new-to-vehicle) and that accepting the dialog sends the diff.
class ParameterDiffDialogUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

private slots:
    void _testDiffDialogCases();

private:
    void _openDiffDialogForFile(const QString& filePath);
    bool _labelTextContains(const QString& objectName, const QString& substring);
};
