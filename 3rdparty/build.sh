#/bin/sh -e

cd yaml-cpp-0.5.1
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=../.. ..
make -j4
make install
