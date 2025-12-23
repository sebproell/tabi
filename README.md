# tabi

Tabi (a type of Japanese sock) is a build system written in C that uses C to 
describe build rules. The only dependencies are a C99 compiler for build system
generation and ninja to execute the generated build files.
The logic follows CMake, but the syntax is C.

Note: currently, this is a toy project that addresses the annoying fact that (meta) build systems are usually
not written in the same language as the code they build.

## Usage

1. Drop a `build.tabi.c` file in your project root. This is similar to a `Makefile` or `CMakeLists.txt`.
2. Bootstrap tabi once, e.g. by
   
```
mkdir build
cd build
cc -o bootstrap ../build.tabi.c && ./bootstrap ../ . && rm bootstrap
```

3. From now on, just run `ninja` in the build folder to build your project. The build system will update itself 
   when `build.tabi.c` changes.

## Acknowledgements

Inspired by [nob](https://github.com/tsoding/nob)


