#pragma once

#include "QGCPalette.h"
#include "UnitTest.h"

class QGCPaletteTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void _initializesApplicationPalette();
    void _syncsApplicationPalette_data();
    void _syncsApplicationPalette();
    void _textContrast_data();
    void _textContrast();
    void _semanticContrast_data();
    void _semanticContrast();

private:
    QGCPalette::Theme _originalTheme = QGCPalette::Dark;
};
