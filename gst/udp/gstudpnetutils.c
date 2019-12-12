/* GStreamer UDP network utility functions
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2006 Joni Valtanen <joni.valtanen@movial.fi>
 * Copyright (C) 2009 Jarkko Palviainen <jarkko.palviainen@sesca.com>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <string.h>

#include "gstudpnetutils.h"

gboolean
gst_udp_parse_uri (const gchar * uristr, gchar ** host, guint16 * port)
{
  gchar *protocol, *location_start;
  gchar *location, *location_end;
  gchar *colptr;

  /* consider no protocol to be udp:// */
  protocol = gst_uri_get_protocol (uristr);
  if (!protocol)
    goto no_protocol;
  if (strcmp (protocol, "udp") != 0)
    goto wrong_protocol;
  g_free (protocol);

  location_start = gst_uri_get_location (uristr);
  if (!location_start)
    return FALSE;

  GST_DEBUG ("got location '%s'", location_start);

  /* VLC compatibility, strip everything before the @ sign. VLC uses that as the
   * remote address. */
  location = g_strstr_len (location_start, -1, "@");
  if (location == NULL)
    location = location_start;
  else
    location += 1;

  if (location[0] == '[') {
    GST_DEBUG ("parse IPV6 address '%s'", location);
    location_end = strchr (location, ']');
    if (location_end == NULL)
      goto wrong_address;

    *host = g_strndup (location + 1, location_end - location - 1);
    colptr = strrchr (location_end, ':');
  } else {
    GST_DEBUG ("parse IPV4 address '%s'", location);
    colptr = strrchr (location, ':');

    if (colptr != NULL) {
      *host = g_strndup (location, colptr - location);
    } else {
      *host = g_strdup (location);
    }
  }
  GST_DEBUG ("host set to '%s'", *host);

  if (colptr != NULL) {
    *port = g_ascii_strtoll (colptr + 1, NULL, 10);
  } else {
    *port = 0;
  }
  g_free (location_start);

  return TRUE;

  /* ERRORS */
no_protocol:
  {
    GST_ERROR ("error parsing uri %s: no protocol", uristr);
    return FALSE;
  }
wrong_protocol:
  {
    GST_ERROR ("error parsing uri %s: wrong protocol (%s != udp)", uristr,
        protocol);
    g_free (protocol);
    return FALSE;
  }
wrong_address:
  {
    GST_ERROR ("error parsing uri %s", uristr);
    g_free (location);
    return FALSE;
  }
}
