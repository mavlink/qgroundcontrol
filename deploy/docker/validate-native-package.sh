#!/usr/bin/env bash
# Validate, install, smoke-test, and uninstall a native Linux package inside
# its matching distro build image.
set -euo pipefail

package=${1:?Usage: validate-native-package.sh <package>}
source_root=${QGC_SOURCE_ROOT:-/project/source}

python3 "${source_root}/.github/scripts/validate_native_package.py" "${package}"

case "${package}" in
    *.deb)
        package_name=$(dpkg-deb --field "${package}" Package)
        apt-get update
        DEBIAN_FRONTEND=noninteractive apt-get install -y "${package}"
        remove_command=(apt-get remove -y "${package_name}")
        ;;
    *.rpm)
        package_name=$(rpm -qp --queryformat '%{NAME}' "${package}")
        dnf install -y "${package}"
        remove_command=(dnf remove -y "${package_name}")
        ;;
    *.pkg.tar.zst)
        package_name=$(pacman -Qp "${package}" | awk 'NR == 1 {print $1}')
        pacman -Sy --noconfirm
        pacman -U --noconfirm "${package}"
        remove_command=(pacman -R --noconfirm "${package_name}")
        ;;
    *)
        echo "Unsupported native package: ${package}" >&2
        exit 2
        ;;
esac

test -x /opt/QGroundControl/bin/QGroundControl
test -L /usr/bin/QGroundControl
env QT_QPA_PLATFORM=offscreen timeout 30s /usr/bin/QGroundControl --help >/dev/null

"${remove_command[@]}"
test ! -e /opt/QGroundControl
test ! -e /usr/bin/QGroundControl

echo "Native package lifecycle validated: $(basename "${package}")"
