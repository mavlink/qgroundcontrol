/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef LIBASS_TYPES_H
#define LIBASS_TYPES_H

#include <stdint.h>

#define VALIGN_SUB 0
#define VALIGN_CENTER 8
#define VALIGN_TOP 4
#define HALIGN_LEFT 1
#define HALIGN_CENTER 2
#define HALIGN_RIGHT 3

/* Opaque objects internally used by libass.  Contents are private. */
typedef struct ass_renderer ASS_Renderer;
typedef struct render_priv ASS_RenderPriv;
typedef struct parser_priv ASS_ParserPriv;
typedef struct ass_library ASS_Library;

/* ASS Style: line */
typedef struct ass_style {
    char *Name;
    char *FontName;
    double FontSize;
    uint32_t PrimaryColour;
    uint32_t SecondaryColour;
    uint32_t OutlineColour;
    uint32_t BackColour;
    int Bold;
    int Italic;
    int Underline;
    int StrikeOut;
    double ScaleX;
    double ScaleY;
    double Spacing;
    double Angle;
    int BorderStyle;
    double Outline;
    double Shadow;
    int Alignment;
    int MarginL;
    int MarginR;
    int MarginV;
    int Encoding;
    int treat_fontname_as_pattern;
    double Blur;
} ASS_Style;

/*
 * ASS_Event corresponds to a single Dialogue line;
 * text is stored as-is, style overrides will be parsed later.
 */
typedef struct ass_event {
    long long Start;            // ms
    long long Duration;         // ms

    int ReadOrder;
    int Layer;
    int Style;
    char *Name;
    int MarginL;
    int MarginR;
    int MarginV;
    char *Effect;
    char *Text;

    ASS_RenderPriv *render_priv;
} ASS_Event;

/**
 * Support for (xy-)vsfilter mangled colors
 *
 * Generally, xy-vsfilter emulates the classic vsfilter behavior of
 * rendering directly into the (usually YCbCr) video. vsfilter is
 * hardcoded to use BT.601(TV) as target colorspace when converting
 * the subtitle RGB color to the video colorspace. This led to major
 * breakage when HDTV video was introduced: HDTV typically uses
 * BT.709(TV), but vsfilter still used BT.601(TV) for conversion.
 *
 * This means classic vsfilter will mangle colors as follows:
 *
 *    screen_rgb = bt_709tv_to_rgb(rgb_to_bt601tv(ass_rgb))
 *
 * Or in general:
 *
 *    screen_rgb = video_csp_to_rgb(rgb_to_bt601tv(ass_rgb))
 *
 * where video_csp is the colorspace of the video with which the
 * subtitle was muxed.
 *
 * xy-vsfilter did not fix this, but instead introduced explicit
 * rules how colors were mangled by adding a "YCbCr Matrix" header.
 * If this header specifies a real colorspace (like BT.601(TV) etc.),
 * xy-vsfilter behaves exactly like vsfilter, but using the specified
 * colorspace for conversion of ASS input RGB to screen RGB:
 *
 *    screen_rgb = video_csp_to_rgb(rgb_to_ycbcr_header_csp(ass_rgb))
 *
 * Further, xy-vsfilter behaves like vsfilter with no changes if the header
 * is missing.
 *
 * The special value "None" means untouched RGB values. Keep in mind that
 * some version of xy-vsfilter are buggy and don't interpret this correctly.
 * It appears some people are advocating that this header value is
 * intended for situations where exact colors do not matter.
 *
 * Note that newer Aegisub versions (the main application to produce ASS
 * subtitle scripts) have an option that tries not to mangle the colors. It
 * is said that if the header is not set to BT.601(TV), the colors are
 * supposed not to be mangled, even if the "YCbCr Matrix" header is not
 * set to "None". In other words, the video colorspace as detected by
 * Aegisub is the same as identified in the file header.
 *
 * In general, misinterpreting this header or not using it will lead to
 * slightly different subtitle colors, which can matter if the subtitle
 * attempts to match solid colored areas in the video.
 *
 * Note that libass doesn't change colors based on this header. It
 * absolutely can't do that, because the video colorspace is required
 * in order to handle this as intended by xy-vsfilter.
 */
typedef enum ASS_YCbCrMatrix {
    YCBCR_DEFAULT = 0,  // Header missing
    YCBCR_UNKNOWN,      // Header could not be parsed correctly
    YCBCR_NONE,         // "None" special value
    YCBCR_BT601_TV,
    YCBCR_BT601_PC,
    YCBCR_BT709_TV,
    YCBCR_BT709_PC,
    YCBCR_SMPTE240M_TV,
    YCBCR_SMPTE240M_PC,
    YCBCR_FCC_TV,
    YCBCR_FCC_PC
} ASS_YCbCrMatrix;

/*
 * ass track represent either an external script or a matroska subtitle stream
 * (no real difference between them); it can be used in rendering after the
 * headers are parsed (i.e. events format line read).
 */
typedef struct ass_track {
    int n_styles;           // amount used
    int max_styles;         // amount allocated
    int n_events;
    int max_events;
    ASS_Style *styles;    // array of styles, max_styles length, n_styles used
    ASS_Event *events;    // the same as styles

    char *style_format;     // style format line (everything after "Format: ")
    char *event_format;     // event format line

    enum {
        TRACK_TYPE_UNKNOWN = 0,
        TRACK_TYPE_ASS,
        TRACK_TYPE_SSA
    } track_type;

    // Script header fields
    int PlayResX;
    int PlayResY;
    double Timer;
    int WrapStyle;
    int ScaledBorderAndShadow;
    int Kerning;
    char *Language;
    ASS_YCbCrMatrix YCbCrMatrix;

    int default_style;      // index of default style
    char *name;             // file name in case of external subs, 0 for streams

    ASS_Library *library;
    ASS_ParserPriv *parser_priv;
} ASS_Track;

#endif /* LIBASS_TYPES_H */
