"""Check the portable CODRONE identity stays separate from upstream QGC."""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class CustomerIdentityTest(unittest.TestCase):
    def test_custom_application_has_independent_identity(self):
        with tempfile.TemporaryDirectory(prefix="qgc-identity-", dir=os.environ.get("TMPDIR")) as directory:
            script = Path(directory) / "identity.cmake"
            script.write_text(
                f'set(CMAKE_SOURCE_DIR "{ROOT.as_posix()}")\n'
                'set(QGC_CUSTOM_DIR "custom")\n'
                'set(QGC_CPACK_GENERATOR "DEB" CACHE STRING "Legacy installer choice")\n'
                f'list(APPEND CMAKE_MODULE_PATH "{ROOT.as_posix()}/cmake" "{ROOT.as_posix()}/cmake/modules")\n'
                f'include("{ROOT.as_posix()}/cmake/CustomOptions.cmake")\n'
                f'include("{ROOT.as_posix()}/custom/cmake/CustomOverrides.cmake")\n'
                'message("IDENTITY|${QGC_APP_NAME}|${QGC_ORG_NAME}|${QGC_ORG_DOMAIN}|${QGC_PACKAGE_NAME}")\n'
                'message("NATIVE_PACKAGE|${QGC_CPACK_GENERATOR}")\n'
            )
            result = subprocess.run(["cmake", "-P", str(script)], text=True, capture_output=True, check=False)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            identities = [line for line in result.stderr.splitlines() if line.startswith("IDENTITY|")]
            self.assertEqual(identities, ["IDENTITY|CODRONEControl|CODRONE|codrone.local|org.codrone.control"])
            self.assertIn("NATIVE_PACKAGE|\n", result.stderr)


if __name__ == "__main__":
    unittest.main()
