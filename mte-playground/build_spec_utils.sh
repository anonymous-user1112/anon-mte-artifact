#!/bin/bash

set -e
# SPECROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
[ -e shrc ] || (echo "This scirpt must be installed at the SPEC root directory" && return)
[ -e spectools.patch ] || (echo "spectools.patch must exist at the SPEC root directory" && return)

# might need sudo if you are not root
if [ ! "$(/bin/sh -c 'echo $BASH_VERSION')" ]; then
  ln -sf /bin/bash /bin/sh && echo "\`ln -sf /bin/bash /bin/sh\` failed! Please do this manually via sudo and run this script again"
fi

# apt install -y build-essential || echo "skip installing build deps as I cannot find apt"
# wget -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
# find tools -path "**/config.guess" -exec cp config.guess {} \;

# wget -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'
# find tools -path "**/config.sub" -exec cp config.sub {} \;

# https://sjp38.github.io/post/spec_cpu2006_install/
if [ ! -e .patched ]; then
  patch -p1 < spectools.patch
  touch .patched
fi

pushd tools/src || return
PERLFLAGS="-A libs=-lm -A libs=-ldl  -Dnoextensions=IPC/SysV -Dperl" CFLAGS="-fpermissive -std=gnu89" CXXFLAGS="-fpermissive -std=c++03" ./buildtools
popd || return

#!/bin/bash
# fileid=
# curl "https://drive.usercontent.google.com/download?id=${fileid}&confirm=xxx" -o spec06.tar.bz2
# bunzip2 spec06.tar.bz2
# mkdir spec06 && tar xvf spec06.tar --directory=spec06
# install.sh
# runspec --config=llvm-mte-aarch64-ral -e default,half,full -a build --tune=base int
# runspec --config=llvm-mte-aarch64-ral -e default,half,full -a clean --tune=base int
#
# # curl https://www.spec.org/cpu2006/src.alt/linux-apm-arm64-118.tar -o linux-apm-arm64.tar
# # tar xvf linux-apm-arm64.tar
#
# runspec -a build -e default-static,half-static,full-static --tune=base int
# ls benchspec/SPEC2006/**/exe
#
# runspec -a build -e default-static,half-static,full-static --size test --iterations 5 int --make_bundle cint_static # -S ON_VM
# adb push cint_static.cpu2006bundle.bz2 /data/androdeb/debian/root/spec
