{ pkgs ? import <nixpkgs> { } }:

with pkgs;

mkShell rec {
  nativeBuildInputs = [
    gcc
  ];
  buildInputs = [
    sfml
  ];
}