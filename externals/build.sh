#!/bin/sh

# portaudio
cd portaudio
./configure --prefix=`pwd`/../
make clean
make
make install

cd bindings/cpp
./configure --prefix=`pwd`/../../../
make clean
make
make install

cd ../../../



