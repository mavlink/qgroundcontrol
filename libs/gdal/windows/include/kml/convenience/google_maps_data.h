// Copyright 2009, Google Inc. All rights reserved.
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

// This file contains the declaration of the GoogleMapsData class.
// Very distantly related to:
// http://code.google.com/p/gdata-cpp-util/source/browse/trunk/gdata/client/
//     doc_list_service.h
// TODO: this interface and implemenation are under construction, expect
// additions and changes.

#ifndef KML_CONVENIENCE_GOOGLE_MAPS_DATA_H_
#define KML_CONVENIENCE_GOOGLE_MAPS_DATA_H_

#include "boost/scoped_ptr.hpp"
#include "kml/dom.h"

// TODO: move Bbox to kmlbase
namespace kmlengine {
class Bbox;
}

namespace kmlconvenience {

class HttpClient;

// This class provides an API to the Google Maps Data API.
// See the "Google Maps Data API HTTP Protocol Guide":
// http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html
// Overall usage:
//   class YourHttpClient : public kmlconvenience::HttpClient {
//    public:
//     virtual bool SendRequest(...) const {
//       ...your network I/O code goes here...
//     };
//   };
//   YourHttpClient* your_http_client = new YourHttpClient;
//   your_http_client->Login("user@gmail.com", "users-password");
//   GoogleMapsData* maps_data = GoogleMapsData::Create(&your_http_client);
//   string map_feed_atom;
//   maps_data->GetMapFeed(&map_feed_atom);
//   kmldom::ElementPtr root = kmldom::ParseAtom(map_feed_atom);
//   kmlconvenience::AtomUtil... for common Atom inspection.
class GoogleMapsData {
 public:
  // Create a GoogleMapsData object.  The HttpClient must already be logged
  // in.  See http_client.h for more information about authentication.
  // The HttpClient is used for all network communication from inside this
  // GoogleMapsData instance.
  static GoogleMapsData* Create(HttpClient* http_client);
  ~GoogleMapsData();

  // This returns the Google Maps Data service name.  This is the name that
  // should be used with ClientLogin authentication.
  static const char* get_service_name();

  // This returns the pathname portion of the Google Maps Data "meta feed".
  static const char* get_metafeed_uri();

  // This returns the scope (hostname:port).
  const string& get_scope() const;

  // This returns the HttpClient.  Ownership is retained by this class.
  HttpClient* get_http_client() const;

  // This returns the "meta feed" for the authenticated user.  The result is an
  // Atom <feed> containing an <entry> for each of the user's maps.  See:
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#RetrievingMetafeed
  bool GetMetaFeedXml(string* atom_feed) const;

  // This calls GetMetaFeedXml and returns the parsed result.
  kmldom::AtomFeedPtr GetMetaFeed() const;

  // This creates a new map with the given title and summary.  This is simply
  // an HTTP POST to the user's maps meta feed.  On success true is returned.
  // If a map_entry_xml string is supplied the <atom:entry> for the new map
  // is saved there.  See:
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#CreatingMaps
  bool CreateMap(const string& title, const string& summary,
                 string* map_entry_xml);

  // TODO:
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#UpdatingMaps

  // TODO:
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#DeletingMaps

  // This returns the URI of the Feature Feed of the given map.
  // "A map's feature feed is published in the map's <content> tag within its
  // src attibute." See:
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#RetrievingMaps
  static bool GetFeatureFeedUri(const kmldom::AtomEntryPtr& map_entry,
                                string* feature_feed_uri);

  // This fetches the given URI and saves the result in the supplied string.
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#RetrievingFeatures
  bool GetFeatureFeedXml(const string& feature_feed_uri,
                         string* atom_feed) const;

  // This calls GetFeatureFeedXml and returns the parsed result.
  kmldom::AtomFeedPtr GetFeatureFeedByUri(
      const string& feature_feed_uri) const;

  // Return the KML Feature child of the Atom <entry>'s <content>.  This
  // returns NULL if the <entry>'s <content> has no KML Feature.
  static kmldom::FeaturePtr GetEntryFeature(const kmldom::AtomEntryPtr& entry);

  // This appends the KML Feature in each of the feed's entry's to the
  // given container.  The number of KML Features appended is returned.
  // Each Feature added to the Container is a full clone from the feed entry.
  static int GetMapKml(const kmldom::AtomFeedPtr& feature_feed,
                       kmldom::ContainerPtr container);

