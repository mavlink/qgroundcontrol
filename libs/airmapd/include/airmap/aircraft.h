#ifndef AIRMAP_AIRCRAFT_H_
#define AIRMAP_AIRCRAFT_H_

#include <string>

namespace airmap {

/// Aircraft describes an aircraft in terms of its model and its manufacturer.
struct Aircraft {
  /// Model bundles up a model id and a product name.
  struct Model {
    std::string id;    ///< The unique id of the model in the context of AirMap.
    std::string name;  ///< The human-readable name of the model.
  };

  /// Manufacturer bundles up an id and a human-readable name.
  /// Please note that the id is only unique/relevant in the context of the
  /// AirMap services.
  struct Manufacturer {
    std::string id;    ///< The unique id of the manufacturer in the context of AirMap.
    std::string name;  ///< The human-readable name of the manufacturer.
  };

  Model model;                ///< Details describing the model of an aircraft.
  Manufacturer manufacturer;  ///< Details about the manufacturer of an aircraft.
};

}  // namespace airmap

#endif  // AIRMAP_AIRCRAFT_H_
