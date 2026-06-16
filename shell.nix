{ pkgs ? import <nixpkgs> { }, ... }:
let
  inherit (pkgs)
    mkShell
    sdl3
    sdl3-ttf
    sdl3-image
    ;
    defaultPkg = pkgs.callPackage ./default.nix { };
    llvm = pkgs.llvmPackages_latest;
in
mkShell.override { stdenv = llvm.stdenv; } {
  packages = with pkgs; [
    llvm.clang-tools
    llvm.lldb

    cmake
    ninja
    pkg-config
  ] ++ defaultPkg.buildInputs;

  env = {
    CMAKE_PREFIX_PATH = "${sdl3.dev}/lib/cmake:${sdl3-ttf}/lib/cmake:${sdl3-image.dev}/lib/cmake";
  };
}
