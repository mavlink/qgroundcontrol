#pragma once

#include "QGCPalette.h"
#include "UnitTest.h"

class QGCStyleTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void _galleryLoads_data();
    void _galleryLoads();
    void _galleryRenders_data();
    void _galleryRenders();
    void _previewRenders_data();
    void _previewRenders();
    void _layoutProfile();
    void _liveUiScaleReflow();
    void _stylePreferencesTouchMode();
    void _styleTypographyMetrics();
    void _styleKitApplies();

private:
    QGCPalette::Theme _originalTheme = QGCPalette::Dark;
};
