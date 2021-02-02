// AirMap Platform SDK
// Copyright © 2018 AirMap, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef AIRMAP_RULE_H_
#define AIRMAP_RULE_H_

#include <airmap/visibility.h>

#include <string>

namespace airmap {

struct AIRMAP_EXPORT Rule {
  // TODO(tvoss): Fill in values once schema is known.
  enum class Type {};
  Type type;
  std::string id;
  std::string name;
  std::string description;
  std::string jurisdiction;
  // TODO(tvoss): Add requirements here.
};

AIRMAP_EXPORT bool operator==(const Rule& lhs, const Rule& rhs);

}  // namespace airmap

#endif  // AIRMAP_RULE_H_
