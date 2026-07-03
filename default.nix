{ pkgs ? import <nixpkgs> {} }:
let
  llvm = pkgs.llvmPackages_latest;
in
llvm.stdenv.mkDerivation {
  pname = "proyecto2";
  version = "0.1.0";

  src = pkgs.lib.cleanSource ./.;

  nativeBuildInputs = with pkgs; [ pkg-config cmake ninja ];
  buildInputs = with pkgs; [ sdl3 sdl3-image sdl3-ttf curl ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
  ];

  env = {
    CMAKE_PREFIX_PATH = builtins.concatStringsSep ":" [
      "${pkgs.sdl3.dev}/lib/cmake"
      "${pkgs.sdl3-ttf}/lib/cmake"
      "${pkgs.sdl3-image.dev}/lib/cmake"
    ];
  };

  meta = with pkgs.lib; {
    description = "RTree - AED Proyecto 2";
    license = licenses.mit;
    platforms = platforms.linux ++ platforms.darwin;
  };
}
