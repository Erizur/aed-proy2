{
  description = "Formacion del RTree para uso multiple";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    forAllSystems = nixpkgs.lib.genAttrs [
      "aarch64-linux"
      "x86_64-linux"
      "aarch64-darwin"
      "x86_64-darwin"
    ];

    pkgsFor = nixpkgs.legacyPackages;
  in {
    packages = forAllSystems (system: rec {
      fruitsheet = pkgsFor.${system}.callPackage ./default.nix {};
      default = fruitsheet;
    });

    devShells = forAllSystems (system: {
      default = pkgsFor.${system}.callPackage ./shell.nix {};
    });
  };
}
