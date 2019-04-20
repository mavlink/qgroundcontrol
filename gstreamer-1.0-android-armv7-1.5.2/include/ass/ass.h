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

#ifndef LIBASS_ASS_H
#define LIBASS_ASS_H

#include <stdio.h>
#include <stdarg.h>
#include "ass_types.h"

#define LIBASS_VERSION 0x01201000

/*
 * A linked list of images produced by an ass renderer.
 *
 * These images have to be rendered in-order for the correct screen
 * composition.  The libass renderer clips these bitmaps to the frame size.
 * w/h can be zero, in this case the bitmap should not be rendered at all.
 * The last bitmap row is not guaranteed to be padded up to stride size,
 * e.g. in the worst case a bitmap has the size stride * (h - 1) + w.
 */
typedef struct ass_image {
    int w, h;                   // Bitmap width/height
    int stride;                 // Bitmap stride
    unsigned char *bitmap;      // 1bpp stride*h alpha buffer
                                // Note: the last row may not be padded to
                                // bitmap stride!
    uint32_t color;             // Bitmap color and alpha, RGBA
    int dst_x, dst_y;           // Bitmap placement inside the video frame

    struct ass_image *next;   // Next image, or NULL

    enum {
        IMAGE_TYPE_CHARACTER,
        IMAGE_TYPE_OUTLINE,
        IMAGE_TYPE_SHADOW
    } type;

} ASS_Image;

/*
 * Hinting type. (see ass_set_hinting below)
 *
 * Setting hinting to anything but ASS_HINTING_NONE will put libass in a mode
 * that reduces compatibility with vsfilter and many ASS scripts. The main
 * problem is that hinting conflicts with smooth scaling, which precludes
 * animations and precise positioning.
 *
 * In other words, enabling hinting might break some scripts severely.
 *
 * FreeType's native hinter is still buggy sometimes and it is recommended
 * to use the light autohinter, ASS_HINTING_LIGHT, instead.  For best
 * compatibility with problematic fonts, disable hinting.
 */
typedef enum {
    ASS_HINTING_NONE = 0,
    ASS_HINTING_LIGHT,
    ASS_HINTING_NORMAL,
    ASS_HINTING_NATIVE
} ASS_Hinting;

/**
 * \brief Text shaping levels.
 *
 * SIMPLE is a fast, font-agnostic shaper that can do only substitutions.
 * COMPLEX is a slower shaper using OpenType for substitutions and positioning.
 *
 * libass uses the best shaper available by default.
 */
typedef enum {
    ASS_SHAPING_SIMPLE = 0,
    ASS_SHAPING_COMPLEX
} ASS_ShapingLevel;

/**
 * \brief Style override options. See
 * ass_set_selective_style_override_enabled() for details.
 */
typedef enum {
    ASS_OVERRIDE_BIT_STYLE = 1,
    ASS_OVERRIDE_BIT_FONT_SIZE = 2,
} ASS_OverrideBits;

/**
 * \brief Return the version of library. This returns the value LIBASS_VERSION
 * was set to when the library was compiled.
 * \return library version
 */
int ass_library_version(void);

/**
 * \brief Initialize the library.
 * \return library handle or NULL if failed
 */
ASS_Library *ass_library_init(void);

/**
 * \brief Finalize the library
 * \param priv library handle
 */
void ass_library_done(ASS_Library *priv);

/**
 * \brief Set additional fonts directory.
 * Optional directory that will be scanned for fonts recursively.  The fonts
 * found are used for font lookup.
 * NOTE: A valid font directory is not needed to support embedded fonts.
 *
 * \param priv library handle
 * \param fonts_dir directory with additional fonts
 */
void ass_set_fonts_dir(ASS_Library *priv, const char *fonts_dir);

/**
 * \brief Whether fonts should be extracted from track data.
 * \param priv library handle
 * \param extract whether to extract fonts
 */
void ass_set_extract_fonts(ASS_Library *priv, int extract);

/**
 * \brief Register style overrides with a library instance.
 * The overrides should have the form [Style.]Param=Value, e.g.
 *   SomeStyle.Font=Arial
 *   ScaledBorderAndShadow=yes
 *
 * \param priv library handle
 * \param list NULL-terminated list of strings
 */
void ass_set_style_overrides(ASS_Library *priv, char **list);

/**
 * \brief Explicitly process style overrides for a track.
 * \param track track handle
 */
void ass_process_force_style(ASS_Track *track);

