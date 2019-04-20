/*
 * This file is part of the Nice GLib ICE library.
 *
 * (C) 2008-2009 Collabora Ltd.
 *  Contact: Youness Alaoui
 * (C) 2007 Nokia Corporation. All rights reserved.
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Nice GLib ICE library.
 *
 * The Initial Developers of the Original Code are Collabora Ltd and Nokia
 * Corporation. All Rights Reserved.
 *
 * Contributors:
 *   Youness Alaoui, Collabora Ltd.
 *
 * Alternatively, the contents of this file may be used under the terms of the
 * the GNU Lesser General Public License Version 2.1 (the "LGPL"), in which
 * case the provisions of LGPL are applicable instead of those above. If you
 * wish to allow use of your version of this file only under the terms of the
 * LGPL and not to allow others to use your version of this file under the
 * MPL, indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the LGPL. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under either the MPL or the LGPL.
 */

#ifndef STUN_DEBUG_H
#define STUN_DEBUG_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * stun_debug_enable:
 *
 * Enable debug messages to stderr
 */
void stun_debug_enable (void);

/**
 * stun_debug_disable:
 *
 * Disable debug messages to stderr
 */
void stun_debug_disable (void);

/**
 * StunDebugHandler:
 * @format: printf()-style debug message format string
 * @ap: Parameters to substitute into message placeholders
 *
 * Callback for a debug message from the STUN code.
 */
#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
typedef void (*StunDebugHandler) (const char *format, va_list ap)
  __attribute__((__format__ (__printf__, 1, 0)));
#else
typedef void (*StunDebugHandler) (const char *format, va_list ap);
#endif

/**
 * stun_set_debug_handler:
 * @handler: (nullable): Handler for STUN debug messages, or %NULL to use the
 *   default
 *
 * Set a callback function to be invoked for each debug message from the STUN
 * code. The callback will only be invoked if STUN debugging is enabled using
 * stun_debug_enable().
 *
 * The default callback prints the formatted debug message to stderr.
 */
void stun_set_debug_handler (StunDebugHandler handler);

#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
void stun_debug (const char *fmt, ...)
  __attribute__((__format__ (__printf__, 1, 2)));
#else
void stun_debug (const char *fmt, ...);
#endif
void stun_debug_bytes (const char *prefix, const void *data, size_t len);


# ifdef __cplusplus
}
# endif

#endif /* STUN_DEBUG_H */
