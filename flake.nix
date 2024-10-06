{
  description = "A very basic flake";

  inputs = { nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable"; };

  outputs = { self, nixpkgs }: {

    packages.x86_64-linux.file-syncer =
      nixpkgs.x86_64-linux.stdenv.mkDerivation {
        name = "libfoo-1.2.3";
        src = ./.;
      };
  };
}
