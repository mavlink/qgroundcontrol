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

// This file declares the KmlHandler specialization of ExpatHandler.
// This is used internally to the Parse() function.  KmlHandler is constructed
// with a list of ParserObservers which essentially layer on an Element-level
// SAX parse as the DOM is built.

// Note: although the parser itself is SAX-driven, we make a best effort
// to preserve all unknown (non-KML) elements found during the parse, and
// will serialize those elements after the known elements on output.

// As of libkml 1.3, we also attempt to parse old (KML 2.0, 2.1 era)
// <Schema> usage found in files created by Google Earth Pro's "GIS Ingest"
// module (created by Google Earth 5.1 and earlier). These files used the
// <Schema> element to subclass the Placemark element and extend it by
// naming typed children in SimpleField elements. This practice was never
// standardized, so we attempt to coerce the nonstandard markup into
// equivalent and valid KML that preserves the data and its typing. This
// implementation only works when the <Schema> element appears before
// any of the children it defines. Conveniently, this is also Google Earth's
// exact behavior.
//
// In short, we turn this:
// <Document>
//   <Schema parent="Placemark" name="S_521_525_SSSSS">
//     <SimpleField type="string" name="Foo"></SimpleField>
//   </Schema>
//   <S_521_525_SSSSS>
//     <Foo>foo 1</Foo>
//   </S_521_525_SSSSS>
// </Document>
//
// into this:
//
//   <Document>
//     <Schema id="S_521_525_SSSSS_id" name="S_521_525_SSSSS">
//       <SimpleField name="Foo" type="string"/>
//     </Schema>
//     <Placemark>
//       <ExtendedData>
//         <SchemaData schemaUrl="S_521_525_SSSSS_id">
//           <SimpleData name="Foo">foo 1</SimpleData>
//         </SchemaData>
//       </ExtendedData>
//     </Placemark>
//   </Document>
//
// Both of those when loaded into Google Earth 4.0 or later produce equivalent
// data displays and interaction models.

#ifndef KML_DOM_KML_HANDLER_H__
#define KML_DOM_KML_HANDLER_H__

#include <stack>
#include "kml/base/expat_handler.h"
#include "kml/dom/element.h"
#include "kml/dom/kml_ptr.h"
#include "kml/dom/parser_observer.h"

namespace kmldom {

class KmlFactory;

// This class implements the expat handlers for parsing KML.  This class is
// handed to expat in the ExpatParser() function.
class KmlHandler : public kmlbase::ExpatHandler {
public:
  KmlHandler(parser_observer_vector_t& observers);
  ~KmlHandler();

  // ExpatHandler methods
  virtual void StartElement(const string& name,
                            const kmlbase::StringVector& atts);
  virtual void EndElement(const string& name);
  virtual void CharData(const string& s);

  // This destructively removes the Element on the top of the stack and
  // transfers ownership of it to the caller.  The intention is to use this
  // after a successful parse.
  ElementPtr PopRoot();

private:
  const KmlFactory& kml_factory_;
  std::stack<ElementPtr> stack_;
  // Char data is managed as a stack to allow for gathering all character data
  // inside unknown elements.
  std::stack<string> char_data_;
  // Helpers for handling unknown elements:
  void InsertUnknownStartElement(const string& name,
                                 const kmlbase::StringVector& atts);
  void InsertUnknownEndElement(const string& name);
  unsigned int skip_depth_;
  unsigned int in_description_;
  unsigned int nesting_depth_;
  // TODO: these next four are for the purpose of handling old-style <Schema>
  // usage. Instead of creating these by default, we could move them into
  // a separate class created only when needed.
  bool in_old_schema_placemark_;
  string old_schema_name_;
  kmlbase::StringVector simplefield_name_vec_;
  std::vector<SimpleDataPtr> simpledata_vec_;

  // This calls the NewElement() method of each ParserObserver.  If any
  // ParserObserver::NewElement() returns false this immediately returns false.
  // If all ParserObserver::NewElement()'s return true this returns true.
  bool CallNewElementObservers(const parser_observer_vector_t& observers,
                               const ElementPtr& element);

  // This calls the EndElement() method of each ParserObserver.  If any
  // ParserObserver::EndElement() returns false the child is NOT added to
  // the parent.
  bool CallEndElementObservers(const parser_observer_vector_t& observers,
                               const ElementPtr& parent,
                               const ElementPtr& child);

  // This calls the AddChild() method of each ParserObserver.  If any
  // ParserObserver::AddChild() returns false this immediately returns false.
  // If all ParserObserver::AddChild()'s return true this returns true.
  bool CallAddChildObservers(const parser_observer_vector_t& observers,
                             const ElementPtr& parent,
                             const ElementPtr& child);

  // Looks in attrs to find the attributes of an old KML 2.0/2.1
  // <Schema parent="Placemark" name="..."> element. Writes the value of the
  // name attribute to old_schema_name.
  static void FindOldSchemaParentName(const kmlbase::StringVector& attrs,
                                      string* old_schema_name);

  // Returns true if name matches the name of a child declared in an
  // old Schema element. Appends a SimpleData element from the name
  // and character data to simpledata_vec for later reparenting.
  static bool ParseOldSchemaChild(
      const string& name,
      const kmlbase::StringVector& simplefield_name_vec,
      std::vector<SimpleDataPtr>* simpledata_vec);

  // Handle reaching the closing old-style </Schema> tag.
  static void HandleOldSchemaEndElement(
      const SchemaPtr& schema,
      const string& old_schema_name,
      kmlbase::StringVector* simplefield_name_vec);

  // Handle reaching the closing of the element discovered by
  // FindOldSchemaParentName.
  void HandleOldSchemaParentEndElement(
      const PlacemarkPtr& placemark,
      const string& old_schema_name,
      const KmlFactory& kml_factory,
      const std::vector<SimpleDataPtr> simpledata_vec);

  const parser_observer_vector_t& observers_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(KmlHandler);
};

} // end namespace kmldom

#endif  // KML_DOM_KML_HANDLER_H__