  // Creates a <Document>, sets the <atom:link> and calls GetMapKml.
  kmldom::DocumentPtr CreateDocumentOfMapFeatures(
      const kmldom::AtomFeedPtr& feature_feed);

  // This adds a new feature to a map.  This is simply an HTTP POST to the
  // given feature feed URI which can be retrieved from the map entry using
  // GetFeatureFeedUri.  On success true is returned.  If a feature_entry_xml
  // string is supplied the <atom:entry> for the new feature is saved there.
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#CreatingFeatures
  bool AddFeature(const string& feature_feed_post_uri,
                  const kmldom::FeaturePtr& feature,
                  string* feature_entry_xml);

  // This is a convenience utility based on AddFeature.  This calls AddFeature
  // on each <Placemark> in the given feature hierarchy.  The total count of
  // <Placemark>s successfully added is returned.  All non-<Placemark> features
  // are ignored including Containers thus flattening any hierarchy.
  int PostPlacemarks(const kmldom::FeaturePtr& root_feature,
                     const string& feature_feed_uri);

  // TODO:
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#UpdatingFeatures

  // TODO:
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#DeletingFeatures

  // This gets the feed representing a search over the specified map.
  // Use GetSearchFeedUri to get the search_feed_uri for a map.
  // Construct the search_parameters using AppendBoxParameter(), etc.
  // http://code.google.com/apis/maps/documentation/mapsdata/developers_guide_protocol.html#Search
  bool GetSearchFeed(const string& search_feed_uri,
                     const string& search_parameters,
                     string* atom_feed);

  // This returns the search feed URI for the given map.
  static bool GetSearchFeedUri(const kmldom::AtomEntryPtr& map_entry,
                               string* search_feed_uri);

  // This is a convenience to format "bbox=w,s,e,n" search parameters.
  static void AppendBoxParameter(const double north, const double south,
                                 const double east, const double west,
                                 string* search_parameters);

  // This is a convenience to format "bbox=w,s,e,n" search parameters.
  static void AppendBoxParameterFromBbox(const kmlengine::Bbox& bbox,
                                         string* search_parameters);

  // This returns a feed to features of the map within the bbox.
  kmldom::AtomFeedPtr SearchMapByBbox(const kmldom::AtomEntryPtr& map_entry,
                                      const kmlengine::Bbox& bbox);

  // TODO: implement these queries
  // "mq" "mq"
  // "lat,lon" "lat
  // "radius"
  // "sortby"

  // This creates a new map and adds a feature for each line of CSV data.
  // See kmlconvenience::CsvParser for details about the CSV format.  This
  // method is strict: the map will not be created if any of the CSV lines
  // are bad.
  kmldom::AtomEntryPtr PostCsv(const string& title, const string& csv_data,
                               string* errors);

  // This creates a new map and adds a feature for each KML feature.  Note
  // that not all OGC KML 2.2 Feature's are recognized by the Google Maps
  // Data API: the unrecognized Feature's are quietly ignored and not added
  // to the Google My Map.  At present support is limited to <Placemark>.
  // On success the standard Google Maps Data <atom:entry> for the new map
  // is returned.  On failure NULL is returned.
  kmldom::AtomEntryPtr PostKml(const string& title, const string& kml_data);

  // This is the common code for PostCsv and PostKml.  The slug and
  // content_type arguments create the Slug: and Content-Type: headers
  // respectively and the data is HTTP POST'ed returning a parsed Atom
  // entry for the created map.  If the POST failed a NULL is returned and
  // an error may be saved to the passed error string if one is supplied.
  // At present Google Maps Data API supports CSV and KML.  Use of the
  // PostCsv() and PostKml() are the recommended methods.
  kmldom::AtomEntryPtr PostMedia(const string& slug,
                                 const string& content_type,
                                 const string& data,
                                 string* errors);

  static bool GetKmlUri(const kmldom::AtomEntryPtr& map_entry, string* kml_uri);

 private:
  // Use static Create().
  GoogleMapsData();
  boost::scoped_ptr<HttpClient> http_client_;
  const string scope_;
};

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_GOOGLE_MAPS_DATA_H_
