#ifndef AIRMAP_GEOMETRY_H_
#define AIRMAP_GEOMETRY_H_

#include <airmap/optional.h>

#include <vector>

namespace airmap {

/// Geometry bundles up different types of geometries.
class Geometry {
 public:
  /// Type enumerates all known geometry types.
  enum class Type {
    invalid,             ///< Marks an invalid geometry.
    point,               ///< Geometry contains a Point.
    multi_point,         ///< Geometry contains a MultiPoint.
    line_string,         ///< Geometry contains a LineString.
    multi_line_string,   ///< Geometry contains a MultiLineString.
    polygon,             ///< Geometry contains a Polygon.
    multi_polygon,       ///< Geometry contains a MultiPolygon.
    geometry_collection  ///< Geometry is a GemetryCollection.
  };

  /// Coordinate marks a point in 3-dimensional space.
  struct Coordinate {
    double latitude;            /// The latitude component of this coordinate in [°].
    double longitude;           /// The longitude component of this coordinate in [°].
    Optional<double> altitude;  /// The altitude component of this coordinate in [m].
    Optional<double> elevation;
  };

  /// CoordinateVector is a collection of points in 3-dimensional space.
  template <Type tag>
  struct CoordinateVector {
    std::vector<Coordinate> coordinates;  ///< The individual coordinates.
  };

  using Point           = Coordinate;
  using MultiPoint      = CoordinateVector<Type::multi_point>;
  using LineString      = CoordinateVector<Type::line_string>;
  using MultiLineString = std::vector<LineString>;
  /// Polygon follows the GeoJSON standard, citing from https://tools.ietf.org/html/rfc7946:
  ///   * For type "Polygon", the "coordinates" member MUST be an array of
  ///     linear ring coordinate arrays.
  ///   * For Polygons with more than one of these rings, the first MUST be
  ///     the exterior ring, and any others MUST be interior rings.  The
  ///     exterior ring bounds the surface, and the interior rings (if
  ///     present) bound holes within the surface.
  struct Polygon {
    CoordinateVector<Type::polygon> outer_ring;
    std::vector<CoordinateVector<Type::polygon>> inner_rings;
  };
  using MultiPolygon       = std::vector<Polygon>;
  using GeometryCollection = std::vector<Geometry>;

  /// point returns a Geometry instance with Type::point at the given coordinate (lat, lon).
  static Geometry point(double lat, double lon);
  /// polygon returns a Geometry instance with Type::polygon with the given 'coordinates'.
  static Geometry polygon(const std::vector<Coordinate>& coordinates);

  /// Initializes a new instance with Type::invalid.
  Geometry();
  /// Geometry initializes a new instance with the given Point.
  explicit Geometry(const Point& other);
  /// Geometry initializes a new instance with the given MultiPoint.
  explicit Geometry(const MultiPoint& other);
  /// Geometry initializes a new instance with the given LineString.
  explicit Geometry(const LineString& other);
  /// Geometry initializes a new instance with the given MultiLineString.
  explicit Geometry(const MultiLineString& other);
  /// Geometry initializes a new instance with the given Polyon.
  explicit Geometry(const Polygon& other);
  /// Geometry initializes a new instance with the given MultiPolygon.
  explicit Geometry(const MultiPolygon& other);
  /// Geometry initializes a new instance with the given GeometryCollection.
  explicit Geometry(const GeometryCollection& other);
  /// @cond
  Geometry(const Geometry& other);
  ~Geometry();
  Geometry& operator=(const Geometry& rhs);
  bool operator==(const Geometry& rhs) const;
  /// @endcond

  /// type returns the Type of the geometry.
  Type type() const;
  /// details_for_point returns an immutable instance to the contained Point instance.
  const Point& details_for_point() const;
  /// details_for_multi_point returns an immutable instance to the contained MultiPoint instance.
  const MultiPoint& details_for_multi_point() const;
  /// details_for_line_string returns an immutable instance to the contained LineString instance.
  const LineString& details_for_line_string() const;
  /// details_for_multi_line_string returns an immutable instance to the contained MultiLineString instance.
  const MultiLineString& details_for_multi_line_string() const;
  /// details_for_polygon returns an immutable instance to the contained Polygon instance.
  const Polygon& details_for_polygon() const;
  /// details_for_multi_polygon returns an immutable instance to the contained MultiPolygon instance.
  const MultiPolygon& details_for_multi_polygon() const;
  /// details_for_geometry_collection returns an immutable instance to the contained GeometryCollection instance.
  const GeometryCollection details_for_geometry_collection() const;

 private:
  struct Invalid {};

  union Data {
    Data();
    ~Data();

    Invalid invalid;
    Point point;
    MultiPoint multi_point;
    LineString line_string;
    MultiLineString multi_line_string;
    Polygon polygon;
    MultiPolygon multi_polygon;
    GeometryCollection geometry_collection;
  };

  Geometry& reset();
  void set_point(const Point& point);
  void set_multi_point(const MultiPoint& multi_point);
  void set_line_string(const LineString& line_string);
  void set_multi_line_string(const MultiLineString& multi_line_string);
  void set_polygon(const Polygon& polygon);
  void set_multi_polygon(const MultiPolygon& multi_polygon);
  void set_geometry_collection(const GeometryCollection& geometry_collection);
  Geometry& set_geometry(const Geometry& other);

  Type type_;
  Data data_;
};

/// @cond
bool operator==(const Geometry::Coordinate& lhs, const Geometry::Coordinate& rhs);

bool operator==(const Geometry::Polygon& lhs, const Geometry::Polygon& rhs);

template <Geometry::Type tag>
bool operator==(const Geometry::CoordinateVector<tag>& lhs, const Geometry::CoordinateVector<tag>& rhs) {
  return lhs.coordinates == rhs.coordinates;
}
/// @endcond

}  // namespace airmap

#endif  // AIRMAP_GEOMETRY_H_
