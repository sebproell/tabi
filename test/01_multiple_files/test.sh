rm -rf build/
mkdir build
cd build
cc -fsanitize=address -o bootstrap ../build.tabi.c && ./bootstrap ../ . && rm bootstrap
ninja
./main
