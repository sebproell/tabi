rm -rf build/
mkdir build
cd build
cc -o bootstrap ../build.tabi.c && ./bootstrap ../ . && rm bootstrap
ninja
./main
