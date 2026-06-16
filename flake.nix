{
  description = "QGroundControl — development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };

        # Qt 6.10+ is required (see .github/build-config.json: qt_minimum_version).
        # nixpkgs currently ships 6.11, which satisfies it.
        qt = pkgs.qt6;

        qtModules = with qt; [
          qtbase
          qtdeclarative
          qttools
          qtgraphs
          qtlocation
          qtpositioning
          qtspeech
          qtmultimedia
          qtserialport
          qtimageformats
          qtshadertools
          qtconnectivity
          qtquick3d
          qtsensors
          qtscxml
          qtwebsockets
          qthttpserver
          qt5compat
        ];

        gstreamerPkgs = with pkgs.gst_all_1; [
          gstreamer
          gst-plugins-base
          gst-plugins-good
          gst-plugins-bad
          gst-plugins-ugly
          gst-libav
        ];
      in
      {
        devShells.default = pkgs.mkShell {
          name = "qgroundcontrol";

          # Tools (run on the build host).
          # NOTE: wrapQtAppsHook is intentionally omitted — it causes an
          # infinite-recursion / stack overflow when used inside mkShell.
          # The qtbase setup hook (pulled in via buildInputs) already exports
          # the Qt env (QMAKE, plugin/QML paths) for an interactive shell.
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            ccache
            mold
            pkg-config
            python3
            git
            gdb
            cppcheck
            nasm
            shaderc
          ];

          # Libraries to link/compile against.
          buildInputs =
            qtModules
            ++ gstreamerPkgs
            ++ (with pkgs; [
              libusb1
              SDL2
              libpulseaudio
              pipewire
              vulkan-loader
              vulkan-headers
              libxkbcommon
              fontconfig
              freetype
            ]);

          # wrapQtAppsHook in a shell exports the Qt plugin/QML paths and sets
          # up dont-wrap so the qmlls / cmake Qt detection works from the shell.
          shellHook = ''
            export QT_QPA_PLATFORM_PLUGIN_PATH="${qt.qtbase}/lib/qt-6/plugins/platforms"
            echo "QGroundControl dev shell — Qt ${qt.qtbase.version}"
            echo "Configure with: cmake -S . -B build -G Ninja"
          '';
        };
      }
    );
}