/**
 * \brief Register a callback for debug/info messages.
 * If a callback is registered, it is called for every message emitted by
 * libass.  The callback receives a format string and a list of arguments,
 * to be used for the printf family of functions. Additionally, a log level
 * from 0 (FATAL errors) to 7 (verbose DEBUG) is passed.  Usually, level 5
 * should be used by applications.
 * If no callback is set, all messages level < 5 are printed to stderr,
 * prefixed with [ass].
 *
 * \param priv library handle
 * \param msg_cb pointer to callback function
 * \param data additional data, will be passed to callback
 */
void ass_set_message_cb(ASS_Library *priv, void (*msg_cb)
                        (int level, const char *fmt, va_list args, void *data),
                        void *data);

/**
 * \brief Initialize the renderer.
 * \param priv library handle
 * \return renderer handle or NULL if failed
 */
ASS_Renderer *ass_renderer_init(ASS_Library *);

/**
 * \brief Finalize the renderer.
 * \param priv renderer handle
 */
void ass_renderer_done(ASS_Renderer *priv);

/**
 * \brief Set the frame size in pixels, including margins.
 * The renderer will never return images that are outside of the frame area.
 * The value set with this function can influence the pixel aspect ratio used
 * for rendering. If the frame size doesn't equal to the video size, you may
 * have to use ass_set_pixel_aspect().
 * @see ass_set_pixel_aspect()
 * @see ass_set_margins()
 * \param priv renderer handle
 * \param w width
 * \param h height
 */
void ass_set_frame_size(ASS_Renderer *priv, int w, int h);

/**
 * \brief Set the source image size in pixels.
 * This is used to calculate the source aspect ratio and the blur scale.
 * The source image size can be reset to default by setting w and h to 0.
 * The value set with this function can influence the pixel aspect ratio used
 * for rendering.
 * @see ass_set_pixel_aspect()
 * \param priv renderer handle
 * \param w width
 * \param h height
 */
void ass_set_storage_size(ASS_Renderer *priv, int w, int h);

/**
 * \brief Set shaping level. This is merely a hint, the renderer will use
 * whatever is available if the request cannot be fulfilled.
 * \param level shaping level
 */
void ass_set_shaper(ASS_Renderer *priv, ASS_ShapingLevel level);

/**
 * \brief Set frame margins.  These values may be negative if pan-and-scan
 * is used. The margins are in pixels. Each value specifies the distance from
 * the video rectangle to the renderer frame. If a given margin value is
 * positive, there will be free space between renderer frame and video area.
 * If a given margin value is negative, the frame is inside the video, i.e.
 * the video has been cropped.
 *
 * The renderer will try to keep subtitles inside the frame area. If possible,
 * text is layout so that it is inside the cropped area. Subtitle events
 * that can't be moved are cropped against the frame area.
 *
 * ass_set_use_margins() can be used to allow libass to render subtitles into
 * the empty areas if margins are positive, i.e. the video area is smaller than
 * the frame. (Traditionally, this has been used to show subtitles in
 * the bottom "black bar" between video bottom screen border when playing 16:9
 * video on a 4:3 screen.)
 *
 * When using this function, it is recommended to calculate and set your own
 * aspect ratio with ass_set_pixel_aspect(), as the defaults won't make any
 * sense.
 * @see ass_set_pixel_aspect()
 * \param priv renderer handle
 * \param t top margin
 * \param b bottom margin
 * \param l left margin
 * \param r right margin
 */
void ass_set_margins(ASS_Renderer *priv, int t, int b, int l, int r);

/**
 * \brief Whether margins should be used for placing regular events.
 * \param priv renderer handle
 * \param use whether to use the margins
 */
void ass_set_use_margins(ASS_Renderer *priv, int use);

/**
 * \brief Set pixel aspect ratio correction.
 * This is the ratio of pixel width to pixel height.
 *
 * Generally, this is (s_w / s_h) / (d_w / d_h), where s_w and s_h is the
 * video storage size, and d_w and d_h is the video display size. (Display
 * and storage size can be different for anamorphic video, such as DVDs.)
 *
 * If the pixel aspect ratio is 0, or if the aspect ratio has never been set
 * by calling this function, libass will calculate a default pixel aspect ratio
 * out of values set with ass_set_frame_size() and ass_set_storage_size(). Note
 * that this is useful only if the frame size corresponds to the video display
 * size. Keep in mind that the margins set with ass_set_margins() are ignored
 * for aspect ratio calculations as well.
 * If the storage size has not been set, a pixel aspect ratio of 1 is assumed.
 * \param priv renderer handle
 * \param par pixel aspect ratio (1.0 means square pixels, 0 means default)
 */
