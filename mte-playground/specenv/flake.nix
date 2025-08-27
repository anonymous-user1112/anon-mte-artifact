{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    nix-environments.url = "github:nix-community/nix-environments";
  };

  outputs = { self, nixpkgs, nix-environments }: let
    # Replace this string with your actual system, e.g. "x86_64-linux"
    # system = "x86_64-linux";
    system = "aarch64-darwin";
  in {
    devShells.${system} = let
      pkgs = import nixpkgs { inherit system; };
    in {
      default = import ./shell.nix { inherit pkgs; extraPkgs = [pkgs.qemu];};
      pydev = pkgs.mkShell {
        packages = [
          (pkgs.python3.withPackages (python-pkgs: with python-pkgs; [
            matplotlib
            numpy
            seaborn
          ]))
        ];
      };
    };
  };
}
