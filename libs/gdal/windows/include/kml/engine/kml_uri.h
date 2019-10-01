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

// This file contains the declaration of the KML URI resolution and parsing
// functions.

#ifndef KML_ENGINE_KML_URI_H__
#define KML_ENGINE_KML_URI_H__

#include "kml/base/util.h"

namespace kmlengine {

// This function implements standard RFC 3986 URI resolution (Section 5).
// In the context of KML the base URI is typically that of the KML file
// (see KmlFile::get_url()), and "relative" is typically the contents of an
// <href> (such as in <NetworkLink>'s <Link> or any Overlay's <Icon>),
// <styleUrl> or schemaUrl=.  The result string is the resolved URI to fetch.
// No network fetching is performed with this function.  This merely
// computes the URI resolution.

// For example,
// if the file overlay.kml is fetched from http://host.com/dir/overlay.kml
//   <GroundOverlay>
//     <Icon>
//       <href>../images/pretty.jpg</href>
//     </Icon>
//   </GroundOverlay>
// ...then the result resolving relative="../images/pretty.jpg" against
// the base of "http://host.com/kml/overlay.kml" will be:
// "http://host.com/images/pretty.jpg".

// Also, note that these same standard rules apply for KMZ URIs.
// Here is an example of the two levels of resolution required to resolve
// each <targetHref> of a given <Model>'s <ResourceMap>.
// First presume the base URI of a KMZ is "http://host.com/dir/model.kmz".
// Within the default KML file of the KMZ is the following:
//   <Model>
//     <Link>
//       <href>geometry/bldg.dae</href>
//     </Link>
//     <ResourceMap>
//       <Alias>
//         <targetHref>../textures/brick.jpg</targetHref>
//         <sourceHref>../files/CU-Macky-4sideturretnoCulling.jpg</sourceHref>
//       </Alias>
//     </ResourceMap>
//   </Model>
// The relative "geometry/bldg.dae" results in
// "http://host.com/dir/model.kmz/geometry/bldg.dae".  (A KMZ-aware fetching
// system is required to split the fetchable URL from the path reference into
// the KMZ, but this result is still a valid standard URI).
// With the resolved base of "http://host.com/dir/model.kmz/geometry/bldg.dae"
// and relative of "../textures/brick.jpg" the resolved result URI is
// "http://host.com/dir/model.kmz/textures/brick.jpg" which is to say a
// path of "textures/brick.jpg" at the KMZ whose URI is
// "http://host.com/dir/model.kmz".  Note that it is perfectly valid for a
// relative reference with a KMZ to refer "up and out" of the KMZ to
// either a file within another KMZ or a single file.
bool ResolveUri(const string& base, const string& relative,
                string* result);

// Performs a syntax-based normalization of uri as per RFC 3986 6.2.2. False is
// returned if result is NULL or upon any internal error.
bool NormalizeUri(const string& uri, string* result);

// Performs a syntax-based normalization of href as per RFC 3986 6.2.2. False
// is returned if result is NULL or upon any internal error.
bool NormalizeHref(const string& href, string* result);

// Converts a URI to its corresponding filename. The implementation
// is platform-specific. Returns false if output is NULL or on any internal
// error in converting the uri.
bool UriToFilename(const string& uri, string* output);

// Converts a filename to its corresponding URI. The implementation is
// platform-specific. Returns false if output is NULL or on any internal
// error in converting the uri.
bool FilenameToUri(const string& filename, string* output);

// This function splits out the various components of a URI:
// uri = scheme://host:port/path?query#fragment
// An output string NULL pointer simply ignores splitting out that component.
// The return value reflects the validity of the uri.  Each desired output
// string should be inspected using empty() to discover if the uri has
// the particular component.
bool SplitUri(const string& uri, string* scheme, string* host,
              string* port, string* path, string* query,
              string* fragment);

// This function returns true if the given uri is valid and has a fragment.
// If it has a fragment and a string pointer is supplied it is saved there
// (without the #).
bool SplitUriFragment(const string& uri, string* fragment);

// This function returns true if the given uri is valid and has a path.
// If is has a path and the string pointer is supplied it is saved there.
bool SplitUriPath(const string& uri, string* path);

// This function returns true if the given uri is valid.  If the fetchable_uri
// output string is supplied a uri w/o the fragment is stored there.
bool GetFetchableUri(const string& uri, string* fetchable_uri);

// Given a url of the form scheme:authority/path/file.kmz/file/in/kmz this
// function splits out the fetchable subfile in the KMZ archive, writing the
// fetchable KMZ URL to kmz_url and the archived file to kmz_path. If the
// fetchable URL does not end in .kmz it returns false. If there is no subfile
// to split out, it sets kmz_path to an empty string.
bool KmzSplit(const string& kml_url, string* kmz_url,
              string* kmz_path);

// Resolve the URL to the Model's targetHref.  The base is the URL
// of the KML file holding the Model.  The geometry_href is the value
// of the Model's Link/href.  The target_href is the value of one of the
// Model's ResourceMap/Alias/targetHref's.  Note that the result URL may
// be into a KMZ and hence might be used with KmzSplit.
bool ResolveModelTargetHref(const string& base,
                            const string& geometry_href,
                            const string& target,
                            string* result);

}  // end namespace kmlengine

#endif  // KML_ENGINE_KML_URI_H__
