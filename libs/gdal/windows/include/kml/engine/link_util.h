// Copyright 2008, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file contains function templates useful for accessing link element data.

#ifndef KML_ENGINE_LINK_UTIL_H__
#define KML_ENGINE_LINK_UTIL_H__

#include "kml/dom.h"
#include "kml/engine.h"

namespace kmlengine {

// This function fetches and parses the KML referenced by this NetworkLink.
// The NetworkLink must be within the KmlFile and the KmlFile must point to
// a KmlCache (such as a KmlFile created by KmlCache).
// If the fetch or parse fail a NULL KmlFilePtr is returned.
KmlFilePtr FetchLink(const KmlFilePtr& kml_file,
                     const kmldom::NetworkLinkPtr& networklink);

// This function fetches the Overlay's Icon image data.  The KmlFile must have
// a KmlCache (see KmlCache).  If the fetch fails false is returned.
bool FetchIcon(const KmlFilePtr& kml_file,
               const kmldom::OverlayPtr& overlay,
               string* data);


// This function template gets the content of the <href> child of <Link>,
// <Icon>, <ItemIcon> and <IconStyle>'s <Icon>.  This returns true if both
// arguments are non-NULL and if the has_href() test passes for the parent,
// else false is returned.  (It is safe to pass all or some NULL arguments).
template<typename HP>
bool GetHref(const HP& href_parent, string* href) {
  if (href && href_parent && href_parent->has_href()) {
    *href = href_parent->get_href();
    return true;
  }
  return false;
}

// This function template gets the content of the <href> of the <Link> child
// of <NetworkLink> and <Model>.  See GetHref() for info about the return.
template<typename LP>
bool GetLinkParentHref(const LP& link_parent, string* href) {
  return GetHref(link_parent->get_link(), href);
}

// This function template gets the content of the <href> of the <Icon> child
// of any Overlay, or of <IconStyle>.  See GetHref() for info about the return.
template<typename IP>
bool GetIconParentHref(const IP& icon_parent, string* href) {
  return GetHref(icon_parent->get_icon(), href);
}

}  // end namespace kmlengine

#endif  // KML_ENGINE_LINK_UTIL_H__
