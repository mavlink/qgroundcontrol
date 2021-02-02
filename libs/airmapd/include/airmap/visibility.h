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
#ifndef AIRMAP_VISIBILITY_H_
#define AIRMAP_VISIBILITY_H_

#if defined(__GNUC__) || defined(__clang__)
#define AIRMAP_EXPORT __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define AIRMAP_EXPORT __declspec(dllexport)
#else
#pragma message("unknown compiler - default AIRMAP_EXPORT to empty")
#define AIRMAP_EXPORT
#endif

#endif  // AIRMAP_VISIBILITY_H_
