#pragma once

#include "UnitTest.h"

class PatchTextureDataTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sizeFormatAndByteCount();
    void _transparencyTracksSourceImage();
};
