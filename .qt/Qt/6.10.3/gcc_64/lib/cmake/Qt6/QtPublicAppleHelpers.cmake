# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(_qt_internal_handle_ios_launch_screen target)
    # Check if user provided a launch screen path via a variable.
    set(launch_screen "")

    # Check if the project provided a launch screen path via a variable.
    # This variable is currently in Technical Preview.
    if(QT_IOS_LAUNCH_SCREEN)
        set(launch_screen "${QT_IOS_LAUNCH_SCREEN}")
    endif()

    # Check if the project provided a launch screen path via a target property, it takes precedence
    # over the variable.
    # This property is currently in Technical Preview.
    get_target_property(launch_screen_from_prop "${target}" QT_IOS_LAUNCH_SCREEN)
    if(launch_screen_from_prop)
        set(launch_screen "${launch_screen_from_prop}")
    endif()

    # If the project hasn't provided a launch screen file path, use a copy of the template
    # that qmake uses.
    # It needs to be a copy because configure_file can't handle all the escaped double quotes
    # present in the qmake template file.
    set(is_default_launch_screen FALSE)
    if(NOT launch_screen AND NOT QT_NO_SET_DEFAULT_IOS_LAUNCH_SCREEN)
        set(is_default_launch_screen TRUE)
        set(launch_screen
            "${__qt_internal_cmake_apple_support_files_path}/LaunchScreen.storyboard")
    endif()

    # Check that the launch screen exists.
    if(launch_screen)
        if(NOT IS_ABSOLUTE "${launch_screen}")
            message(FATAL_ERROR
                "Provided launch screen value should be an absolute path: '${launch_screen}'")
        endif()

        if(NOT EXISTS "${launch_screen}")
            message(FATAL_ERROR
                "Provided launch screen file does not exist: '${launch_screen}'")
        endif()
    endif()

    if(launch_screen AND NOT QT_NO_ADD_IOS_LAUNCH_SCREEN_TO_BUNDLE)
        get_filename_component(launch_screen_name "${launch_screen}" NAME)

        # Make a copy of the default launch screen template for this target and replace the
        # label inside the template with the target name.
        if(is_default_launch_screen)
            # Configure our default template and place it in the build dir.
            set(launch_screen_in_path "${launch_screen}")

            string(MAKE_C_IDENTIFIER "${target}" target_identifier)
            set(launch_screen_out_dir
                "${CMAKE_CURRENT_BINARY_DIR}/.qt/launch_screen_storyboards/${target_identifier}")

            set(launch_screen_out_path
                "${launch_screen_out_dir}/${launch_screen_name}")

            file(MAKE_DIRECTORY "${launch_screen_out_dir}")

            configure_file("${launch_screen_in_path}" "${launch_screen_out_path}" COPYONLY)

            set(final_launch_screen_path "${launch_screen_out_path}")
        else()
            set(final_launch_screen_path "${launch_screen}")
        endif()

        # Add the launch screen storyboard file as a source file, otherwise CMake doesn't consider
        # it as a resource file and MACOSX_PACKAGE_LOCATION processing will be skipped.
        target_sources("${target}" PRIVATE "${final_launch_screen_path}")

        # Ensure Xcode compiles the storyboard file and installs the compiled storyboard .nib files
        # into the app bundle.
        # We use target_sources and the MACOSX_PACKAGE_LOCATION source file property for that
        # instead of the RESOURCE target property, becaues the latter could potentially end up
        # needlessly installing the source storyboard file.
        #
        # We can't rely on policy CMP0118 since user project controls it.
        set(scope_args)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
            set(scope_args TARGET_DIRECTORY ${target})
        endif()
        set_source_files_properties("${final_launch_screen_path}" ${scope_args}
            PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

        # Save the launch screen name, so its value is added as an UILaunchStoryboardName entry
        # in the Qt generated Info.plist file.
        # Xcode expects an Info.plist storyboard entry without an extension.
        get_filename_component(launch_screen_base_name "${launch_screen}" NAME_WE)
        set_target_properties("${target}" PROPERTIES
                              _qt_ios_launch_screen_name "${launch_screen_name}"
                              _qt_ios_launch_screen_base_name "${launch_screen_base_name}"
                              _qt_ios_launch_screen_path "${final_launch_screen_path}")
    endif()
endfunction()

function(_qt_internal_find_apple_development_team_id out_var)
    get_property(team_id GLOBAL PROPERTY _qt_internal_ios_development_team_id)
    get_property(team_id_computed GLOBAL PROPERTY _qt_internal_apple_development_team_id_computed)
    if(team_id_computed)
        # Just in case if the value is non-empty but still booly FALSE.
        if(NOT team_id)
            set(team_id "")
        endif()
        set("${out_var}" "${team_id}" PARENT_SCOPE)
        return()
    endif()

    set_property(GLOBAL PROPERTY _qt_internal_apple_development_team_id_computed "TRUE")

    set(home_dir "$ENV{HOME}")
    set(xcode_preferences_path "${home_dir}/Library/Preferences/com.apple.dt.Xcode.plist")

    # Extract the first account name (email) from the user's Xcode preferences
    message(DEBUG "Trying to extract an Xcode development team id from '${xcode_preferences_path}'")

    # Try Xcode 16.2 format first
    _qt_internal_plist_buddy("${xcode_preferences_path}"
        COMMANDS "print IDEProvisioningTeamByIdentifier"
        EXTRA_ARGS -x
        OUTPUT_VARIABLE teams_xml
        ERROR_VARIABLE plist_error
    )

    # Then fall back to older format
    if(plist_error OR NOT teams_xml)
        _qt_internal_plist_buddy("${xcode_preferences_path}"
            COMMANDS "print IDEProvisioningTeams"
            EXTRA_ARGS -x
            OUTPUT_VARIABLE teams_xml
            ERROR_VARIABLE plist_error
        )
    endif()

    # Parsing state.
    set(is_free "")
    set(current_team_id "")
    set(parsing_is_free FALSE)
    set(parsing_team_id FALSE)
    set(first_team_id "")

    # Parse the xml output and return the first encountered non-free team id. If no non-free team id
    # is found, return the first encountered free team id.
    # If no team is found, return an empty string.
    #
    # Example input:
    #<plist version="1.0">
    #<dict>
    #    <key>marty@planet.local</key>
    #    <array>
    #        <dict>
    #            <key>isFreeProvisioningTeam</key>
    #            <false/>
    #            <key>teamID</key>
    #            <string>AAA</string>
    #            ...
    #        </dict>
    #        <dict>
    #            <key>isFreeProvisioningTeam</key>
    #            <true/>
    #            <key>teamID</key>
    #            <string>BBB</string>
    #            ...
    #        </dict>
    #    </array>
    #    <key>AAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE</key>
    #    <array>
    #        <dict>
    #            <key>isFreeProvisioningTeam</key>
    #            <false/>
    #            <key>teamID</key>
    #            <string>CCC</string>
    #            ...
    #        </dict>
    #    </array>
    #</dict>
    #</plist>
    if(teams_xml AND NOT plist_error)
        string(REPLACE "\n" ";" teams_xml_lines "${teams_xml}")

        foreach(xml_line ${teams_xml_lines})
            string(STRIP "${xml_line}" xml_line)
            if(xml_line STREQUAL "<dict>")
                # Clean any previously found values when a new team dict is matched.
                set(is_free "")
                set(current_team_id "")

            elseif(xml_line STREQUAL "<key>isFreeProvisioningTeam</key>")
                set(parsing_is_free TRUE)

            elseif(parsing_is_free)
                set(parsing_is_free FALSE)

                if(xml_line MATCHES "true")
                    set(is_free TRUE)
                else()
                    set(is_free FALSE)
                endif()

            elseif(xml_line STREQUAL "<key>teamID</key>")
                set(parsing_team_id TRUE)

            elseif(parsing_team_id)
                set(parsing_team_id FALSE)
                if(xml_line MATCHES "<string>([^<]+)</string>")
                    set(current_team_id "${CMAKE_MATCH_1}")
                else()
                    continue()
                endif()

                string(STRIP "${current_team_id}" current_team_id)

                # If this is the first team id we found so far, remember that, regardless if's free
                # or not.
                if(NOT first_team_id AND current_team_id)
                    set(first_team_id "${current_team_id}")
                endif()

                # Break early if we found a non-free team id and use it, because we prefer
                # a non-free team for signing, just like qmake.
                if(NOT is_free AND current_team_id)
                    set(first_team_id "${current_team_id}")
                    break()
                endif()
            endif()
        endforeach()
    endif()

    if(NOT first_team_id)
        message(DEBUG "Failed to extract an Xcode development team id.")
        set("${out_var}" "" PARENT_SCOPE)
    else()
        message(DEBUG "Successfully extracted the first encountered Xcode development team id.")
        set_property(GLOBAL PROPERTY _qt_internal_ios_development_team_id "${first_team_id}")
        set("${out_var}" "${first_team_id}" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_get_apple_bundle_identifier_prefix out_var)
    get_property(prefix GLOBAL PROPERTY _qt_internal_ios_bundle_identifier_prefix)
    get_property(prefix_computed GLOBAL PROPERTY
                 _qt_internal_ios_bundle_identifier_prefix_computed)
    if(prefix_computed)
        # Just in case if the value is non-empty but still booly FALSE.
        if(NOT prefix)
            set(prefix "")
        endif()
        set("${out_var}" "${prefix}" PARENT_SCOPE)
        return()
    endif()

    set_property(GLOBAL PROPERTY _qt_internal_ios_bundle_identifier_prefix_computed "TRUE")

    set(home_dir "$ENV{HOME}")
    set(xcode_preferences_path "${home_dir}/Library/Preferences/com.apple.dt.Xcode.plist")

    message(DEBUG "Trying to extract the default bundle identifier prefix from Xcode preferences.")
    execute_process(COMMAND "/usr/libexec/PlistBuddy"
                            -c "print IDETemplateOptions:bundleIdentifierPrefix"
                            "${xcode_preferences_path}"
                    OUTPUT_VARIABLE prefix
                    ERROR_VARIABLE prefix_error)
    if(prefix AND NOT prefix_error)
        message(DEBUG "Successfully extracted the default bundle identifier prefix.")
        string(STRIP "${prefix}" prefix)
    else()
        message(DEBUG "Failed to extract the default bundle identifier prefix.")
    endif()

    if(prefix AND NOT prefix_error)
        set_property(GLOBAL PROPERTY _qt_internal_ios_bundle_identifier_prefix "${prefix}")
        set("${out_var}" "${prefix}" PARENT_SCOPE)
    else()
        set("${out_var}" "" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_escape_rfc_1034_identifier value out_var)
    # According to https://datatracker.ietf.org/doc/html/rfc1034#section-3.5
    # we can only use letters, digits, dot (.) and hyphens (-).
    # Underscores are not allowed.
    string(REGEX REPLACE "[^A-Za-z0-9.]" "-" value "${value}")

    set("${out_var}" "${value}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_default_apple_bundle_identifier target out_var)
    _qt_internal_get_apple_bundle_identifier_prefix(prefix)
    if(NOT prefix)
        set(prefix "com.yourcompany")

        # For a better out-of-the-box experience, try to create a unique prefix by appending
        # the sha1 of the team id, if one is found.
        _qt_internal_find_apple_development_team_id(team_id)
        if(team_id)
            string(SHA1 hash "${team_id}")
            string(SUBSTRING "${hash}" 0 8 infix)
            string(APPEND prefix ".${infix}")
        endif()

        if(CMAKE_GENERATOR STREQUAL "Xcode")
            message(WARNING
                "No organization bundle identifier prefix could be retrieved from Xcode preferences. \
                This can lead to code signing issues due to a non-unique bundle \
                identifier. Please set up an organization prefix by creating a new project within \
                Xcode, or consider providing a custom bundle identifier by specifying the \
                MACOSX_BUNDLE_GUI_IDENTIFIER or XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER property."
                )
        endif()
    endif()

    # Escape the prefix according to rfc 1034, it's important for code-signing. If an invalid
    # identifier is used, calling xcodebuild on the command line says that no provisioning profile
    # could be found, with no additional error message. If one opens the generated project with
    # Xcode and clicks on 'Try again' to get a new profile, it shows a semi-useful error message
    # that the identifier is invalid.
    _qt_internal_escape_rfc_1034_identifier("${prefix}" prefix)

    if(CMAKE_GENERATOR STREQUAL "Xcode")
        set(identifier "${prefix}.$(PRODUCT_NAME:rfc1034identifier)")
    else()
        set(identifier "${prefix}.${target}")
    endif()

    set("${out_var}" "${identifier}" PARENT_SCOPE)
endfunction()

function(_qt_internal_set_placeholder_apple_bundle_version target)
    # If user hasn't provided neither a bundle version nor a bundle short version string for the
    # app, set a placeholder value for both which will add them to the generated Info.plist file.
    # This is required so that the app launches in the simulator (but apparently not for running
    # on-device).
    get_target_property(bundle_version "${target}" MACOSX_BUNDLE_BUNDLE_VERSION)
    get_target_property(bundle_short_version "${target}" MACOSX_BUNDLE_SHORT_VERSION_STRING)

    if(NOT MACOSX_BUNDLE_BUNDLE_VERSION AND
       NOT MACOSX_BUNDLE_SHORT_VERSION_STRING AND
       NOT bundle_version AND
       NOT bundle_short_version AND
       NOT QT_NO_SET_XCODE_BUNDLE_VERSION
    )
        get_target_property(version "${target}" VERSION)
        if(NOT version)
            set(version "${PROJECT_VERSION}")
            if(NOT version)
                set(version "1.0.0")
            endif()
        endif()

        # Use x.y for short version and x.y.z for full version
        # Any versions longer than this will fail App Store
        # submission.
        string(REPLACE "." ";" version_list ${version})
        list(LENGTH version_list version_list_length)
        list(GET version_list 0 version_major)
        set(bundle_short_version "${version_major}")
        if(version_list_length GREATER 1)
            list(GET version_list 1 version_minor)
            string(APPEND bundle_short_version ".${version_minor}")
        endif()
        set(bundle_version "${bundle_short_version}")
        if(version_list_length GREATER 2)
            list(GET version_list 2 version_patch)
            string(APPEND bundle_version ".${version_patch}")
        endif()


        if(NOT CMAKE_XCODE_ATTRIBUTE_MARKETING_VERSION
            AND NOT QT_NO_SET_XCODE_ATTRIBUTE_MARKETING_VERSION
            AND NOT CMAKE_XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION
            AND NOT QT_NO_SET_XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION
            AND CMAKE_GENERATOR STREQUAL "Xcode")
            get_target_property(marketing_version "${target}"
                XCODE_ATTRIBUTE_MARKETING_VERSION)
            get_target_property(current_project_version "${target}"
                XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION)
            if(NOT marketing_version AND NOT current_project_version)
                set_target_properties("${target}"
                    PROPERTIES
                        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${bundle_version}"
                        XCODE_ATTRIBUTE_MARKETING_VERSION "${bundle_short_version}"
                )
                set(bundle_version "$(CURRENT_PROJECT_VERSION)")
                set(bundle_short_version "$(MARKETING_VERSION)")
            endif()
        endif()

        set_target_properties("${target}"
                               PROPERTIES
                               MACOSX_BUNDLE_BUNDLE_VERSION "${bundle_version}"
                               MACOSX_BUNDLE_SHORT_VERSION_STRING "${bundle_short_version}"
                               )
    endif()
endfunction()

function(_qt_internal_set_xcode_development_team_id target)
    # If user hasn't provided a development team id, try to find the first one specified
    # in the Xcode preferences.
    if(NOT CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM AND NOT QT_NO_SET_XCODE_DEVELOPMENT_TEAM_ID)
        get_target_property(existing_team_id "${target}" XCODE_ATTRIBUTE_DEVELOPMENT_TEAM)
        if(NOT existing_team_id)
            _qt_internal_find_apple_development_team_id(team_id)
            set_target_properties("${target}"
                                  PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${team_id}")
        endif()
    endif()
endfunction()

function(_qt_internal_set_apple_bundle_identifier target)
    # Skip all logic if requested.
    if(QT_NO_SET_XCODE_BUNDLE_IDENTIFIER)
        return()
    endif()

    # There are two fields to consider: the CFBundleIdentifier key (ie., cmake_bundle_identifier)
    # to be written to Info.plist and the PRODUCT_BUNDLE_IDENTIFIER (ie., xcode_bundle_identifier)
    # property to set in the Xcode project. The `cmake_bundle_identifier` set by
    # MACOSX_BUNDLE_GUI_IDENTIFIER applies to both Xcode, and other generators, while
    # `xcode_bundle_identifier` set by XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER is
    # Xcode specific.
    #
    # If Ninja is the generator, we set the value of `MACOSX_BUNDLE_GUI_IDENTIFIER`
    # and don't touch the `XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER`.
    # If Xcode is the generator, we set the value of `XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER`,
    # and additionally, to silence a Xcode's warning, we set the `MACOSX_BUNDLE_GUI_IDENTIFIER` to
    # `${PRODUCT_BUNDLE_IDENTIFIER}` so that Xcode could sort it out.

    get_target_property(existing_cmake_bundle_identifier "${target}"
                        MACOSX_BUNDLE_GUI_IDENTIFIER)
    get_target_property(existing_xcode_bundle_identifier "${target}"
                        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER)

    set(is_cmake_bundle_identifier_given FALSE)
    if(existing_cmake_bundle_identifier)
        set(is_cmake_bundle_identifier_given TRUE)
    elseif(MACOSX_BUNDLE_GUI_IDENTIFIER)
        set(is_cmake_bundle_identifier_given TRUE)
        set(existing_cmake_bundle_identifier ${MACOSX_BUNDLE_GUI_IDENTIFIER})
    endif()

    set(is_xcode_bundle_identifier_given FALSE)
    if(existing_xcode_bundle_identifier)
        set(is_xcode_bundle_identifier_given TRUE)
    elseif(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER)
        set(is_xcode_bundle_identifier_given TRUE)
        set(existing_xcode_bundle_identifier ${CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER})
    endif()

    if(is_cmake_bundle_identifier_given
        AND is_xcode_bundle_identifier_given
            AND NOT existing_cmake_bundle_identifier STREQUAL existing_xcode_bundle_identifier)
        message(WARNING
            "MACOSX_BUNDLE_GUI_IDENTIFIER and XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "
            "are set to different values. You only need to set one of them. ")
    endif()

    if(NOT is_xcode_bundle_identifier_given
        AND NOT is_cmake_bundle_identifier_given)
        _qt_internal_get_default_apple_bundle_identifier("${target}" bundle_id)
    elseif(is_cmake_bundle_identifier_given)
        set(bundle_id ${existing_cmake_bundle_identifier})
    elseif(is_xcode_bundle_identifier_given)
        set(bundle_id ${existing_xcode_bundle_identifier})
    endif()

    if(CMAKE_GENERATOR STREQUAL "Xcode")
        set_target_properties("${target}"
                              PROPERTIES
                              XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${bundle_id}"
                              MACOSX_BUNDLE_GUI_IDENTIFIER "$(PRODUCT_BUNDLE_IDENTIFIER)")
    else()
        set_target_properties("${target}"
                              PROPERTIES
                              MACOSX_BUNDLE_GUI_IDENTIFIER "${bundle_id}")
    endif()
endfunction()

function(_qt_internal_set_xcode_targeted_device_family target)
    if(NOT CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY
            AND NOT QT_NO_SET_XCODE_TARGETED_DEVICE_FAMILY)
        get_target_property(existing_device_family
            "${target}" XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY)
        if(NOT existing_device_family)
            set(device_family_iphone_and_ipad "1,2")
            set_target_properties("${target}"
                                  PROPERTIES
                                  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY
                                  "${device_family_iphone_and_ipad}")
        endif()
    endif()
endfunction()

function(_qt_internal_set_xcode_code_sign_style target)
    if(NOT CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE
            AND NOT QT_NO_SET_XCODE_CODE_SIGN_STYLE)
        get_target_property(existing_code_style
            "${target}" XCODE_ATTRIBUTE_CODE_SIGN_STYLE)
        if(NOT existing_code_style)
            set(existing_code_style "Automatic")
            set_target_properties("${target}"
                                  PROPERTIES
                                  XCODE_ATTRIBUTE_CODE_SIGN_STYLE
                                  "${existing_code_style}")
        endif()
    endif()
endfunction()

# Workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/15183
function(_qt_internal_set_xcode_install_path target)
    if(NOT CMAKE_XCODE_ATTRIBUTE_INSTALL_PATH
            AND NOT QT_NO_SET_XCODE_INSTALL_PATH)
        get_target_property(existing_install_path
            "${target}" XCODE_ATTRIBUTE_INSTALL_PATH)
        if(NOT existing_install_path)
            set_target_properties("${target}"
                                  PROPERTIES
                                  XCODE_ATTRIBUTE_INSTALL_PATH
                                  "$(inherited)")
        endif()
    endif()
endfunction()

# Explicitly set the debug information format for each build configuration to match the values
# of a new project created via Xcode directly. This ensures debug information is included during
# archiving.
function(_qt_internal_set_xcode_debug_information_format target)
    if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT
            AND NOT QT_NO_SET_XCODE_DEBUG_INFORMATION_FORMAT)
        get_target_property(existing "${target}" XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT)
        if(NOT existing)
            # The CMake Xcode generator searches for [variant=${config}], removes that substring,
            # and generates the attribute only for the config that is specified as the "variant".
            set_target_properties("${target}" PROPERTIES
                "XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Debug]" "dwarf")
            set_target_properties("${target}" PROPERTIES
                "XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release]" "dwarf-with-dsym")
            set_target_properties("${target}" PROPERTIES
                "XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=MinSizeRel]" "dwarf-with-dsym")
            set_target_properties("${target}" PROPERTIES
                "XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=RelWithDebInfo]"
                "dwarf-with-dsym")
        endif()
    endif()
endfunction()

# Make sure to always generate debug symbols, to match the values of a new project created via
# Xcode directly.
function(_qt_internal_set_xcode_generate_debugging_symbols target)
    if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS
            AND NOT QT_NO_SET_XCODE_GCC_GENERATE_DEBUGGING_SYMBOLS)
        get_target_property(existing "${target}" XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS)
        if(NOT existing)
            set_target_properties("${target}" PROPERTIES
                "XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS" "YES")
        endif()
    endif()
endfunction()

# CMake generates a project where this setting is set to an absolute path build dir.
# Provide an opt-in to work around an Xcode issue where archiving does not find the project dSYMs
# unless the configuration build dir starts with $(BUILD_DIR) or is set to $(inherited).
# It is an opt-in, because it breaks certain CMake behavior like $<TARGET_FILE:${target}> genex
# evaluation as well as ignoring the value of CMAKE_RUNTIME_OUTPUT_DIRECTORY.
# So projects have to do it at their own risk.
function(_qt_internal_set_xcode_configuration_build_dir target)
    if(QT_USE_RISKY_DSYM_ARCHIVING_WORKAROUND)
        set_target_properties("${target}" PROPERTIES
            XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "$(inherited)")
    endif()
endfunction()

function(_qt_internal_set_xcode_bundle_display_name target)
    # We want the value of CFBundleDisplayName to be ${PRODUCT_NAME}, but we can't put that
    # into the Info.plist.in template file directly, because the implicit configure_file(Info.plist)
    # done by CMake is not using the @ONLY option, so CMake would treat the assignment as
    # variable expansion. Escaping using backslashes does not help.
    # Work around it by assigning the dollar char to a separate cache var, and expand it, so that
    # the final value in the file will be ${PRODUCT_NAME}, to be evaluated at build time by Xcode.
    set(QT_INTERNAL_DOLLAR_VAR "$" CACHE STRING "")
endfunction()

# Adds ${PRODUCT_NAME} to the Info.plist file, which is then evaluated by Xcode itself.
function(_qt_internal_set_xcode_bundle_name target)
    if(QT_NO_SET_XCODE_BUNDLE_NAME)
        return()
    endif()

    get_target_property(existing_bundle_name "${target}" MACOSX_BUNDLE_BUNDLE_NAME)
    if(NOT MACOSX_BUNDLE_BUNDLE_NAME AND NOT existing_bundle_name)
        if(CMAKE_GENERATOR STREQUAL Xcode)
            set_target_properties("${target}"
                                  PROPERTIES
                                  MACOSX_BUNDLE_BUNDLE_NAME "$(PRODUCT_NAME)")
        else()
            set_target_properties("${target}"
                                  PROPERTIES
                                  MACOSX_BUNDLE_BUNDLE_NAME "${target}")
        endif()
    endif()
endfunction()

function(_qt_internal_set_xcode_bitcode_enablement target)
    if(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE
        OR QT_NO_SET_XCODE_ENABLE_BITCODE)
        return()
    endif()

    get_target_property(existing_bitcode_enablement
        "${target}" XCODE_ATTRIBUTE_ENABLE_BITCODE)
    if(NOT existing_bitcode_enablement MATCHES "-NOTFOUND")
        return()
    endif()

    # Disable bitcode to match Xcode 14's new default
    set_target_properties("${target}"
        PROPERTIES
        XCODE_ATTRIBUTE_ENABLE_BITCODE
        "NO")
endfunction()

function(_qt_internal_copy_info_plist target)
    # If the project already specifies a custom file, we don't override it.
    get_target_property(info_plist_in "${target}" MACOSX_BUNDLE_INFO_PLIST)
    if(NOT info_plist_in)
        set(info_plist_in "${__qt_internal_cmake_apple_support_files_path}/Info.plist.app.in")
    endif()

    string(MAKE_C_IDENTIFIER "${target}" target_identifier)
    set(info_plist_out_dir
        "${CMAKE_CURRENT_BINARY_DIR}/.qt/info_plist/${target_identifier}")
    set(info_plist_out "${info_plist_out_dir}/Info.plist")

    # Check if we need to specify a custom launch screen storyboard entry.
    get_target_property(launch_screen_base_name "${target}" _qt_ios_launch_screen_base_name)
    if(launch_screen_base_name)
        set(qt_ios_launch_screen_plist_entry "${launch_screen_base_name}")
    endif()

    # Call configure_file to substitute Qt-specific @FOO@ values, not ${FOO} values.
    #
    # The output file will be another template file to be fed to CMake via the
    # MACOSX_BUNDLE_INFO_PLIST property. CMake will then call configure_file on it to provide
    # content for regular entries like CFBundleName, etc.
    #
    # We require this extra configure_file call so we can create unique Info.plist files for each
    # target in a project, while also providing a way to add Qt specific entries that CMake
    # does not support out of the box (e.g. a launch screen name).
    configure_file(
        "${info_plist_in}"
        "${info_plist_out}"
        @ONLY
    )

    set_target_properties("${target}" PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${info_plist_out}")
endfunction()

function(_qt_internal_plist_buddy plist_file)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "" "OUTPUT_VARIABLE;ERROR_VARIABLE;EXTRA_ARGS" "COMMANDS")
    foreach(command ${arg_COMMANDS})
        execute_process(COMMAND "/usr/libexec/PlistBuddy"
                                ${arg_EXTRA_ARGS} -c "${command}" "${plist_file}"
                    OUTPUT_VARIABLE plist_buddy_output
                    ERROR_VARIABLE plist_buddy_error)
        string(STRIP "${plist_buddy_output}" plist_buddy_output)
        if(arg_OUTPUT_VARIABLE)
            list(APPEND ${arg_OUTPUT_VARIABLE} ${plist_buddy_output})
            set(${arg_OUTPUT_VARIABLE} ${${arg_OUTPUT_VARIABLE}} PARENT_SCOPE)
        endif()
        if(arg_ERROR_VARIABLE)
            list(APPEND ${arg_ERROR_VARIABLE} ${plist_buddy_error})
            set(${arg_ERROR_VARIABLE} ${${arg_ERROR_VARIABLE}} PARENT_SCOPE)
        endif()
        if(plist_buddy_error)
            return()
        endif()
    endforeach()
endfunction()

function(_qt_internal_set_apple_localizations target)
    if(QT_NO_SET_PLIST_LOCALIZATIONS)
        return()
    endif()

    set(supported_languages "${QT_I18N_TRANSLATED_LANGUAGES}")
    if("${QT_I18N_TRANSLATED_LANGUAGES}" STREQUAL "")
        get_target_property(supported_languages "${target}" _qt_apple_supported_languages)
        if("${supported_languages}" STREQUAL "supported_languages-NOTFOUND")
            return()
        endif()
    endif()
    get_target_property(plist_file "${target}" MACOSX_BUNDLE_INFO_PLIST)
    if (NOT plist_file)
        return()
    endif()

    _qt_internal_plist_buddy("${plist_file}"
        COMMANDS "print CFBundleLocalizations"
        OUTPUT_VARIABLE existing_localizations
    )
    if(NOT existing_localizations)
        list(TRANSFORM supported_languages PREPEND
            "Add CFBundleLocalizations: string ")

        _qt_internal_plist_buddy("${plist_file}"
            COMMANDS
                "Add CFBundleLocalizations array"
                ${supported_languages}
                "Delete CFBundleAllowMixedLocalizations"
        )
    endif()

    if(NOT "${QT_I18N_SOURCE_LANGUAGE}" STREQUAL "")
        _qt_internal_plist_buddy("${plist_file}"
            COMMANDS "print CFBundleDevelopmentRegion"
            OUTPUT_VARIABLE existing_dev_region
        )
        if(NOT existing_dev_region)
            _qt_internal_plist_buddy("${plist_file}"
                COMMANDS
                    "Add CFBundleDevelopmentRegion string"
                    "Set CFBundleDevelopmentRegion ${QT_I18N_SOURCE_LANGUAGE}"
            )
        endif()
    endif()
endfunction()

function(_qt_internal_set_ios_simulator_arch target)
    if(CMAKE_XCODE_ATTRIBUTE_ARCHS
        OR QT_NO_SET_XCODE_ARCHS)
        return()
    endif()

    get_target_property(existing_archs
        "${target}" XCODE_ATTRIBUTE_ARCHS)
    if(NOT existing_archs MATCHES "-NOTFOUND")
        return()
    endif()

    if(NOT x86_64 IN_LIST QT_OSX_ARCHITECTURES)
        return()
    endif()

    if(CMAKE_OSX_ARCHITECTURES AND NOT x86_64 IN_LIST CMAKE_OSX_ARCHITECTURES)
        return()
    endif()

    set_target_properties("${target}"
        PROPERTIES
        "XCODE_ATTRIBUTE_ARCHS[sdk=iphonesimulator*]"
        "x86_64")
endfunction()

function(_qt_internal_set_xcode_entrypoint_attribute target entrypoint)
    if(CMAKE_XCODE_ATTRIBUTE_LD_ENTRY_POINT
        OR QT_NO_SET_XCODE_LD_ENTRY_POINT)
        return()
    endif()

    get_target_property(existing_entrypoint
        "${target}" XCODE_ATTRIBUTE_LD_ENTRY_POINT)
    if(NOT existing_entrypoint MATCHES "-NOTFOUND")
        return()
    endif()

    set_target_properties("${target}"
        PROPERTIES
        "XCODE_ATTRIBUTE_LD_ENTRY_POINT"
        "${entrypoint}")
endfunction()


# Export Apple platform sdk and xcode version requirements to Qt6ConfigExtras.cmake.
# Always exported, even on non-Apple platforms, so that we can use them when building
# documentation.
function(_qt_internal_export_apple_sdk_and_xcode_version_requirements out_var)
    set(vars_to_assign
        QT_SUPPORTED_MIN_IOS_SDK_VERSION
        QT_SUPPORTED_MAX_IOS_SDK_VERSION
        QT_SUPPORTED_MIN_IOS_XCODE_VERSION
        QT_SUPPORTED_MIN_IOS_VERSION
        QT_SUPPORTED_MAX_IOS_VERSION_TESTED

        QT_SUPPORTED_MIN_VISIONOS_SDK_VERSION
        QT_SUPPORTED_MAX_VISIONOS_SDK_VERSION
        QT_SUPPORTED_MIN_VISIONOS_XCODE_VERSION
        QT_SUPPORTED_MIN_VISIONOS_VERSION
        QT_SUPPORTED_MAX_VISIONOS_VERSION_TESTED

        QT_SUPPORTED_MIN_MACOS_SDK_VERSION
        QT_SUPPORTED_MAX_MACOS_SDK_VERSION
        QT_SUPPORTED_MIN_MACOS_XCODE_VERSION
        QT_SUPPORTED_MIN_MACOS_VERSION
        QT_SUPPORTED_MAX_MACOS_VERSION_TESTED
    )

    set(assignments "")
    foreach(var IN LISTS vars_to_assign)
        set(value "${${var}}")
        list(APPEND assignments
            "
if(NOT ${var})
    set(${var} \"${value}\")
endif()")
    endforeach()

    list(JOIN assignments "\n" assignments)
    set(${out_var} "${assignments}" PARENT_SCOPE)
endfunction()

# Returns the active apple sdk name that was either explicitly set by the user via QT_APPLE_SDK or
# or CMAKE_OSX_SYSROOT, or return the default approximated value, based on what CMake does
# internally.
#
# TODO: Handle case when CMAKE_OSX_SYSROOT is set to an sdk path, from which we need to retrieve the
# sdk name.
function(_qt_internal_get_apple_sdk_name out_var)
    if(NOT APPLE)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    # If CMake or the user has set an explicit sdk name, consider it.
    if(QT_APPLE_SDK)
        set(explicit_sdk_name "${QT_APPLE_SDK}")
    elseif(CMAKE_OSX_SYSROOT)
        set(explicit_sdk_name "${CMAKE_OSX_SYSROOT}")
    else()
        set(explicit_sdk_name "")
    endif()

    set(output_sdk_name "")

    # Detect (or check if already set) that the sdk name is one that Qt knows about.
    if(CMAKE_SYSTEM_NAME STREQUAL iOS)
        if(explicit_sdk_name STREQUAL "iphoneos" OR explicit_sdk_name STREQUAL "iphonesimulator")
            set(output_sdk_name "${explicit_sdk_name}")
        else()
            # Default case.
            set(output_sdk_name "iphoneos")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL visionOS)
        if(explicit_sdk_name STREQUAL "xros" OR explicit_sdk_name STREQUAL "xrsimulator")
            set(output_sdk_name "${explicit_sdk_name}")
        else()
            # Default case.
            set(output_sdk_name "xros")
        endif()
    else()
        # Default case.
        set(output_sdk_name "macosx")
    endif()

    set(${out_var} "${output_sdk_name}" PARENT_SCOPE)
endfunction()

function(_qt_internal_execute_xcrun out_var)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        XCRUN_ARGS
        OUT_ERROR_VAR
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    set(output "")
    set(xcrun_error "")

    if(NOT APPLE)
        message(FATAL_ERROR
            "Executing xcrun should only happen happen when targeting Apple plaforms")
    endif()

    find_program(QT_XCRUN xcrun)
    if(NOT QT_XCRUN)
        message(FATAL_ERROR "Can't find xcrun in PATH")
    endif()

    execute_process(COMMAND "${QT_XCRUN}" ${arg_XCRUN_ARGS}
                    OUTPUT_VARIABLE output
                    ERROR_VARIABLE xcrun_error)

    if(arg_OUT_ERROR_VAR)
        set(${arg_OUT_ERROR_VAR} "${xcrun_error}" PARENT_SCOPE)
    endif()

    set(${out_var} "${output}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_apple_sdk_path out_var)
    set(sdk_path "")
    if(APPLE)
        _qt_internal_get_apple_sdk_name(sdk_name)
        _qt_internal_execute_xcrun(sdk_path
            XCRUN_ARGS --sdk ${sdk_name} --show-sdk-path
            OUT_ERROR_VAR xcrun_error
        )

        if(NOT sdk_path)
            message(FATAL_ERROR
                    "Can't determine darwin ${sdk_name} SDK path. Error: ${xcrun_error}")
        endif()

        string(STRIP "${sdk_path}" sdk_path)
    endif()
    set(${out_var} "${sdk_path}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_apple_sdk_version out_var)
    set(sdk_version "")
    if(APPLE)
        _qt_internal_get_apple_sdk_name(sdk_name)
        _qt_internal_execute_xcrun(sdk_version
            XCRUN_ARGS --sdk ${sdk_name} --show-sdk-version
            OUT_ERROR_VAR xcrun_error
        )

        if(NOT sdk_version)
            message(FATAL_ERROR
                    "Can't determine darwin ${sdk_name} SDK version. Error: ${xcrun_error}")
        endif()

        string(STRIP "${sdk_version}" sdk_version)
    endif()
    set(${out_var} "${sdk_version}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_xcode_version_raw out_var)
    set(xcode_version "")
    if(APPLE)
        _qt_internal_execute_xcrun(xcode_version
            XCRUN_ARGS xcodebuild -version
            OUT_ERROR_VAR xcrun_error
        )

        string(REPLACE "\n" " " xcode_version "${xcode_version}")
        string(STRIP "${xcode_version}" xcode_version)
    endif()
    set(${out_var} "${xcode_version}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_xcode_version out_var)
    if(APPLE)
        _qt_internal_get_xcode_version_raw(xcode_version_raw)

        # The raw output is something like after the newlines are replaced with spaces:
        # Xcode 14.3 Build version 14E222b
        # We want only the '14.3' part. We could be more specific with the regex to match only
        # digits separated by dots, but you never know how Apple might change the format.
        string(REGEX REPLACE "Xcode (([^ ])+)" "\\2" xcode_version "${xcode_version_raw}")
        if(xcode_version_raw MATCHES "Xcode ([^ ]+)")
            set(xcode_version "${CMAKE_MATCH_1}")
        else()
            message(DEBUG "Failed to extract Xcode version from '${xcode_version_raw}'")
            set(xcode_version "${xcode_version_raw}")
        endif()

        set(${out_var} "${xcode_version}" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_get_cached_apple_sdk_version out_var)
    if(QT_INTERNAL_APPLE_SDK_VERSION)
        set(sdk_version "${QT_INTERNAL_APPLE_SDK_VERSION}")
    else()
        _qt_internal_get_apple_sdk_version(sdk_version)
        set(QT_INTERNAL_APPLE_SDK_VERSION "${sdk_version}" CACHE STRING "Apple SDK version")
    endif()

    set(${out_var} "${sdk_version}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_cached_xcode_version out_var)
    if(QT_INTERNAL_XCODE_VERSION)
        set(xcode_version "${QT_INTERNAL_XCODE_VERSION}")
    else()
        _qt_internal_get_xcode_version(xcode_version)
        if(QT_NO_XCODE_MIN_VERSION_CHECK)
            set(xcode_version "")
        else()
            set(QT_INTERNAL_XCODE_VERSION "${xcode_version}" CACHE STRING "Xcode version")
        endif()
    endif()

    set(${out_var} "${xcode_version}" PARENT_SCOPE)
endfunction()

# Warn or error out when the platform SDK or Xcode version are not supported.
#
# The messages are currently only shown when building Qt, not when building user projects
# with CMake.
# The warnings ARE shown for qmake user projects.
#
# The qmake equivalent for user projects is in mkspecs/features/mac/default_post.prf.
function(_qt_internal_check_apple_sdk_and_xcode_versions)
    if(NOT APPLE)
        return()
    endif()

    if(QT_NO_APPLE_SDK_AND_XCODE_CHECK)
        return()
    endif()

    # Only run the check once in a top-level build.
    get_property(check_done GLOBAL PROPERTY _qt_internal_apple_sdk_and_xcode_check_done)
    if(check_done)
        return()
    endif()
    set_property(GLOBAL PROPERTY _qt_internal_apple_sdk_and_xcode_check_done "TRUE")

    if(IOS)
        set(min_sdk_version "${QT_SUPPORTED_MIN_IOS_SDK_VERSION}")
        set(max_sdk_version "${QT_SUPPORTED_MAX_IOS_SDK_VERSION}")
        set(min_xcode_version "${QT_SUPPORTED_MIN_IOS_XCODE_VERSION}")
    elseif(VISIONOS)
        set(min_sdk_version "${QT_SUPPORTED_MIN_VISIONOS_SDK_VERSION}")
        set(max_sdk_version "${QT_SUPPORTED_MAX_VISIONOS_SDK_VERSION}")
        set(min_xcode_version "${QT_SUPPORTED_MIN_VISIONOS_XCODE_VERSION}")
    else()
        set(min_sdk_version "${QT_SUPPORTED_MIN_MACOS_SDK_VERSION}")
        set(max_sdk_version "${QT_SUPPORTED_MAX_MACOS_SDK_VERSION}")
        set(min_xcode_version "${QT_SUPPORTED_MIN_MACOS_XCODE_VERSION}")
    endif()

    _qt_internal_get_cached_apple_sdk_version(sdk_version)

    if(NOT max_sdk_version MATCHES "^[0-9]+$")
        message(FATAL_ERROR
            "Invalid max SDK version: ${max_sdk_version} "
            "It should be a major version number, without minor or patch version components.")
    endif()

    # The default differs in different branches.
    set(failed_check_should_error TRUE)

    if(failed_check_should_error)
        # Allow downgrading the error into a warning.
        #
        # Our cmake build tests might be executed on older not officially supported Xcode or SDK
        # versions in the CI. Downgrade the error in this case as well.
        if(QT_FORCE_WARN_APPLE_SDK_AND_XCODE_CHECK OR QT_INTERNAL_IS_CMAKE_BUILD_TEST)
            set(message_type WARNING)
            set(extra_message " Due to QT_FORCE_WARN_APPLE_SDK_AND_XCODE_CHECK being ON "
                "the build will continue, but it will likely fail. Use at your own risk.")
        else()
            set(message_type FATAL_ERROR)
            set(extra_message " You can turn this error into a warning by configuring with "
                "-DQT_FORCE_WARN_APPLE_SDK_AND_XCODE_CHECK=ON, but the build will likely fail. "
                "Use at your own risk.")
        endif()
    else()
        # Allow upgrading the warning into an error.
        if(QT_FORCE_FATAL_APPLE_SDK_AND_XCODE_CHECK)
            set(message_type FATAL_ERROR)
            set(extra_message " Erroring out due to QT_FORCE_FATAL_APPLE_SDK_AND_XCODE_CHECK "
                "being ON.")
        else()
            set(message_type WARNING)
            set(extra_message " You can turn this warning into an error by configuring with "
                "-DQT_FORCE_FATAL_APPLE_SDK_AND_XCODE_CHECK=ON. ")
        endif()
    endif()

    if(sdk_version VERSION_LESS min_sdk_version AND NOT QT_NO_APPLE_SDK_MIN_VERSION_CHECK)
        message(${message_type}
            "Qt requires at least version ${min_sdk_version} of the platform SDK, "
            "you're building against version ${sdk_version}. Please upgrade."
            ${extra_message}
        )
    endif()

    if(NOT QT_NO_XCODE_MIN_VERSION_CHECK)
        _qt_internal_get_cached_xcode_version(xcode_version)
        if(NOT xcode_version)
            message(FATAL_ERROR
                    "Can't determine Xcode version. Is Xcode installed?"
                    " Error details:\n${xcrun_error}")
        endif()
        if(xcode_version VERSION_LESS min_xcode_version)
            message(${message_type}
                "Qt requires at least version ${min_xcode_version} of Xcode, "
                "you're building against version ${xcode_version}. Please upgrade."
                ${extra_message}
            )
        endif()
    endif()

    if(QT_NO_APPLE_SDK_MAX_VERSION_CHECK)
        return()
    endif()

    # Make sure we warn only when the current version is greater than the max supported version.
    math(EXPR next_after_max_sdk_version "${max_sdk_version} + 1")
    if(sdk_version VERSION_GREATER_EQUAL next_after_max_sdk_version)
        message(WARNING
            "Qt has only been tested with version ${max_sdk_version} "
            "of the platform SDK, you're using ${sdk_version}. "
            "This is an unsupported configuration. You may experience build issues, "
            "and by using "
            "the ${sdk_version} SDK you are opting in to new features "
            "that Qt has not been prepared for. "
            "Please downgrade the SDK you use to build your app to version "
            "${max_sdk_version}, or configure "
            "with -DQT_NO_APPLE_SDK_MAX_VERSION_CHECK=ON to silence this warning."
        )
    endif()
endfunction()

function(_qt_internal_finalize_apple_app target)
    # Shared between macOS and UIKit apps

    _qt_internal_copy_info_plist("${target}")
    _qt_internal_set_apple_localizations("${target}")

    # Only set the various properties if targeting the Xcode generator, otherwise the various
    # Xcode tokens are embedded as-is instead of being dynamically evaluated.
    # This affects things like the version number or application name as reported by Qt API.
    if(CMAKE_GENERATOR STREQUAL "Xcode")
        _qt_internal_set_xcode_development_team_id("${target}")
        _qt_internal_set_xcode_code_sign_style("${target}")
        _qt_internal_set_xcode_bundle_display_name("${target}")
        _qt_internal_set_xcode_install_path("${target}")
        _qt_internal_set_xcode_configuration_build_dir("${target}")
        _qt_internal_set_xcode_debug_information_format("${target}")
        _qt_internal_set_xcode_generate_debugging_symbols("${target}")
    endif()

    _qt_internal_set_xcode_bundle_name("${target}")
    _qt_internal_set_apple_bundle_identifier("${target}")
    _qt_internal_set_placeholder_apple_bundle_version("${target}")
endfunction()

function(_qt_internal_finalize_uikit_app target)
    if(CMAKE_SYSTEM_NAME STREQUAL iOS)
        _qt_internal_finalize_ios_app("${target}")
    else()
        _qt_internal_finalize_apple_app("${target}")
    endif()
endfunction()

function(_qt_internal_finalize_ios_app target)
    # Must be called before we generate the Info.plist
    _qt_internal_handle_ios_launch_screen("${target}")

    _qt_internal_finalize_apple_app("${target}")
    _qt_internal_set_xcode_targeted_device_family("${target}")
    _qt_internal_set_xcode_bitcode_enablement("${target}")
    _qt_internal_set_ios_simulator_arch("${target}")

    _qt_internal_set_xcode_entrypoint_attribute("${target}" "_qt_main_wrapper")
endfunction()

function(_qt_internal_finalize_macos_app target)
    get_target_property(is_bundle ${target} MACOSX_BUNDLE)
    if(NOT is_bundle)
        return()
    endif()

    _qt_internal_finalize_apple_app("${target}")

    # Make sure the install rpath has at least the minimum needed if the app
    # has any non-static frameworks. We can't rigorously know if the app will
    # have any, even with a static Qt, so always add this. If there are no
    # frameworks, it won't do any harm.
    get_property(install_rpath TARGET ${target} PROPERTY INSTALL_RPATH)
    list(APPEND install_rpath "@executable_path/../Frameworks")
    list(REMOVE_DUPLICATES install_rpath)
    set_property(TARGET ${target} PROPERTY INSTALL_RPATH "${install_rpath}")
endfunction()
