cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Json)

set(_json [=[
{
  "gstreamer": {
    "plugins": {
      "common": ["coreelements", "playback", "udp"],
      "linux":  ["va"]
    },
    "empty_section": {}
  }
}
]=])

_qgc_json_array_to_list(_common "${_json}" gstreamer plugins common)
list(LENGTH _common _common_len)
qgc_test_assert_streq("common length" "3" "${_common_len}")
qgc_test_assert_in_list("common contains coreelements" coreelements _common)
qgc_test_assert_in_list("common contains udp" udp _common)
qgc_test_pass("happy path nested array")

_qgc_json_array_to_list(_linux "${_json}" gstreamer plugins linux)
qgc_test_assert_streq("linux length" "1" "1")
qgc_test_assert_in_list("linux has va" va _linux)
qgc_test_pass("single-element array")

_qgc_json_array_to_list(_missing "${_json}" gstreamer plugins darwin)
list(LENGTH _missing _missing_len)
qgc_test_assert_streq("missing path length" "0" "${_missing_len}")
qgc_test_pass("missing path returns empty")

_qgc_json_array_to_list(_obj "${_json}" gstreamer empty_section)
list(LENGTH _obj _obj_len)
qgc_test_assert_streq("object-at-path length" "0" "${_obj_len}")
qgc_test_pass("object at path returns empty")
