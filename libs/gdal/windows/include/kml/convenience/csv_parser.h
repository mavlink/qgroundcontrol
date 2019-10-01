// Copyright 2010, Google Inc. All rights reserved.
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

// This file contains the declaration of the CsvParser class.

#ifndef KML_CONVENIENCE_CSV_PARSER_H_
#define KML_CONVENIENCE_CSV_PARSER_H_

#include "kml/base/string_util.h"
#include "kml/dom.h"

namespace kmlbase {
class CsvSplitter;
}

namespace kmlconvenience {

enum CsvParserStatus {
  CSV_PARSER_STATUS_OK = 0,
  CSV_PARSER_STATUS_BLANK_LINE,
  CSV_PARSER_STATUS_NO_LAT_LON,
  CSV_PARSER_STATUS_BAD_LAT_LON,
  CSV_PARSER_STATUS_INVALID_DATA,
  CSV_PARSER_STATUS_COMMENT
};

// This class is used as the output and error reporting mechanism for the
// CsvParser.  Application code should subclass this and implement HandleLine.
// This "default" implementation acts as a data sink.
class CsvParserHandler {
 public:
  virtual ~CsvParserHandler() {}

  // This method is called for each line in the CSV data.  The line is the line
  // number of the CSV.  The status indicates the success of creating
  // a KML Point Placemark from the CSV line.  A Placemark is always created
  // but it may be devoid of children depending on the strictness state of
  // CsvParser.  In strict mode CSV_PARSER_STATUS_OK indicates the Placemark
  // has at least a Point with lat and lon.  The caller takes ownership of
  // the placemark.  The return value indicates if CSV parsing is to continue
  // to the next line in the file.  Returning false immediately halts all
  // further processing of the CsvParse.
  virtual bool HandleLine(int line, CsvParserStatus status,
                          kmldom::PlacemarkPtr placemark) {
    return true;  // Always continue to the next line.
  }
};

// This class converts CSV data to KML.  Overall usage:
//  CsvSplitter csv_splitter(csv_data);
//  class YourCsvParserHandler : public CsvParserHandler {
//   public:
//    virtual bool HandleLine(int line, CsvParserStatus status,
//                            kmldom::PlacemarkPtr placemark) {
//      ...inspect status and/or save/process placemark...
//      return true;  // Or false to stop CSV parsing.
//    }
//  };
//  YourCsvParserHandler your_csv_parser_handler;
//  CsvParser::ParseCsv(&csv_splitter, &your_csv_parser_handler);
class CsvParser {
 public:
  // This method uses CsvSplitter to split each line which CsvParser converts
  // to KML which is handed to the CsvParserHandler.
  static bool ParseCsv(kmlbase::CsvSplitter* csv_splitter,
                       CsvParserHandler* csv_parser_handler);

  // All of the below should really be private.

  // Use the static ParseCsv method.
  CsvParser(kmlbase::CsvSplitter* csv_splitter,
            CsvParserHandler* csv_parser_handler);

  // This gets the internal CSV schema.
  typedef std::map<int, string> CsvSchema;
  const CsvSchema& GetSchema() const {
    return csv_schema_;
  }

  // This internal method sets the schema for subsequent lines of CSV data.
  // This sets the mappings from column to field.  Here is how the data for
  // each column is used.  VAL is substituted for the value in the cell:
  //   name - <name>VAL</name>
  //   feature-id - <Placemark id="feature-VAL">
  //   description - <description>VAL</description>
  //   style-id - <styleUrl>style.kml#style-VAL</styleUrl>
  //   latitude - <Point><coordinates>xxx,VAL</coordinates></Point>
  //   longitude - <Point><coordinates>VAL,xxx</coordinates></Point>
  //   other - <Data name="other"><value>VAL</value></Data>
  //   # - comment causes CSV_PARSER_STATUS_COMMENT for that line
  // The "latitude" and "longitude" columns specify which columns are used
  // for the latitude and longitude of the <Point>.  All other columns specify
  // <ExtendedData>/<Data> names.  The csv_schema must contain at least
  // "latitude" and "longitude".  Any schema term may be mixed case.
  CsvParserStatus SetSchema(const kmlbase::StringVector& csv_schema);

  // This internal method sets the fields of the given placemark from the
  // csv_line as per the state of the csv schema.  The csv_line size must
  // match the CSV schema.
  CsvParserStatus CsvLineToPlacemark(kmlbase::StringVector& csv_line,
                                     kmldom::PlacemarkPtr placemark) const;

  // This internal method iterates over each line using the CsvSplitter and
  // and passes the created KML to the CsvParserHandler.
  bool ParseCsvData();

 private:
  kmlbase::CsvSplitter* csv_splitter_;
  CsvParserHandler* csv_parser_handler_;
  size_t schema_size_;
  size_t name_col_;
  size_t description_col_;
  size_t lat_col_;
  size_t lon_col_;
  size_t feature_id_;
  size_t style_id_;
  string style_url_base_;
  kmldom::KmlFactory* kml_factory_;
  CsvSchema csv_schema_;
  LIBKML_DISALLOW_EVIL_CONSTRUCTORS(CsvParser);
};

}  // end namespace kmlconvenience

#endif  // KML_CONVENIENCE_CSV_PARSER_H_
