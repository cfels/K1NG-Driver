{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    gnumake
    libusb1
    pkg-config
    pkgs.nil
  ];
}
