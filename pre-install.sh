#! /bin/sh
# sdsl for maw
cd ./external/sdsl-lite
./install.sh "$(pwd)"/libsdsl
# Solon's Tool
cd ../maw-master
tar -xvf sdsl-lite.tar.gz
cd sdsl-lite
./install.sh "$(pwd)"/libsdsl
mv libsdsl/ ..
make -f Makefile.64-bit.gcc

