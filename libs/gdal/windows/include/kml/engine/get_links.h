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

// This file contains the declaration of the GetLinks() function and the
// definition of the GetLinksParserObserver.

#ifndef KML_ENGINE_GET_LINKS_H__
#define KML_ENGINE_GET_LINKS_H__

#include <vector>
#include "kml/dom.h"
#include "kml/dom/parser_observer.h"

namespace kmlengine {

typedef std::vector<string> href_vector_t;

// This ParserObserver looks for all elements with an "href" and saves the
// content of each to the passed vector.
class GetLinksParserObserver : public kmldom::ParserObserver {
 public:
  GetLinksParserObserver(href_vector_t* href_vector)
      : href_vector_(href_vector) {}

  virtual ~GetLinksParserObserver() {}

  virtual bool AddChild(const kmldom::ElementPtr& parent,
                        const kmldom::ElementPtr& child) {
    switch (child->Type()) {
      default:
        break;
      case kmldom::Type_href:
        // NetworkLink/Link/href, Overlay/Icon/href, ItemIcon/href
        // Model/Link/href, IconStyle/Icon/href
        href_vector_->push_back(child->get_char_data());
        break;
      case kmldom::Type_targetHref:
        if (kmldom::Type_Alias == parent->Type()) {
          href_vector_->push_back(child->get_char_data());
        }
        break;
      case kmldom::Type_styleUrl:
        href_vector_->push_back(child->get_char_data());
        break;
      case kmldom::Type_SchemaData:
        kmldom::SchemaDataPtr schemadata = kmldom::AsSchemaData(child);
        if (schemadata->has_schemaurl()) {
          href_vector_->push_back(schemadata->get_schemaurl());
        }
      // TODO: HTML links in description and BalloonStyle/text
        break;
    }
    return true;
  }

 private:
  href_vector_t* href_vector_;
};

// This function saves to the vector all href's found in the given KML.
// This returns false if the vector is NULL or on any parse error. This does
// not search the balloon text for links.
bool GetLinks(const string& kml, href_vector_t* href_vector);

// As GetLinks, but considers only those href's that are relative (local) to
// the given KML. This does not search the balloon text for links.
bool GetRelativeLinks(const string& kml, href_vector_t* href_vector);

}  // end namespace kmlengine

#endif  // KML_ENGINE_GET_LINKS_H__
