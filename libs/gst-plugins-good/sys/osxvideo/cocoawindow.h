/* GStreamer
 * Copyright (C) 2004 Zaheer Abbas Merali <zaheerabbas at merali dot org>
 * Copyright (C) 2007 Pioneers of the Inevitable <songbird@songbirdnest.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * The development of this code was made possible due to the involvement of Pioneers 
 * of the Inevitable, the creators of the Songbird Music player
 * 
 */

/* inspiration gained from looking at source of osx video out of xine and vlc 
 * and is reflected in the code
 */

#import <Cocoa/Cocoa.h>
#import <glib.h>
#import <gst/video/navigation.h>

struct _GstOSXImage;

@interface GstGLView : NSOpenGLView
{
    int i_effect;
    unsigned int pi_texture;
    float f_x;
    float f_y;
    int initDone;
    char* data;
    int width, height;
    BOOL fullscreen;
    BOOL keepAspectRatio;
    NSOpenGLContext* fullScreenContext; 
    NSOpenGLContext* actualContext;
    NSTrackingArea *trackingArea;
    GstNavigation *navigation;
    NSRect drawingBounds;
    NSThread *mainThread;
    NSUInteger savedModifierFlags;
}
- (void) drawQuad;
- (void) drawRect: (NSRect) rect;
- (id) initWithFrame: (NSRect) frame;
- (void) initTextures;
- (void) reloadTexture;
- (void) cleanUp;
- (void) displayTexture;
- (char*) getTextureBuffer;
- (void) setFullScreen: (BOOL) flag;
- (void) setKeepAspectRatio: (BOOL) flag;
- (void) reshape;
- (void) setVideoSize:(int)w : (int)h;
- (NSRect) getDrawingBounds;
- (BOOL) haveSuperview;
- (void) haveSuperviewReal: (NSMutableArray *)closure;
- (void) addToSuperview: (NSView *)superview;
- (void) removeFromSuperview: (id)unused;
- (void) setNavigation: (GstNavigation *) nav;
#ifndef GSTREAMER_GLIB_COCOA_NSAPPLICATION
- (void) setMainThread: (NSThread *) thread;
#endif

@end

@interface GstOSXVideoSinkWindow: NSWindow {
   int width, height;
   GstGLView *gstview;
}

- (void) setContentSize: (NSSize) size;
- (GstGLView *) gstView;
- (id)initWithContentNSRect:(NSRect)contentRect styleMask:(unsigned int)styleMask backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag screen:(NSScreen *)aScreen;
@end
