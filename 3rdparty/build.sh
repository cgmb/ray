#/bin/sh -e

cd yaml-cpp-0.5.1
mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=../.. ..
make -j4
make install
