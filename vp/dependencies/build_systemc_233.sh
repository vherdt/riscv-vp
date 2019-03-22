#!/usr/bin/env bash

NPROCS=$(grep -c ^processor /proc/cpuinfo)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PREFIX=$DIR/systemc-dist

version=2.3.3
source=systemc-$version.tar.gz

if [ ! -f "$source" ]; then
	wget http://www.accellera.org/images/downloads/standards/systemc/$source
fi

tar xzf $source
cd systemc-$version
mkdir build && cd build
../configure CXXFLAGS='-std=c++14' --prefix=$PREFIX --with-arch-suffix=
make -j$NPROCS
make install

cd $DIR