void ass_set_pixel_aspect(ASS_Renderer *priv, double par);

/**
 * \brief Set aspect ratio parameters.
 * This calls ass_set_pixel_aspect(priv, dar / sar).
 * @deprecated New code should use ass_set_pixel_aspect().
 * \param priv renderer handle
 * \param dar display aspect ratio (DAR), prescaled for output PAR
 * \param sar storage aspect ratio (SAR)
 */
void ass_set_aspect_ratio(ASS_Renderer *priv, double dar, double sar);

/**
 * \brief Set a fixed font scaling factor.
 * \param priv renderer handle
 * \param font_scale scaling factor, default is 1.0
 */
void ass_set_font_scale(ASS_Renderer *priv, double font_scale);

/**
 * \brief Set font hinting method.
 * \param priv renderer handle
 * \param ht hinting method
 */
void ass_set_hinting(ASS_Renderer *priv, ASS_Hinting ht);

/**
 * \brief Set line spacing. Will not be scaled with frame size.
 * \param priv renderer handle
 * \param line_spacing line spacing in pixels
 */
void ass_set_line_spacing(ASS_Renderer *priv, double line_spacing);

/**
 * \brief Set vertical line position.
 * \param priv renderer handle
 * \param line_position vertical line position of subtitles in percent
 * (0-100: 0 = on the bottom (default), 100 = on top)
 */
void ass_set_line_position(ASS_Renderer *priv, double line_position);

/**
 * \brief Set font lookup defaults.
 * \param default_font path to default font to use. Must be supplied if
 * fontconfig is disabled or unavailable.
 * \param default_family fallback font family for fontconfig, or NULL
 * \param fc whether to use fontconfig
 * \param config path to fontconfig configuration file, or NULL.  Only relevant
 * if fontconfig is used.
 * \param update whether fontconfig cache should be built/updated now.  Only
 * relevant if fontconfig is used.
 *
 * NOTE: font lookup must be configured before an ASS_Renderer can be used.
 */
void ass_set_fonts(ASS_Renderer *priv, const char *default_font,
                   const char *default_family, int fc, const char *config,
                   int update);

/**
 * \brief Set selective style override mode.
 * If enabled, the renderer attempts to override the ASS script's styling of
 * normal subtitles, without affecting explicitly positioned text. If an event
 * looks like a normal subtitle, parts of the font style are copied from the
 * user style set with ass_set_selective_style_override().
 * Warning: the heuristic used for deciding when to override the style is rather
 *          rough, and enabling this option can lead to incorrectly rendered
 *          subtitles. Since the ASS format doesn't have any support for
 *          allowing end-users to customize subtitle styling, this feature can
 *          only be implemented on "best effort" basis, and has to rely on
 *          heuristics that can easily break.
 * \param priv renderer handle
 * \param bits bit mask comprised of ASS_OverrideBits values. If the value is
 *  0, all override features are disabled, and libass will behave like libass
 *  versions before this feature was introduced. Possible values:
 *      ASS_OVERRIDE_BIT_STYLE: apply the style as set with
 *          ass_set_selective_style_override() on events which look like
 *          dialogue. Other style overrides are also applied this way, except
 *          ass_set_font_scale(). How ass_set_font_scale() is applied depends
 *          on the ASS_OVERRIDE_BIT_FONT_SIZE flag.
 *      ASS_OVERRIDE_BIT_FONT_SIZE: apply ass_set_font_scale() only on events
 *          which look like dialogue. If not set, it is applied to all
 *          events.
 *      0: ignore ass_set_selective_style_override(), but apply all other
 *          overrides (traditional behavior).
 */
void ass_set_selective_style_override_enabled(ASS_Renderer *priv, int bits);

/**
 * \brief Set style for selective style override.
 * See ass_set_selective_style_override_enabled().
 * \param style style settings to use if override is enabled. Applications
 * should initialize it with {0} before setting fields. Strings will be copied
 * by the function.
 */
void ass_set_selective_style_override(ASS_Renderer *priv, ASS_Style *style);

/**
 * \brief Update/build font cache.  This needs to be called if it was
 * disabled when ass_set_fonts was set.
 *
 * \param priv renderer handle
 * \return success
 */
int ass_fonts_update(ASS_Renderer *priv);

