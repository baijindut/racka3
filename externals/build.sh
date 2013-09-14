#!/bin/sh

# portaudio
cd portaudio
./configure --prefix=`pwd`/../
make
make install

cd bindings/cpp
./configure --prefix=`pwd`/../../../
make
make install

cd ../../../



