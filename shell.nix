{
  pkgs ? import <nixpkgs> { },
}:

let
  qtEnv = pkgs.qt6.env "qt6-env" [
    pkgs.qt6.qtbase
    pkgs.qt6.qtdeclarative
  ];
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    cmake
    pkg-config
    qt6.wrapQtAppsHook
  ];

  buildInputs = with pkgs; [
    # driver
    gcc
    gnumake
    libusb1
    nil

    # ui
    qt6.qtbase
    qt6.qtdeclarative
    qtEnv
    libglvnd
    fontconfig
  ];

  shellHook = ''
    export QT_PLUGIN_PATH="${qtEnv}/lib/qt-6/plugins"
    export QML_IMPORT_PATH="${qtEnv}/lib/qt-6/qml"
  '';
}
