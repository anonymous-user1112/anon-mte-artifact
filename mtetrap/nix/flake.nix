{
  description = "C/C++ environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils, ... }@inputs:
  utils.lib.eachDefaultSystem (
    system:
    let
      p = import nixpkgs {
        inherit system;
      };
      llvm = p.llvmPackages_latest;
      pc = import nixpkgs {
        inherit system;
        crossSystem = nixpkgs.lib.systems.examples.aarch64-multiplatform; #// { useLLVM = true; };
      };

        # simple script which replaces the functionality of make
        # it works with <math.h> and includes debugging symbols by default
        # it will be updated as per needs

        # arguments: outfile
        # basic usage example: mk main [flags]
        mymake = p.writeShellScriptBin "mk" ''
        if [ -f "$1.c" ]; then
        i="$1.c"
        c=$CC
        else
        i="$1.cpp"
        c=$CXX
        fi
        o=$1
        shift
        $c -ggdb $i -o $o -lm -Wall $@
        '';
    in
    {
      devShells = {
        native = p.mkShell rec {
          packages = [
            p.android-tools
            p.cmake
            p.gnumake
            p.gdb

            #dynamorio
            p.xxHash
            p.lz4
            p.zlib
            p.libunwind
            p.snappy
            p.doxygen
          ];
        };
        cross = pc.mkShell rec {
          hardeningDisable = [ "all" ];

          inputsFrom = [ pc.linuxPackages_5_15.kernel.dev ];

          depsBuildBuild = [
            pc.buildPackages.stdenv.cc # HOSTCC = gcc
          ];
          # alias of depsBuildHost
          nativeBuildInputs = with pc.buildPackages; [
            git-repo
            android-tools
            cmake
            gnumake
            gdb
            pkg-config
            universal-ctags
          ];
          # alias of depsHostTarget
          buildInputs = [
            pc.glibc.static
            pc.xxHash
            pc.lz4
            pc.zlib
            pc.libunwind
            pc.snappy
            pc.doxygen
          ];
          # CLANGD_FLAGS = "--query-driver=aarch64-unknown-linux-gnu-gcc";

          # make -j$(nproc) ARCH=arm64 CC=$CC LD=$LD AR=$AR NM=$NM OBJCOPY=$OBJCOPY OBJDUMP=$OBJDUMP READELF=$READELF STRIP=$STRIP \
          #                 HOSTCC=$CC_FOR_BUILD HOSTAR=$AR_FOR_BUILD HOSTLD=$LD_FOR_BUILD
        };
        fhsenv = (p.buildFHSUserEnv {
          name = "fhsenv";
          targetPkgs = pkgs: ( with pkgs; [
            android-tools
            git-repo
            zlib
          ]);
          runScript = "bash";
        }).env;
      };
    }
    );
  }
