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

// This file contains the declaration of the XsdFile class.

#ifndef KML_XSD_XSD_FILE_H__
#define KML_XSD_XSD_FILE_H__

#include <map>
#include <stack>
#include <vector>
#include "boost/scoped_ptr.hpp"
#include "kml/base/xmlns.h"
#include "kml/xsd/xsd_element.h"
#include "kml/xsd/xsd_complex_type.h"
#include "kml/xsd/xsd_schema.h"
#include "kml/xsd/xsd_simple_type.h"
#include "kml/xsd/xsd_type.h"

namespace kmlxsd {

typedef std::map<string, string> XsdAliasMap;
typedef std::map<string, XsdElementPtr> XsdElementMap;
typedef std::vector<XsdElementPtr> XsdElementVector;
typedef std::map<string, XsdTypePtr> XsdTypeMap;
typedef std::vector<XsdTypePtr> XsdTypeVector;

// This class holds the state of a processed XSD file.  Overall usage:
//   // Process: fetch and parse.
//   const string xsd_data = fetch_the_xsd_file();
//   string errors;
//   XsdFile* xsd_file = XsdFile::CreateFromParse(xsd_data, &errors);
//   // Access XSD file info.
//   xsd_file.GetElementNames(...);
//   xsd_file.IsComplex(...);
//   GetChildElementNames(...);
class XsdFile {
 public:
  static XsdFile* CreateFromParse(const string& xsd_data,
                                  string* errors);

  XsdFile() {}  // Use static CreateFromParse().

  // Set <xs:schema> info.  The attributes are those of the <schema> element
  // in the XSD file.
  void set_schema(const XsdSchemaPtr& xsd_schema) {
    xsd_schema_ = xsd_schema;
  }

  // Add global <xs:element> info.  A "global" <xs:element> is a child of
  // <xs::schema>.
  void add_element(const XsdElementPtr& xsd_element) {
    element_map_[xsd_element->get_name()] = xsd_element;
  }

  // Add a <xs:complexType> or <xs:simpleType>.
  void add_type(const XsdTypePtr& xsd_type) {
    type_map_[xsd_type->get_name()] = xsd_type;
  }

  const string& get_target_namespace() const {
    return xsd_schema_->get_target_namespace();
  }
  const string& get_target_namespace_prefix() const {
    return xsd_schema_->get_target_namespace_prefix();
  }

  // Create an alias.  For example, "AbstractFeatureGroup" == "Feature".
  void set_alias(const string& real_name,
                 const string& alias_name) {
    alias_map_[real_name] = alias_name;
  }

  // Returns the alias for this name or NULL if this name has no alias.  For
  // example, if set_alias("AbstractGeometryGroup", "Geometry") was used then
  // get_alias("AbstractGeometryGroup") returns "Geometry".
  const string get_alias(const string& real_name) const {
    XsdAliasMap::const_iterator iter = alias_map_.find(real_name);
    return iter == alias_map_.end() ? "" : iter->second;
  }

  // Return all <xs:element> children of <xs:schema>.  Order is not preserved
  // w.r.t. to the XSD file.
  void GetAllElements(XsdElementVector* elements) const;

  // Return all <xs:complexType> and <xs:simpleType> children of <xs:schema>.
  //  Order is not preserved w.r.t. to the XSD file.
  void GetAllTypes(XsdTypeVector* type) const;

  // Return the names of the immediate child elements of the given complex
  // element.  Each <xs:element ref="..."> is resolved.
  void FindChildElements(const XsdComplexTypePtr& complex_element,
                         XsdElementVector* elements) const;
  void GetChildElements(const string& complex_element_name,
                        XsdElementVector* elements) const;


  // This looks up the given element by name.
  const XsdElementPtr FindElement(const string& element_name) const;

  // Return the XsdType for the given element.  If there is no type for this
  // element in the target namespace NULL is returned.
  const XsdTypePtr FindElementType(const XsdElementPtr& element) const;

  // This looks up the given type by name.
  const XsdTypePtr FindType(const string& type_name) const;

  // Return the global <xs:element> for the given <xs:element ref="..."/>.
  // For example, element ref of "kml:name" returns the XsdElement describing
  // the <xs:element name="name".../> child of <xs:schema/>.
  const XsdElementPtr ResolveRef(const string& element_ref) const;

  // Return the XsdComplexType of the complex_type's extension base.  NULL is
  // returned if the complex_type has no extension base or if the extension
  // base is not found within this XsdFile's target namespace.
  XsdComplexTypePtr GetBaseType(const XsdComplexTypePtr& complex_type) const;

  // Return the inheritance hierarchy of a given <xs:complexType>.  The first
  // item in the vector is this type's extension base, the next that type's
  // extension base and so on until a type with no extension base is reached.
  bool GetTypeHierarchy(const XsdComplexTypePtr& complex_type,
                        std::vector<XsdComplexTypePtr>* type_hier) const;

  // This appends all abstract elements to the given vector.
  void GetAbstractElements(XsdElementVector* elements) const;

  // This appends all concrete complex elements to the given vector.
  void GetComplexElements(XsdElementVector* elements) const;

  // This appends all concrete simple elements to the given vector.
  void GetSimpleElements(XsdElementVector* elements) const;

  // This sorts the elements in this XSD file into 3 ranges in this order and
  // then alphabetical order within each group:  1) abstract elements,
  // 2) complex elements, 3) simple elements.  Offset 0 is reserved/invalid.
  void GenerateElementIdVector(XsdElementVector* elements,
                               size_t* begin_complex,
                               size_t *begin_simple) const;

  // If find_type is a base type of complex_type return true, else false.
  bool SearchTypeHierarchy(const XsdComplexTypePtr& complex_type,
                           const XsdComplexTypePtr& find_type) const;

  // Return all elements derived from the given complex type.
  void GetElementsOfType(const XsdComplexTypePtr& complex_type,
                         XsdElementVector* elements) const;

  // Return all elements derived from the given complex type name.
  void GetElementsOfTypeByName(const string& type_name,
                               XsdElementVector* elements) const;

 private:
  XsdSchemaPtr xsd_schema_;
  XsdElementMap element_map_;
  XsdTypeMap type_map_;
  XsdAliasMap alias_map_;
};

}  // end namespace kmlxsd

#endif // KML_XSD_XSD_FILE_H__
