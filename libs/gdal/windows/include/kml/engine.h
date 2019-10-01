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

// This is the main include file for the KMLENGINE library. Clients of
// kmlengine should include only this header file.

#ifndef KML_ENGINE_H__
#define KML_ENGINE_H__

#include "kml/engine/bbox.h"
#include "kml/engine/clone.h"
#include "kml/engine/engine_types.h"
#include "kml/engine/entity_mapper.h"
#include "kml/engine/feature_balloon.h"
#include "kml/engine/feature_view.h"
#include "kml/engine/feature_visitor.h"
#include "kml/engine/find.h"
#include "kml/engine/find_xml_namespaces.h"
#include "kml/engine/get_links.h"
#include "kml/engine/href.h"
#include "kml/engine/id_mapper.h"
#include "kml/engine/kml_cache.h"
#include "kml/engine/kml_file.h"
#include "kml/engine/kml_stream.h"
#include "kml/engine/kml_uri.h"
#include "kml/engine/kmz_file.h"
#include "kml/engine/link_util.h"
#include "kml/engine/location_util.h"
#include "kml/engine/merge.h"
#include "kml/engine/object_id_parser_observer.h"
#include "kml/engine/shared_style_parser_observer.h"
#include "kml/engine/style_inliner.h"
#include "kml/engine/style_merger.h"
#include "kml/engine/style_resolver.h"
#include "kml/engine/style_splitter.h"
#include "kml/engine/update.h"

#endif  // KML_ENGINE_H__
