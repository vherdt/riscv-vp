#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PREFIX=$DIR/softfloat-dist

version=3e
source=SoftFloat-$version.zip

if [ ! -f "$source" ]; then
	wget http://www.jhauser.us/arithmetic/$source
fi

unzip -n $source
make -C SoftFloat-$version/build/Linux-x86_64-GCC/ -j4
mkdir -p softfloat-dist/lib
mkdir -p softfloat-dist/include/softfloat
cp SoftFloat-$version/build/Linux-x86_64-GCC/softfloat.a softfloat-dist/lib/libsoftfloat.a
cp SoftFloat-$version/build/Linux-x86_64-GCC/platform.h softfloat-dist/include/softfloat/
cp SoftFloat-$version/source/include/* softfloat-dist/include/softfloat/
cp SoftFloat-$version/source/include/8086-SSE/specialize.h softfloat-dist/include/softfloat/

header_file=softfloat-dist/include/softfloat/softfloat.hpp
echo "#pragma once" > $header_file
echo "extern \"C\" {" >> $header_file
echo "#include \"softfloat.h\"" >> $header_file
echo "}" >> $header_file
