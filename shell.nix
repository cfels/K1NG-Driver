{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  packages = with pkgs; [
    gcc
    gnumake
    pkg-config
    libusb1
    nil
  ];
}
