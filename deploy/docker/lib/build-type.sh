# shellcheck shell=sh
# Validate a CMake build type. Usage: validate_build_type "<value>"
validate_build_type() {
    case "${1:-}" in
        Release|Debug|RelWithDebInfo|MinSizeRel) return 0 ;;
        *)
            echo "Error: Invalid BUILD_TYPE: ${1:-}" >&2
            echo "Usage: expected one of Release|Debug|RelWithDebInfo|MinSizeRel" >&2
            return 1
            ;;
    esac
}
