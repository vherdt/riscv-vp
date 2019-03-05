#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PREFIX=$DIR/systemc-dist

version=2.3.3
source=systemc-$version.tar.gz

if [ ! -f "$source" ]; then
	wget http://www.accellera.org/images/downloads/standards/systemc/$source
fi

##--enable-pthreads
##../configure CXX=clang++ CC=clang ...

tar xzf $source
cd systemc-$version
mkdir build && cd build
../configure CXXFLAGS='-std=c++14' --prefix=$PREFIX --enable-debug --with-arch-suffix=
make -j8
make install

cd $DIR