/**
 * \brief Set hard cache limits.  Do not set, or set to zero, for reasonable
 * defaults.
 *
 * \param priv renderer handle
 * \param glyph_max maximum number of cached glyphs
 * \param bitmap_max_size maximum bitmap cache size (in MB)
 */
void ass_set_cache_limits(ASS_Renderer *priv, int glyph_max,
                          int bitmap_max_size);

/**
 * \brief Render a frame, producing a list of ASS_Image.
 * \param priv renderer handle
 * \param track subtitle track
 * \param now video timestamp in milliseconds
 * \param detect_change compare to the previous call and set to 1
 * if positions changed, or set to 2 if content changed.
 */
ASS_Image *ass_render_frame(ASS_Renderer *priv, ASS_Track *track,
                            long long now, int *detect_change);


/*
 * The following functions operate on track objects and do not need
 * an ass_renderer
 */

/**
 * \brief Allocate a new empty track object.
 * \param library handle
 * \return pointer to empty track
 */
ASS_Track *ass_new_track(ASS_Library *);

/**
 * \brief Deallocate track and all its child objects (styles and events).
 * \param track track to deallocate
 */
void ass_free_track(ASS_Track *track);

/**
 * \brief Allocate new style.
 * \param track track
 * \return newly allocated style id
 */
int ass_alloc_style(ASS_Track *track);

/**
 * \brief Allocate new event.
 * \param track track
 * \return newly allocated event id
 */
int ass_alloc_event(ASS_Track *track);

/**
 * \brief Delete a style.
 * \param track track
 * \param sid style id
 * Deallocates style data. Does not modify track->n_styles.
 */
void ass_free_style(ASS_Track *track, int sid);

/**
 * \brief Delete an event.
 * \param track track
 * \param eid event id
 * Deallocates event data. Does not modify track->n_events.
 */
void ass_free_event(ASS_Track *track, int eid);

/**
 * \brief Parse a chunk of subtitle stream data.
 * \param track track
 * \param data string to parse
 * \param size length of data
 */
void ass_process_data(ASS_Track *track, char *data, int size);

/**
 * \brief Parse Codec Private section of the subtitle stream, in Matroska
 * format.  See the Matroska specification for details.
 * \param track target track
 * \param data string to parse
 * \param size length of data
 */
void ass_process_codec_private(ASS_Track *track, char *data, int size);

/**
 * \brief Parse a chunk of subtitle stream data. A chunk contains exactly one
 * event in Matroska format.  See the Matroska specification for details.
 * \param track track
 * \param data string to parse
 * \param size length of data
 * \param timecode starting time of the event (milliseconds)
 * \param duration duration of the event (milliseconds)
 */
void ass_process_chunk(ASS_Track *track, char *data, int size,
                       long long timecode, long long duration);

/**
 * \brief Flush buffered events.
 * \param track track
*/
void ass_flush_events(ASS_Track *track);

/**
 * \brief Read subtitles from file.
 * \param library library handle
 * \param fname file name
 * \param codepage encoding (iconv format)
 * \return newly allocated track
*/
ASS_Track *ass_read_file(ASS_Library *library, char *fname,
                         char *codepage);

/**
 * \brief Read subtitles from memory.
 * \param library library handle
 * \param buf pointer to subtitles text
 * \param bufsize size of buffer
 * \param codepage encoding (iconv format)
 * \return newly allocated track
*/
ASS_Track *ass_read_memory(ASS_Library *library, char *buf,
                           size_t bufsize, char *codepage);
/**
 * \brief Read styles from file into already initialized track.
 * \param fname file name
 * \param codepage encoding (iconv format)
 * \return 0 on success
 */
int ass_read_styles(ASS_Track *track, char *fname, char *codepage);

/**
 * \brief Add a memory font.
 * \param library library handle
 * \param name attachment name
 * \param data binary font data
 * \param data_size data size
*/
void ass_add_font(ASS_Library *library, char *name, char *data,
                  int data_size);

/**
 * \brief Remove all fonts stored in an ass_library object.
 * \param library library handle
 */
void ass_clear_fonts(ASS_Library *library);

/**
 * \brief Calculates timeshift from now to the start of some other subtitle
 * event, depending on movement parameter.
 * \param track subtitle track
 * \param now current time in milliseconds
 * \param movement how many events to skip from the one currently displayed
 * +2 means "the one after the next", -1 means "previous"
 * \return timeshift in milliseconds
 */
long long ass_step_sub(ASS_Track *track, long long now, int movement);

#endif /* LIBASS_ASS_H */
