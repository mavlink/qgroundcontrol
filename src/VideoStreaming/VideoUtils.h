/****************************************************************************
 *
 * Copyright (c) 2016, Intel Corporation
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Streaming Utils
 *   @author Ricardo de Almeida Gonzaga <ricardo.gonzaga@intel.com>
 */

#include <QHash>
#include <QStringList>

#ifndef VIDEO_UTILS_H
#define VIDEO_UTILS_H

#define fourcc(a,b,c,d) ((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24)

enum PixelFormat {
    // RGB formats
    RGB1  = fourcc('R', 'G', 'B', '1'),
    R444  = fourcc('R', '4', '4', '4'),
    RGB0  = fourcc('R', 'G', 'B', '0'),
    RGBP  = fourcc('R', 'G', 'B', 'P'),
    RGBQ  = fourcc('R', 'G', 'B', 'Q'),
    RGBR  = fourcc('R', 'G', 'B', 'R'),
    BGR3  = fourcc('B', 'G', 'R', '3'),
    RGB3  = fourcc('R', 'G', 'B', '3'),
    BGR4  = fourcc('B', 'G', 'R', '4'),
    RGB4  = fourcc('R', 'G', 'B', '4'),
    // Grey formats
    GREY  = fourcc('G', 'R', 'E', 'Y'),
    Y10   = fourcc('Y', '1', '0', ' '),
    Y16   = fourcc('Y', '1', '6', ' '),
    // Palette formats
    PAL8  = fourcc('P', 'A', 'L', '8'),
    // Luminance+Chrominance formats
    YVU9  = fourcc('Y', 'V', 'U', '9'),
    YV12  = fourcc('Y', 'V', '1', '2'),
    YUYV  = fourcc('Y', 'U', 'Y', 'V'),
    YYUV  = fourcc('Y', 'Y', 'U', 'V'),
    YVYU  = fourcc('Y', 'V', 'Y', 'U'),
    UYVY  = fourcc('U', 'Y', 'V', 'Y'),
    VYUY  = fourcc('V', 'Y', 'U', 'Y'),
    F422P = fourcc('4', '2', '2', 'P'),
    F411P = fourcc('4', '1', '1', 'P'),
    Y41P  = fourcc('Y', '4', '1', 'P'),
    Y444  = fourcc('Y', '4', '4', '4'),
    YUVO  = fourcc('Y', 'U', 'V', 'O'),
    YUVP  = fourcc('Y', 'U', 'V', 'P'),
    YUV4  = fourcc('Y', 'U', 'V', '4'),
    YUV9  = fourcc('Y', 'U', 'V', '9'),
    YU12  = fourcc('Y', 'U', '1', '2'),
    HI24  = fourcc('H', 'I', '2', '4'),
    HM12  = fourcc('H', 'M', '1', '2'),
    // Two planes -- one Y, one Cr + Cb interleaved
    NV12  = fourcc('N', 'V', '1', '2'),
    NV21  = fourcc('N', 'V', '2', '1'),
    NV16  = fourcc('N', 'V', '1', '6'),
    NV61  = fourcc('N', 'V', '6', '1'),
    // Bayer formats
    BA81  = fourcc('B', 'A', '8', '1'),
    GBRG  = fourcc('G', 'B', 'R', 'G'),
    GRBG  = fourcc('G', 'R', 'B', 'G'),
    RGGB  = fourcc('R', 'G', 'G', 'B'),
    BG10  = fourcc('B', 'G', '1', '0'),
    GB10  = fourcc('G', 'B', '1', '0'),
    BA10  = fourcc('B', 'A', '1', '0'),
    RG10  = fourcc('R', 'G', '1', '0'),
    BD10  = fourcc('B', 'D', '1', '0'),
    BYR2  = fourcc('B', 'Y', 'R', '2'),
    // Compressed formats
    MJPG  = fourcc('M', 'J', 'P', 'G'),
    JPEG  = fourcc('J', 'P', 'E', 'G'),
    dvsd  = fourcc('d', 'v', 's', 'd'),
    MPEG  = fourcc('M', 'P', 'E', 'G'),
    // Vendor-specific formats
    CPIA  = fourcc('C', 'P', 'I', 'A'),
    WNVA  = fourcc('W', 'N', 'V', 'A'),
    S910  = fourcc('S', '9', '1', '0'),
    S920  = fourcc('S', '9', '2', '0'),
    PWC1  = fourcc('P', 'W', 'C', '1'),
    PWC2  = fourcc('P', 'W', 'C', '2'),
    E625  = fourcc('E', '6', '2', '5'),
    S501  = fourcc('S', '5', '0', '1'),
    S505  = fourcc('S', '5', '0', '5'),
    S508  = fourcc('S', '5', '0', '8'),
    S561  = fourcc('S', '5', '6', '1'),
    P207  = fourcc('P', '2', '0', '7'),
    M310  = fourcc('M', '3', '1', '0'),
    SONX  = fourcc('S', 'O', 'N', 'X'),
    F905C = fourcc('9', '0', '5', 'C'),
    PJPG  = fourcc('P', 'J', 'P', 'G'),
    F0511 = fourcc('0', '5', '1', '1'),
    F0518 = fourcc('0', '5', '1', '8'),
    S680  = fourcc('S', '6', '8', '0'),
};

#undef fourcc

struct FormatPipelineElements {
    const char *caps;
    const char *demux;
    const char *parser;
    const char *decoder;
};

extern QString pixelFormatToFourCC(PixelFormat f);

extern struct FormatPipelineElements getFormatPipelineElements(PixelFormat f);
extern QStringList getStreams();
extern PixelFormat getStreamFormat();

#endif // VIDEO_UTILS_H
