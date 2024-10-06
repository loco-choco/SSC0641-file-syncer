{
  description = "SSC0641 file-syncer";

  inputs = { nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable"; };

  outputs = { self, nixpkgs }: {

    packages.x86_64-linux.default =
      nixpkgs.legacyPackages.x86_64-linux.stdenv.mkDerivation {
        name = "file-syncer";
        src = ./.;
        installPhase = ''
          mkdir -p $out/bin
          cp out/daemon $out/bin
          cp out/client $out/bin
        '';
      };
  };
}
