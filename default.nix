{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  name = "doom-poc";
  src = ./.; # Use the current directory as the source

  # Dependencies needed to build the project
  nativeBuildInputs = [
    pkgs.pkg-config
  ];
  buildInputs = [
    pkgs.sfml
  ];

  # The command to build the project.
  # Nix sets up the environment so pkg-config works correctly here.
  buildPhase = ''
    g++ map.cpp -o map $(pkg-config --libs --cflags sfml-all)
  '';

  # The command to "install" the result into the Nix store.
  installPhase = ''
    mkdir -p $out/bin
    cp map $out/bin/
  '';
}